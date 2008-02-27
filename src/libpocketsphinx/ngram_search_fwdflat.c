/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 2008 Carnegie Mellon University.  All rights
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

/**
 * @file ngram_search_fwdflat.c Flat lexicon search.
 */

/* System headers. */
#include <string.h>

/* SphinxBase headers. */
#include <ckd_alloc.h>
#include <listelem_alloc.h>

/* Local headers. */
#include "ngram_search.h"

void
ngram_fwdflat_init(ngram_search_t *ngs)
{
    int n_words, i;

    n_words = ngs->dict->dict_entry_count;
    ngs->fwdflat_wordlist = ckd_calloc(n_words + 1, sizeof(*ngs->fwdflat_wordlist));
    ngs->expand_word_flag = bitvec_alloc(n_words);
    ngs->expand_word_list = ckd_calloc(n_words + 1, sizeof(*ngs->expand_word_list));

    ngs->min_ef_width = cmd_ln_int32_r(ngs->config, "-fwdflatefwid");
    ngs->max_sf_win = cmd_ln_int32_r(ngs->config, "-fwdflatsfwin");
    E_INFO("fwdflat: min_ef_width = %d, max_sf_win = %d\n",
           ngs->min_ef_width, ngs->max_sf_win);

    /* No tree-search; pre-build the expansion list, including all LM words. */
    if (!cmd_ln_boolean_r(ngs->config, "-fwdtree")) {
        ngs->n_expand_words = 0;
        bitvec_clear_all(ngs->expand_word_flag, ngs->dict->dict_entry_count);
        for (i = 0; i < ngs->start_wid; i++) {
            if (ngram_model_set_known_wid(ngs->lmset,
                                          ngs->dict->dict_list[i]->wid)) {
                ngs->fwdflat_wordlist[ngs->n_expand_words] = i;
                ngs->expand_word_list[ngs->n_expand_words] = i;
                bitvec_set(ngs->expand_word_flag, i);
                ngs->n_expand_words++;
            }
        }
        ngs->expand_word_list[ngs->n_expand_words] = -1;
        ngs->fwdflat_wordlist[ngs->n_expand_words] = -1;
    }
}

void
ngram_fwdflat_deinit(ngram_search_t *ngs)
{
    ckd_free(ngs->fwdflat_wordlist);
    bitvec_free(ngs->expand_word_flag);
    ckd_free(ngs->expand_word_list);
}

#if 0
/**
 * Find all active words in backpointer table and sort by frame.
 */
static void
build_fwdflat_wordlist(ngram_search_t *ngs)
{
    int32 i, f, sf, ef, wid, nwd;
    bptbl_t *bp;
    latnode_t *node, *prevnode, *nextnode;
    dict_entry_t *de;

    /* No tree-search, use statically allocated wordlist. */
    if (!cmd_ln_boolean_r(ngs->config, "-fwdtree"))
        return;

    memset(ngs->frm_wordlist, 0, MAX_FRAMES * sizeof(latnode_t *));

    /* Scan the backpointer table for all active words and record
     * their exit frames. */
    for (i = 0, bp = ngs->bp_table; i < ngs->bpidx; i++, bp++) {
        sf = (bp->bp < 0) ? 0 : ngs->bp_table[bp->bp].frame + 1;
        ef = bp->frame;
        wid = bp->wid;

        /*
         * NOTE: fwdflat_wordlist excludes <s>, <sil> and noise words;
         * it includes </s>.  That is, it includes anything to which a
         * transition can be made in the LM.
         */
        /* Ignore silence and <s> */
        if ((wid >= ngs->silence_wid) || (wid == ngs->start_wid))
            continue;

        /* Look for it in the wordlist. */
        de = ngs->word_dict->dict_list[wid];
        for (node = ngs->frm_wordlist[sf]; node && (node->wid != wid);
             node = node->next);

        /* Update last end frame. */
        if (node)
            node->lef = ef;
        else {
            /* New node; link to head of list */
            node = listelem_malloc(ngs->latnode_alloc);
            node->wid = wid;
            node->fef = node->lef = ef;

            node->next = ngs->frm_wordlist[sf];
            ngs->frm_wordlist[sf] = node;
        }
    }

    /* Eliminate "unlikely" words, for which there are too few end points */
    for (f = 0; f <= LastFrame; f++) {
        prevnode = NULL;
        for (node = ngs->frm_wordlist[f]; node; node = nextnode) {
            nextnode = node->next;
            if ((node->lef - node->fef < ngs->min_ef_width) ||
                ((node->wid == ngs->finish_wid)
                 && (node->lef < LastFrame - 1))) {
                if (!prevnode)
                    ngs->frm_wordlist[f] = nextnode;
                else
                    prevnode->next = nextnode;
                listelem_free(ngs->latnode_alloc, node);
            }
            else
                prevnode = node;
        }
    }

    /* Form overall wordlist for 2nd pass */
    nwd = 0;
    memset(word_active, 0, NumWords * sizeof(int32));
    for (f = 0; f <= LastFrame; f++) {
        for (node = ngs->frm_wordlist[f]; node; node = node->next) {
            if (!ngs->word_active[node->wid]) {
                ngs->word_active[node->wid] = 1;
                ngs->fwdflat_wordlist[nwd++] = node->wid;
            }
        }
    }
    ngs->fwdflat_wordlist[nwd] = -1;
}

/**
 * Destroy wordlist from the current utterance.
 */
static void
destroy_fwdflat_wordlist(ngram_search_t *ngs)
{
    latnode_t *node, *tnode;
    int32 f;

    if (!cmd_ln_boolean_r(ngs->config, "-fwdtree"))
        return;

    for (f = 0; f <= LastFrame; f++) {
        for (node = ngs->frm_wordlist[f]; node; node = tnode) {
            tnode = node->next;
            listelem_free(ngs->latnode_alloc, node);
        }
    }
}

/**
 * Build HMM network for one utterance of fwdflat search.
 */
static void
build_fwdflat_chan(ngram_search_t *ngs)
{
    int32 i, wid, p;
    dict_entry_t *de;
    root_chan_t *rhmm;
    chan_t *hmm, *prevhmm;

    /* Build word HMMs for each word in the lattice. */
    for (i = 0; ngs->fwdflat_wordlist[i] >= 0; i++) {
        wid = ngs->fwdflat_wordlist[i];
        de = ngs->dict->dict_list[wid];

        /* Omit single-phone words as they are permanently allocated */
        if (de->len == 1)
            continue;

        assert(de->mpx);
        assert(ngs->word_chan[wid] == NULL);

        /* Multiplex root HMM for first phone (one root per word, flat
         * lexicon) */
        rhmm = listelem_malloc(ngs->root_chan_alloc);
        rhmm->diphone = de->phone_ids[0];
        rhmm->ciphone = de->ci_phone_ids[0];
        rhmm->next = NULL;
        hmm_init(ngs->hmmctx, &rhmm->hmm, TRUE, rhmm->diphone, rhmm->ciphone);

        /* HMMs for word-internal phones */
        prevhmm = NULL;
        for (p = 1; p < de->len - 1; p++) {
            hmm = listelem_malloc(ngs->chan_alloc);
            hmm->ciphone = de->ci_phone_ids[p];
            hmm->info.rc_id = p + 1 - de->len;
            hmm->next = NULL;
            hmm_init(ngs->hmmctx, &hmm->hmm, FALSE, de->phone_ids[p], hmm->ciphone);

            if (prevhmm)
                prevhmm->next = hmm;
            else
                rhmm->next = hmm;

            prevhmm = hmm;
        }

        /* Right-context phones */
        alloc_all_rc(ngs, wid);

        /* Link in just allocated right-context phones */
        if (prevhmm)
            prevhmm->next = ngs->word_chan[wid];
        else
            rhmm->next = ngs->word_chan[wid];
        ngs->word_chan[wid] = (chan_t *) rhmm;
    }
}

/**
 * Free HMM network for one utterance of fwdflat search.
 */
static void
destroy_fwdflat_chan(ngram_search_t *ngs)
{
    int32 i, wid;
    dict_entry_t *de;

    for (i = 0; ngs->fwdflat_wordlist[i] >= 0; i++) {
        wid = ngs->fwdflat_wordlist[i];
        de = ngs->dict->dict_list[wid];

        if (de->len == 1)
            continue;

        assert(de->mpx);
        assert(ngs->word_chan[wid] != NULL);

        free_all_rc(ngs, wid);
    }
}

void
ngram_fwdflat_start(ngram_search_t *ngs)
{
    root_chan_t *rhmm;
    int i;

    build_fwdflat_wordlist(ngs);
    build_fwdflat_chan(ngs);

    ngs->bpidx = 0;
    ngs->bss_head = 0;

    for (i = 0; i < ngs->dict->dict_entry_count; i++)
        ngs->word_lat_idx[i] = NO_BP;

    /* Start search with <s>; word_chan[<s>] is permanently allocated */
    rhmm = (root_chan_t *) ngs->word_chan[ngs->start_wid];
    hmm_enter(&rhmm->hmm, 0, NO_BP, 0);
    ngs->active_word_list[0][0] = ngs->start_wid;
    ngs->n_active_word[0] = 1;

    ngs->best_score = 0;
    ngs->renormalized = FALSE;

    for (i = 0; i < ngs->dict->dict_entry_count; i++)
        ngs->last_ltrans[i].sf = -1;

    ngs->st.n_fwdflat_chan = 0;
    ngs->st.n_fwdflat_words = 0;
    ngs->st.n_fwdflat_word_transition = 0;
    ngs->st.n_senone_active_utt = 0;
}

static void
compute_fwdflat_senone_active(ngram_search_t *ngs)
{
    int32 i, cf, w;
    int32 *awl;
    root_chan_t *rhmm;
    chan_t *hmm;

    sen_active_clear();

    cf = CurrentFrame;

    i = n_active_word[cf & 0x1];
    awl = active_word_list[cf & 0x1];

    for (w = *(awl++); i > 0; --i, w = *(awl++)) {
        rhmm = (root_chan_t *) word_chan[w];
        if (hmm_frame(&rhmm->hmm) == cf) {
            hmm_sen_active(&rhmm->hmm);
        }

        for (hmm = rhmm->next; hmm; hmm = hmm->next) {
            if (hmm_frame(&hmm->hmm) == cf) {
                hmm_sen_active(&hmm->hmm);
            }
        }
    }

    sen_active_flags2list();
}

static void
fwdflat_eval_chan(ngram_search_t *ngs)
{
    int32 i, cf, w, bestscore;
    int32 *awl;
    root_chan_t *rhmm;
    chan_t *hmm;

    cf = CurrentFrame;
    i = n_active_word[cf & 0x1];
    awl = active_word_list[cf & 0x1];
    bestscore = WORST_SCORE;

    n_fwdflat_words += i;

    hmm_context_set_senscore(hmmctx, senone_scores);

    /* Scan all active words. */
    for (w = *(awl++); i > 0; --i, w = *(awl++)) {
        rhmm = (root_chan_t *) word_chan[w];
        if (hmm_frame(&rhmm->hmm) == cf) {
            chan_v_eval(rhmm);
            n_fwdflat_chan++;
        }
        if ((bestscore < hmm_bestscore(&rhmm->hmm)) && (w != FinishWordId))
            bestscore = hmm_bestscore(&rhmm->hmm);

        for (hmm = rhmm->next; hmm; hmm = hmm->next) {
            if (hmm_frame(&hmm->hmm) == cf) {
                int32 score;

                score = chan_v_eval(hmm);
                if (bestscore < score)
                    bestscore = score;
                n_fwdflat_chan++;
            }
        }
    }

    BestScore = bestscore;
}

static void
fwdflat_prune_chan(ngram_search_t *ngs)
{
    int32 i, cf, nf, w, pip, newscore, thresh, wordthresh;
    int32 *awl;
    root_chan_t *rhmm;
    chan_t *hmm, *nexthmm;
    dict_entry_t *de;

    cf = CurrentFrame;
    nf = cf + 1;
    i = n_active_word[cf & 0x1];
    awl = active_word_list[cf & 0x1];
    memset(word_active, 0, NumWords * sizeof(int32));

    thresh = BestScore + FwdflatLogBeamWidth;
    wordthresh = BestScore + FwdflatLogWordBeamWidth;
    pip = logPhoneInsertionPenalty;

    /* Scan all active words. */
    for (w = *(awl++); i > 0; --i, w = *(awl++)) {
        de = g_word_dict->dict_list[w];

        rhmm = (root_chan_t *) word_chan[w];
        /* Propagate active root channels */
        if (hmm_frame(&rhmm->hmm) == cf
            && hmm_bestscore(&rhmm->hmm) > thresh) {
            hmm_frame(&rhmm->hmm) = nf;
            word_active[w] = 1;

            /* Transitions out of root channel */
            newscore = hmm_out_score(&rhmm->hmm);
            if (rhmm->next) {
                assert(de->len > 1);

                newscore += pip;
                if (newscore > thresh) {
                    hmm = rhmm->next;
                    /* Enter all right context phones */
                    if (hmm->info.rc_id >= 0) {
                        for (; hmm; hmm = hmm->next) {
                            if ((hmm_frame(&hmm->hmm) < cf)
                                || (hmm_in_score(&hmm->hmm) < newscore)) {
                                hmm_enter(&hmm->hmm, newscore,
                                          hmm_out_history(&rhmm->hmm), nf);
                            }
                        }
                    }
                    /* Just a normal word internal phone */
                    else {
                        if ((hmm_frame(&hmm->hmm) < cf)
                            || (hmm_in_score(&hmm->hmm) < newscore)) {
                                hmm_enter(&hmm->hmm, newscore,
                                          hmm_out_history(&rhmm->hmm), nf);
                        }
                    }
                }
            }
            else {
                assert(de->len == 1);

                /* Word exit for single-phone words (where did their
                 * whmms come from?) */
                if (newscore > wordthresh) {
                    save_bwd_ptr(w, newscore,
                                 hmm_out_history(&rhmm->hmm), 0);
                }
            }
        }

        /* Transitions out of non-root channels. */
        for (hmm = rhmm->next; hmm; hmm = hmm->next) {
            if (hmm_frame(&hmm->hmm) >= cf) {
                /* Propagate forward HMMs inside the beam. */
                if (hmm_bestscore(&hmm->hmm) > thresh) {
                    hmm_frame(&hmm->hmm) = nf;
                    word_active[w] = 1;

                    newscore = hmm_out_score(&hmm->hmm);
                    /* Word-internal phones */
                    if (hmm->info.rc_id < 0) {
                        newscore += pip;
                        if (newscore > thresh) {
                            nexthmm = hmm->next;
                            /* Enter all right-context phones. */
                            if (nexthmm->info.rc_id >= 0) {
                                for (; nexthmm; nexthmm = nexthmm->next) {
                                    if ((hmm_frame(&nexthmm->hmm) < cf)
                                        || (hmm_in_score(&nexthmm->hmm)
                                            < newscore)) {
                                        hmm_enter(&nexthmm->hmm,
                                                  newscore,
                                                  hmm_out_history(&hmm->hmm),
                                                  nf);
                                    }
                                }
                            }
                            /* Enter single word-internal phone. */
                            else {
                                if ((hmm_frame(&nexthmm->hmm) < cf)
                                    || (hmm_in_score(&nexthmm->hmm)
                                        < newscore)) {
                                    hmm_enter(&nexthmm->hmm, newscore,
                                              hmm_out_history(&hmm->hmm), nf);
                                }
                            }
                        }
                    }
                    /* Right-context phones - apply word beam and exit. */
                    else {
                        if (newscore > wordthresh) {
                            save_bwd_ptr(w, newscore,
                                         hmm_out_history(&hmm->hmm),
                                         hmm->info.rc_id);
                        }
                    }
                }
                /* Zero out inactive HMMs. */
                else if (hmm_frame(&hmm->hmm) != nf) {
                    hmm_clear_scores(&hmm->hmm);
                }
            }
        }
    }
}

static void
fwdflat_word_transition(ngram_search_t *ngs)
{
    int32 cf, nf, b, thresh, pip, i, w, newscore;
    int32 best_silrc_score = 0, best_silrc_bp = 0;      /* FIXME: good defaults? */
    BPTBL_T *bp;
    dict_entry_t *de, *newde;
    int32 *rcpermtab, *rcss;
    root_chan_t *rhmm;
    int32 *awl;
    float32 lwf;

    cf = CurrentFrame;
    nf = cf + 1;
    thresh = BestScore + FwdflatLogBeamWidth;
    pip = logPhoneInsertionPenalty;
    best_silrc_score = WORST_SCORE;
    lwf = fwdflat_fwdtree_lw_ratio;

    /* Search for all words starting within a window of this frame.
     * These are the successors for words exiting now. */
    get_expand_wordlist(cf, max_sf_win);

    /* Scan words exited in current frame */
    for (b = BPTableIdx[cf]; b < BPIdx; b++) {
        bp = BPTable + b;
        WordLatIdx[bp->wid] = NO_BP;

        if (bp->wid == FinishWordId)
            continue;

        de = g_word_dict->dict_list[bp->wid];
        rcpermtab =
            (bp->r_diph >=
             0) ? g_word_dict->rcFwdPermTable[bp->r_diph] : zeroPermTab;
        rcss = BScoreStack + bp->s_idx;

        /* Transition to all successor words. */
        for (i = 0; expand_word_list[i] >= 0; i++) {
            int32 n_used;
            w = expand_word_list[i];
            newde = g_word_dict->dict_list[w];
            /* Get the exit score we recorded in save_bwd_ptr(), or
             * something approximating it. */
            newscore = rcss[rcpermtab[newde->ci_phone_ids[0]]];
            newscore += lwf
                * ngram_tg_score(g_lmset, newde->wid, bp->real_wid,
                                 bp->prev_real_wid, &n_used);
            newscore += pip;

            /* Enter the next word */
            if (newscore > thresh) {
                rhmm = (root_chan_t *) word_chan[w];
                if ((hmm_frame(&rhmm->hmm) < cf)
                    || (hmm_in_score(&rhmm->hmm) < newscore)) {
                    hmm_enter(&rhmm->hmm, newscore, b, nf);
                    if (hmm_is_mpx(&rhmm->hmm)) {
                        rhmm->hmm.s.mpx_ssid[0] =
                            g_word_dict->lcFwdTable[rhmm->diphone]
                            [de->ci_phone_ids[de->len-1]];
                    }

                    word_active[w] = 1;
                }
            }
        }

        /* Get the best exit into silence. */
        if (best_silrc_score < rcss[rcpermtab[SilencePhoneId]]) {
            best_silrc_score = rcss[rcpermtab[SilencePhoneId]];
            best_silrc_bp = b;
        }
    }

    /* Transition to <sil> */
    newscore = best_silrc_score + SilenceWordPenalty + pip;
    if ((newscore > thresh) && (newscore > WORST_SCORE)) {
        w = SilenceWordId;
        rhmm = (root_chan_t *) word_chan[w];
        if ((hmm_frame(&rhmm->hmm) < cf)
            || (hmm_in_score(&rhmm->hmm) < newscore)) {
            hmm_enter(&rhmm->hmm, newscore,
                      best_silrc_bp, nf);
            word_active[w] = 1;
        }
    }
    /* Transition to noise words */
    newscore = best_silrc_score + FillerWordPenalty + pip;
    if ((newscore > thresh) && (newscore > WORST_SCORE)) {
        for (w = SilenceWordId + 1; w < NumWords; w++) {
            rhmm = (root_chan_t *) word_chan[w];
            if ((hmm_frame(&rhmm->hmm) < cf)
                || (hmm_in_score(&rhmm->hmm) < newscore)) {
                hmm_enter(&rhmm->hmm, newscore,
                          best_silrc_bp, nf);
                word_active[w] = 1;
            }
        }
    }

    /* Reset initial channels of words that have become inactive even after word trans. */
    i = n_active_word[cf & 0x1];
    awl = active_word_list[cf & 0x1];
    for (w = *(awl++); i > 0; --i, w = *(awl++)) {
        rhmm = (root_chan_t *) word_chan[w];

        if (hmm_frame(&rhmm->hmm) == cf) {
            hmm_clear_scores(&rhmm->hmm);
        }
    }
}

static void
fwdflat_renormalize_scores(ngram_search_t *ngs, int32 norm)
{
    root_chan_t *rhmm;
    chan_t *hmm;
    int32 i, cf, w, *awl;

    cf = CurrentFrame;

    /* Renormalize individual word channels */
    i = ngs->n_active_word[cf & 0x1];
    awl = ngs->active_word_list[cf & 0x1];
    for (w = *(awl++); i > 0; --i, w = *(awl++)) {
        rhmm = (root_chan_t *) word_chan[w];
        if (hmm_frame(&rhmm->hmm) == cf) {
            hmm_normalize(&rhmm->hmm, norm);
        }
        for (hmm = rhmm->next; hmm; hmm = hmm->next) {
            if (hmm_frame(&hmm->hmm) == cf) {
                hmm_normalize(&hmm->hmm, norm);
            }
        }
    }

    ngs->renormalized = TRUE;
}

void
ngram_fwdflat_search(ngram_search_t *ngs)
{
    int32 nf, i, j;
    int32 *nawl;

    if (!compute_all_senones) {
        compute_fwdflat_senone_active();
        senscr_active(feat, CurrentFrame);
    }
    else {
        senscr_all(feat, CurrentFrame);
    }
    n_senone_active_utt += n_senone_active;

    BPTableIdx[CurrentFrame] = BPIdx;

    /* Need to renormalize? */
    if ((BestScore + (2 * LogBeamWidth)) < WORST_SCORE) {
        E_INFO("Renormalizing Scores at frame %d, best score %d\n",
               CurrentFrame, BestScore);
        fwdflat_renormalize_scores(BestScore);
    }

    ngs->best_score = WORST_SCORE;
    fwdflat_eval_chan(ngs);
    fwdflat_prune_chan(ngs);
    fwdflat_word_transition(ngs);

    /* Create next active word list */
    nf = CurrentFrame + 1;
    nawl = ngs->active_word_list[nf & 0x1];
    for (i = 0, j = 0; ngs->fwdflat_wordlist[i] >= 0; i++) {
        if (bitvec_is_set(ngs->word_active, ngs->fwdflat_wordlist[i])) {
            *(nawl++) = ngs->fwdflat_wordlist[i];
            j++;
        }
    }
    for (i = ngs->start_wid; i < ngs->dict->dict_entry_count; i++) {
        if (bitvec_is_set(ngs->word_active, i)) {
            *(nawl++) = i;
            j++;
        }
    }
    ngs->n_active_word[nf & 0x1] = j;

    CurrentFrame = nf;
}

void
ngram_fwdflat_finish(ngram_search_t *ngs)
{
}

#endif
