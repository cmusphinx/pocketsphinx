/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 2013 Carnegie Mellon University.  All rights
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
 * kws_search.c -- Search object for key phrase spotting.
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <sphinxbase/err.h>
#include <sphinxbase/ckd_alloc.h>
#include <sphinxbase/strfuncs.h>
#include <sphinxbase/cmd_ln.h>

#include "pocketsphinx_internal.h"
#include "kws_search.h"


/* Cap functions to meet ps_search api */
static ps_seg_t *
kws_search_seg_iter(ps_search_t * search, int32 * out_score)
{
    *out_score = 0;
    return NULL;
}

static ps_lattice_t *
kws_search_lattice(ps_search_t * search)
{
    return NULL;
}

static int
kws_search_prob(ps_search_t * search)
{
    return 0;
}

static ps_searchfuncs_t kws_funcs = {
    /* name: */ "kws",
    /* start: */ kws_search_start,
    /* step: */ kws_search_step,
    /* finish: */ kws_search_finish,
    /* reinit: */ kws_search_reinit,
    /* free: */ kws_search_free,
    /* lattice: */ kws_search_lattice,
    /* hyp: */ kws_search_hyp,
    /* prob: */ kws_search_prob,
    /* seg_iter: */ kws_search_seg_iter,
};

/* Scans the dictionary and check if all words are present. */
static int
kws_search_check_dict(kws_search_t * kwss)
{
    dict_t *dict;
    char **wrdptr;
    char *tmp_keyphrase;
    int32 nwrds, wid;
    int i;
    uint8 success;

    success = TRUE;
    dict = ps_search_dict(kwss);
    tmp_keyphrase = (char *) ckd_salloc(kwss->keyphrase);
    nwrds = str2words(tmp_keyphrase, NULL, 0);
    wrdptr = (char **) ckd_calloc(nwrds, sizeof(*wrdptr));
    str2words(tmp_keyphrase, wrdptr, nwrds);
    for (i = 0; i < nwrds; i++) {
        wid = dict_wordid(dict, wrdptr[i]);
        if (wid == BAD_S3WID) {
            E_ERROR("The word '%s' is missing in the dictionary\n",
                    wrdptr[i]);
            success = FALSE;
            break;
        }
    }
    ckd_free(wrdptr);
    ckd_free(tmp_keyphrase);
    return success;
}

/* Activate senones for scoring */
static void
kws_search_sen_active(kws_search_t * kwss)
{
    int i;

    acmod_clear_active(ps_search_acmod(kwss));

    /* active phone loop hmms */
    for (i = 0; i < kwss->n_pl; i++)
        acmod_activate_hmm(ps_search_acmod(kwss), &kwss->pl_hmms[i]);

    /* activate hmms in active nodes */
    for (i = 0; i < kwss->n_nodes; i++) {
        if (kwss->nodes[i].active)
            acmod_activate_hmm(ps_search_acmod(kwss), &kwss->nodes[i].hmm);
    }
}

/*
 * Evaluate all the active HMMs.
 * (Executed once per frame.)
 */
static void
kws_search_hmm_eval(kws_search_t * kwss, int16 const *senscr)
{
    int32 i;
    int32 bestscore = WORST_SCORE;

    hmm_context_set_senscore(kwss->hmmctx, senscr);

    /* evaluate hmms from phone loop */
    for (i = 0; i < kwss->n_pl; ++i) {
        hmm_t *hmm = &kwss->pl_hmms[i];
        int32 score;

        score = hmm_vit_eval(hmm);
        if (score BETTER_THAN bestscore)
            bestscore = score;
    }
    /* evaluate hmms for active nodes */
    for (i = 0; i < kwss->n_nodes; i++) {
        if (kwss->nodes[i].active) {
            hmm_t *hmm = &kwss->nodes[i].hmm;
            int32 score;

            score = hmm_vit_eval(hmm);
            if (score BETTER_THAN bestscore)
                bestscore = score;
        }
    }

    kwss->bestscore = bestscore;
}

/*
 * (Beam) prune the just evaluated HMMs, determine which ones remain
 * active. Executed once per frame.
 */
static void
kws_search_hmm_prune(kws_search_t *kwss)
{
    int32 thresh, i;

    thresh = kwss->bestscore + kwss->beam;

    for (i = 0; i < kwss->n_nodes; i++) {
        if (kwss->nodes[i].active) {
            if (hmm_bestscore(&kwss->nodes[i].hmm) < thresh) {
        	kwss->nodes[i].active = FALSE;
        	hmm_clear(&kwss->nodes[i].hmm);
    	    }
    	}
    }
    return;
}


/**
 * Do phone transitions
 */
static void
kws_search_trans(kws_search_t * kwss)
{
    hmm_t *pl_best_hmm = NULL;
    int32 best_out_score = WORST_SCORE;
    int i;

    /* select best hmm in phone-loop to be a predecessor */
    for (i = 0; i < kwss->n_pl; i++)
        if (hmm_out_score(&kwss->pl_hmms[i]) BETTER_THAN best_out_score) {
            best_out_score = hmm_out_score(&kwss->pl_hmms[i]);
            pl_best_hmm = &kwss->pl_hmms[i];
        }

    /* out probs are not ready yet */
    if (!pl_best_hmm)
        return;

    /* Check whether keyword wasn't spotted yet */
    if (kwss->nodes[kwss->n_nodes - 1].active
        && hmm_out_score(pl_best_hmm) BETTER_THAN WORST_SCORE) {

        /* E_INFO("%d; %d\n",
           hmm_out_score(&kwss->nodes[kwss->n_nodes-1].hmm),
           hmm_out_score(pl_best_hmm),
           hmm_out_score(&kwss->nodes[kwss->n_nodes-1].hmm) -
           hmm_out_score(pl_best_hmm)); */

        if (hmm_out_score(&kwss->nodes[kwss->n_nodes - 1].hmm) -
            hmm_out_score(pl_best_hmm) >= kwss->threshold) {

            kwss->n_detect++;
            E_INFO(">>>>DETECTED IN FRAME [%d]\n", kwss->frame);
            pl_best_hmm = &kwss->nodes[kwss->n_nodes - 1].hmm;

            /* set all keyword nodes inactive for next occurrence search */
            for (i = 0; i < kwss->n_nodes; i++) {
                kwss->nodes[i].active = FALSE;
                hmm_clear_scores(&kwss->nodes[i].hmm);
            }
        }

    }

    /* Make transition for all phone loop hmms */
    for (i = 0; i < kwss->n_pl; i++) {
        if (hmm_out_score(pl_best_hmm) + kwss->plp BETTER_THAN
	    hmm_in_score(&kwss->nodes[0].hmm)) {
	    hmm_enter(&kwss->pl_hmms[i],
    	          hmm_out_score(pl_best_hmm) + kwss->plp,
                  hmm_out_history(pl_best_hmm), kwss->frame + 1);
	}
    }

    /* Activate new keyword nodes, enter their hmms */
    for (i = kwss->n_nodes - 1; i > 0; i--) {
        if (kwss->nodes[i - 1].active) {
            hmm_t *pred_hmm = &kwss->nodes[i - 1].hmm;
            if (!kwss->nodes[i].active
                || hmm_out_score(pred_hmm) BETTER_THAN
                hmm_in_score(&kwss->nodes[i].hmm)) {
                hmm_enter(&kwss->nodes[i].hmm, hmm_out_score(pred_hmm),
                          hmm_out_history(pred_hmm), kwss->frame + 1);
        	kwss->nodes[i].active = TRUE;
    	    }
        }
    }
    /* Enter keyword start node from phone loop */
    if (hmm_out_score(pl_best_hmm) BETTER_THAN
        hmm_in_score(&kwss->nodes[0].hmm)) {
        kwss->nodes[0].active = TRUE;
        hmm_enter(&kwss->nodes[0].hmm, hmm_out_score(pl_best_hmm),
                  hmm_out_history(pl_best_hmm), kwss->frame + 1);
    }
}

ps_search_t *
kws_search_init(const char *key_phrase,
                cmd_ln_t * config,
                acmod_t * acmod, dict_t * dict, dict2pid_t * d2p)
{
    kws_search_t *kwss = (kws_search_t *) ckd_calloc(1, sizeof(*kwss));
    ps_search_init(ps_search_base(kwss), &kws_funcs, config, acmod, dict, d2p);

    kwss->beam = 
        (int32) logmath_log(acmod->lmath, cmd_ln_float64_r(config, "-beam")) >> SENSCR_SHIFT;

    kwss->plp =
        (int32) logmath_log(acmod->lmath, cmd_ln_float32_r(config, "-kws_plp")) >> SENSCR_SHIFT;

    kwss->threshold =
        (int32) logmath_log(acmod->lmath, cmd_ln_float64_r(config, "-kws_threshold")) >> SENSCR_SHIFT;

    E_INFO("KWS(beam: %d, plp: %d, threshold %d)\n",
           kwss->beam, kwss->plp, kwss->threshold);

    kwss->keyphrase = ckd_salloc(key_phrase);

    /* Check if all words are in dictionary */
    if (!kws_search_check_dict(kwss)) {
        kws_search_free(ps_search_base(kwss));
        return NULL;
    }

    /* Reinit for provided keyword */
    if (kws_search_reinit(ps_search_base(kwss),
                          ps_search_dict(kwss),
                          ps_search_dict2pid(kwss)) < 0) {
        ps_search_free(ps_search_base(kwss));
        return NULL;
    }

    return ps_search_base(kwss);
}

void
kws_search_free(ps_search_t * search)
{
    kws_search_t *kwss = (kws_search_t *) search;

    ps_search_deinit(search);
    hmm_context_free(kwss->hmmctx);

    ckd_free(kwss->pl_hmms);
    ckd_free(kwss->nodes);
    ckd_free(kwss->keyphrase);
    ckd_free(kwss);
}

int
kws_search_reinit(ps_search_t * search, dict_t * dict, dict2pid_t * d2p)
{
    char **wrdptr;
    char *tmp_keyphrase;
    int32 wid, pronlen;
    int32 n_nodes, n_wrds;
    int32 ssid, tmatid;
    int i, j, p;
    kws_search_t *kwss = (kws_search_t *) search;
    bin_mdef_t *mdef = search->acmod->mdef;
    int32 silcipid = bin_mdef_silphone(mdef);

    /* Free old dict2pid, dict */
    ps_search_base_reinit(search, dict, d2p);

    /* Initialize HMM context. */
    if (kwss->hmmctx)
        hmm_context_free(kwss->hmmctx);
    kwss->hmmctx =
        hmm_context_init(bin_mdef_n_emit_state(search->acmod->mdef),
                         search->acmod->tmat->tp, NULL,
                         search->acmod->mdef->sseq);
    if (kwss->hmmctx == NULL)
        return -1;

    /* Initialize phone loop HMMs. */
    if (kwss->pl_hmms) {
        for (i = 0; i < kwss->n_pl; ++i)
            hmm_deinit((hmm_t *) & kwss->pl_hmms[i]);
        ckd_free(kwss->pl_hmms);
    }
    kwss->n_pl = bin_mdef_n_ciphone(search->acmod->mdef);
    kwss->pl_hmms =
        (hmm_t *) ckd_calloc(kwss->n_pl, sizeof(*kwss->pl_hmms));
    for (i = 0; i < kwss->n_pl; ++i) {
        hmm_init(kwss->hmmctx, (hmm_t *) & kwss->pl_hmms[i],
                 FALSE,
                 bin_mdef_pid2ssid(search->acmod->mdef, i),
                 bin_mdef_pid2tmatid(search->acmod->mdef, i));
    }

    /* Initialize keyphrase HMMs */
    tmp_keyphrase = (char *) ckd_salloc(kwss->keyphrase);
    n_wrds = str2words(tmp_keyphrase, NULL, 0);
    wrdptr = (char **) ckd_calloc(n_wrds, sizeof(*wrdptr));
    str2words(tmp_keyphrase, wrdptr, n_wrds);

    /* count amount of nodes */
    n_nodes = 0;
    for (i = 0; i < n_wrds; i++) {
        wid = dict_wordid(dict, wrdptr[i]);
        pronlen = dict_pronlen(dict, wid);
        n_nodes += pronlen;
    }

    /* allocate node array */
    if (kwss->nodes)
        ckd_free(kwss->nodes);
    kwss->nodes = (kws_node_t *) ckd_calloc(n_nodes, sizeof(kws_node_t));
    kwss->n_nodes = n_nodes;

    /* fill node array */
    j = 0;
    for (i = 0; i < n_wrds; i++) {
        wid = dict_wordid(dict, wrdptr[i]);
        pronlen = dict_pronlen(dict, wid);
        for (p = 0; p < pronlen; p++) {
            int32 ci = dict_pron(dict, wid, p);
            if (p == 0) {
                /* first phone of word */
                int32 rc =
                    pronlen > 1 ? dict_pron(dict, wid, 1) : silcipid;
                ssid = dict2pid_ldiph_lc(d2p, ci, rc, silcipid);
            }
            else if (p == pronlen - 1) {
                /* last phone of the word */
                int32 lc = dict_pron(dict, wid, p - 1);
                xwdssid_t *rssid = dict2pid_rssid(d2p, ci, lc);
                int j = rssid->cimap[silcipid];
                ssid = rssid->ssid[j];
            }
            else {
                /* word internal phone */
                ssid = dict2pid_internal(d2p, wid, p);
            }
            tmatid = bin_mdef_pid2tmatid(mdef, ci);
            hmm_init(kwss->hmmctx, &kwss->nodes[j].hmm, FALSE, ssid,
                     tmatid);
            kwss->nodes[j].active = FALSE;
            j++;
        }
    }

    ckd_free(wrdptr);
    ckd_free(tmp_keyphrase);
    return 0;
}

int
kws_search_start(ps_search_t * search)
{
    int i;
    kws_search_t *kwss = (kws_search_t *) search;

    kwss->frame = 0;
    kwss->n_detect = 0;
    kwss->bestscore = 0;

    /* Reset and enter all phone-loop HMMs. */
    for (i = 0; i < kwss->n_pl; ++i) {
        hmm_t *hmm = (hmm_t *) & kwss->pl_hmms[i];
        hmm_clear(hmm);
        hmm_enter(hmm, 0, -1, 0);
    }
    return 0;
}

int
kws_search_step(ps_search_t * search, int frame_idx)
{
    int16 const *senscr;
    kws_search_t *kwss = (kws_search_t *) search;
    acmod_t *acmod = search->acmod;

    /* Activate senones */
    if (!acmod->compallsen)
        kws_search_sen_active(kwss);

    /* Calculate senone scores for current frame. */
    senscr = acmod_score(acmod, &frame_idx);

    /* Evaluate hmms in phone loop and in active keyword nodes */
    kws_search_hmm_eval(kwss, senscr);

    /* Prune hmms with low prob */
    kws_search_hmm_prune(kwss);

    /* Do hmms transitions */
    kws_search_trans(kwss);

    ++kwss->frame;
    return 0;
}

int
kws_search_finish(ps_search_t * search)
{
    /* Nothing here */
    return 0;
}

char const *
kws_search_hyp(ps_search_t * search, int32 * out_score,
               int32 * out_is_final)
{
    kws_search_t *kwss = (kws_search_t *) search;

    if (kwss->n_detect > 0) {
        if (out_score)
            *out_score = kwss->n_detect;
        return kwss->keyphrase;
    }
    else {
        if (out_score)
            *out_score = 0;
        return NULL;
    }
}
