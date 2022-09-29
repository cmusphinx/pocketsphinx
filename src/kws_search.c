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

#include <pocketsphinx.h>

#include "util/ckd_alloc.h"
#include "util/strfuncs.h"
#include "util/pio.h"
#include "pocketsphinx_internal.h"
#include "kws_search.h"

/** Access macros */
#define hmm_is_active(hmm) ((hmm)->frame > 0)
#define kws_nth_hmm(keyphrase,n) (&((keyphrase)->hmms[n]))

/* Value selected experimentally as maximum difference between triphone
score and phone loop score, used in confidence computation to make sure
that confidence value is less than 1. This might be different for
different models. Corresponds to threshold of about 1e+50 */ 
#define KWS_MAX 1500

static ps_lattice_t *
kws_search_lattice(ps_search_t * search)
{
    (void)search;
    return NULL;
}

static int
kws_search_prob(ps_search_t * search)
{
    (void)search;
    return 0;
}

static void
kws_seg_free(ps_seg_t *seg)
{
    kws_seg_t *itor = (kws_seg_t *)seg;
    ckd_free(itor->detections);
    ckd_free(itor);
}

static void
kws_seg_fill(kws_seg_t *itor)
{
    kws_detection_t* detection = itor->detections[itor->pos];

    itor->base.text = detection->keyphrase;
    itor->base.wid = BAD_S3WID;
    itor->base.sf = detection->sf;
    itor->base.ef = detection->ef;
    itor->base.prob = detection->prob;
    itor->base.ascr = detection->ascr;
    itor->base.lscr = 0;
}

static ps_seg_t *
kws_seg_next(ps_seg_t *seg)
{
    kws_seg_t *itor = (kws_seg_t *)seg;

    if (++itor->pos == itor->n_detections) {
        kws_seg_free(seg);
        return NULL;
    }

    kws_seg_fill(itor);

    return seg;
}

static ps_segfuncs_t kws_segfuncs = {
    /* seg_next */ kws_seg_next,
    /* seg_free */ kws_seg_free
};

static ps_seg_t *
kws_search_seg_iter(ps_search_t * search)
{
    kws_search_t *kwss = (kws_search_t *)search;
    kws_seg_t *itor;
    gnode_t *detect_head = kwss->detections->detect_list;
    gnode_t *gn;
    
    while (detect_head != NULL && ((kws_detection_t*)gnode_ptr(detect_head))->ef > kwss->frame - kwss->delay)
	detect_head = gnode_next(detect_head);
    
    if (!detect_head)
        return NULL;

    /* Count and reverse them into the vector. */
    itor = ckd_calloc(1, sizeof(*itor));
    for (gn = detect_head; gn; gn = gnode_next(gn))
        itor->n_detections++;
    itor->detections = ckd_calloc(itor->n_detections, sizeof(*itor->detections));
    itor->pos = itor->n_detections - 1;
    for (gn = detect_head; gn; gn = gnode_next(gn))
        itor->detections[itor->pos--] = (kws_detection_t *)gnode_ptr(gn);
    itor->pos = 0;

    /* Copy the detections here to get them in the right order and
     * avoid undefined behaviour if recognition continues during
     * iteration. */

    itor->base.vt = &kws_segfuncs;
    itor->base.search = search;
    itor->base.lwf = 1.0;
    itor->last_frame = kwss->frame - kwss->delay;
    kws_seg_fill(itor);
    return (ps_seg_t *)itor;
}

static ps_searchfuncs_t kws_funcs = {
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


/* Activate senones for scoring */
static void
kws_search_sen_active(kws_search_t * kwss)
{
    int i;
    gnode_t *gn;

    acmod_clear_active(ps_search_acmod(kwss));

    /* active phone loop hmms */
    for (i = 0; i < kwss->n_pl; i++)
        acmod_activate_hmm(ps_search_acmod(kwss), &kwss->pl_hmms[i]);

    /* activate hmms in active nodes */
    for (gn = kwss->keyphrases; gn; gn = gnode_next(gn)) {
        kws_keyphrase_t *keyphrase = gnode_ptr(gn);
        for (i = 0; i < keyphrase->n_hmms; i++) {
            if (hmm_is_active(kws_nth_hmm(keyphrase, i)))
                acmod_activate_hmm(ps_search_acmod(kwss), kws_nth_hmm(keyphrase, i));
        }
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
    gnode_t *gn;
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
    for (gn = kwss->keyphrases; gn; gn = gnode_next(gn)) {
        kws_keyphrase_t *keyphrase = gnode_ptr(gn);
        for (i = 0; i < keyphrase->n_hmms; i++) {
            hmm_t *hmm = kws_nth_hmm(keyphrase, i);

            if (hmm_is_active(hmm)) {
                int32 score;
                score = hmm_vit_eval(hmm);
                if (score BETTER_THAN bestscore)
                    bestscore = score;
            }
        }
    }

    kwss->bestscore = bestscore;
}

/*
* (Beam) prune the just evaluated HMMs, determine which ones remain
* active. Executed once per frame.
*/
static void
kws_search_hmm_prune(kws_search_t * kwss)
{
    int32 thresh, i;
    gnode_t *gn;

    thresh = kwss->bestscore + kwss->beam;

    for (gn = kwss->keyphrases; gn; gn = gnode_next(gn)) {
        kws_keyphrase_t *keyphrase = gnode_ptr(gn);
        for (i = 0; i < keyphrase->n_hmms; i++) {
    	    hmm_t *hmm = kws_nth_hmm(keyphrase, i);
            if (hmm_is_active(hmm) && hmm_bestscore(hmm) < thresh)
                hmm_clear(hmm);
        }
    }
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
    gnode_t *gn;

    /* select best hmm in phone-loop to be a predecessor */
    for (i = 0; i < kwss->n_pl; i++)
        if (hmm_out_score(&kwss->pl_hmms[i]) BETTER_THAN best_out_score) {
            best_out_score = hmm_out_score(&kwss->pl_hmms[i]);
            pl_best_hmm = &kwss->pl_hmms[i];
        }

    /* out probs are not ready yet */
    if (!pl_best_hmm)
        return;

    /* Check whether keyphrase wasn't spotted yet */
    for (gn = kwss->keyphrases; gn; gn = gnode_next(gn)) {
        kws_keyphrase_t *keyphrase = gnode_ptr(gn);
        hmm_t *last_hmm;
        
        if (keyphrase->n_hmms < 1)
    	    continue;
        
        last_hmm = kws_nth_hmm(keyphrase, keyphrase->n_hmms - 1);

        if (hmm_is_active(last_hmm)
            && hmm_out_score(pl_best_hmm) BETTER_THAN WORST_SCORE) {

            if (hmm_out_score(last_hmm) - hmm_out_score(pl_best_hmm) 
                >= keyphrase->threshold) {

                int32 prob = hmm_out_score(last_hmm) - hmm_out_score(pl_best_hmm) - KWS_MAX;
                kws_detections_add(kwss->detections, keyphrase->word,
                                  hmm_out_history(last_hmm),
                                  kwss->frame, prob,
                                  hmm_out_score(last_hmm));
            } /* keyphrase is spotted */
        } /* last hmm of keyphrase is active */
    } /* keyphrase loop */

    /* Make transition for all phone loop hmms */
    for (i = 0; i < kwss->n_pl; i++) {
        if (hmm_out_score(pl_best_hmm) + kwss->plp BETTER_THAN
            hmm_in_score(&kwss->pl_hmms[i])) {
            hmm_enter(&kwss->pl_hmms[i],
                      hmm_out_score(pl_best_hmm) + kwss->plp,
                      hmm_out_history(pl_best_hmm), kwss->frame + 1);
        }
    }

    /* Activate new keyphrase nodes, enter their hmms */
    for (gn = kwss->keyphrases; gn; gn = gnode_next(gn)) {
        kws_keyphrase_t *keyphrase = gnode_ptr(gn);
        
        if (keyphrase->n_hmms < 1)
    	    continue;
        
        for (i = keyphrase->n_hmms - 1; i > 0; i--) {
            hmm_t *pred_hmm = kws_nth_hmm(keyphrase, i - 1);
            hmm_t *hmm = kws_nth_hmm(keyphrase, i);

            if (hmm_is_active(pred_hmm)) {    
                if (!hmm_is_active(hmm)
                    || hmm_out_score(pred_hmm) BETTER_THAN
                    hmm_in_score(hmm))
                        hmm_enter(hmm, hmm_out_score(pred_hmm),
                                  hmm_out_history(pred_hmm), kwss->frame + 1);
            }
        }

        /* Enter keyphrase start node from phone loop */
        if (hmm_out_score(pl_best_hmm) BETTER_THAN
            hmm_in_score(kws_nth_hmm(keyphrase, 0)))
                hmm_enter(kws_nth_hmm(keyphrase, 0), hmm_out_score(pl_best_hmm),
                    kwss->frame, kwss->frame + 1);
    }
}

static int
kws_search_read_list(kws_search_t *kwss, const char* keyfile)
{
    FILE *list_file;
    lineiter_t *li;
    char *line;
    
    if ((list_file = fopen(keyfile, "r")) == NULL) {
        E_ERROR_SYSTEM("Failed to open keyphrase file '%s'", keyfile);
        return -1;
    }

    kwss->keyphrases = NULL;

    /* read keyphrases */
    for (li = lineiter_start_clean(list_file); li; li = lineiter_next(li)) {
        size_t begin, end;
        kws_keyphrase_t *keyphrase;
	
	if (li->len == 0)
	    continue;

	keyphrase = ckd_calloc(1, sizeof(kws_keyphrase_t));

        line = li->buf;
        end = strlen(line) - 1;
	begin = end - 1;
        if (line[end] == '/') {
            while (line[begin] != '/' && begin > 0)
                begin--;
            line[end] = 0;
            line[begin] = 0;
            keyphrase->threshold = (int32) logmath_log(kwss->base.acmod->lmath, atof_c(line + begin + 1)) 
                                          >> SENSCR_SHIFT;
        } else {
            keyphrase->threshold = kwss->def_threshold;
        }

        keyphrase->word = ckd_salloc(line);

        kwss->keyphrases = glist_add_ptr(kwss->keyphrases, keyphrase);
    }

    fclose(list_file);
    return 0;
}

ps_search_t *
kws_search_init(const char *name,
                const char *keyphrase,
                const char *keyfile,
                ps_config_t * config,
                acmod_t * acmod, dict_t * dict, dict2pid_t * d2p)
{
    kws_search_t *kwss = (kws_search_t *) ckd_calloc(1, sizeof(*kwss));
    ps_search_init(ps_search_base(kwss), &kws_funcs, PS_SEARCH_TYPE_KWS, name, config, acmod, dict,
                   d2p);

    kwss->detections = (kws_detections_t *)ckd_calloc(1, sizeof(*kwss->detections));

    kwss->beam =
        (int32) logmath_log(acmod->lmath,
                            ps_config_float(config, "beam")) >> SENSCR_SHIFT;

    kwss->plp =
        (int32) logmath_log(acmod->lmath,
                            ps_config_float(config, "kws_plp")) >> SENSCR_SHIFT;


    kwss->def_threshold =
        (int32) logmath_log(acmod->lmath,
                            ps_config_float(config,
                                            "kws_threshold")) >> SENSCR_SHIFT;

    kwss->delay = (int32) ps_config_int(config, "kws_delay");

    E_INFO("KWS(beam: %d, plp: %d, default threshold %d, delay %d)\n",
           kwss->beam, kwss->plp, kwss->def_threshold, kwss->delay);

    if (keyfile) {
	if (kws_search_read_list(kwss, keyfile) < 0) {
	    E_ERROR("Failed to create kws search\n");
	    kws_search_free(ps_search_base(kwss));
	    return NULL;
	}
    } else {
        kws_keyphrase_t *k = ckd_calloc(1, sizeof(kws_keyphrase_t));
        k->threshold = kwss->def_threshold;
        k->word = ckd_salloc(keyphrase);
        kwss->keyphrases = glist_add_ptr(NULL, k);
    }

    /* Reinit for provided keyphrase */
    if (kws_search_reinit(ps_search_base(kwss),
                          ps_search_dict(kwss),
                          ps_search_dict2pid(kwss)) < 0) {
        ps_search_free(ps_search_base(kwss));
        return NULL;
    }
    
    ptmr_init(&kwss->perf);

    return ps_search_base(kwss);
}

void
kws_search_free(ps_search_t * search)
{
    kws_search_t *kwss;
    double n_speech;
    gnode_t *gn;

    kwss = (kws_search_t *) search;

    n_speech = (double)kwss->n_tot_frame
        / ps_config_int(ps_search_config(kwss), "frate");

    E_INFO("TOTAL kws %.2f CPU %.3f xRT\n",
           kwss->perf.t_tot_cpu,
           kwss->perf.t_tot_cpu / n_speech);
    E_INFO("TOTAL kws %.2f wall %.3f xRT\n",
           kwss->perf.t_tot_elapsed,
           kwss->perf.t_tot_elapsed / n_speech);


    ps_search_base_free(search);
    hmm_context_free(kwss->hmmctx);
    kws_detections_reset(kwss->detections);
    ckd_free(kwss->detections);

    ckd_free(kwss->pl_hmms);
    for (gn = kwss->keyphrases; gn; gn = gnode_next(gn)) {
	kws_keyphrase_t *keyphrase = gnode_ptr(gn);
        ckd_free(keyphrase->hmms);
        ckd_free(keyphrase->word);
        ckd_free(keyphrase);
    }
    glist_free(kwss->keyphrases);
    ckd_free(kwss);
}

int
kws_search_reinit(ps_search_t * search, dict_t * dict, dict2pid_t * d2p)
{
    char **wrdptr;
    char *tmp_keyphrase;
    int32 wid, pronlen, in_dict;
    int32 n_hmms, n_wrds;
    int32 ssid, tmatid;
    int i, j, p;
    kws_search_t *kwss = (kws_search_t *) search;
    bin_mdef_t *mdef = search->acmod->mdef;
    int32 silcipid = bin_mdef_silphone(mdef);
    gnode_t *gn;

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

    for (gn = kwss->keyphrases; gn; gn = gnode_next(gn)) {
        kws_keyphrase_t *keyphrase = gnode_ptr(gn);

        /* Initialize keyphrase HMMs */
        tmp_keyphrase = (char *) ckd_salloc(keyphrase->word);
        n_wrds = str2words(tmp_keyphrase, NULL, 0);
        wrdptr = (char **) ckd_calloc(n_wrds, sizeof(*wrdptr));
        str2words(tmp_keyphrase, wrdptr, n_wrds);

        /* count amount of hmms */
        n_hmms = 0;
        in_dict = TRUE;
        for (i = 0; i < n_wrds; i++) {
            wid = dict_wordid(dict, wrdptr[i]);
            if (wid == BAD_S3WID) {
        	E_ERROR("Word '%s' in phrase '%s' is missing in the dictionary\n", wrdptr[i], keyphrase->word);
        	in_dict = FALSE;
        	break;
            }
            pronlen = dict_pronlen(dict, wid);
            n_hmms += pronlen;
        }
        
        if (!in_dict) {
            ckd_free(wrdptr);
            ckd_free(tmp_keyphrase);
    	    continue;
        }

        /* allocate node array */
        if (keyphrase->hmms)
            ckd_free(keyphrase->hmms);
        keyphrase->hmms = (hmm_t *) ckd_calloc(n_hmms, sizeof(hmm_t));
        keyphrase->n_hmms = n_hmms;

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
                hmm_init(kwss->hmmctx, &keyphrase->hmms[j], FALSE, ssid,
                         tmatid);
                j++;
            }
        }

        ckd_free(wrdptr);
        ckd_free(tmp_keyphrase);
    }

    

    return 0;
}

int
kws_search_start(ps_search_t * search)
{
    int i;
    kws_search_t *kwss = (kws_search_t *) search;

    kwss->frame = 0;
    kwss->bestscore = 0;
    kws_detections_reset(kwss->detections);

    /* Reset and enter all phone-loop HMMs. */
    for (i = 0; i < kwss->n_pl; ++i) {
        hmm_t *hmm = (hmm_t *) & kwss->pl_hmms[i];
        hmm_clear(hmm);
        hmm_enter(hmm, 0, -1, 0);
    }

    ptmr_reset(&kwss->perf);
    ptmr_start(&kwss->perf);

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

    /* Evaluate hmms in phone loop and in active keyphrase nodes */
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
    kws_search_t *kwss;
    int32 cf;

    kwss = (kws_search_t *) search;

    kwss->n_tot_frame += kwss->frame;

    /* Print out some statistics. */
    ptmr_stop(&kwss->perf);
    /* This is the number of frames processed. */
    cf = ps_search_acmod(kwss)->output_frame;
    if (cf > 0) {
        double n_speech = (double) (cf + 1)
            / ps_config_int(ps_search_config(kwss), "frate");
        E_INFO("kws %.2f CPU %.3f xRT\n",
               kwss->perf.t_cpu, kwss->perf.t_cpu / n_speech);
        E_INFO("kws %.2f wall %.3f xRT\n",
               kwss->perf.t_elapsed, kwss->perf.t_elapsed / n_speech);
    }

    return 0;
}

char const *
kws_search_hyp(ps_search_t * search, int32 * out_score)
{
    kws_search_t *kwss = (kws_search_t *) search;
    if (out_score)
        *out_score = 0;

    if (search->hyp_str)
        ckd_free(search->hyp_str);
    search->hyp_str = kws_detections_hyp_str(kwss->detections, kwss->frame, kwss->delay);

    return search->hyp_str;
}

char * 
kws_search_get_keyphrases(ps_search_t * search)
{
    int c, len;
    kws_search_t *kwss;
    char* line;
    gnode_t *gn;

    kwss = (kws_search_t *) search;

    len = 0;
    for (gn = kwss->keyphrases; gn; gn = gnode_next(gn))
        len += strlen(((kws_keyphrase_t *)gnode_ptr(gn))->word) + 1;

    c = 0;
    line = (char *)ckd_calloc(len, sizeof(*line));
    for (gn = kwss->keyphrases; gn; gn = gnode_next(gn)) {
        const char *str = ((kws_keyphrase_t *)gnode_ptr(gn))->word;
        memcpy(&line[c], str, strlen(str));
        c += strlen(str);
        line[c++] = '\n';
    }
    line[--c] = '\0';

    return line;
}
