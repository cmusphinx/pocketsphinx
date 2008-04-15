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
#include <assert.h>

/* SphinxBase headers. */
#include <ckd_alloc.h>
#include <listelem_alloc.h>

/* Local headers. */
#include "ngram_search.h"

/* Turn this on to dump channels for debugging */
#define __CHAN_DUMP__		0
#if __CHAN_DUMP__
#define chan_v_eval(chan) hmm_dump_vit_eval(&(chan)->hmm, stderr)
#else
#define chan_v_eval(chan) hmm_vit_eval(&(chan)->hmm)
#endif

static void
ngram_fwdflat_expand_all(ngram_search_t *ngs)
{
    int n_words, i;

    ngs->n_expand_words = 0;
    n_words = ps_search_n_words(ngs);
    bitvec_clear_all(ngs->expand_word_flag, ps_search_n_words(ngs));
    for (i = 0; i < ngs->start_wid; i++) {
        if (ngram_model_set_known_wid(ngs->lmset,
                                      ps_search_dict(ngs)->dict_list[i]->wid)) {
            ngs->fwdflat_wordlist[ngs->n_expand_words] = i;
            ngs->expand_word_list[ngs->n_expand_words] = i;
            bitvec_set(ngs->expand_word_flag, i);
            ngs->n_expand_words++;
        }
    }
    ngs->expand_word_list[ngs->n_expand_words] = -1;
    ngs->fwdflat_wordlist[ngs->n_expand_words] = -1;
}

void
ngram_fwdflat_init(ngram_search_t *ngs)
{
    int n_words, i;

    n_words = ps_search_n_words(ngs);
    ngs->fwdflat_wordlist = ckd_calloc(n_words + 1, sizeof(*ngs->fwdflat_wordlist));
    ngs->expand_word_flag = bitvec_alloc(n_words);
    ngs->expand_word_list = ckd_calloc(n_words + 1, sizeof(*ngs->expand_word_list));
    ngs->frm_wordlist = ckd_calloc(ngs->n_frame_alloc, sizeof(*ngs->frm_wordlist));
    ngs->min_ef_width = cmd_ln_int32_r(ps_search_config(ngs), "-fwdflatefwid");
    ngs->max_sf_win = cmd_ln_int32_r(ps_search_config(ngs), "-fwdflatsfwin");
    E_INFO("fwdflat: min_ef_width = %d, max_sf_win = %d\n",
           ngs->min_ef_width, ngs->max_sf_win);

    /* No tree-search; pre-build the expansion list, including all LM words. */
    if (!ngs->fwdtree) {
        int w;

        /* Build full expansion list from LM words. */
        ngram_fwdflat_expand_all(ngs);

        /* Allocate single-phone words, since they won't have
         * been allocated for us by fwdtree initialization. */
        ngs->n_1ph_words = 0;
        for (w = 0; w < n_words; w++) {
            dict_entry_t *de = ps_search_dict(ngs)->dict_list[w];
            if (de->len == 1)
                ++ngs->n_1ph_words;
        }
        ngs->all_rhmm = ckd_calloc(ngs->n_1ph_words, sizeof(*ngs->all_rhmm));
        i = 0;
        for (w = 0; w < n_words; w++) {
            dict_entry_t *de = ps_search_dict(ngs)->dict_list[w];
            if (de->len != 1)
                continue;

            ngs->all_rhmm[i].diphone = de->phone_ids[0];
            ngs->all_rhmm[i].ciphone = de->ci_phone_ids[0];
            hmm_init(ngs->hmmctx, &ngs->all_rhmm[i].hmm, de->mpx,
                     de->phone_ids[0], de->ci_phone_ids[0]);
            ngs->all_rhmm[i].next = NULL;
            ngs->word_chan[w] = (chan_t *) &(ngs->all_rhmm[i]);
            i++;
        }
    }
}

void
ngram_fwdflat_deinit(ngram_search_t *ngs)
{
    /* Free single-phone words if we allocated them. */
    if (!ngs->fwdtree) {
        ckd_free(ngs->all_rhmm);
    }
    ckd_free(ngs->fwdflat_wordlist);
    bitvec_free(ngs->expand_word_flag);
    ckd_free(ngs->expand_word_list);
    ckd_free(ngs->frm_wordlist);
}

int
ngram_fwdflat_reinit(ngram_search_t *ngs)
{
    /* No tree-search; re-build the expansion list from all LM words. */
    if (!ngs->fwdtree) {
        /* Rebuild full expansion list from LM words. */
        ngram_fwdflat_expand_all(ngs);
    }
    /* Otherwise there is nothing to do since the wordlist is
     * generated anew every utterance. */
    return 0;
}

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
    if (!ngs->fwdtree)
        return;

    memset(ngs->frm_wordlist, 0, ngs->n_frame_alloc * sizeof(*ngs->frm_wordlist));

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
        if (ISA_FILLER_WORD(ngs, wid) || (wid == ngs->start_wid))
            continue;

        /* Look for it in the wordlist. */
        de = ps_search_dict(ngs)->dict_list[wid];
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
    for (f = 0; f < ngs->n_frame; f++) {
        prevnode = NULL;
        for (node = ngs->frm_wordlist[f]; node; node = nextnode) {
            nextnode = node->next;
            /* Word has too few endpoints */
            if ((node->lef - node->fef < ngs->min_ef_width) ||
                /* Word is </s> and doesn't actually end in last frame */
                ((node->wid == ngs->finish_wid) && (node->lef < ngs->n_frame - 1))) {
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
    bitvec_clear_all(ngs->word_active, ps_search_n_words(ngs));
    for (f = 0; f < ngs->n_frame; f++) {
        for (node = ngs->frm_wordlist[f]; node; node = node->next) {
            if (!bitvec_is_set(ngs->word_active, node->wid)) {
                bitvec_set(ngs->word_active, node->wid);
                ngs->fwdflat_wordlist[nwd++] = node->wid;
            }
        }
    }
    ngs->fwdflat_wordlist[nwd] = -1;
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
        de = ps_search_dict(ngs)->dict_list[wid];

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
        ngram_search_alloc_all_rc(ngs, wid);

        /* Link in just allocated right-context phones */
        if (prevhmm)
            prevhmm->next = ngs->word_chan[wid];
        else
            rhmm->next = ngs->word_chan[wid];
        ngs->word_chan[wid] = (chan_t *) rhmm;
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

    for (i = 0; i < ps_search_n_words(ngs); i++)
        ngs->word_lat_idx[i] = NO_BP;

    /* Start search with <s>; word_chan[<s>] is permanently allocated */
    rhmm = (root_chan_t *) ngs->word_chan[ngs->start_wid];
    hmm_enter(&rhmm->hmm, 0, NO_BP, 0);
    ngs->active_word_list[0][0] = ngs->start_wid;
    ngs->n_active_word[0] = 1;

    ngs->best_score = 0;
    ngs->renormalized = FALSE;

    for (i = 0; i < ps_search_n_words(ngs); i++)
        ngs->last_ltrans[i].sf = -1;

    if (!ngs->fwdtree)
        ngs->n_frame = 0;

    ngs->st.n_fwdflat_chan = 0;
    ngs->st.n_fwdflat_words = 0;
    ngs->st.n_fwdflat_word_transition = 0;
    ngs->st.n_senone_active_utt = 0;
}

static void
compute_fwdflat_sen_active(ngram_search_t *ngs, int frame_idx)
{
    int32 i, w;
    int32 *awl;
    root_chan_t *rhmm;
    chan_t *hmm;

    acmod_clear_active(ps_search_acmod(ngs));

    i = ngs->n_active_word[frame_idx & 0x1];
    awl = ngs->active_word_list[frame_idx & 0x1];

    for (w = *(awl++); i > 0; --i, w = *(awl++)) {
        rhmm = (root_chan_t *)ngs->word_chan[w];
        if (hmm_frame(&rhmm->hmm) == frame_idx) {
            acmod_activate_hmm(ps_search_acmod(ngs), &rhmm->hmm);
        }

        for (hmm = rhmm->next; hmm; hmm = hmm->next) {
            if (hmm_frame(&hmm->hmm) == frame_idx) {
                acmod_activate_hmm(ps_search_acmod(ngs), &hmm->hmm);
            }
        }
    }
}

static void
fwdflat_eval_chan(ngram_search_t *ngs, int frame_idx)
{
    int32 i, w, bestscore;
    int32 *awl;
    root_chan_t *rhmm;
    chan_t *hmm;

    i = ngs->n_active_word[frame_idx & 0x1];
    awl = ngs->active_word_list[frame_idx & 0x1];
    bestscore = WORST_SCORE;

    ngs->st.n_fwdflat_words += i;

    /* Scan all active words. */
    for (w = *(awl++); i > 0; --i, w = *(awl++)) {
        rhmm = (root_chan_t *) ngs->word_chan[w];
        if (hmm_frame(&rhmm->hmm) == frame_idx) {
            int32 score = chan_v_eval(rhmm);
            if ((bestscore < score) && (w != ngs->finish_wid))
                bestscore = score;
            ngs->st.n_fwdflat_chan++;
        }

        for (hmm = rhmm->next; hmm; hmm = hmm->next) {
            if (hmm_frame(&hmm->hmm) == frame_idx) {
                int32 score = chan_v_eval(hmm);
                if (bestscore < score)
                    bestscore = score;
                ngs->st.n_fwdflat_chan++;
            }
        }
    }

    ngs->best_score = bestscore;
}

static void
fwdflat_prune_chan(ngram_search_t *ngs, int frame_idx)
{
    int32 i, cf, nf, w, pip, newscore, thresh, wordthresh;
    int32 *awl;
    root_chan_t *rhmm;
    chan_t *hmm, *nexthmm;
    dict_entry_t *de;

    cf = frame_idx;
    nf = cf + 1;
    i = ngs->n_active_word[cf & 0x1];
    awl = ngs->active_word_list[cf & 0x1];
    bitvec_clear_all(ngs->word_active, ps_search_n_words(ngs));

    thresh = ngs->best_score + ngs->fwdflatbeam;
    wordthresh = ngs->best_score + ngs->fwdflatwbeam;
    pip = ngs->pip;

    /* Scan all active words. */
    for (w = *(awl++); i > 0; --i, w = *(awl++)) {
        de = ps_search_dict(ngs)->dict_list[w];

        rhmm = (root_chan_t *) ngs->word_chan[w];
        /* Propagate active root channels */
        if (hmm_frame(&rhmm->hmm) == cf
            && hmm_bestscore(&rhmm->hmm) > thresh) {
            hmm_frame(&rhmm->hmm) = nf;
            bitvec_set(ngs->word_active, w);

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
                    ngram_search_save_bp(ngs, cf, w, newscore,
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
                    bitvec_set(ngs->word_active, w);

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
                            ngram_search_save_bp(ngs, cf, w, newscore,
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
get_expand_wordlist(ngram_search_t *ngs, int32 frm, int32 win)
{
    int32 f, sf, ef;
    latnode_t *node;

    if (!ngs->fwdtree) {
        ngs->st.n_fwdflat_word_transition += ngs->n_expand_words;
        return;
    }

    sf = frm - win;
    if (sf < 0)
        sf = 0;
    ef = frm + win;
    if (ef > ngs->n_frame)
        ef = ngs->n_frame;

    bitvec_clear_all(ngs->expand_word_flag, ps_search_n_words(ngs));
    ngs->n_expand_words = 0;

    for (f = sf; f < ef; f++) {
        for (node = ngs->frm_wordlist[f]; node; node = node->next) {
            if (!bitvec_is_set(ngs->expand_word_flag, node->wid)) {
                ngs->expand_word_list[ngs->n_expand_words++] = node->wid;
                bitvec_set(ngs->expand_word_flag, node->wid);
            }
        }
    }
    ngs->expand_word_list[ngs->n_expand_words] = -1;
    ngs->st.n_fwdflat_word_transition += ngs->n_expand_words;
}

static void
fwdflat_word_transition(ngram_search_t *ngs, int frame_idx)
{
    int32 cf, nf, b, thresh, pip, i, w, newscore;
    int32 best_silrc_score = 0, best_silrc_bp = 0;      /* FIXME: good defaults? */
    bptbl_t *bp;
    dict_entry_t *de, *newde;
    int32 *rcpermtab, *rcss;
    root_chan_t *rhmm;
    int32 *awl;
    float32 lwf;

    cf = frame_idx;
    nf = cf + 1;
    thresh = ngs->best_score + ngs->fwdflatbeam;
    pip = ngs->pip;
    best_silrc_score = WORST_SCORE;
    lwf = ngs->fwdflat_fwdtree_lw_ratio;

    /* Search for all words starting within a window of this frame.
     * These are the successors for words exiting now. */
    get_expand_wordlist(ngs, cf, ngs->max_sf_win);

    /* Scan words exited in current frame */
    for (b = ngs->bp_table_idx[cf]; b < ngs->bpidx; b++) {
        bp = ngs->bp_table + b;
        ngs->word_lat_idx[bp->wid] = NO_BP;

        if (bp->wid == ngs->finish_wid)
            continue;

        de = ps_search_dict(ngs)->dict_list[bp->wid];
        rcpermtab =
            (bp->r_diph >=
             0) ? ps_search_dict(ngs)->rcFwdPermTable[bp->r_diph] : ngs->zeroPermTab;
        rcss = ngs->bscore_stack + bp->s_idx;

        /* Transition to all successor words. */
        for (i = 0; ngs->expand_word_list[i] >= 0; i++) {
            int32 n_used;
            w = ngs->expand_word_list[i];
            newde = ps_search_dict(ngs)->dict_list[w];
            /* Get the exit score we recorded in save_bwd_ptr(), or
             * something approximating it. */
            newscore = rcss[rcpermtab[newde->ci_phone_ids[0]]];
            /* FIXME: Floating point... */
            newscore += lwf
                * ngram_tg_score(ngs->lmset, newde->wid, bp->real_wid,
                                 bp->prev_real_wid, &n_used);
            newscore += pip;

            /* Enter the next word */
            if (newscore > thresh) {
                rhmm = (root_chan_t *) ngs->word_chan[w];
                if ((hmm_frame(&rhmm->hmm) < cf)
                    || (hmm_in_score(&rhmm->hmm) < newscore)) {
                    hmm_enter(&rhmm->hmm, newscore, b, nf);
                    if (hmm_is_mpx(&rhmm->hmm)) {
                        rhmm->hmm.s.mpx_ssid[0] =
                            ps_search_dict(ngs)->lcFwdTable[rhmm->diphone]
                            [de->ci_phone_ids[de->len-1]];
                    }

                    bitvec_set(ngs->word_active, w);
                }
            }
        }

        /* Get the best exit into silence. */
        if (best_silrc_score < rcss[rcpermtab[ps_search_acmod(ngs)->mdef->sil]]) {
            best_silrc_score = rcss[rcpermtab[ps_search_acmod(ngs)->mdef->sil]];
            best_silrc_bp = b;
        }
    }

    /* Transition to <sil> */
    newscore = best_silrc_score + ngs->silpen + pip;
    if ((newscore > thresh) && (newscore > WORST_SCORE)) {
        w = ngs->silence_wid;
        rhmm = (root_chan_t *) ngs->word_chan[w];
        if ((hmm_frame(&rhmm->hmm) < cf)
            || (hmm_in_score(&rhmm->hmm) < newscore)) {
            hmm_enter(&rhmm->hmm, newscore,
                      best_silrc_bp, nf);
            bitvec_set(ngs->word_active, w);
        }
    }
    /* Transition to noise words */
    newscore = best_silrc_score + ngs->fillpen + pip;
    if ((newscore > thresh) && (newscore > WORST_SCORE)) {
        for (w = ngs->silence_wid + 1; w < ps_search_n_words(ngs); w++) {
            rhmm = (root_chan_t *) ngs->word_chan[w];
            if ((hmm_frame(&rhmm->hmm) < cf)
                || (hmm_in_score(&rhmm->hmm) < newscore)) {
                hmm_enter(&rhmm->hmm, newscore,
                          best_silrc_bp, nf);
                bitvec_set(ngs->word_active, w);
            }
        }
    }

    /* Reset initial channels of words that have become inactive even after word trans. */
    i = ngs->n_active_word[cf & 0x1];
    awl = ngs->active_word_list[cf & 0x1];
    for (w = *(awl++); i > 0; --i, w = *(awl++)) {
        rhmm = (root_chan_t *) ngs->word_chan[w];
        if (hmm_frame(&rhmm->hmm) == cf) {
            hmm_clear_scores(&rhmm->hmm);
        }
    }
}

static void
fwdflat_renormalize_scores(ngram_search_t *ngs, int frame_idx, int32 norm)
{
    root_chan_t *rhmm;
    chan_t *hmm;
    int32 i, cf, w, *awl;

    cf = frame_idx;

    /* Renormalize individual word channels */
    i = ngs->n_active_word[cf & 0x1];
    awl = ngs->active_word_list[cf & 0x1];
    for (w = *(awl++); i > 0; --i, w = *(awl++)) {
        rhmm = (root_chan_t *) ngs->word_chan[w];
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

int
ngram_fwdflat_search(ngram_search_t *ngs)
{
    int16 const *senscr;
    int frame_idx, best_senid;
    int16 best_senscr;
    int32 nf, i, j;
    int32 *nawl;

    /* Determine if we actually have a frame to process. */
    if (ps_search_acmod(ngs)->n_feat_frame == 0)
        return 0;

    /* Activate our HMMs for the current frame if need be. */
    if (!ps_search_acmod(ngs)->compallsen)
        compute_fwdflat_sen_active(ngs, acmod_frame_idx(ps_search_acmod(ngs)));

    /* Compute GMM scores for the current frame. */
    senscr = acmod_score(ps_search_acmod(ngs), &frame_idx,
                         &best_senscr, &best_senid);
    ngs->st.n_senone_active_utt += ps_search_acmod(ngs)->n_senone_active;

    /* Mark backpointer table for current frame. */
    ngram_search_mark_bptable(ngs, frame_idx);

    /* Renormalize if necessary (FIXME: Make sure to test this) */
    if (ngs->best_score + (2 * ngs->beam) < WORST_SCORE) {
        E_INFO("Renormalizing Scores at frame %d, best score %d\n",
               frame_idx, ngs->best_score);
        fwdflat_renormalize_scores(ngs, frame_idx, ngs->best_score);
    }

    ngs->best_score = WORST_SCORE;
    hmm_context_set_senscore(ngs->hmmctx, senscr);

    /* Evaluate HMMs */
    fwdflat_eval_chan(ngs, frame_idx);
    /* Prune HMMs and do phone transitions. */
    fwdflat_prune_chan(ngs, frame_idx);
    /* Do word transitions. */
    fwdflat_word_transition(ngs, frame_idx);

    /* Create next active word list */
    nf = frame_idx + 1;
    nawl = ngs->active_word_list[nf & 0x1];
    for (i = 0, j = 0; ngs->fwdflat_wordlist[i] >= 0; i++) {
        if (bitvec_is_set(ngs->word_active, ngs->fwdflat_wordlist[i])) {
            *(nawl++) = ngs->fwdflat_wordlist[i];
            j++;
        }
    }
    for (i = ngs->start_wid; i < ps_search_n_words(ngs); i++) {
        if (bitvec_is_set(ngs->word_active, i)) {
            *(nawl++) = i;
            j++;
        }
    }
    if (!ngs->fwdtree)
        ++ngs->n_frame;
    ngs->n_active_word[nf & 0x1] = j;

    /* Return the number of frames processed. */
    return 1;
}

/**
 * Destroy wordlist from the current utterance.
 */
static void
destroy_fwdflat_wordlist(ngram_search_t *ngs)
{
    latnode_t *node, *tnode;
    int32 f;

    if (!ngs->fwdtree)
        return;

    for (f = 0; f < ngs->n_frame; f++) {
        for (node = ngs->frm_wordlist[f]; node; node = tnode) {
            tnode = node->next;
            listelem_free(ngs->latnode_alloc, node);
        }
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
        de = ps_search_dict(ngs)->dict_list[wid];

        if (de->len == 1)
            continue;

        assert(de->mpx);
        assert(ngs->word_chan[wid] != NULL);

        ngram_search_free_all_rc(ngs, wid);
    }
}

void
ngram_fwdflat_finish(ngram_search_t *ngs)
{
    int32 cf;

    destroy_fwdflat_chan(ngs);
    destroy_fwdflat_wordlist(ngs);
    bitvec_clear_all(ngs->word_active, ps_search_n_words(ngs));

    /* This is the number of frames processed. */
    cf = acmod_frame_idx(ps_search_acmod(ngs));
    /* Add a mark in the backpointer table for one past the final frame. */
    ngram_search_mark_bptable(ngs, cf);

    /* Print out some statistics. */
    if (cf > 0) {
        E_INFO("%8d words recognized (%d/fr)\n",
               ngs->bpidx, (ngs->bpidx + (cf >> 1)) / (cf + 1));
        E_INFO("%8d senones evaluated (%d/fr)\n", ngs->st.n_senone_active_utt,
               (ngs->st.n_senone_active_utt + (cf >> 1)) / (cf + 1));
        E_INFO("%8d channels searched (%d/fr)\n",
               ngs->st.n_fwdflat_chan, ngs->st.n_fwdflat_chan / (cf + 1));
        E_INFO("%8d words searched (%d/fr)\n",
               ngs->st.n_fwdflat_words, ngs->st.n_fwdflat_words / (cf + 1));
        E_INFO("%8d word transitions (%d/fr)\n",
               ngs->st.n_fwdflat_word_transition,
               ngs->st.n_fwdflat_word_transition / (cf + 1));
    }
}
