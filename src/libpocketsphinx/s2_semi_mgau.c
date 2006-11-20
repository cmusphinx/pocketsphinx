/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
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
 * $Log$
 * Revision 1.2  2006/04/06  14:03:02  dhdfu
 * Prevent confusion among future generations by calling this s2_semi_mgau instead of sc_vq
 * 
 * Revision 1.1  2006/04/05 20:14:26  dhdfu
 * Add cut-down support for Sphinx-2 fast GMM computation (from
 * PocketSphinx).  This does *not* support Sphinx2 format models, but
 * rather semi-continuous Sphinx3 models.  I'll try to write a model
 * converter at some point soon.
 *
 * Unfortunately the smallest models I have for testing don't do so well
 * on the AN4 test sentence (should use AN4 models, maybe...) so it comes
 * with a "don't panic" warning.
 *
 * Revision 1.4  2006/04/04 15:31:31  dhuggins
 * Remove redundant acoustic score scaling in senone computation.
 *
 * Revision 1.3  2006/04/04 15:24:29  dhuggins
 * Get the meaning of LOG_BASE right (oops!).  Seems to work fine now, at
 * least at logbase=1.0001.
 *
 * Revision 1.2  2006/04/04 14:54:40  dhuggins
 * Add support for s2_semi_mgau - it doesn't crash, but it doesn't work either :)
 *
 * Revision 1.1  2006/04/04 04:25:17  dhuggins
 * Add a cut-down version of sphinx2 fast GMM computation (SCVQ) from
 * PocketSphinx.  Not enabled or tested yet.  Doesn't support Sphinx2
 * models (write an external conversion tool instead, please).  Hopefully
 * this will put an end to me complaining about Sphinx3 being too slow :-)
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

/* System headers */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <math.h>

/* SphinxBase headers */
#include <sphinx_config.h>
#include <cmd_ln.h>
#include <fixpoint.h>
#include <ckd_alloc.h>
#include <bio.h>
#include <err.h>

/* Local headers */
#include "s2types.h"
#include "sphinx_types.h"
#include "log.h"
#include "s2_semi_mgau.h"
#include "kdtree.h"
#include "kb.h"
#include "s2io.h"
#include "senscr.h"
#include "posixwin32.h"

#define MGAU_MIXW_VERSION	"1.0"   /* Sphinx-3 file format version for mixw */
#define MGAU_PARAM_VERSION	"1.0"   /* Sphinx-3 file format version for mean/var */
#define NONE		-1
#define WORST_DIST	(int32)(0x80000000)

/*
 * In terms of already shifted and negated quantities (i.e. dealing with
 * 8-bit quantized values):
 */
#define LOG_ADD(p1,p2)	(logadd_tbl[(p1<<8)+(p2)])

/** Subtract GMM component b (assumed to be positive) and saturate */
#define GMMSUB(a,b) \
	(((a)-(b) > a) ? (INT_MIN) : ((a)-(b)))
/** Add GMM component b (assumed to be positive) and saturate */
#define GMMADD(a,b) \
	(((a)+(b) < a) ? (INT_MAX) : ((a)+(b)))

extern const unsigned char logadd_tbl[];

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

/*
 * Compute senone scores.
 */
static int32 SCVQComputeScores(s2_semi_mgau_t * s, int32 compallsen);

/*
 * Optimization for various topN cases, PDF-size(#bits) cases of
 * SCVQComputeScores() and SCVQComputeScores_all().
 */
static int32 get_scores4_8b(s2_semi_mgau_t * s);
static int32 get_scores2_8b(s2_semi_mgau_t * s);
static int32 get_scores1_8b(s2_semi_mgau_t * s);
static int32 get_scores_8b(s2_semi_mgau_t * s);
static int32 get_scores4_8b_all(s2_semi_mgau_t * s);
static int32 get_scores2_8b_all(s2_semi_mgau_t * s);
static int32 get_scores1_8b_all(s2_semi_mgau_t * s);
static int32 get_scores_8b_all(s2_semi_mgau_t * s);

static void
eval_topn(s2_semi_mgau_t *s, int32 feat, mfcc_t *z)
{
    int32 i, ceplen;
    vqFeature_t *topn;

    topn = s->f[feat];
    ceplen = s->veclen[feat];

    for (i = 0; i < s->topN; i++) {
        mean_t *mean, diff, sqdiff, compl; /* diff, diff^2, component likelihood */
        vqFeature_t vtmp;
        var_t *var, d;
        mfcc_t *obs;
        int32 cw, j;

        cw = topn[i].codeword;
        mean = s->means[feat] + cw * ceplen;
        var = s->vars[feat] + cw * ceplen;
        d = s->dets[feat][cw];
        obs = z;
        for (j = 0; j < ceplen; j++) {
            diff = *obs++ - *mean++;
            sqdiff = MFCCMUL(diff, diff);
            compl = MFCCMUL(sqdiff, *var);
            d = GMMSUB(d, compl);
            ++var;
        }
        topn[i].val.dist = (int32) d;
        if (i == 0)
            continue;
        vtmp = topn[i];
        for (j = i - 1; j >= 0 && (int32) d > topn[j].val.dist; j--) {
            topn[j + 1] = topn[j];
        }
        topn[j + 1] = vtmp;
    }
}

static void
eval_cb_kdtree(s2_semi_mgau_t *s, int32 feat, mfcc_t *z,
               kd_tree_node_t *node, uint32 maxbbi)
{
    vqFeature_t *worst, *best, *topn;
    int32 i, ceplen;

    best = topn = s->f[feat];
    worst = topn + (s->topN - 1);
    ceplen = s->veclen[feat];

    for (i = 0; i < maxbbi; ++i) {
        mean_t *mean, diff, sqdiff, compl; /* diff, diff^2, component likelihood */
        var_t *var, d;
        mfcc_t *obs;
        vqFeature_t *cur;
        int32 cw, j, k;

        cw = node->bbi[i];
        mean = s->means[feat] + cw * ceplen;
        var = s->vars[feat] + cw * ceplen;
        d = s->dets[feat][cw];
        obs = z;
        for (j = 0; (j < ceplen) && (d >= worst->val.dist); j++) {
            diff = *obs++ - *mean++;
            sqdiff = MFCCMUL(diff, diff);
            compl = MFCCMUL(sqdiff, *var);
            d = GMMSUB(d, compl);
            ++var;
        }
        if (j < ceplen)
            continue;
        if (d < worst->val.dist)
            continue;
        for (k = 0; k < s->topN; k++) {
            /* already there, so don't need to insert */
            if (topn[k].codeword == cw)
                break;
        }
        if (k < s->topN)
            continue;       /* already there.  Don't insert */
        /* remaining code inserts codeword and dist in correct spot */
        for (cur = worst - 1; cur >= best && d >= cur->val.dist; --cur)
            memcpy(cur + 1, cur, sizeof(vqFeature_t));
        ++cur;
        cur->codeword = cw;
        cur->val.dist = (int32) d;
    }
}

static void
eval_cb(s2_semi_mgau_t *s, int32 feat, mfcc_t *z)
{
    vqFeature_t *worst, *best, *topn;
    mean_t *mean;
    var_t *var;
    int32 *det, *detP, *detE;
    int32 i, ceplen;

    best = topn = s->f[feat];
    worst = topn + (s->topN - 1);
    mean = s->means[feat];
    var = s->vars[feat];
    det = s->dets[feat];
    detE = det + s->n_density;
    ceplen = s->veclen[feat];

    for (detP = det; detP < detE; ++detP) {
        mean_t diff, sqdiff, compl; /* diff, diff^2, component likelihood */
        var_t d;
        mfcc_t *obs;
        vqFeature_t *cur;
        int32 cw, j;

        d = *detP;
        obs = z;
        cw = detP - det;
        for (j = 0; (j < ceplen) && (d >= worst->val.dist); ++j) {
            diff = *obs++ - *mean++;
            sqdiff = MFCCMUL(diff, diff);
            compl = MFCCMUL(sqdiff, *var);
            d = GMMSUB(d, compl);
            ++var;
        }
        if (j < ceplen) {
            /* terminated early, so not in topn */
            mean += (ceplen - j);
            var += (ceplen - j);
            continue;
        }
        if (d < worst->val.dist)
            continue;
        for (i = 0; i < s->topN; i++) {
            /* already there, so don't need to insert */
            if (topn[i].codeword == cw)
                break;
        }
        if (i < s->topN)
            continue;       /* already there.  Don't insert */
        /* remaining code inserts codeword and dist in correct spot */
        for (cur = worst - 1; cur >= best && d >= cur->val.dist; --cur)
            memcpy(cur + 1, cur, sizeof(vqFeature_t));
        ++cur;
        cur->codeword = cw;
        cur->val.dist = (int32) d;
    }
}

static void
mgau_dist(s2_semi_mgau_t * s, int32 frame, int32 feat, mfcc_t * z)
{
    /* Initialize topn codewords to topn codewords from previous
     * frame, and calculate their densities. */
    memcpy(s->f[feat], s->lastf[feat], sizeof(vqFeature_t) * s->topN);
    eval_topn(s, feat, z);

    /* If this frame is skipped, do nothing else. */
    if (frame % s->ds_ratio)
        return;

    /* Evaluate the rest of the codebook (or subset thereof). */
    if (s->kdtrees) {
        kd_tree_node_t *node;
        uint32 maxbbi;

        node =
            eval_kd_tree(s->kdtrees[feat], z, s->kd_maxdepth);
        maxbbi = s->kd_maxbbi == -1 ? node->n_bbi : MIN(node->n_bbi,
                                                        s->
                                                        kd_maxbbi);
        eval_cb_kdtree(s, feat, z, node, maxbbi);
    }
    else {
        eval_cb(s, feat, z);
    }

    /* Make a copy of current topn. */
    memcpy(s->lastf[feat], s->f[feat], sizeof(vqFeature_t) * s->topN);
}

/*
 * Compute senone scores for the active senones.
 */
int32
s2_semi_mgau_frame_eval(s2_semi_mgau_t * s,
			mfcc_t ** featbuf, int32 frame,
			int32 compallsen)
{
    int i, j;

    for (i = 0; i < s->n_feat; ++i)
        mgau_dist(s, frame, i, featbuf[i]);

    /* normalize the topN feature scores */
    for (j = 0; j < s->n_feat; j++) {
        s->score_tmp[j] = s->f[j][0].val.score;
    }
    for (i = 1; i < s->topN; i++)
        for (j = 0; j < s->n_feat; j++) {
            s->score_tmp[j] = ADD(s->score_tmp[j], s->f[j][i].val.score);
        }
    for (i = 0; i < s->topN; i++)
        for (j = 0; j < s->n_feat; j++) {
            s->f[j][i].val.score -= s->score_tmp[j];
            if (s->f[j][i].val.score > 0)
                s->f[j][i].val.score = INT_MIN; /* tkharris++ */
            /* E_FATAL("**ERROR** VQ score= %d\n", f[j][i].val.score); */
        }


    return SCVQComputeScores(s, compallsen);
}


static int32
SCVQComputeScores(s2_semi_mgau_t * s, int32 compallsen)
{
    if (compallsen) {
	switch (s->topN) {
	case 4:
	    return get_scores4_8b_all(s);
	    break;
	case 2:
	    return get_scores2_8b_all(s);
	    break;
	case 1:
	    return get_scores1_8b_all(s);
	    break;
	default:
	    return get_scores_8b_all(s);
	    break;
	}
    }
    else {
	switch (s->topN) {
	case 4:
	    return get_scores4_8b(s);
	    break;
	case 2:
	    return get_scores2_8b(s);
	    break;
	case 1:
	    return get_scores1_8b(s);
	    break;
	default:
	    return get_scores_8b(s);
	    break;
	}
    }
}

static int32
get_scores_8b(s2_semi_mgau_t * s)
{
    E_FATAL("get_scores_8b() not implemented\n");
    return 0;
}

static int32
get_scores_8b_all(s2_semi_mgau_t * s)
{
    E_FATAL("get_scores_8b_all() not implemented\n");
    return 0;
}

/*
 * Like get_scores4, but uses OPDF_8B with FAST8B:
 *     LogProb(feature f, codeword c, senone s) =
 *         OPDF_8B[f]->prob[c][s]
 * Also, uses true 8-bit probs, so addition in logspace is an easy lookup.
 */
static int32
get_scores4_8b(s2_semi_mgau_t * s)
{
    register int32 j, n, k;
    int32 tmp1, tmp2;
    unsigned char *pid_cw0, *pid_cw1, *pid_cw2, *pid_cw3;
    int32 w0, w1, w2, w3;       /* weights */

    memset(senone_scores, 0, s->CdWdPDFMod * sizeof(*senone_scores));
    for (j = 0; j < s->n_feat; j++) {
        /* ptrs to senone prob ids */
        pid_cw0 = s->OPDF_8B[j][s->f[j][0].codeword];
        pid_cw1 = s->OPDF_8B[j][s->f[j][1].codeword];
        pid_cw2 = s->OPDF_8B[j][s->f[j][2].codeword];
        pid_cw3 = s->OPDF_8B[j][s->f[j][3].codeword];

        w0 = s->f[j][0].val.score;
        w1 = s->f[j][1].val.score;
        w2 = s->f[j][2].val.score;
        w3 = s->f[j][3].val.score;

        /* Floor w0..w3 to 256<<10 - 162k */
        if (w3 < -99000)
            w3 = -99000;
        if (w2 < -99000)
            w2 = -99000;
        if (w1 < -99000)
            w1 = -99000;
        if (w0 < -99000)
            w0 = -99000;        /* Condition should never be TRUE */

        /* Quantize */
        w3 = (511 - w3) >> 10;
        w2 = (511 - w2) >> 10;
        w1 = (511 - w1) >> 10;
        w0 = (511 - w0) >> 10;

        for (k = 0; k < n_senone_active; k++) {
	    n = senone_active[k];

            tmp1 = pid_cw0[n] + w0;
            tmp2 = pid_cw1[n] + w1;
            tmp1 = LOG_ADD(tmp1, tmp2);
            tmp2 = pid_cw2[n] + w2;
            tmp1 = LOG_ADD(tmp1, tmp2);
            tmp2 = pid_cw3[n] + w3;
            tmp1 = LOG_ADD(tmp1, tmp2);

            senone_scores[n] -= tmp1 << 10;
        }
    }
    return 0;
}

static int32
get_scores4_8b_all(s2_semi_mgau_t * s)
{
    register int32 j, k;
    int32 tmp1, tmp2;
    unsigned char *pid_cw0, *pid_cw1, *pid_cw2, *pid_cw3;
    int32 w0, w1, w2, w3;       /* weights */

    n_senone_active = s->CdWdPDFMod;
    memset(senone_scores, 0, s->CdWdPDFMod * sizeof(*senone_scores));
    for (j = 0; j < s->n_feat; j++) {
        /* ptrs to senone prob ids */
        pid_cw0 = s->OPDF_8B[j][s->f[j][0].codeword];
        pid_cw1 = s->OPDF_8B[j][s->f[j][1].codeword];
        pid_cw2 = s->OPDF_8B[j][s->f[j][2].codeword];
        pid_cw3 = s->OPDF_8B[j][s->f[j][3].codeword];

        w0 = s->f[j][0].val.score;
        w1 = s->f[j][1].val.score;
        w2 = s->f[j][2].val.score;
        w3 = s->f[j][3].val.score;

        /* Floor w0..w3 to 256<<10 - 162k */
        if (w3 < -99000)
            w3 = -99000;
        if (w2 < -99000)
            w2 = -99000;
        if (w1 < -99000)
            w1 = -99000;
        if (w0 < -99000)
            w0 = -99000;        /* Condition should never be TRUE */

        /* Quantize */
        w3 = (511 - w3) >> 10;
        w2 = (511 - w2) >> 10;
        w1 = (511 - w1) >> 10;
        w0 = (511 - w0) >> 10;

        for (k = 0; k < s->CdWdPDFMod; k++) {
            tmp1 = pid_cw0[k] + w0;
            tmp2 = pid_cw1[k] + w1;
            tmp1 = LOG_ADD(tmp1, tmp2);
            tmp2 = pid_cw2[k] + w2;
            tmp1 = LOG_ADD(tmp1, tmp2);
            tmp2 = pid_cw3[k] + w3;
            tmp1 = LOG_ADD(tmp1, tmp2);

            senone_scores[k] -= tmp1 << 10;
        }
    }
    return 0;
}

static int32
get_scores2_8b(s2_semi_mgau_t * s)
{
    register int32 n, k;
    int32 tmp1, tmp2;
    unsigned char *pid_cw00, *pid_cw10, *pid_cw01, *pid_cw11,
        *pid_cw02, *pid_cw12, *pid_cw03, *pid_cw13;
    int32 w00, w10, w01, w11, w02, w12, w03, w13;

    memset(senone_scores, 0, s->CdWdPDFMod * sizeof(*senone_scores));
    /* ptrs to senone prob ids */
    pid_cw00 = s->OPDF_8B[0][s->f[0][0].codeword];
    pid_cw10 = s->OPDF_8B[0][s->f[0][1].codeword];
    pid_cw01 = s->OPDF_8B[1][s->f[1][0].codeword];
    pid_cw11 = s->OPDF_8B[1][s->f[1][1].codeword];
    pid_cw02 = s->OPDF_8B[2][s->f[2][0].codeword];
    pid_cw12 = s->OPDF_8B[2][s->f[2][1].codeword];
    pid_cw03 = s->OPDF_8B[3][s->f[3][0].codeword];
    pid_cw13 = s->OPDF_8B[3][s->f[3][1].codeword];

    w00 = s->f[0][0].val.score;
    w10 = s->f[0][1].val.score;
    w01 = s->f[1][0].val.score;
    w11 = s->f[1][1].val.score;
    w02 = s->f[2][0].val.score;
    w12 = s->f[2][1].val.score;
    w03 = s->f[3][0].val.score;
    w13 = s->f[3][1].val.score;

    /* Floor w0..w3 to 256<<10 - 162k */
    /* Condition should never be TRUE */
    if (w10 < -99000)
        w10 = -99000;
    if (w00 < -99000)
        w00 = -99000;
    if (w11 < -99000)
        w11 = -99000;
    if (w01 < -99000)
        w01 = -99000;
    if (w12 < -99000)
        w12 = -99000;
    if (w02 < -99000)
        w02 = -99000;
    if (w13 < -99000)
        w13 = -99000;
    if (w03 < -99000)
        w03 = -99000;

    /* Quantize */
    w10 = (511 - w10) >> 10;
    w00 = (511 - w00) >> 10;
    w11 = (511 - w11) >> 10;
    w01 = (511 - w01) >> 10;
    w12 = (511 - w12) >> 10;
    w02 = (511 - w02) >> 10;
    w13 = (511 - w13) >> 10;
    w03 = (511 - w03) >> 10;

    for (k = 0; k < n_senone_active; k++) {
	n = senone_active[k];

        tmp1 = pid_cw00[n] + w00;
        tmp2 = pid_cw10[n] + w10;
        tmp1 = LOG_ADD(tmp1, tmp2);
        senone_scores[n] -= tmp1 << 10;
        tmp1 = pid_cw01[n] + w01;
        tmp2 = pid_cw11[n] + w11;
        tmp1 = LOG_ADD(tmp1, tmp2);
        senone_scores[n] -= tmp1 << 10;
        tmp1 = pid_cw02[n] + w02;
        tmp2 = pid_cw12[n] + w12;
        tmp1 = LOG_ADD(tmp1, tmp2);
        senone_scores[n] -= tmp1 << 10;
        tmp1 = pid_cw03[n] + w03;
        tmp2 = pid_cw13[n] + w13;
        tmp1 = LOG_ADD(tmp1, tmp2);
        senone_scores[n] -= tmp1 << 10;
    }
    return 0;
}

static int32
get_scores2_8b_all(s2_semi_mgau_t * s)
{
    register int32 n;
    int32 tmp1, tmp2;
    unsigned char *pid_cw00, *pid_cw10, *pid_cw01, *pid_cw11,
        *pid_cw02, *pid_cw12, *pid_cw03, *pid_cw13;
    int32 w00, w10, w01, w11, w02, w12, w03, w13;

    n_senone_active = s->CdWdPDFMod;
    memset(senone_scores, 0, s->CdWdPDFMod * sizeof(*senone_scores));
    /* ptrs to senone prob ids */
    pid_cw00 = s->OPDF_8B[0][s->f[0][0].codeword];
    pid_cw10 = s->OPDF_8B[0][s->f[0][1].codeword];
    pid_cw01 = s->OPDF_8B[1][s->f[1][0].codeword];
    pid_cw11 = s->OPDF_8B[1][s->f[1][1].codeword];
    pid_cw02 = s->OPDF_8B[2][s->f[2][0].codeword];
    pid_cw12 = s->OPDF_8B[2][s->f[2][1].codeword];
    pid_cw03 = s->OPDF_8B[3][s->f[3][0].codeword];
    pid_cw13 = s->OPDF_8B[3][s->f[3][1].codeword];

    w00 = s->f[0][0].val.score;
    w10 = s->f[0][1].val.score;
    w01 = s->f[1][0].val.score;
    w11 = s->f[1][1].val.score;
    w02 = s->f[2][0].val.score;
    w12 = s->f[2][1].val.score;
    w03 = s->f[3][0].val.score;
    w13 = s->f[3][1].val.score;

    /* Floor w0..w3 to 256<<10 - 162k */
    /* Condition should never be TRUE */
    if (w10 < -99000)
        w10 = -99000;
    if (w00 < -99000)
        w00 = -99000;
    if (w11 < -99000)
        w11 = -99000;
    if (w01 < -99000)
        w01 = -99000;
    if (w12 < -99000)
        w12 = -99000;
    if (w02 < -99000)
        w02 = -99000;
    if (w13 < -99000)
        w13 = -99000;
    if (w03 < -99000)
        w03 = -99000;

    /* Quantize */
    w10 = (511 - w10) >> 10;
    w00 = (511 - w00) >> 10;
    w11 = (511 - w11) >> 10;
    w01 = (511 - w01) >> 10;
    w12 = (511 - w12) >> 10;
    w02 = (511 - w02) >> 10;
    w13 = (511 - w13) >> 10;
    w03 = (511 - w03) >> 10;

    for (n = 0; n < s->CdWdPDFMod; n++) {
        tmp1 = pid_cw00[n] + w00;
        tmp2 = pid_cw10[n] + w10;
        tmp1 = LOG_ADD(tmp1, tmp2);
        senone_scores[n] -= tmp1 << 10;
        tmp1 = pid_cw01[n] + w01;
        tmp2 = pid_cw11[n] + w11;
        tmp1 = LOG_ADD(tmp1, tmp2);
        senone_scores[n] -= tmp1 << 10;
        tmp1 = pid_cw02[n] + w02;
        tmp2 = pid_cw12[n] + w12;
        tmp1 = LOG_ADD(tmp1, tmp2);
        senone_scores[n] -= tmp1 << 10;
        tmp1 = pid_cw03[n] + w03;
        tmp2 = pid_cw13[n] + w13;
        tmp1 = LOG_ADD(tmp1, tmp2);
        senone_scores[n] -= tmp1 << 10;
    }
    return 0;
}

static int32
get_scores1_8b(s2_semi_mgau_t * s)
{
    int32 j, k;
    unsigned char *pid_cw0, *pid_cw1, *pid_cw2, *pid_cw3;

    /* Ptrs to senone prob values for the top codeword of all codebooks */
    pid_cw0 = s->OPDF_8B[0][s->f[0][0].codeword];
    pid_cw1 = s->OPDF_8B[1][s->f[1][0].codeword];
    pid_cw2 = s->OPDF_8B[2][s->f[2][0].codeword];
    pid_cw3 = s->OPDF_8B[3][s->f[3][0].codeword];

    for (k = 0; k < n_senone_active; k++) {
        j = senone_active[k];

        /* ** HACK!! ** <<10 hardwired!! */
        senone_scores[j] =
            -((pid_cw0[j] + pid_cw1[j] + pid_cw2[j] + pid_cw3[j]) << 10);
    }
    return 0;
}

static int32
get_scores1_8b_all(s2_semi_mgau_t * s)
{
    int32 j;
    unsigned char *pid_cw0, *pid_cw1, *pid_cw2, *pid_cw3;
    int *senscr = senone_scores;

    n_senone_active = s->CdWdPDFMod;

    /* Ptrs to senone prob values for the top codeword of all codebooks */
    pid_cw0 = s->OPDF_8B[0][s->f[0][0].codeword];
    pid_cw1 = s->OPDF_8B[1][s->f[1][0].codeword];
    pid_cw2 = s->OPDF_8B[2][s->f[2][0].codeword];
    pid_cw3 = s->OPDF_8B[3][s->f[3][0].codeword];

    for (j = 0; j < s->CdWdPDFMod; j++) {
        /* ** HACK!! ** <<10 hardwired!! */
	*senscr++ =
            -((pid_cw0[j] + pid_cw1[j] + pid_cw2[j] + pid_cw3[j]) << 10);
    }
    return 0;
}

int32
s2_semi_mgau_load_kdtree(s2_semi_mgau_t * s, const char *kdtree_path,
                         uint32 maxdepth, int32 maxbbi)
{
    if (read_kd_trees(kdtree_path, &s->kdtrees, &s->n_kdtrees,
                      maxdepth, maxbbi) == -1)
        E_FATAL("Failed to read kd-trees from %s\n", kdtree_path);
    if (s->n_kdtrees != s->n_feat)
        E_FATAL("Number of kd-trees != %d\n", s->n_feat);

    s->kd_maxdepth = maxdepth;
    s->kd_maxbbi = maxbbi;
    return 0;
}

static int32
load_senone_dists_8bits(s2_semi_mgau_t *s, char const *file)
{
    FILE *fp;
    char line[1000];
    int32 i, n;
    int32 use_mmap, do_swap;
    size_t filesize, offset;
    int n_clust = 256;          /* Number of clusters (if zero, we are just using
                                 * 8-bit quantized weights) */
    int r = s->n_density;
    int c = bin_mdef_n_sen(mdef);

    use_mmap = cmd_ln_boolean("-mmap");

    if ((fp = fopen(file, "rb")) == NULL)
        return -1;

    E_INFO("Loading senones from dump file %s\n", file);
    /* Read title size, title */
    fread(&n, sizeof(int32), 1, fp);
    /* This is extremely bogus */
    do_swap = 0;
    if (n < 1 || n > 999) {
        SWAP_INT32(&n);
        if (n < 1 || n > 999) {
            E_FATAL("Title length %x in dump file %s out of range\n", n, file);
        }
        do_swap = 1;
    }
    if (do_swap && use_mmap) {
        E_ERROR("Dump file is byte-swapped, cannot use memory-mapping\n");
        use_mmap = 0;
    }
    if (fread(line, sizeof(char), n, fp) != n)
        E_FATAL("Cannot read title\n");
    if (line[n - 1] != '\0')
        E_FATAL("Bad title in dump file\n");
    E_INFO("%s\n", line);

    /* Read header size, header */
    fread(&n, 1, sizeof(n), fp);
    if (do_swap) SWAP_INT32(&n);
    if (fread(line, sizeof(char), n, fp) != n)
        E_FATAL("Cannot read header\n");
    if (line[n - 1] != '\0')
        E_FATAL("Bad header in dump file\n");

    /* Read other header strings until string length = 0 */
    for (;;) {
        fread(&n, 1, sizeof(n), fp);
        if (do_swap) SWAP_INT32(&n);
        if (n == 0)
            break;
        if (fread(line, sizeof(char), n, fp) != n)
            E_FATAL("Cannot read header\n");
        /* Look for a cluster count, if present */
        if (!strncmp(line, "cluster_count ", strlen("cluster_count "))) {
            n_clust = atoi(line + strlen("cluster_count "));
        }
    }

    /* Read #codewords, #pdfs */
    fread(&r, 1, sizeof(r), fp);
    if (do_swap) SWAP_INT32(&r);
    fread(&c, 1, sizeof(c), fp);
    if (do_swap) SWAP_INT32(&c);

    if (n_clust) {
	E_ERROR ("Dump file is incompatible with PocketSphinx\n");
	fclose(fp);
	return -1;
    }
    if (use_mmap) {
            E_INFO("Using memory-mapped I/O for senones\n");
    }
    /* Verify alignment constraints for using mmap() */
    if ((c & 3) != 0) {
        /* This will cause us to run very slowly, so don't do it. */
        E_ERROR
            ("Number of PDFs (%d) not padded to multiple of 4, will not use mmap()\n",
             c);
        use_mmap = 0;
    }
    offset = ftell(fp);
    fseek(fp, 0, SEEK_END);
    filesize = ftell(fp);
    fseek(fp, offset, SEEK_SET);
    if ((offset & 3) != 0) {
        E_ERROR
            ("PDFs are not aligned to 4-byte boundary in file, will not use mmap()\n");
        use_mmap = 0;
    }

    /* Allocate memory for pdfs */
    if (use_mmap) {
        s->OPDF_8B = ckd_calloc(s->n_feat + 1, sizeof(unsigned char**));
        for (i = 0; i < s->n_feat; i++) {
            /* Pointers into the mmap()ed 2d array */
	    s->OPDF_8B[i] = 
                (unsigned char **) ckd_calloc(r, sizeof(unsigned char *));
        }

	/* Use the last index to hide a pointer to the entire file's memory */
        s->OPDF_8B[s->n_feat] = s2_mmap(file);
        for (n = 0; n < s->n_feat; n++) {
            for (i = 0; i < r; i++) {
                s->OPDF_8B[n][i] = (char *) ((caddr_t) s->OPDF_8B[s->n_feat] + offset);
                offset += c;
            }
        }
    }
    else {
        s->OPDF_8B = (unsigned char ***)
            ckd_calloc_3d(s->n_feat, r, c, sizeof(unsigned char));
        /* Read pdf values and ids */
        for (n = 0; n < s->n_feat; n++) {
            for (i = 0; i < r; i++) {
                if (fread(s->OPDF_8B[n][i], sizeof(unsigned char), c, fp) != (size_t) c)
                    E_FATAL("fread failed\n");
            }
        }
    }

    fclose(fp);
    return 0;
}

static void
dump_probs_8b(s2_semi_mgau_t *s,
              char const *file, /* output file */
              char const *dir)
{                               /* **ORIGINAL** HMM directory */
    FILE *fp;
    int32 f, i, k, aligned_c;
    static char const *title = "V6 Senone Probs, Smoothed, Normalized";
    static char const *clust_hdr = "cluster_count 0";

    E_INFO("Dumping quantized HMMs to dump file %s\n", file);

    if ((fp = fopen(file, "wb")) == NULL) {
        E_ERROR("fopen(%s,wb) failed\n", file);
        return;
    }

#define fwrite_int32(f,v) fwrite(&v,sizeof(v),1,f)
    /* Write title size and title (directory name) */
    k = strlen(title) + 1;      /* including trailing null-char */
    fwrite_int32(fp, k);
    fwrite(title, sizeof(char), k, fp);

    /* Write header size and header (directory name) */
    k = strlen(dir) + 1;        /* including trailing null-char */
    fwrite_int32(fp, k);
    fwrite(dir, sizeof(char), k, fp);

    /* Write cluster count = 0 to indicate this is pre-quantized */
    k = strlen(clust_hdr) + 1;
    fwrite_int32(fp, k);
    fwrite(clust_hdr, sizeof(char), k, fp);

    /* Pad it out for alignment purposes */
    k = ftell(fp) & 3;
    if (k > 0) {
        k = 4 - k;
        fwrite_int32(fp, k);
        fwrite("!!!!", 1, 4, fp);
    }

    /* Write 0, terminating header strings */
    k = 0;
    fwrite_int32(fp, k);

    /* Align the number of pdfs to a 4-byte boundary. */
    aligned_c = (s->n_density + 3) & ~3;

    /* Write #rows, #cols; this also indicates whether pdfs already transposed */
    fwrite_int32(fp, s->CdWdPDFMod);
    fwrite_int32(fp, aligned_c);

    /* Now write out the quantized senones. */
    for (f = 0; f < s->n_feat; ++f) {
        for (i = 0; i < s->CdWdPDFMod; i++) {
            fwrite(s->OPDF_8B[f][i], sizeof(char), s->n_density, fp);
            /* Pad them out for alignment purposes */
            fwrite("\0\0\0\0", 1, aligned_c - s->n_density, fp);
        }
    }

    fclose(fp);
}

static int32
read_dists_s3(s2_semi_mgau_t * s, char const *file_name, double SmoothMin)
{
    char **argname, **argval;
    char eofchk;
    FILE *fp;
    int32 byteswap, chksum_present;
    uint32 chksum;
    float32 *pdf;
    int32 i, f, c, n;
    int32 n_sen;
    int32 n_feat;
    int32 n_comp;
    int32 n_err;
    char *dumpfile;

    dumpfile = kb_get_senprob_dump_file();
    if (dumpfile) {
        if (load_senone_dists_8bits(s, dumpfile) == 0)
            return bin_mdef_n_sen(mdef);
        else
            E_INFO
                ("No senone dump file found, will compress mixture weights on-line\n");
    }
    E_INFO("Reading mixture weights file '%s'\n", file_name);

    if ((fp = fopen(file_name, "rb")) == NULL)
        E_FATAL("fopen(%s,rb) failed\n", file_name);

    /* Read header, including argument-value info and 32-bit byteorder magic */
    if (bio_readhdr(fp, &argname, &argval, &byteswap) < 0)
        E_FATAL("bio_readhdr(%s) failed\n", file_name);

    /* Parse argument-value list */
    chksum_present = 0;
    for (i = 0; argname[i]; i++) {
        if (strcmp(argname[i], "version") == 0) {
            if (strcmp(argval[i], MGAU_MIXW_VERSION) != 0)
                E_WARN("Version mismatch(%s): %s, expecting %s\n",
                       file_name, argval[i], MGAU_MIXW_VERSION);
        }
        else if (strcmp(argname[i], "chksum0") == 0) {
            chksum_present = 1; /* Ignore the associated value */
        }
    }
    bio_hdrarg_free(argname, argval);
    argname = argval = NULL;

    chksum = 0;

    /* Read #senones, #features, #codewords, arraysize */
    if ((bio_fread(&n_sen, sizeof(int32), 1, fp, byteswap, &chksum) != 1)
        || (bio_fread(&n_feat, sizeof(int32), 1, fp, byteswap, &chksum) !=
            1)
        || (bio_fread(&n_comp, sizeof(int32), 1, fp, byteswap, &chksum) !=
            1)
        || (bio_fread(&n, sizeof(int32), 1, fp, byteswap, &chksum) != 1)) {
        E_FATAL("bio_fread(%s) (arraysize) failed\n", file_name);
    }
    if (n_feat != s->n_feat)
        E_FATAL("#Features streams(%d) != %d\n", n_feat, s->n_feat);
    if (n != n_sen * n_feat * n_comp) {
        E_FATAL
            ("%s: #float32s(%d) doesn't match header dimensions: %d x %d x %d\n",
             file_name, i, n_sen, n_feat, n_comp);
    }

    /* Quantized mixture weight arrays. */
    s->OPDF_8B = (unsigned char ***)
        ckd_calloc_3d(s->n_feat, s->n_density, n_sen, sizeof(unsigned char));

    /* Temporary structure to read in floats before conversion to (int32) logs3 */
    pdf = (float32 *) ckd_calloc(n_comp, sizeof(float32));

    /* Read senone probs data, normalize, floor, convert to logs3, truncate to 8 bits */
    n_err = 0;
    for (i = 0; i < n_sen; i++) {
        for (f = 0; f < n_feat; f++) {
            if (bio_fread((void *) pdf, sizeof(float32),
                          n_comp, fp, byteswap, &chksum) != n_comp) {
                E_FATAL("bio_fread(%s) (arraydata) failed\n", file_name);
            }

            /* Normalize and floor */
            if (vector_sum_norm(pdf, n_comp) <= 0.0)
                n_err++;
            vector_floor(pdf, n_comp, SmoothMin);
            vector_sum_norm(pdf, n_comp);

            /* Convert to LOG, quantize, and transpose */
            for (c = 0; c < n_comp; c++) {
                int32 qscr;

                qscr = LOG(pdf[c]);
                /* ** HACK!! ** hardwired threshold!!! */
                if (qscr < -161900)
                    E_FATAL("**ERROR** Too low senone PDF value: %d\n",
                            qscr);
                qscr = (511 - qscr) >> 10;
                if ((qscr > 255) || (qscr < 0))
                    E_FATAL("scr(%d,%d,%d) = %d\n", f, c, i, qscr);
                s->OPDF_8B[f][c][i] = qscr;
            }
        }
    }
    if (n_err > 0)
        E_ERROR("Weight normalization failed for %d senones\n", n_err);

    ckd_free(pdf);

    if (chksum_present)
        bio_verify_chksum(fp, byteswap, chksum);

    if (fread(&eofchk, 1, 1, fp) == 1)
        E_FATAL("More data than expected in %s\n", file_name);

    fclose(fp);

    E_INFO("Read %d x %d x %d mixture weights\n", n_sen, n_feat, n_comp);

    if ((dumpfile = kb_get_senprob_dump_file()) != NULL) {
        dump_probs_8b(s, dumpfile, file_name);
    }
    return n_sen;
}


/* Read a Sphinx3 mean or variance file. */
static int32
s3_read_mgau(s2_semi_mgau_t *s, const char *file_name, float32 ***out_cb)
{
    char tmp;
    FILE *fp;
    int32 i, blk, n;
    int32 n_mgau;
    int32 n_feat;
    int32 n_density;
    int32 *veclen;
    int32 byteswap, chksum_present;
    char **argname, **argval;
    uint32 chksum;

    E_INFO("Reading S3 mixture gaussian file '%s'\n", file_name);

    if ((fp = fopen(file_name, "rb")) == NULL)
        E_FATAL("fopen(%s,rb) failed\n", file_name);

    /* Read header, including argument-value info and 32-bit byteorder magic */
    if (bio_readhdr(fp, &argname, &argval, &byteswap) < 0)
        E_FATAL("bio_readhdr(%s) failed\n", file_name);

    /* Parse argument-value list */
    chksum_present = 0;
    for (i = 0; argname[i]; i++) {
        if (strcmp(argname[i], "version") == 0) {
            if (strcmp(argval[i], MGAU_PARAM_VERSION) != 0)
                E_WARN("Version mismatch(%s): %s, expecting %s\n",
                       file_name, argval[i], MGAU_PARAM_VERSION);
        }
        else if (strcmp(argname[i], "chksum0") == 0) {
            chksum_present = 1; /* Ignore the associated value */
        }
    }
    bio_hdrarg_free(argname, argval);
    argname = argval = NULL;

    chksum = 0;

    /* #Codebooks */
    if (bio_fread(&n_mgau, sizeof(int32), 1, fp, byteswap, &chksum) != 1)
        E_FATAL("fread(%s) (#codebooks) failed\n", file_name);
    if (n_mgau != 1)
        E_FATAL("%s: #codebooks (%d) != 1\n", file_name, n_mgau);

    /* #Features/codebook */
    if (bio_fread(&n_feat, sizeof(int32), 1, fp, byteswap, &chksum) != 1)
        E_FATAL("fread(%s) (#features) failed\n", file_name);
    if (s->n_feat == 0)
        s->n_feat = n_feat;
    else if (n_feat != s->n_feat)
        E_FATAL("#Features streams(%d) != %d\n", n_feat, s->n_feat);

    /* #Gaussian densities/feature in each codebook */
    if (bio_fread(&n_density, sizeof(int32), 1, fp,
                  byteswap, &chksum) != 1)
        E_FATAL("fread(%s) (#density/codebook) failed\n", file_name);
    if (s->n_density == 0)
        s->n_density = n_density;
    else if (n_density != s->n_density)
        E_FATAL("%s: Number of densities per feature(%d) != %d\n",
                file_name, n_mgau, s->n_density);

    /* Vector length of feature stream */
    if (s->veclen == NULL)
        s->veclen = ckd_calloc(s->n_feat, sizeof(int32));
    veclen = ckd_calloc(s->n_feat, sizeof(int32));
    if (bio_fread(veclen, sizeof(int32), s->n_feat,
                  fp, byteswap, &chksum) != s->n_feat)
        E_FATAL("fread(%s) (feature vector-length) failed\n", file_name);
    for (i = 0, blk = 0; i < s->n_feat; ++i) {
        if (s->veclen[i] == 0)
            s->veclen[i] = veclen[i];
        else if (veclen[i] != s->veclen[i])
            E_FATAL("feature stream length %d is inconsistent (%d != %d)\n",
                    i, veclen[i], s->veclen[i]);
        blk += veclen[i];
    }

    /* #Floats to follow; for the ENTIRE SET of CODEBOOKS */
    if (bio_fread(&n, sizeof(int32), 1, fp, byteswap, &chksum) != 1)
        E_FATAL("fread(%s) (total #floats) failed\n", file_name);
    if (n != n_mgau * n_density * blk)
        E_FATAL
            ("%s: #float32s(%d) doesn't match dimensions: %d x %d x %d\n",
             file_name, n, n_mgau, n_density, blk);

    *out_cb = ckd_calloc(s->n_feat, sizeof(float32 *));
    for (i = 0; i < s->n_feat; ++i) {
        (*out_cb)[i] =
            (float32 *) ckd_calloc(n_density * veclen[i],
                                   sizeof(float32));
        if (bio_fread
            ((*out_cb)[i], sizeof(float32),
             n_density * veclen[i], fp,
             byteswap, &chksum) != n_density * veclen[i])
            E_FATAL("fread(%s, %d) of feat %d failed\n", file_name,
                    n_density * veclen[i], i);
    }
    ckd_free(veclen);

    if (chksum_present)
        bio_verify_chksum(fp, byteswap, chksum);

    if (fread(&tmp, 1, 1, fp) == 1)
        E_FATAL("%s: More data than expected\n", file_name);

    fclose(fp);

    E_INFO("%d mixture Gaussians, %d components, veclen %d\n", n_mgau,
           n_density, blk);

    return n;
}

static int32
s3_precomp(s2_semi_mgau_t *s, float32 vFloor)
{
    int feat;

    for (feat = 0; feat < s->n_feat; ++feat) {
        float32 *fmp;
        mean_t *mp;
        var_t *vp;
        int32 *dp, vecLen, i;

        vecLen = s->veclen[feat];
        fmp = (float32 *) s->means[feat];
        mp = s->means[feat];
        vp = s->vars[feat];
        dp = s->dets[feat];

        for (i = 0; i < s->n_density; ++i) {
            int32 d, j;

            d = 0;
            for (j = 0; j < vecLen; ++j, ++vp, ++mp, ++fmp) {
                float64 fvar;

#ifdef FIXED_POINT
                *mp = FLOAT2FIX(*fmp);
#endif
                /* Always do these pre-calculations in floating point */
                fvar = *(float32 *) vp;
                if (fvar < vFloor)
                    fvar = vFloor;
                d += LOG(1 / sqrt(fvar * 2.0 * PI));
                *vp = (var_t) (1.0 / (2.0 * fvar * LOG_BASE));
            }
            *dp++ = d;
        }
    }
    return 0;
}

s2_semi_mgau_t *
s2_semi_mgau_init(const char *mean_path, const char *var_path,
                  float64 varfloor, const char *mixw_path,
                  float64 mixwfloor, int32 topn)
{
    s2_semi_mgau_t *s;
    int i;

    s = ckd_calloc(1, sizeof(*s));

    /* Read means and variances. */
    if (s3_read_mgau(s, mean_path, (float32 ***)&s->means) < 0) {
        /* FIXME: This actually is not enough to really free everything. */
        ckd_free(s);
        return NULL;
    }
    if (s3_read_mgau(s, var_path, (float32 ***)&s->vars) < 0) {
        /* FIXME: This actually is not enough to really free everything. */
        ckd_free(s);
        return NULL;
    }
    /* Precompute (and fixed-point-ize) means, variances, and determinants. */
    s->dets = (int32 **)ckd_calloc_2d(s->n_feat, s->n_density, sizeof(int32));
    s3_precomp(s, varfloor);

    /* Read mixture weights (gives us CdWdPDFMod = number of
     * mixture weights per codeword, which is fixed at the number
     * of senones since we have only one codebook) */
    s->CdWdPDFMod = read_dists_s3(s, mixw_path, mixwfloor);
    s->topN = topn;
    s->ds_ratio = 1;

    /* Top-N scores from current, previous frame */
    s->f = (vqFeature_t **) ckd_calloc_2d(s->n_feat, topn,
                                          sizeof(vqFeature_t));
    s->lastf = (vqFeature_t **) ckd_calloc_2d(s->n_feat, topn,
                                              sizeof(vqFeature_t));
    for (i = 0; i < s->n_feat; ++i) {
        int32 j;

        for (j = 0; j < topn; ++j) {
            s->lastf[i][j].val.dist = WORST_DIST;
            s->lastf[i][j].codeword = j;
        }
    }

    /* Temporary array used in senone eval */
    s->score_tmp = ckd_calloc(s->n_feat, sizeof(int32));

    return s;
}

void
s2_semi_mgau_free(s2_semi_mgau_t * s)
{
    /* FIXME: Need to free stuff. */
    ckd_free(s);
}
