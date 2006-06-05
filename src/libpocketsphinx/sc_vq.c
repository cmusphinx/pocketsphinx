/* ====================================================================
 * Copyright (c) 1999-2004 Carnegie Mellon University.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * This work was supported in part by funding from the Defense Advanced 
 * Research Projects Agency and the National Science Foundation of the 
 * United States of America, and the CMU Sphinx Speech Consortium.
 *
 * THIS SOFTWARE IS PROVIDED BY CARNEGIE MELLON UNIVERSITY ``AS IS'' AND 
 * ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
 * NOR ITS EMPLOYEES BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ====================================================================
 *
 */
/*
 *
 * HISTORY
 * 
 * $Log: sc_vq.c,v $
 * Revision 1.1.1.1  2006/05/23 18:45:01  dhuggins
 * re-importation
 *
 * Revision 1.12  2004/12/10 16:48:56  rkm
 * Added continuous density acoustic model handling
 *
 * 
 * 22-Nov-2004  M K Ravishankar (rkm@cs) at Carnegie-Mellon University
 * 		Moved best senone score and best senone within phone
 * 		computation out of here and into senscr module, for
 * 		integrating continuous  models into sphinx2.
 * 
 * 19-Nov-97  M K Ravishankar (rkm@cs) at Carnegie-Mellon University
 * 	Added ability to read power variance file if it exists.
 * 
 * 19-Jun-95  M K Ravishankar (rkm@cs) at Carnegie-Mellon University
 * 	Added scvq_set_psen() and scvq_set_bestpscr().  Modified SCVQScores_all to
 * 	also compute best senone score/phone.
 * 
 * 19-May-95  M K Ravishankar (rkm@cs) at Carnegie-Mellon University
 * 	Added check for bad VQ scores in SCVQScores and SCVQScores_all.
 * 
 * 01-Jul-94  M K Ravishankar (rkm@cs) at Carnegie-Mellon University
 * 	In SCVQScores, returned result from SCVQComputeScores_opt().
 * 
 * 01-Nov-93  M K Ravishankar (rkm@cs) at Carnegie-Mellon University
 * 	Added compressed, 16-bit senone probs option.
 * 
 *  6-Apr-92  Fil Alleva (faa) at Carnegie-Mellon University
 *	- added SCVQAgcSet() and agcType.
 *
 * 08-Oct-91  Eric Thayer (eht) at Carnegie-Mellon University
 *	Created from system by Xuedong Huang
 * 22-Oct-91  Eric Thayer (eht) at Carnegie-Mellon University
 *	Installed some efficiency improvements to acoustic scoring
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

#if defined(WIN32) && !defined(GNUWINCE) && !defined(CYGWIN)
#include <windows.h>
#include <time.h>
#endif

#include "s2types.h"
#include "log.h"
#include "logmsg.h"
#include "scvq.h"
#include "sc_vq_internal.h"
#include "msd.h"
#include "hmm_tied_r.h"
#include "err.h"
#include "search.h"
#include "kdtree.h"
#include "kb.h"
#include "ckd_alloc.h"

#include "fixpoint.h"

#define RPCC(x) asm ("rpcc %t0;" "stq %t0,(%0)",&x)

static int32   detArr[NUM_FEATURES*NUM_ALPHABET];	/* storage for det vectors */
static int32   *dets[NUM_FEATURES];	/* det values foreach feature */
static mean_t  *means[NUM_FEATURES];	/* mean vectors foreach feature */
static var_t   *vars[NUM_FEATURES];	/* var vectors foreach feature */

static int32 topN = MAX_TOPN;
static int32 CdWdPDFMod;

static int32 *scrPass = NULL;

static kd_tree_t **kdtrees;
static uint32 n_kdtrees;
static uint32 kd_maxdepth;
static int32 kd_maxbbi = -1;

#ifdef FIXED_POINT
static fixed32 dcep80msWeight = DEFAULT_RADIX;
#else
static float64 dcep80msWeight = 1.0;
#endif

static int32 use20ms_diff_pow = FALSE;
/* static int32 useMeanNormalization = FALSE; */

static int32 num_frames = 0;
static int32 frame_ds_ratio = 1;

static vqFeature_t lcfrm[MAX_TOPN];
static vqFeature_t ldfrm[MAX_TOPN];
static vqFeature_t lxfrm[MAX_TOPN];
static vqFeature_t vtmp;

#if defined(WIN32) && !defined(GNUWINCE) && !defined(CYGWIN)
static HANDLE pid;
static FILETIME t_create, t_exit, kst, ket, ust, uet;
static double vq_time;
static double scr_time;
extern double win32_cputime();
#endif

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

static void cepDist0(vqFeature_t *topn, mfcc_t *z)
{
    register int32	i, j, k, cw;
    vqFeature_t		*worst, *best = topn, *cur; /*, *src; */
    mean_t		diff, sqdiff, compl; /* diff, diff^2, component likelihood */
    mfcc_t		*obs;
    mean_t		*mean;
    var_t		*var  =  vars[(int32)CEP_FEAT];
    int32		*det  =  dets[(int32)CEP_FEAT], *detP;
    int32		*detE = det + NUM_ALPHABET;
    var_t		d;
    kd_tree_node_t	*node;

    assert(z    != NULL);
    assert(topn != NULL);
    memcpy (topn, lcfrm, sizeof(vqFeature_t)*topN);
    worst = topn + (topN-1);
    if (kdtrees)
	    node = eval_kd_tree(kdtrees[(int32)CEP_FEAT], z+1, kd_maxdepth);
    else
	    node = NULL;
    /* initialize topn codewords to topn codewords from previous frame */
    for (i = 0; i < topN; i++) {
	cw = topn[i].codeword;
	mean = means[(int32)CEP_FEAT] + cw*CEP_VECLEN + 1;
	var  =  vars[(int32)CEP_FEAT] + cw*CEP_VECLEN + 1;
	d = det[cw];
	obs = z+1;
	for (j = 1; j < CEP_VECLEN; j++) {
	    diff = *obs++ - *mean++;
	    sqdiff = MFCCMUL(diff, diff);
	    compl = MFCCMUL(sqdiff, *var);
	    d = GMMSUB(d,compl);
	    ++var;
	}
	topn[i].val.dist = (int32)d;
	if (i == 0) continue;
	vtmp = topn[i];
	for (j = i-1; j >= 0 && (int32)d > topn[j].val.dist; j--) {
	    topn[j+1] = topn[j];
	}
	topn[j+1] = vtmp;
    }
    if (searchCurrentFrame() % frame_ds_ratio)
	    return;
    if (node) {
	uint32 maxbbi = kd_maxbbi == -1 ? node->n_bbi : MIN(node->n_bbi,kd_maxbbi);
	for (i = 0; i < maxbbi; ++i) {
	    cw = node->bbi[i];
	    mean = means[(int32)CEP_FEAT] + cw*CEP_VECLEN + 1;
	    var  =  vars[(int32)CEP_FEAT] + cw*CEP_VECLEN + 1;
	    d = det[cw];
	    obs = z+1;
	    for (j = 1; (j < CEP_VECLEN) && (d >= worst->val.dist); j++) {
		diff = *obs++ - *mean++;
		sqdiff = MFCCMUL(diff, diff);
		compl = MFCCMUL(sqdiff, *var);
		d = GMMSUB(d,compl);
		++var;
	    }
	    if (j < CEP_VECLEN)
		continue;
	    if (d < worst->val.dist) continue;
	    for (k = 0; k < topN; k++) {
		/* already there, so don't need to insert */
		if (topn[k].codeword == cw) break;
	    }
	    if (k < topN) continue;	/* already there.  Don't insert */
	    /* remaining code inserts codeword and dist in correct spot */
	    for (cur = worst-1; cur >= best && d >= cur->val.dist; --cur)
		memcpy (cur+1, cur, sizeof(vqFeature_t));
	    ++cur;
	    cur->codeword = cw;
	    cur->val.dist = (int32)d;
	}
    }
    else {
	mean = means[(int32)CEP_FEAT];
	var = vars[(int32)CEP_FEAT];
	for (detP = det, ++mean, ++var; detP < detE; detP++, ++mean, ++var) {
	    d = *detP; obs = z+1;
	    cw = detP - det;
	    for (j = 1; (j < CEP_VECLEN) && (d >= worst->val.dist); j++) {
		diff = *obs++ - *mean++;
		sqdiff = MFCCMUL(diff, diff);
		compl = MFCCMUL(sqdiff, *var);
		d = GMMSUB(d,compl);
		++var;
	    }
	    if (j < CEP_VECLEN) {
		/* terminated early, so not in topn */
		mean += (CEP_VECLEN - j);
		var  += (CEP_VECLEN - j);
		continue;
	    }
	    if (d < worst->val.dist) continue;
	    for (i = 0; i < topN; i++) {
		/* already there, so don't need to insert */
		if (topn[i].codeword == cw) break;
	    }
	    if (i < topN) continue;	/* already there.  Don't insert */
	    /* remaining code inserts codeword and dist in correct spot */
	    for (cur = worst-1; cur >= best && d >= cur->val.dist; --cur)
		memcpy (cur+1, cur, sizeof(vqFeature_t));
	    ++cur;
	    cur->codeword = cw;
	    cur->val.dist = (int32)d;
	}
    }

    memcpy(lcfrm, topn, sizeof(vqFeature_t)*topN);
}

#if 0
static void cepDist3(vqFeature_t *topn, mfcc_t *z)
/*------------------------------------------------------------*
 * Identical functionality with respect to cepDist0, but faster
 * since the input vector is loaded into registers.
 */
{
    register int32	cw;
    long		i, j;
    vqFeature_t		*bottom, *top, *cur;
    mean_t		diff;
    var_t		d, worst_dist;
    mean_t		*mean;
    var_t		*var  =  vars[(int32)CEP_FEAT];
    int32			*det  =  dets[(int32)CEP_FEAT], *detP;
    int32			*detE = det + NUM_ALPHABET;
    mfcc_t		c1 = z[1],
			c2 = z[2],
			c3 = z[3],
			c4 = z[4],
			c5 = z[5],
			c6 = z[6],
			c7 = z[7],
			c8 = z[8],
			c9 = z[9],
			c10 = z[10],
			c11 = z[11],
			c12 = z[12];

    assert(z    != NULL);
    assert(topn != NULL);

#ifdef FIXED_POINT
    worst_dist = INT_MAX;
#else
    worst_dist = 1e+300;
#endif
    top = &topn[0];
    bottom = &topn[topN-1];
    /*
     * initialize worst score
     */
    for (i = 0; i < topN; i++) {
	cw = lcfrm[i].codeword;
	mean = means[(int32)CEP_FEAT] + cw*CEP_VECLEN;
	var  =  vars[(int32)CEP_FEAT] + cw*CEP_VECLEN;
	d = det[cw];

	diff = c1 - mean[1];
	d = GMMSUB(d,MFCCMUL(MFCCMUL(diff, diff), var[1]));
	diff = c2 - mean[2];
	d = GMMSUB(d,MFCCMUL(MFCCMUL(diff, diff), var[2]));
	diff = c3 - mean[3];
	d = GMMSUB(d,MFCCMUL(MFCCMUL(diff, diff), var[3]));
	diff = c4 - mean[4];
	d = GMMSUB(d,MFCCMUL(MFCCMUL(diff, diff), var[4]));
	diff = c5 - mean[5];
	d = GMMSUB(d,MFCCMUL(MFCCMUL(diff, diff), var[5]));
	diff = c6 - mean[6];
	d = GMMSUB(d,MFCCMUL(MFCCMUL(diff, diff), var[6]));
	diff = c7 - mean[7];
	d = GMMSUB(d,MFCCMUL(MFCCMUL(diff, diff), var[7]));
	diff = c8 - mean[8];
	d = GMMSUB(d,MFCCMUL(MFCCMUL(diff, diff), var[8]));
	diff = c9 - mean[9];
	d = GMMSUB(d,MFCCMUL(MFCCMUL(diff, diff), var[9]));
	diff = c10 - mean[10];
	d = GMMSUB(d,MFCCMUL(MFCCMUL(diff, diff), var[10]));
	diff = c11 - mean[11];
	d = GMMSUB(d,MFCCMUL(MFCCMUL(diff, diff), var[11]));
	diff = c12 - mean[12];
	d = GMMSUB(d,MFCCMUL(MFCCMUL(diff, diff), var[12]));

	if (d < worst_dist)
	    worst_dist = d;
    }

    /*
     * Worst score must be small enough to guarentee that
     * the top 4 scores for this frame are better than worst_score.
     */
    for (i = 0; i < topN; i++) {
	topn[i].val.dist = worst_dist - 3;
	topn[i].codeword = lcfrm[i].codeword;
    }

    mean = means[(int32)CEP_FEAT];
    var = vars[(int32)CEP_FEAT];
    for (detP = det; detP < detE; detP++, mean += CEP_VECLEN, var += CEP_VECLEN) {
#ifdef FIXED_POINT
#define int_d d
#else
	int32 int_d;
#endif

	d = *detP;

	diff = c1 - mean[1];
	d = GMMSUB(d,MFCCMUL(MFCCMUL(diff, diff), var[1]));
	diff = c2 - mean[2];
	d = GMMSUB(d,MFCCMUL(MFCCMUL(diff, diff), var[2]));
	diff = c3 - mean[3];
	d = GMMSUB(d,MFCCMUL(MFCCMUL(diff, diff), var[3]));
	diff = c4 - mean[4];
	d = GMMSUB(d,MFCCMUL(MFCCMUL(diff, diff), var[4]));
	if (d < worst_dist)
	    continue;
	diff = c5 - mean[5];
	d = GMMSUB(d,MFCCMUL(MFCCMUL(diff, diff), var[5]));
	diff = c6 - mean[6];
	d = GMMSUB(d,MFCCMUL(MFCCMUL(diff, diff), var[6]));
	diff = c7 - mean[7];
	d = GMMSUB(d,MFCCMUL(MFCCMUL(diff, diff), var[7]));
	diff = c8 - mean[8];
	d = GMMSUB(d,MFCCMUL(MFCCMUL(diff, diff), var[8]));
	if (d < worst_dist)
	    continue;
	diff = c9 - mean[9];
	d = GMMSUB(d,MFCCMUL(MFCCMUL(diff, diff), var[9]));
	diff = c10 - mean[10];
	d = GMMSUB(d,MFCCMUL(MFCCMUL(diff, diff), var[10]));
	diff = c11 - mean[11];
	d = GMMSUB(d,MFCCMUL(MFCCMUL(diff, diff), var[11]));
	diff = c12 - mean[12];
	d = GMMSUB(d,MFCCMUL(MFCCMUL(diff, diff), var[12]));
	if (d < worst_dist)
	    continue;

	cw = detP - det;

	/* 
	 * remaining code inserts codeword and dist in correct spot
	 */
#ifndef FIXED_POINT
	int_d = (int32)(d - 0.5);
#endif
	for (cur = bottom-1; cur >= top && int_d >= cur->val.dist; --cur) {
	    cur[1].codeword = cur[0].codeword;
	    cur[1].val.dist = cur[0].val.dist;
	}
	++cur;
	cur->codeword = cw;
	cur->val.dist = int_d;
	worst_dist = (var_t) bottom->val.dist;
    }

    memcpy (lcfrm, topn, sizeof(vqFeature_t)*topN);
}
#endif

static void dcepDist0(vqFeature_t *topn, mfcc_t *dzs, mfcc_t *dzl)
{
    register int32	i, j, k, cw;
    vqFeature_t		*worst, *best = topn, *cur; /* , *src; */
    mean_t		diff, sqdiff, compl;
    mfcc_t		*obs1, *obs2;
    mean_t		*mean;
    var_t		*var;
    int32		*det  =  dets[(int32)DCEP_FEAT], *detP;
    int32		*detE = det + NUM_ALPHABET; /*, tmp; */
    var_t		d;
    kd_tree_node_t	*node;

    assert(dzs != NULL);
    assert(dzl != NULL);
    assert(topn != NULL);

    if (kdtrees) {
	    mfcc_t dceps[(CEP_VECLEN-1)*2];
	    memcpy(dceps, dzs+1, (CEP_VECLEN-1) * sizeof(*dzs));
	    memcpy(dceps+CEP_VECLEN-1, dzl+1, (CEP_VECLEN-1) * sizeof(*dzl));
	    node = eval_kd_tree(kdtrees[(int32)DCEP_FEAT], dceps, kd_maxdepth);
    }
    else
	    node = NULL;

    memcpy (topn, ldfrm, sizeof(vqFeature_t)*topN);
    worst = topn + (topN-1);
    /* initialize topn codewords to topn codewords from previous frame */
    for (i = 0; i < topN; i++) {
	cw = topn[i].codeword;
	mean = means[(int32)DCEP_FEAT] + cw*DCEP_VECLEN + 1;
	var  =  vars[(int32)DCEP_FEAT] + cw*DCEP_VECLEN + 1;
	d = det[cw];
	obs1 = dzs+1;
	obs2 = dzl+1;
	for (j = 1; j < CEP_VECLEN; j++, obs1++, obs2++, mean++, var++) {
	    diff = *obs1 - *mean;
	    sqdiff = MFCCMUL(diff, diff);
	    compl = MFCCMUL(sqdiff, *var);
	    d = GMMSUB(d,compl);
	    diff = MFCCMUL((*obs2 - mean[CEP_VECLEN-1]), dcep80msWeight);
	    sqdiff = MFCCMUL(diff, diff);
	    compl = MFCCMUL(sqdiff, var[CEP_VECLEN-1]);
	    d = GMMSUB(d,compl);
	}
	topn[i].val.dist = (int32)d;
	if (i == 0) continue;
	vtmp = topn[i];
	for (j = i-1; j >= 0 && (int32)d > topn[j].val.dist; j--) {
	    topn[j+1] = topn[j];
	}
	topn[j+1] = vtmp;
    }
    if (searchCurrentFrame() % frame_ds_ratio)
	return;
    if (node) {
	uint32 maxbbi = kd_maxbbi == -1 ? node->n_bbi : MIN(node->n_bbi,kd_maxbbi);
	for (i = 0; i < maxbbi; ++i) {
	    cw = node->bbi[i];
	    mean = means[(int32)DCEP_FEAT] + cw*DCEP_VECLEN + 1;
	    var  =  vars[(int32)DCEP_FEAT] + cw*DCEP_VECLEN + 1;
	    d = det[cw];
	    obs1 = dzs+1;
	    obs2 = dzl+1;
	    for (j = 1; j < CEP_VECLEN && (d NEARER worst->val.dist);
		 j++, obs1++, obs2++, mean++, var++) {
		diff = *obs1 - *mean;
		sqdiff = MFCCMUL(diff, diff);
		compl = MFCCMUL(sqdiff, *var);
		d = GMMSUB(d,compl);
		diff = MFCCMUL((*obs2 - mean[CEP_VECLEN-1]), dcep80msWeight);
		sqdiff = MFCCMUL(diff, diff);
		compl = MFCCMUL(sqdiff, var[CEP_VECLEN-1]);
		d = GMMSUB(d,compl);
	    }
	    if (j < CEP_VECLEN)
		continue;
	    if (d < worst->val.dist) continue;
	    for (k = 0; k < topN; k++) {
		/* already there, so don't need to insert */
		if (topn[k].codeword == cw) break;
	    }
	    if (k < topN) continue;	/* already there.  Don't insert */
	    /* remaining code inserts codeword and dist in correct spot */
	    for (cur = worst-1; cur >= best && (int32)d >= cur->val.dist; --cur)
		memcpy(cur+1, cur, sizeof(vqFeature_t));
	    ++cur;
	    cur->codeword = cw;
	    cur->val.dist = (int32)d;
	}
    }
    else {
	mean = means[(int32)DCEP_FEAT];
	var = vars[(int32)DCEP_FEAT];
	/* one distance value for each codeword */
	for (detP = det; detP < detE; detP++) {
	    d = *detP;
	    obs1 = dzs+1;		/* reset observed */
	    obs2 = dzl+1;		/* reset observed */
	    cw = detP - det;
	    for (j = 1, ++mean, ++var;
		 (j < CEP_VECLEN) && (d NEARER worst->val.dist);
		 j++, obs1++, obs2++, mean++, var++) {
		diff = *obs1 - *mean;
		sqdiff = MFCCMUL(diff, diff);
		compl = MFCCMUL(sqdiff, *var);
		d = GMMSUB(d,compl);
		diff = MFCCMUL((*obs2 - mean[CEP_VECLEN-1]), dcep80msWeight);
		sqdiff = MFCCMUL(diff, diff);
		compl = MFCCMUL(sqdiff, var[CEP_VECLEN-1]);
		d = GMMSUB(d,compl);
	    }
	    mean += CEP_VECLEN-1;
	    var += CEP_VECLEN-1;
	    if (j < CEP_VECLEN) {
		mean += (CEP_VECLEN-j);	
		var  += (CEP_VECLEN-j);
		continue;
	    }
	    if (d < worst->val.dist) continue;
	    for (i = 0; i < topN; i++) {
		/* already there, so don't need to insert */
		if (topn[i].codeword == cw) break;
	    }
	    if (i < topN) continue;	/* already there.  Don't insert */
	    /* remaining code inserts codeword and dist in correct spot */
	    for (cur = worst-1; cur >= best && (int32)d >= cur->val.dist; --cur)
		memcpy(cur+1, cur, sizeof(vqFeature_t));
	    ++cur;
	    cur->codeword = cw;
	    cur->val.dist = (int32)d;
	}
    }
    memcpy (ldfrm, topn, sizeof(vqFeature_t)*topN);
}

static void ddcepDist0(vqFeature_t *topn, mfcc_t *z)
{
    register int32 i, j, k, cw;
    vqFeature_t		*worst, *best = topn, *cur; /*, *src; */
    mean_t		diff, sqdiff, compl;
    mfcc_t		*obs;
    mean_t		*mean;
    var_t		*var;
    int32		*det  =  dets[(int32)DDCEP_FEAT];
    int32		*detE = det + NUM_ALPHABET;
    int32		*detP; /*, tmp; */
    var_t		d;
    kd_tree_node_t	*node;

    assert(z    != NULL);
    assert(topn != NULL);

    if (kdtrees)
	    node = eval_kd_tree(kdtrees[(int32)DDCEP_FEAT], z+1, kd_maxdepth);
    else
	    node = NULL;

    memcpy (topn, lxfrm, sizeof(vqFeature_t)*topN);
    worst = topn + (topN-1);
    /* initialize topn codewords to topn codewords from previous frame */
    for (i = 0; i < topN; i++) {
	cw = topn[i].codeword;
	mean = means[(int32)DDCEP_FEAT] + cw*CEP_VECLEN + 1;
	var  =  vars[(int32)DDCEP_FEAT] + cw*CEP_VECLEN + 1;
	d = det[cw];
	obs = z+1;
	for (j = 1; j < CEP_VECLEN; j++) {
	    diff = *obs++ - *mean++;
	    sqdiff = MFCCMUL(diff, diff);
	    compl = MFCCMUL(sqdiff, *var);
	    d = GMMSUB(d,compl);
	    ++var;
	}
	topn[i].val.dist = (int32)d;
	if (i == 0) continue;
	vtmp = topn[i];
	for (j = i-1; j >= 0 && (int32)d > topn[j].val.dist; j--) {
	    topn[j+1] = topn[j];
	}
	topn[j+1] = vtmp;
    }
    if (searchCurrentFrame() % frame_ds_ratio)
	return;
    if (node) {
	uint32 maxbbi = kd_maxbbi == -1 ? node->n_bbi : MIN(node->n_bbi,kd_maxbbi);
	for (i = 0; i < maxbbi; ++i) {
	    cw = node->bbi[i];
	    mean = means[(int32)DDCEP_FEAT] + cw*CEP_VECLEN + 1;
	    var  =  vars[(int32)DDCEP_FEAT] + cw*CEP_VECLEN + 1;
	    d = det[cw];
	    obs = z+1;
	    for (j = 1; (j < CEP_VECLEN) && (d >= worst->val.dist); j++) {
		diff = *obs++ - *mean++;
		sqdiff = MFCCMUL(diff, diff);
		compl = MFCCMUL(sqdiff, *var);
		d = GMMSUB(d,compl);
		++var;
	    }
	    if (j < CEP_VECLEN)
		continue;
	    if (d < worst->val.dist) continue;
	    for (k = 0; k < topN; k++) {
		/* already there, so don't need to insert */
		if (topn[k].codeword == cw) break;
	    }
	    if (k < topN) continue;	/* already there.  Don't insert */
	    /* remaining code inserts codeword and dist in correct spot */
	    for (cur = worst-1; cur >= best && (int32)d >= cur->val.dist; --cur)
		memcpy(cur+1, cur, sizeof(vqFeature_t));
	    ++cur;
	    cur->codeword = cw;
	    cur->val.dist = (int32)d;
	}
    }
    else {
	mean = means[(int32)DDCEP_FEAT];
	var = vars[(int32)DDCEP_FEAT];
	for (detP = det, ++mean, ++var; detP < detE; detP++, ++mean, ++var) {
	    d = *detP; obs = z+1;
	    cw = detP - det;
	    for (j = 1; (j < CEP_VECLEN) && (d >= worst->val.dist); j++) {
		diff = *obs++ - *mean++;
		sqdiff = MFCCMUL(diff, diff);
		compl = MFCCMUL(sqdiff, *var);
		d = GMMSUB(d,compl);
		++var;
	    }
	    if (j < CEP_VECLEN) {
		/* terminated early, so not in topn */
		mean += (CEP_VECLEN - j);
		var  += (CEP_VECLEN - j);
		continue;
	    }
	    if (d < worst->val.dist) continue;

	    for (i = 0; i < topN; i++) {
		/* already there, so don't need to insert */
		if (topn[i].codeword == cw) break;
	    }
	    if (i < topN) continue;	/* already there.  Don't insert */
	    /* remaining code inserts codeword and dist in correct spot */
	    for (cur = worst-1; cur >= best && (int32)d >= cur->val.dist; --cur)
		memcpy(cur+1, cur, sizeof(vqFeature_t));
	    ++cur;
	    cur->codeword = cw;
	    cur->val.dist = (int32)d;
	}
    }

    memcpy(lxfrm, topn, sizeof(vqFeature_t)*topN);
}

static void powDist(vqFeature_t *topn, mfcc_t *pz)
{
  register int32	i, j, cw;
  var_t			nextBest;
  var_t			dist[NUM_ALPHABET];
  mean_t		diff, sqdiff, compl;
  var_t			*dP, *dE = dist + NUM_ALPHABET;
  mfcc_t		*obs;
  mean_t		*mean = means[(int32)POW_FEAT];
  var_t			*var  =  vars[(int32)POW_FEAT];
  int32			*det  =  dets[(int32)POW_FEAT];
  var_t			d;

  if (searchCurrentFrame() % frame_ds_ratio)
	  return;

  assert(pz  != NULL);
  assert(topn != NULL);

  /* one distance value for each codeword */
  for (dP = dist; dP < dE; dP++) {
    cw = dP - dist;
    d = 0; obs = pz;
    for (j = 0; j < POW_VECLEN; j++, obs++, mean++, var++) {
      diff = *obs - *mean;
      sqdiff = MFCCMUL(diff, diff);
      compl = MFCCMUL(sqdiff, *var);
      d = GMMADD(d, compl);
    }
    *dP = *det++ - d;
  }
  /* compute top N codewords */
  for (i = 0; i < topN; i++) {
    dP = dist;
    nextBest = *dP++; cw = 0;
    for (; dP < dE; dP++) {
      if (*dP NEARER nextBest) {
	nextBest = *dP;
	cw = dP - dist;
      }
    }
    topn[i].codeword = cw;
    topn[i].val.dist = (int32)nextBest;

    dist[cw] = WORST_DIST;
  }
}

/* the cepstrum circular buffer */
static mfcc_t  inBufArr[CEP_VECLEN*(INPUT_MASK+1)];
static int32    inIdx = 0;

/* the diff cepstrum circular buffer */
static mfcc_t  dBufArr[CEP_VECLEN*(DIFF_MASK+1)];
static int32    dIdx = 0;

void SCVQInit(int32 top, int32 numModels, int32 numDist, double vFloor, 
	      int32 use20msdp)
{
    int32 i;

    assert((top <= MAX_TOPN) && (top > 0));
    assert(numModels > 0);
    assert(numDist > 0);
    assert(vFloor >= 0.0);

    use20ms_diff_pow = use20msdp;

    for (i = 0; i < MAX_TOPN; i++) {
	lxfrm[i].val.dist = ldfrm[i].val.dist = lcfrm[i].val.dist = WORST_DIST;
	lxfrm[i].codeword = ldfrm[i].codeword = lcfrm[i].codeword = i;
    }
    E_INFO("SCVQInit: top %d, %d models, %d dist, %f var floor.\n",
	     top, numModels, numDist, vFloor);
    /*
     * Configure TopN
     */
    topN = top;

    CdWdPDFMod = numModels * numDist; /* # prob values per codeword */
    
    if ((scrPass    = (int32 *)calloc(CdWdPDFMod, sizeof(int32))) == NULL)
	E_FATAL("calloc(%d,%d) failed\n", CdWdPDFMod, sizeof(int32));
    
    setVarFloor(vFloor);	/* see sc_cbook_r.c */

#if defined(WIN32) && !defined(GNUWINCE) && !defined(CYGWIN)
    pid = GetCurrentProcess();
#endif
}

void SCVQNewUtt(void)
{
    num_frames = 0;

#if defined(WIN32) && !defined(GNUWINCE) && !defined(CYGWIN)
    vq_time = scr_time = 0.0;
#endif
}

void SCVQEndUtt ( void )
{
#if defined(WIN32) && !defined(GNUWINCE) && !defined(CYGWIN)
    E_INFO ("VQ-TIME= %.1fsec, SCR-TIME= %.1fsec (CPU)\n", vq_time, scr_time);
#endif
}

int SCVQComputeFeatures(mfcc_t **cep,
			mfcc_t **dcep,
			mfcc_t **dcep_80ms,
			mfcc_t **pow,
			mfcc_t **ddcep,
			mfcc_t *in)
{
    register int32 i; /*, j; */
    register mfcc_t *df, *db, *dout;
    static mfcc_t	ldBufArr[CEP_VECLEN];
    static mfcc_t	ddBufArr[CEP_VECLEN];
    static mfcc_t	pBufArr[POW_VECLEN];
    /* int32		tmp[NUM_FEATURES]; */

    memcpy((char *)(inBufArr + inIdx*CEP_VECLEN), (char *)in,
	  sizeof(mfcc_t)*CEP_VECLEN);

    /* compute short duration difference */
    dout = dBufArr + dIdx*CEP_VECLEN;
    df  = inBufArr + inIdx*CEP_VECLEN;
    db  = inBufArr + INPUT_INDEX(inIdx-4)*CEP_VECLEN;
    i  = CEP_VECLEN;
    do {
	*dout++ = (*df++ - *db++);
    } while (--i);

    /* compute long duration difference */
    dout = ldBufArr;
    df = inBufArr + inIdx*CEP_VECLEN;
    db = inBufArr + INPUT_INDEX(inIdx-8)*CEP_VECLEN;
    i  = CEP_VECLEN;
    do {
	*dout++ = (*df++ - *db++);
    } while (--i);

    /* compute 2nd order difference */
    dout = ddBufArr;
    df = dBufArr + DIFF_INDEX(dIdx-1)*CEP_VECLEN;
    db = dBufArr + DIFF_INDEX(dIdx-3)*CEP_VECLEN;
    i  = CEP_VECLEN;
    do {
	*dout++ = (*df++ - *db++);
    } while (--i);

    /* pow , diff pow, and 2nd diff pow */
    if (use20ms_diff_pow)
	pBufArr[0] = *(inBufArr + INPUT_INDEX(inIdx-3)*CEP_VECLEN) -
		     *(inBufArr + INPUT_INDEX(inIdx-5)*CEP_VECLEN);
    else
	pBufArr[0] = *(inBufArr + INPUT_INDEX(inIdx-4)*CEP_VECLEN);
    pBufArr[1] = *(dBufArr  +  DIFF_INDEX( dIdx-2)*CEP_VECLEN);
    pBufArr[2] = *ddBufArr;

    *cep  = inBufArr + INPUT_INDEX(inIdx-4)*CEP_VECLEN;
	
    *dcep = dBufArr + DIFF_INDEX(dIdx-2)*CEP_VECLEN;
    
    *dcep_80ms = ldBufArr;

    *pow = pBufArr;

    *ddcep = ddBufArr;

    ++num_frames;

    inIdx = INPUT_INDEX(inIdx+1);
    dIdx  =  DIFF_INDEX(dIdx+1);

    return (num_frames  >= MAX_DIFF_WINDOW);
}

/* Output PDF/feature (32-bit logprobs) */
int32 *OPDF[NUM_FEATURES] = {NULL, NULL, NULL, NULL};
OPDF_8BIT_T *OPDF_8B[NUM_FEATURES] = {NULL, NULL, NULL, NULL};

/* Senone prob size (bits).  Can be 8/32 */
static int32 prob_size = 32;


/*
 * Compute senone scores.
 */
static void SCVQComputeScores (int32 *scores, vqFeature_t frm[][MAX_TOPN]);
static void SCVQComputeScores_all (int32 *scores, vqFeature_t frm[][MAX_TOPN]);


/*
 * Optimization for various topN cases, PDF-size(#bits) cases of
 * SCVQCoomputeScores() and SCVQCoomputeScores_all().
 */
static void get_scores4 (int32 *scores, vqFeature_t frm[][MAX_TOPN]);
static void get_scores1 (int32 *scores, vqFeature_t frm[][MAX_TOPN]);
static void get_scores (int32 *scores, vqFeature_t frm[][MAX_TOPN]);
static void get_scores4_all (int32 *scores, vqFeature_t frm[][MAX_TOPN]);
static void get_scores1_all (int32 *scores, vqFeature_t frm[][MAX_TOPN]);
static void get_scores_all (int32 *scores, vqFeature_t frm[][MAX_TOPN]);
static void get_scores4_8b (int32 *scores, vqFeature_t frm[][MAX_TOPN]);
static void get_scores2_8b (int32 *scores, vqFeature_t frm[][MAX_TOPN]);
static void get_scores1_8b (int32 *scores, vqFeature_t frm[][MAX_TOPN]);
static void get_scores_8b (int32 *scores, vqFeature_t frm[][MAX_TOPN]);
static void get_scores4_8b_all (int32 *scores, vqFeature_t frm[][MAX_TOPN]);
static void get_scores2_8b_all (int32 *scores, vqFeature_t frm[][MAX_TOPN]);
static void get_scores1_8b_all (int32 *scores, vqFeature_t frm[][MAX_TOPN]);
static void get_scores_8b_all (int32 *scores, vqFeature_t frm[][MAX_TOPN]);


/*
 * Array (list) of active senones in the current frame, and the active list
 * size.  Array allocated and maintained by the search module.
 * (Extern is ugly, but there it is, for now (rkm@cs).)
 */
extern int32 *senone_active;
extern int32 n_senone_active;


/*
 * Compute senone scores for the active senones.
 */
void SCVQScores (int32 *scores,
		 mfcc_t *cep,
		 mfcc_t *dcep,
		 mfcc_t *dcep_80ms,
		 mfcc_t *pcep,
		 mfcc_t *ddcep)
{
    static vqFeature_t f[NUM_FEATURES][MAX_TOPN];
    int	      i, j;
    int32     tmp[NUM_FEATURES];


#if defined(WIN32) && !defined(GNUWINCE) && !defined(CYGWIN)
    GetProcessTimes (pid, &t_create, &t_exit, &kst, &ust);
#endif

    cepDist0(f[(int32) CEP_FEAT], cep);

    dcepDist0(f[(int32) DCEP_FEAT], dcep, dcep_80ms);

    powDist(f[(int32) POW_FEAT], pcep);

    ddcepDist0(f[(int32) DDCEP_FEAT], ddcep);

    /* normalize the topN feature scores */
    for (j = 0; j < NUM_FEATURES; j++) {
	tmp[j] = f[j][0].val.score;
    }
    for (i = 1; i < topN; i++)
	for (j = 0; j < NUM_FEATURES; j++) {
	    tmp[j] = ADD(tmp[j], f[j][i].val.score);
	}
    for (i = 0; i < topN; i++)
	for (j = 0; j < NUM_FEATURES; j++) {
	    f[j][i].val.score -= tmp[j];
	    if (f[j][i].val.score > 0)
		f[j][i].val.score = INT_MIN; /* tkharris++ */
	    /* E_FATAL("**ERROR** VQ score= %d\n", f[j][i].val.score); */
	}

#if defined(WIN32) && !defined(GNUWINCE) && !defined(CYGWIN)
    GetProcessTimes (pid, &t_create, &t_exit, &ket, &uet);
    vq_time += win32_cputime (&ust, &uet);
#endif

    SCVQComputeScores(scores, f);
    
#if defined(WIN32) && !defined(GNUWINCE) && !defined(CYGWIN)
    GetProcessTimes (pid, &t_create, &t_exit, &kst, &ust);
    scr_time += win32_cputime (&uet, &ust);
#endif
}


/*
 * Compute scores for all senones.
 */
void SCVQScores_all (int32 *scores,
		     mfcc_t *cep,
		     mfcc_t *dcep,
		     mfcc_t *dcep_80ms,
		     mfcc_t *pcep,
		     mfcc_t *ddcep)
{
    static vqFeature_t f[NUM_FEATURES][MAX_TOPN];
    int	      i, j;
    int32     tmp[NUM_FEATURES];

#if defined(WIN32) && !defined(GNUWINCE) && !defined(CYGWIN)
    GetProcessTimes (pid, &t_create, &t_exit, &kst, &ust);
#endif

    cepDist0(f[(int32) CEP_FEAT], cep);
	     
    dcepDist0(f[(int32) DCEP_FEAT], dcep, dcep_80ms);
    
    powDist(f[(int32) POW_FEAT], pcep);

    ddcepDist0(f[(int32) DDCEP_FEAT], ddcep);

    /* normalize the topN feature scores */
    for (j = 0; j < NUM_FEATURES; j++) {
	tmp[j] = f[j][0].val.score;
    }
    for (i = 1; i < topN; i++)
	for (j = 0; j < NUM_FEATURES; j++) {
	    tmp[j] = ADD(tmp[j], f[j][i].val.score);
	}
    for (i = 0; i < topN; i++)
	for (j = 0; j < NUM_FEATURES; j++) {
	    f[j][i].val.score -= tmp[j];
	    if (f[j][i].val.score > 0)
	      f[j][i].val.score = INT_MIN; /* tkharris++ */
	    /* E_FATAL("**ERROR** VQ score= %d\n", f[j][i].val.score); */
	}
    
#if defined(WIN32) && !defined(GNUWINCE) && !defined(CYGWIN)
    GetProcessTimes (pid, &t_create, &t_exit, &ket, &uet);
    vq_time += win32_cputime (&ust, &uet);
#endif

    SCVQComputeScores_all (scores, f);
    
#if defined(WIN32) && !defined(GNUWINCE) && !defined(CYGWIN)
    GetProcessTimes (pid, &t_create, &t_exit, &kst, &ust);
    scr_time += win32_cputime (&uet, &ust);
#endif
}


static void SCVQComputeScores(int32 *scores, vqFeature_t frm[][MAX_TOPN])
{
    if (prob_size == 8) {
	switch (topN) {
	case 4:	 get_scores4_8b(scores, frm); break;
	case 2:	 get_scores2_8b(scores, frm);  break;
	case 1:	 get_scores1_8b(scores, frm);  break;
	default: get_scores_8b(scores, frm);   break;
	}
    } else {	/* 32 bit PDFs */
	switch (topN) {
	case 4:	 get_scores4(scores, frm); break;
	case 1:	 get_scores1(scores, frm);  break;
	default: get_scores(scores, frm);   break;
	}
    }
}

static void SCVQComputeScores_all(int32 *scores, vqFeature_t frm[][MAX_TOPN])
{
    if (prob_size == 8) {
	switch (topN) {
	case 4:	 get_scores4_8b_all(scores, frm); break;
	case 2:	 get_scores2_8b_all(scores, frm); break;
	case 1:	 get_scores1_8b_all(scores, frm);  break;
	default: get_scores_8b_all(scores, frm); break;
	}
    } else {
	switch (topN) {
	case 4:	 get_scores4_all(scores, frm); break;
	case 1:	 get_scores1_all(scores, frm);  break;
	default: get_scores_all(scores, frm);   break;
	}
    }
}


static void get_scores (int32 *scores, vqFeature_t frm[][MAX_TOPN])
{
    get_scores_all (scores, frm);
}

static void get_scores_all (int32 *scores, vqFeature_t frm[][MAX_TOPN])
{
    long  i, j, k;
    int32 tmp1, tmp2;		/* for distribution score comp. */
    int32 *pdf;			/* current view into output pdfs */
    int32 *scr;			/* observation score */
    int32 *ascr;
    int32 **opdf = OPDF;
    register int32   ts = Table_Size;
    register int16 *at = Addition_Table;
    
    n_senone_active = CdWdPDFMod;
    
    for (j = 0; j < NUM_FEATURES; j++) {
	for (i = 0; i < topN; i++) {
	    frm[j][i].codeword *= CdWdPDFMod;
	}
    }

    for (j = 0; j < NUM_FEATURES; j++) {
       /*
        *  Assumes that codeword is premultiplyed by CdWdPDFMod
        */
	pdf = *opdf + frm[j]->codeword;

	scr = scrPass;
	tmp1 = frm[j]->val.score;
	/*
	 * Compute Score for top 1
	 */
	for (k = CdWdPDFMod; k > 0; k--) {
	    *scr = *pdf + tmp1;
	    scr++; pdf++;
	}
	for (i = 1; i < topN; i++) { /* for each next ranking codeword */
	    /*
             *  Assumes that codeword is premultiplyed by CdWdPDFMod
             */
	    scr = scrPass;
	    tmp1 = (frm[j]+i)->val.score;
	    pdf = *opdf + (frm[j]+i)->codeword;
	    for (k = CdWdPDFMod; k > 0; k--) {
		tmp2 =  *pdf + tmp1;
		FAST_ADD (*scr, *scr, tmp2, at, ts);
		scr++; pdf++;
	    }
	}
	ascr = scores;
	scr  = scrPass;
	if (j == 0) {
	    for (k = CdWdPDFMod; k > 0; k--)
		*ascr++ = *scr++;
	}
	else {
	    for (k = CdWdPDFMod; k > 0; k--, ascr++, scr++) {
		*ascr += *scr;
	    }
	}
	++opdf;
    }
}


static void get_scores4(int32 *scores, vqFeature_t frm[][MAX_TOPN])
{
    register int32 i, j, k, n;
    int32 tmp1, tmp2;				/* for score comp. */
    int32 *pdf0, *pdf1, *pdf2, *pdf3;		/* pdf pointers */
    int32 *scr;
    int32 w0, w1, w2, w3;			/* weights */
    register int32   ts = Table_Size;
    register int16 *at = Addition_Table;

    for (j = 0; j < NUM_FEATURES; j++) {
	for (k = 0; k < topN; k++) {
	    frm[j][k].codeword *= CdWdPDFMod;
	}
    }

    /*
     *  Assumes that codeword is premultiplyed by CdWdPDFMod
     */
    pdf0 = OPDF[0] + frm[0][0].codeword;
    pdf1 = OPDF[0] + frm[0][1].codeword;
    pdf2 = OPDF[0] + frm[0][2].codeword;
    pdf3 = OPDF[0] + frm[0][3].codeword;
    
    w0 = frm[0][0].val.score;
    w1 = frm[0][1].val.score;
    w2 = frm[0][2].val.score;
    w3 = frm[0][3].val.score;

    scr = scores;
    
    for (i = 0; i < n_senone_active; i++) {
	k = senone_active[i];

	tmp1 = pdf0[k] + w0;
	tmp2 = pdf1[k] + w1;
	FAST_ADD (tmp1, tmp1, tmp2, at, ts);
	tmp2 = pdf2[k] + w2;
	FAST_ADD (tmp1, tmp1, tmp2, at, ts);
	tmp2 = pdf3[k] + w3;
	FAST_ADD (tmp1, tmp1, tmp2, at, ts);
	scr[k] = tmp1;
    }
    
    for (j = 1; j < NUM_FEATURES; j++) {
        /*
         *  Assumes that codeword is premultiplyed by CdWdPDFMod
         */
	pdf0 = OPDF[j] + frm[j][0].codeword;
	pdf1 = OPDF[j] + frm[j][1].codeword;
	pdf2 = OPDF[j] + frm[j][2].codeword;
	pdf3 = OPDF[j] + frm[j][3].codeword;

	w0 = frm[j][0].val.score;
	w1 = frm[j][1].val.score;
	w2 = frm[j][2].val.score;
	w3 = frm[j][3].val.score;

        scr = scores;
	for (k = 0; k < n_senone_active; k++) {
	    n = senone_active[k];

	    tmp1 = pdf0[n] + w0;
	    tmp2 = pdf1[n] + w1;
	    FAST_ADD (tmp1, tmp1, tmp2, at, ts);
	    tmp2 = pdf2[n] + w2;
	    FAST_ADD (tmp1, tmp1, tmp2, at, ts);
	    tmp2 = pdf3[n] + w3;
	    FAST_ADD (tmp1, tmp1, tmp2, at, ts);
	    scr[n] += tmp1;
	}
    }
}

static void get_scores4_all (int32 *scores, vqFeature_t frm[][MAX_TOPN])
{
    int32 i;
    
    for (i = 0; i < CdWdPDFMod; i++)
	senone_active[i] = i;
    n_senone_active = CdWdPDFMod;
    
    get_scores4 (scores, frm);
}


static void get_scores1(int32 *scores, vqFeature_t frm[][MAX_TOPN])
{
    get_scores1_all (scores, frm);
}

static void get_scores1_all(int32 *scores, vqFeature_t frm[][MAX_TOPN])
{
    int32 i;
    int32 *pdf0, *pdf1, *pdf2, *pdf3;		/* pdf pointers */
    
    n_senone_active = CdWdPDFMod;

    for (i = 0; i < NUM_FEATURES; i++)
	frm[i][0].codeword *= CdWdPDFMod;
    
    /*
     *  Assumes that codeword is premultiplyed by CdWdPDFMod
     */
    pdf0 = OPDF[0] + frm[0][0].codeword;
    pdf1 = OPDF[1] + frm[1][0].codeword;
    pdf2 = OPDF[2] + frm[2][0].codeword;
    pdf3 = OPDF[3] + frm[3][0].codeword;
    
    for (i = 0; i < CdWdPDFMod; i++)
      scores[i] = pdf0[i] + pdf1[i] + pdf2[i] + pdf3[i];
}


static void get_scores_8b(int32 *scores, vqFeature_t frm[][MAX_TOPN])
{
    E_FATAL("get_scores_8b() not implemented\n");
}

static void get_scores_8b_all(int32 *scores, vqFeature_t frm[][MAX_TOPN])
{
    E_FATAL("get_scores_8b_all() not implemented\n");
}


/*
 * Like get_scores4, but uses OPDF_8B with FAST8B:
 *     LogProb(feature f, codeword c, senone s) =
 *         OPDF_8B[f]->prob[c][s]
 * Also, uses true 8-bit probs, so addition in logspace is an easy lookup.
 */
static void get_scores4_8b(int32 *scores, vqFeature_t frm[][MAX_TOPN])
{
    register int32 j, k, n;
    int32 tmp1, tmp2;
    unsigned char *pid_cw0, *pid_cw1, *pid_cw2, *pid_cw3;
    int32 w0, w1, w2, w3;			/* weights */

    memset(scores, 0, CdWdPDFMod*sizeof(*scores));
    for (j = 0; j < NUM_FEATURES; j++) {
	/* ptrs to senone prob ids */
	pid_cw0 = OPDF_8B[j]->id[frm[j][0].codeword];
	pid_cw1 = OPDF_8B[j]->id[frm[j][1].codeword];
	pid_cw2 = OPDF_8B[j]->id[frm[j][2].codeword];
	pid_cw3 = OPDF_8B[j]->id[frm[j][3].codeword];

	w0 = frm[j][0].val.score;
	w1 = frm[j][1].val.score;
	w2 = frm[j][2].val.score;
	w3 = frm[j][3].val.score;

	/* Floor w0..w3 to 256<<10 - 162k */
	if (w3 < -99000) w3 = -99000;
	if (w2 < -99000) w2 = -99000;
	if (w1 < -99000) w1 = -99000;
	if (w0 < -99000) w0 = -99000;	/* Condition should never be TRUE */

	/* Quantize */
	w3 = (511-w3) >> 10;
	w2 = (511-w2) >> 10;
	w1 = (511-w1) >> 10;
	w0 = (511-w0) >> 10;

	for (k = 0; k < n_senone_active; k++) {
	    n = senone_active[k];

	    tmp1 = pid_cw0[n] + w0;
	    tmp2 = pid_cw1[n] + w1;
	    tmp1 = LOG_ADD(tmp1, tmp2);
	    tmp2 = pid_cw2[n] + w2;
	    tmp1 = LOG_ADD(tmp1, tmp2);
	    tmp2 = pid_cw3[n] + w3;
	    tmp1 = LOG_ADD(tmp1, tmp2);

	    scores[n] -= tmp1 << 10;
	}
    }
}

/*
 * Like get_scores4, but uses OPDF_8B:
 *     LogProb(feature f, codeword c, senone s) =
 *         OPDF_8B[f]->prob[c][OPDF_8B[f]->id[c][s]]
 * Compute all senone scores.
 */
static void get_scores4_8b_all (int32 *scores, vqFeature_t frm[][MAX_TOPN])
{
    register int32 j, k;
    int32 tmp1, tmp2;
    unsigned char *pid_cw0, *pid_cw1, *pid_cw2, *pid_cw3;
    int32 w0, w1, w2, w3;			/* weights */
    /* register int32   ts = Table_Size;
       register int16 *at = Addition_Table;
       int32 ff, tt, ii; */

    n_senone_active = CdWdPDFMod;
    memset(scores, 0, CdWdPDFMod*sizeof(*scores));
    for (j = 0; j < NUM_FEATURES; j++) {
	/* ptrs to senone prob ids */
	pid_cw0 = OPDF_8B[j]->id[frm[j][0].codeword];
	pid_cw1 = OPDF_8B[j]->id[frm[j][1].codeword];
	pid_cw2 = OPDF_8B[j]->id[frm[j][2].codeword];
	pid_cw3 = OPDF_8B[j]->id[frm[j][3].codeword];

	w0 = frm[j][0].val.score;
	w1 = frm[j][1].val.score;
	w2 = frm[j][2].val.score;
	w3 = frm[j][3].val.score;

	/* Floor w0..w3 to 256<<10 - 162k */
	if (w3 < -99000) w3 = -99000;
	if (w2 < -99000) w2 = -99000;
	if (w1 < -99000) w1 = -99000;
	if (w0 < -99000) w0 = -99000;	/* Condition should never be TRUE */

	/* Quantize */
	w3 = (511-w3) >> 10;
	w2 = (511-w2) >> 10;
	w1 = (511-w1) >> 10;
	w0 = (511-w0) >> 10;

	for (k = 0; k < CdWdPDFMod; k++) {
	    tmp1 = pid_cw0[k] + w0;
	    tmp2 = pid_cw1[k] + w1;
	    tmp1 = LOG_ADD(tmp1, tmp2);
	    tmp2 = pid_cw2[k] + w2;
	    tmp1 = LOG_ADD(tmp1, tmp2);
	    tmp2 = pid_cw3[k] + w3;
	    tmp1 = LOG_ADD(tmp1, tmp2);
		
	    scores[k] -= tmp1 << 10;
	}
    }
}


static void get_scores2_8b (int32 *scores, vqFeature_t frm[][MAX_TOPN])
{
    register int32 k,n;
    int32 tmp1, tmp2;
    unsigned char *pid_cw00, *pid_cw10, *pid_cw01, *pid_cw11,
	*pid_cw02, *pid_cw12, *pid_cw03, *pid_cw13;
    int32 w00, w10, w01, w11, w02, w12, w03, w13;

    memset(scores, 0, CdWdPDFMod*sizeof(*scores));
    /* ptrs to senone prob ids */
    pid_cw00 = OPDF_8B[0]->id[frm[0][0].codeword];
    pid_cw10 = OPDF_8B[0]->id[frm[0][1].codeword];
    pid_cw01 = OPDF_8B[1]->id[frm[1][0].codeword];
    pid_cw11 = OPDF_8B[1]->id[frm[1][1].codeword];
    pid_cw02 = OPDF_8B[2]->id[frm[2][0].codeword];
    pid_cw12 = OPDF_8B[2]->id[frm[2][1].codeword];
    pid_cw03 = OPDF_8B[3]->id[frm[3][0].codeword];
    pid_cw13 = OPDF_8B[3]->id[frm[3][1].codeword];

    w00 = frm[0][0].val.score;
    w10 = frm[0][1].val.score;
    w01 = frm[1][0].val.score;
    w11 = frm[1][1].val.score;
    w02 = frm[2][0].val.score;
    w12 = frm[2][1].val.score;
    w03 = frm[3][0].val.score;
    w13 = frm[3][1].val.score;

    /* Floor w0..w3 to 256<<10 - 162k */ 
    /* Condition should never be TRUE */
    if (w10 < -99000) w10 = -99000;
    if (w00 < -99000) w00 = -99000;
    if (w11 < -99000) w11 = -99000;
    if (w01 < -99000) w01 = -99000;
    if (w12 < -99000) w12 = -99000;
    if (w02 < -99000) w02 = -99000;
    if (w13 < -99000) w13 = -99000;
    if (w03 < -99000) w03 = -99000;

    /* Quantize */
    w10 = (511-w10) >> 10;
    w00 = (511-w00) >> 10;
    w11 = (511-w11) >> 10;
    w01 = (511-w01) >> 10;
    w12 = (511-w12) >> 10;
    w02 = (511-w02) >> 10;
    w13 = (511-w13) >> 10;
    w03 = (511-w03) >> 10;

    for (k = 0; k < n_senone_active; k++) {
	n = senone_active[k];
	tmp1 = pid_cw00[n] + w00;
	tmp2 = pid_cw10[n] + w10;
	tmp1 = LOG_ADD(tmp1, tmp2);
	scores[n] -= tmp1 << 10;
	tmp1 = pid_cw01[n] + w01;
	tmp2 = pid_cw11[n] + w11;
	tmp1 = LOG_ADD(tmp1, tmp2);
	scores[n] -= tmp1 << 10;
	tmp1 = pid_cw02[n] + w02;
	tmp2 = pid_cw12[n] + w12;
	tmp1 = LOG_ADD(tmp1, tmp2);
	scores[n] -= tmp1 << 10;
	tmp1 = pid_cw03[n] + w03;
	tmp2 = pid_cw13[n] + w13;
	tmp1 = LOG_ADD(tmp1, tmp2);
	scores[n] -= tmp1 << 10;
    }
}

static void get_scores2_8b_all (int32 *scores, vqFeature_t frm[][MAX_TOPN])
{
    int32 n, tmp1, tmp2;
    unsigned char *pid_cw00, *pid_cw10, *pid_cw01, *pid_cw11,
	*pid_cw02, *pid_cw12, *pid_cw03, *pid_cw13;
    int32 w00, w10, w01, w11, w02, w12, w03, w13;

    n_senone_active = CdWdPDFMod;
    memset(scores, 0, CdWdPDFMod*sizeof(*scores));
    /* ptrs to senone prob ids */
    pid_cw00 = OPDF_8B[0]->id[frm[0][0].codeword];
    pid_cw10 = OPDF_8B[0]->id[frm[0][1].codeword];
    pid_cw01 = OPDF_8B[1]->id[frm[1][0].codeword];
    pid_cw11 = OPDF_8B[1]->id[frm[1][1].codeword];
    pid_cw02 = OPDF_8B[2]->id[frm[2][0].codeword];
    pid_cw12 = OPDF_8B[2]->id[frm[2][1].codeword];
    pid_cw03 = OPDF_8B[3]->id[frm[3][0].codeword];
    pid_cw13 = OPDF_8B[3]->id[frm[3][1].codeword];

    w00 = frm[0][0].val.score;
    w10 = frm[0][1].val.score;
    w01 = frm[1][0].val.score;
    w11 = frm[1][1].val.score;
    w02 = frm[2][0].val.score;
    w12 = frm[2][1].val.score;
    w03 = frm[3][0].val.score;
    w13 = frm[3][1].val.score;

    /* Floor w0..w3 to 256<<10 - 162k */ 
    /* Condition should never be TRUE */
    if (w10 < -99000) w10 = -99000;
    if (w00 < -99000) w00 = -99000;
    if (w11 < -99000) w11 = -99000;
    if (w01 < -99000) w01 = -99000;
    if (w12 < -99000) w12 = -99000;
    if (w02 < -99000) w02 = -99000;
    if (w13 < -99000) w13 = -99000;
    if (w03 < -99000) w03 = -99000;

    /* Quantize */
    w10 = (511-w10) >> 10;
    w00 = (511-w00) >> 10;
    w11 = (511-w11) >> 10;
    w01 = (511-w01) >> 10;
    w12 = (511-w12) >> 10;
    w02 = (511-w02) >> 10;
    w13 = (511-w13) >> 10;
    w03 = (511-w03) >> 10;

    for (n = 0; n < CdWdPDFMod; n++) {
	tmp1 = pid_cw00[n] + w00;
	tmp2 = pid_cw10[n] + w10;
	tmp1 = LOG_ADD(tmp1, tmp2);
	scores[n] -= tmp1 << 10;
	tmp1 = pid_cw01[n] + w01;
	tmp2 = pid_cw11[n] + w11;
	tmp1 = LOG_ADD(tmp1, tmp2);
	scores[n] -= tmp1 << 10;
	tmp1 = pid_cw02[n] + w02;
	tmp2 = pid_cw12[n] + w12;
	tmp1 = LOG_ADD(tmp1, tmp2);
	scores[n] -= tmp1 << 10;
	tmp1 = pid_cw03[n] + w03;
	tmp2 = pid_cw13[n] + w13;
	tmp1 = LOG_ADD(tmp1, tmp2);
	scores[n] -= tmp1 << 10;
    }
}

static void get_scores1_8b(int32 *scores, vqFeature_t frm[][MAX_TOPN])
{
    int32 j, k;
    unsigned char *pid_cw0, *pid_cw1, *pid_cw2, *pid_cw3;

    /* Ptrs to senone prob values for the top codeword of all codebooks */
    pid_cw0 = OPDF_8B[0]->id[frm[0][0].codeword];
    pid_cw1 = OPDF_8B[1]->id[frm[1][0].codeword];
    pid_cw2 = OPDF_8B[2]->id[frm[2][0].codeword];
    pid_cw3 = OPDF_8B[3]->id[frm[3][0].codeword];
    
    for (k = 0; k < n_senone_active; k++) {
	j = senone_active[k];

	/* ** HACK!! ** <<10 hardwired!! */
	scores[j] = -((pid_cw0[j] + pid_cw1[j] + pid_cw2[j] + pid_cw3[j]) << 10);
    }
}

static void get_scores1_8b_all (int32 *scores, vqFeature_t frm[][MAX_TOPN])
{
    int32 i;
    unsigned char *pid_cw0, *pid_cw1, *pid_cw2, *pid_cw3;

    n_senone_active = CdWdPDFMod;
    
    /* Ptrs to senone prob values for the top codeword of all codebooks */
    pid_cw0 = OPDF_8B[0]->id[frm[0][0].codeword];
    pid_cw1 = OPDF_8B[1]->id[frm[1][0].codeword];
    pid_cw2 = OPDF_8B[2]->id[frm[2][0].codeword];
    pid_cw3 = OPDF_8B[3]->id[frm[3][0].codeword];
    
    for (i = 0; i < CdWdPDFMod; i++) {
      /* ** HACK!! ** <<10 hardwired!! */
      *scores = -((*pid_cw0++ + *pid_cw1++ + *pid_cw2++ + *pid_cw3++) << 10);
      scores++;
    }
}


/*
 * Parameter and setup code
 */

/*
 * Read & initialize SC codebooks & output pdfs
 */
int32 SCVQInitFeat(feat_t feat, char *meanPath, char *varPath, int32 *opdf)
{
    int32   *detP;

    assert(((int32)feat < NUM_FEATURES) && ((int32)feat >= 0));
    assert(meanPath != NULL);
    assert(varPath != NULL);
    assert(opdf != NULL);

    if (readMeanCBFile(feat, means + (int32)feat, meanPath) < 0)
	return -1;

    detP = dets[(int32)feat] = detArr + (int32)feat * NUM_ALPHABET;
    if (readVarCBFile(feat, detP, vars + (int32)feat, varPath) < 0) {
	if (feat != POW_FEAT)
	    return -1;
	else {
	    log_debug("Synthesizing power codebook variances\n");
	    if (setPowVar(detP, vars + (int32)feat,
			  (use20ms_diff_pow ? 0.125 : 0.05)) < 0)
		return -1;
	}
    }
    
    if (prob_size == 32)
	OPDF[(int32)feat] = opdf;
    else if (prob_size == 8) {
	OPDF_8B[(int32)feat] = (OPDF_8BIT_T *) opdf;
    } else
	E_FATAL("Illegal prob size: %d\n", prob_size);
    
    return 0;
}

int32 SCVQS3InitFeat(char *meanPath, char *varPath,
		     int32 *opdf0, int32 *opdf1,
		     int32 *opdf2, int32 *opdf3)
{
    int i;

    assert(meanPath != NULL);
    assert(varPath != NULL);
    assert(opdf0 != NULL);
    assert(opdf1 != NULL);
    assert(opdf2 != NULL);
    assert(opdf3 != NULL);

    /* Read means and variances. */
    if (s3_read_mgau(meanPath, (float32 **)means) < 0)
	return -1;
    if (s3_read_mgau(varPath, (float32 **)vars) < 0)
	return -1;

    /* Precompute means, variances, and determinants. */
    for (i = 0; i < 4; ++i)
	 dets[i] = detArr + i * NUM_ALPHABET;
    s3_precomp(means, vars, dets);

    if (prob_size == 32) {
	OPDF[0] = opdf0;
	OPDF[1] = opdf1;
	OPDF[2] = opdf2;
	OPDF[3] = opdf3;
    }
    else if (prob_size == 8) {
	OPDF_8B[0] = (OPDF_8BIT_T *) opdf0;
	OPDF_8B[1] = (OPDF_8BIT_T *) opdf1;
	OPDF_8B[2] = (OPDF_8BIT_T *) opdf2;
	OPDF_8B[3] = (OPDF_8BIT_T *) opdf3;
    }

    return 0;
}


/*
 * SCVQSetSenoneCompression:  Must be called before SCVQInitFeat (if senone-probs
 * are compressed).
 */
void
SCVQSetSenoneCompression (int32 size)
{
    if ((size != 8) && (size != 32))
	E_FATAL("Bad #bits/sen-prob: %d\n", size);
    prob_size = size;
}

void SCVQSetdcep80msWeight (double arg)
{
#ifdef FIXED_POINT
    dcep80msWeight = FLOAT2FIX(arg);
#else
    dcep80msWeight = arg;
#endif
}

void SCVQSetDownsamplingRatio (int32 ratio)
{
	frame_ds_ratio = ratio;
}

int32 SCVQLoadKDTree(const char *kdtree_file_name, uint32 maxdepth, int32 maxbbi)
{
	if (read_kd_trees(kdtree_file_name, &kdtrees, &n_kdtrees,
			  maxdepth, maxbbi) == -1)
		E_FATAL("Failed to read kd-trees from %s\n", kdtree_file_name);
	if (n_kdtrees != NUM_FEATURES)
		E_FATAL("Number of kd-trees != %d\n", NUM_FEATURES);

	kd_maxdepth = maxdepth;
	kd_maxbbi = maxbbi;
	return 0;
}
