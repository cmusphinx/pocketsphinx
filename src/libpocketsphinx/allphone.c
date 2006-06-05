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
 * allphone.c -- All CI phone decoding.
 *
 * HISTORY
 * 
 * $Log: allphone.c,v $
 * Revision 1.2  2006/05/26 13:52:46  dhuggins
 * fix a subtle allphone bug
 *
 * Revision 1.1.1.1  2006/05/23 18:44:59  dhuggins
 * re-importation
 *
 * Revision 1.12  2004/12/10 16:48:56  rkm
 * Added continuous density acoustic model handling
 *
 * 
 * 22-Nov-2004	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Modified to use unified semi-continuous/continuous acoustic
 * 		model evaluation module (senscr).
 * 
 * 06-Aug-2004	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Added phonetp (phone transition probs matrix) to search.
 * 
 * 10-Sep-98	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Modified to let allphone_utt return phone segmentation result,
 * 		instead of simply printing it to the standard log file.
 * 		Added print_back_trace option.
 * 
 * 08-Sep-98	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Added check for presence of SILb/SILe in allphone_start_utt and
 * 		allphone_result.
 * 
 * 01-Jan-95	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Created.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "s2types.h"
#include "CM_macros.h"
#include "basic_types.h"
#include "search_const.h"
#include "linklist.h"
#include "list.h"
#include "hash.h"
#include "phone.h"
#include "err.h"
#include "log.h"
#include "scvq.h"
#include "senscr.h"
#include "msd.h"
#include "dict.h"
#include "hmm_tied_r.h"
#include "lmclass.h"
#include "lm_3g.h"
#include "kb.h"
#include "fbs.h"
#include "search.h"

static SMD *Models;		/* static model types */
static int32 *senscr;

static CHAN_T *ci_chan;		/* hmm model instances for each CI phone */
static int32 n_ciphone;
static int32 *renorm_scr;

static int32 allphone_bw;	/* beam width */
static int32 allphone_exitbw;	/* phone exit beam width */
static int32 allphone_pip;	/* phone insertion penalty */
static int32 **phonetp;		/* phone transition LOGprob */

typedef struct {
    int32 f;
    int32 p;
    int32 scr;
    int32 bp;
} allphone_bp_t;
static allphone_bp_t *allphone_bp;
static int32 n_bp;
#define ALLPHONE_BP_MAX		65536

static search_hyp_t *allp_seghyp = NULL, *allp_seghyp_tail;

extern int32 *senone_active;
extern int32 n_senone_active;

static void allphone_start_utt ( void )
{
    int32 s, p;
    
    for (p = 0; p < n_ciphone; p++)
	ci_chan[p].active = -1;

#if 0 /* note that SILb isn't in the current 4k models --kal */
    p = phone_to_id ("SILb", TRUE);
    if (p < 0) {
      p = phone_to_id ("SIL", TRUE);
      if (p < 0)
	E_FATAL("SILb/SIL not found\n");
    }
#else
    p = phone_to_id ("SIL", TRUE);
    if (p < 0)
      E_FATAL("SILb/SIL not found\n");
#endif
    
    ci_chan[p].score[0] = 0;
    for (s = 1; s < HMM_LAST_STATE; s++)
	ci_chan[p].score[s] = WORST_SCORE;
    ci_chan[p].path[0] = -1;
    ci_chan[p].active = 0;

    n_bp = 0;
}

static void allphone_senone_active ( void )
{
    int32 p, s, n, *dist;
    
    n = 0;
    for (p = 0; p < n_ciphone; p++) {
	dist = Models[ci_chan[p].sseqid].dist;
	for (s = 0; s < TRANS_CNT; s += 3)
	    senone_active[n++] = dist[s];
    }
    n_senone_active = n;
}

static int32 allphone_eval_ci_chan (int32 f)
{
    int32 p, bestscr;
    
    bestscr = WORST_SCORE;
    for (p = 0; p < n_ciphone; p++) {
	if (ci_chan[p].active != f)
	    continue;
	
	chan_v_eval (ci_chan+p);

	if (bestscr < ci_chan[p].bestscore)
	    bestscr = ci_chan[p].bestscore;
    }
    
    return (bestscr);
}

static void allphone_bp_entry (int32 f, int32 p)
{
    if (n_bp == ALLPHONE_BP_MAX-2)
	E_ERROR("BP table full\n");
    
    if (n_bp >= ALLPHONE_BP_MAX)
	return;
    
    allphone_bp[n_bp].f = f;
    allphone_bp[n_bp].p = p;
    allphone_bp[n_bp].scr = ci_chan[p].score[HMM_LAST_STATE];
    allphone_bp[n_bp].bp = ci_chan[p].path[HMM_LAST_STATE];
    n_bp++;
}

static void allphone_chan_prune (int32 f, int32 bestscr)
{
    int32 p, thresh, exit_thresh;
    
    thresh = bestscr + allphone_bw;
    exit_thresh = bestscr + allphone_exitbw;

    for (p = 0; p < n_ciphone; p++) {
	if (ci_chan[p].active != f)
	    continue;
	
	if (ci_chan[p].bestscore > thresh) {
	    ci_chan[p].active = f+1;
	    
	    if (ci_chan[p].score[HMM_LAST_STATE] > exit_thresh)
		allphone_bp_entry (f, p);
	}
    }
}

static void allphone_chan_trans (int32 f, int32 bp)
{
    int32 p, scr, s, predp;

    predp = allphone_bp[bp].p;
    
    for (p = 0; p < n_ciphone; p++) {
      scr = allphone_bp[bp].scr + phonetp[predp][p];
      
      if ((ci_chan[p].active < f) || (ci_chan[p].score[0] < scr)) {
	ci_chan[p].score[0] = scr;
	
	if (ci_chan[p].active < f)
	  for (s = 1; s < HMM_LAST_STATE; s++)
	    ci_chan[p].score[s] = WORST_SCORE;
	
	ci_chan[p].path[0] = bp;
	ci_chan[p].active = f+1;
      }
    }
}

static void allphone_renorm (int32 f, int32 bestscr)
{
    int32 p, s;
    
    for (p = 0; p < n_ciphone; p++) {
	if (ci_chan[p].active == f) {
	    for (s = 0; s < HMM_LAST_STATE; s++)
		if (ci_chan[p].score[s] > WORST_SCORE)
		    ci_chan[p].score[s] -= bestscr;
	}
    }

    renorm_scr[f] = bestscr;
}

/* Special version which frees the phone strings too. */
static void allphone_utt_seghyp_free (search_hyp_t *h)
{
    search_hyp_t *tmp;

    while (h) {
        tmp = h->next;
	free((void*)h->word);
        listelem_free (h, sizeof(search_hyp_t));
        h = tmp;
    }
}


static void allphone_backtrace (int32 bp)
{
    int32 bf, f, nf, bscr, escr;
    search_hyp_t *h;
    
    if (bp < 0)
	return;

    allphone_backtrace (allphone_bp[bp].bp);

    if (allphone_bp[bp].bp < 0) {
	bf = 0;
	bscr = 0;
    } else {
	bf = allphone_bp[allphone_bp[bp].bp].f+1;
	bscr = allphone_bp[allphone_bp[bp].bp].scr;
    }
    for (escr = allphone_bp[bp].scr, f = bf; f <= allphone_bp[bp].f; f++)
	escr += renorm_scr[f];
    
    nf = allphone_bp[bp].f - bf + 1;

    h = (search_hyp_t *) listelem_alloc (sizeof(search_hyp_t));
    h->wid = allphone_bp[bp].p;
    /* The result of phone_from_id is ephemeral in PocketSphinx.  So
     * makesure to use allphone_utt_seghyp_free(). */
    h->word = strdup(phone_from_id (allphone_bp[bp].p));
    h->sf = bf;
    h->ef = allphone_bp[bp].f;
    h->next = NULL;
    if (allp_seghyp_tail)
	allp_seghyp_tail->next = h;
    else
	allp_seghyp = h;
    allp_seghyp_tail = h;
    
    if (query_back_trace()) {
	printf ("\t%5d %5d %10d %11d %s\n",
		allp_seghyp_tail->sf,
		allp_seghyp_tail->ef, (escr-bscr)/nf, escr-bscr,
		allp_seghyp_tail->word);
    }
}

static void allphone_result ( void )
{
    int32 i, b, f, sile, bestbp, scr;
    
    if (n_bp <= 0) {
	printf ("NO ALIGNMENT\n");
	return;
    }

    f = allphone_bp[n_bp-1].f;
    for (b = n_bp-2; (b >= 0) && (allphone_bp[b].f == f); --b);
    b++;
    
    sile = phone_to_id ("SILe", TRUE);
    if (sile < 0)
	sile = phone_to_id ("SIL", TRUE);
    /* No need to check if SIL exists; already checked in allphone_start_utt */

    for (i = b; (i < n_bp) && (allphone_bp[i].p != sile); i++);
    if (i >= n_bp) {
	printf ("UTTERANCE DID NOT END IN SILe\n");
	bestbp = b;
	for (i = b+1; i < n_bp; i++)
	    if (allphone_bp[i].scr > allphone_bp[bestbp].scr)
		bestbp = i;
    } else
	bestbp = i;
    
    if (query_back_trace()) {
      printf ("\t%5s %5s %10s %11s %s (Allphone) (%s)\n",
	      "SFrm", "EFrm", "AScr/Frm", "AScr", "Phone",
	      uttproc_get_uttid());
      printf ("\t------------------------------------------------------------\n");
    }
    allphone_backtrace (bestbp);
    if (query_back_trace() && (bestbp >= 0)) {
      assert (allphone_bp[bestbp].f >= 0);
      
      scr = allphone_bp[bestbp].scr;
      for (f = 0; f <= allphone_bp[bestbp].f; f++)
	scr += renorm_scr[f];
      
      printf ("\t------------------------------------------------------------\n");
      f = allphone_bp[bestbp].f;
      if (f >= 0) {
	printf ("\t%5d %5d %10d %11d %s(TOTAL)\n",
		0, f,
		scr / (f + 1), scr,
		uttproc_get_uttid());
      }
    }
}

search_hyp_t *allphone_utt (int32 nfr,
			    mfcc_t *cep,
			    mfcc_t *dcep,
			    mfcc_t *dcep_80ms,
			    mfcc_t *pcep,
			    mfcc_t *ddcep)
{
    int32 i, f, c, p;
    int32 bestscr;
    int32 lastbp, bestbp;
    
    if (allp_seghyp)
	allphone_utt_seghyp_free (allp_seghyp);
    allp_seghyp = NULL;
    allp_seghyp_tail = NULL;
    
    allphone_senone_active ();
    allphone_start_utt();

    renorm_scr[0] = 0;
    
    for (f = 0, c = 0, p = 0; f < nfr; f++, c += CEP_SIZE, p += POW_SIZE) {
	senscr_active (senscr, cep+c, dcep+c, dcep_80ms+c, pcep+p, ddcep+c);

	if ((bestscr = allphone_eval_ci_chan (f)) <= WORST_SCORE) {
	    E_ERROR ("POOR MATCH: bestscore= %d\n", bestscr);
	    break;
	}
	
	lastbp = n_bp;
	allphone_chan_prune (f, bestscr);

	if (lastbp < n_bp) {
	    bestbp = lastbp;
	    for (i = lastbp+1; i < n_bp; i++) {
		if (allphone_bp[i].scr > allphone_bp[bestbp].scr)
		    bestbp = i;
	    }
	    
	    allphone_chan_trans (f, bestbp);
	}

	allphone_renorm (f+1, bestscr);
    }

    allphone_result ();

    return allp_seghyp;
}

void
allphone_init (double bw, double exitbw, double pip)
{
    int32 i;
    
    n_ciphone = phoneCiCount();
    
    ci_chan = (CHAN_T *) CM_calloc (n_ciphone, sizeof(CHAN_T));
    for (i = 0; i < n_ciphone; i++) {
	ci_chan[i].sseqid = hmm_pid2sid (i);
	ci_chan[i].ciphone = i;
    }

    renorm_scr = (int32 *) CM_calloc (MAX_FRAMES, sizeof(int32));
    
    Models = kb_get_models ();
    senscr = search_get_dist_scores ();

    allphone_bp = (allphone_bp_t *) CM_calloc (ALLPHONE_BP_MAX, sizeof(allphone_bp_t));

    allphone_bw = LOG(bw) * 8;
    allphone_exitbw = LOG(exitbw) * 8;
    allphone_pip = LOG(pip);

    phonetp = kb_get_phonetp();
    
    E_INFO("bw= %d, wordbw= %d, pip= %d\n",
	   allphone_bw, allphone_exitbw, allphone_pip);
}
