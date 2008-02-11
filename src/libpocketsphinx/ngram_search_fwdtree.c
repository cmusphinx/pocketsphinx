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
 * @file ngram_search_fwdtree.c Lexicon tree search.
 */

/* System headers. */
#include <string.h>

/* SphinxBase headers. */
#include <ckd_alloc.h>
#include <linklist.h>

/* Local headers. */
#include "ngram_search.h"

/*
 * Allocate that part of the search channel tree structure that is independent of the
 * LM in use.
 */
static void
init_search_tree(ngram_search_t *ngs)
{
    int32 w, mpx, max_ph0, i, n_words, n_main_words;
    dict_entry_t *de;

    n_words = ngs->dict->dict_entry_count;
    n_main_words = dict_get_num_main_words(ngs->dict);
    ngs->homophone_set = ckd_calloc(n_main_words, sizeof(*ngs->homophone_set));

    /* Find #single phone words, and #unique first diphones (#root channels) in dict. */
    max_ph0 = -1;
    ngs->n_1ph_words = 0;
    mpx = ngs->dict->dict_list[0]->mpx;
    for (w = 0; w < n_main_words; w++) {
        de = ngs->dict->dict_list[w];

        /* Paranoia Central; this check can probably be removed (RKM) */
        if (de->mpx != mpx)
            E_FATAL("HMM tree words not all mpx or all non-mpx\n");

        if (de->len == 1)
            ngs->n_1ph_words++;
        else {
            if (max_ph0 < de->phone_ids[0])
                max_ph0 = de->phone_ids[0];
        }
    }

    /* Add remaining dict words (</s>, <s>, <sil>, noise words) to single-phone words */
    ngs->n_1ph_words += (n_words - n_main_words);
    ngs->n_root_chan_alloc = max_ph0 + 1;

    /* Allocate and initialize root channels */
    ngs->root_chan =
        ckd_calloc(ngs->n_root_chan_alloc, sizeof(*ngs->root_chan));
    for (i = 0; i < ngs->n_root_chan_alloc; i++) {
        hmm_init(ngs->hmmctx, &ngs->root_chan[i].hmm, mpx, -1, -1);
        ngs->root_chan[i].penult_phn_wid = -1;
        ngs->root_chan[i].next = NULL;
    }

    /* Allocate space for left-diphone -> root-chan map */
    ngs->first_phone_rchan_map =
        ckd_calloc(ngs->n_root_chan_alloc,
                   sizeof(*ngs->first_phone_rchan_map));

    /* Permanently allocate channels for single-phone words (1/word) */
    ngs->all_rhmm = ckd_calloc(ngs->n_1ph_words, sizeof(*ngs->all_rhmm));
    i = 0;
    for (w = 0; w < n_words; w++) {
        de = ngs->dict->dict_list[w];
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

    ngs->single_phone_wid = ckd_calloc(ngs->n_1ph_words,
                                       sizeof(*ngs->single_phone_wid));
    E_INFO("%d root, %d non-root channels, %d single-phone words\n",
           ngs->n_root_chan, ngs->n_nonroot_chan, ngs->n_1ph_words);
}

/*
 * One-time initialization of internal channels in HMM tree.
 */
static void
init_nonroot_chan(ngram_search_t *ngs, chan_t * hmm, int32 ph, int32 ci)
{
    hmm->next = NULL;
    hmm->alt = NULL;
    hmm->info.penult_phn_wid = -1;
    hmm->ciphone = ci;
    hmm_init(ngs->hmmctx, &hmm->hmm, FALSE, ph, ci);
}

/*
 * Allocate and initialize search channel-tree structure.
 * At this point, all the root-channels have been allocated and partly initialized
 * (as per init_search_tree()), and channels for all the single-phone words have been
 * allocated and initialized.  None of the interior channels of search-trees have
 * been allocated.
 * This routine may be called on every utterance, after reinit_search_tree() clears
 * the search tree created for the previous utterance.  Meant for reconfiguring the
 * search tree to suit the currently active LM.
 */
static void
create_search_tree(ngram_search_t *ngs)
{
    dict_entry_t *de;
    chan_t *hmm;
    root_chan_t *rhmm;
    int32 w, i, j, p, ph;
    int32 n_words, n_main_words;

    n_words = ngs->dict->dict_entry_count;
    n_main_words = dict_get_num_main_words(ngs->dict);

    E_INFO("Creating search tree\n");

    for (w = 0; w < n_main_words; w++)
        ngs->homophone_set[w] = -1;

    for (i = 0; i < ngs->n_root_chan_alloc; i++)
        ngs->first_phone_rchan_map[i] = -1;

    E_INFO("%d root, %d non-root channels, %d single-phone words\n",
           ngs->n_root_chan, ngs->n_nonroot_chan, ngs->n_1ph_words);

    ngs->n_1ph_LMwords = 0;
    ngs->n_root_chan = 0;
    ngs->n_nonroot_chan = 0;

    for (w = 0; w < n_main_words; w++) {
        de = ngs->dict->dict_list[w];

        /* Ignore dictionary words not in LM */
        if (!ngram_model_set_known_wid(ngs->lmset, de->wid))
            continue;

        /* Handle single-phone words individually; not in channel tree */
        if (de->len == 1) {
            ngs->single_phone_wid[ngs->n_1ph_LMwords++] = w;
            continue;
        }

        /* Insert w into channel tree; first find or allocate root channel */
        if (ngs->first_phone_rchan_map[de->phone_ids[0]] < 0) {
            ngs->first_phone_rchan_map[de->phone_ids[0]] = ngs->n_root_chan;
            rhmm = &(ngs->root_chan[ngs->n_root_chan]);
            if (hmm_is_mpx(&rhmm->hmm))
                rhmm->hmm.s.mpx_ssid[0] = de->phone_ids[0];
            else
                rhmm->hmm.s.ssid = de->phone_ids[0];
            rhmm->hmm.tmatid = de->ci_phone_ids[0];
            rhmm->diphone = de->phone_ids[0];
            rhmm->ciphone = de->ci_phone_ids[0];

            ngs->n_root_chan++;
        }
        else
            rhmm = &(ngs->root_chan[ngs->first_phone_rchan_map[de->phone_ids[0]]]);

        /* Now, rhmm = root channel for w.  Go on to remaining phones */
        if (de->len == 2) {
            /* Next phone is the last; not kept in tree; add w to penult_phn_wid set */
            if ((j = rhmm->penult_phn_wid) < 0)
                rhmm->penult_phn_wid = w;
            else {
                for (; ngs->homophone_set[j] >= 0; j = ngs->homophone_set[j]);
                ngs->homophone_set[j] = w;
            }
        }
        else {
            /* Add remaining phones, except the last, to tree */
            ph = de->phone_ids[1];
            hmm = rhmm->next;
            if (hmm == NULL) {
                rhmm->next = hmm = (chan_t *) listelem_alloc(sizeof(*hmm));
                init_nonroot_chan(ngs, hmm, ph, de->ci_phone_ids[1]);
                ngs->n_nonroot_chan++;
            }
            else {
                chan_t *prev_hmm = NULL;

                for (; hmm && (hmm->hmm.s.ssid != ph); hmm = hmm->alt)
                    prev_hmm = hmm;
                if (!hmm) {     /* thanks, rkm! */
                    prev_hmm->alt = hmm = listelem_alloc(sizeof(*hmm));
                    init_nonroot_chan(ngs, hmm, ph, de->ci_phone_ids[1]);
                    ngs->n_nonroot_chan++;
                }
            }
            /* de->phone_ids[1] now in tree; pointed to by hmm */

            for (p = 2; p < de->len - 1; p++) {
                ph = de->phone_ids[p];
                if (!hmm->next) {
                    hmm->next = listelem_alloc(sizeof(*hmm->next));
                    hmm = hmm->next;
                    init_nonroot_chan(ngs, hmm, ph, de->ci_phone_ids[p]);
                    ngs->n_nonroot_chan++;
                }
                else {
                    chan_t *prev_hmm = NULL;

                    for (hmm = hmm->next; hmm && (hmm->hmm.s.ssid != ph);
                         hmm = hmm->alt)
                        prev_hmm = hmm;
                    if (!hmm) { /* thanks, rkm! */
                        prev_hmm->alt = hmm = listelem_alloc(sizeof(*hmm));
                        init_nonroot_chan(ngs, hmm, ph, de->ci_phone_ids[p]);
                        ngs->n_nonroot_chan++;
                    }
                }
            }

            /* All but last phone of w in tree; add w to hmm->info.penult_phn_wid set */
            if ((j = hmm->info.penult_phn_wid) < 0)
                hmm->info.penult_phn_wid = w;
            else {
                for (; ngs->homophone_set[j] >= 0; j = ngs->homophone_set[j]);
                ngs->homophone_set[j] = w;
            }
        }
    }

    ngs->n_1ph_words = ngs->n_1ph_LMwords;
    ngs->n_1ph_LMwords++;            /* including </s> */

    /* FIXME: I'm not really sure why n_1ph_words got reset above. */
    for (w = dict_to_id(ngs->dict, "</s>"); w < n_words; ++w) {
        de = ngs->dict->dict_list[w];
        /* Skip any non-fillers that aren't in the LM. */
        if ((!dict_is_filler_word(ngs->dict, w))
            && (!ngram_model_set_known_wid(ngs->lmset, de->wid)))
            continue;
        ngs->single_phone_wid[ngs->n_1ph_words++] = w;
    }

    if (ngs->n_nonroot_chan >= ngs->max_nonroot_chan) {
        /* Give some room for channels for new words added dynamically at run time */
        ngs->max_nonroot_chan = ngs->n_nonroot_chan + 128;
        E_INFO("max nonroot chan increased to %d\n", ngs->max_nonroot_chan);

        /* Free old active channel list array if any and allocate new one */
        if (ngs->active_chan_list)
            ckd_free_2d(ngs->active_chan_list);
        ngs->active_chan_list = ckd_calloc_2d(2, ngs->max_nonroot_chan,
                                              sizeof(**ngs->active_chan_list));
    }

    E_INFO("%d root, %d non-root channels, %d single-phone words\n",
           ngs->n_root_chan, ngs->n_nonroot_chan, ngs->n_1ph_words);
}

static void
reinit_search_subtree(chan_t * hmm)
{
    chan_t *child, *sibling;

    /* First free all children under hmm */
    for (child = hmm->next; child; child = sibling) {
        sibling = child->alt;
        reinit_search_subtree(child);
    }

    /* Now free hmm */
    hmm_deinit(&hmm->hmm);
    listelem_free(hmm, sizeof(*hmm));
}

/*
 * Delete search tree by freeing all interior channels within search tree and
 * restoring root channel state to the init state (i.e., just after init_search_tree()).
 */
static void
reinit_search_tree(ngram_search_t *ngs)
{
    int32 i;
    chan_t *hmm, *sibling;

    for (i = 0; i < ngs->n_root_chan; i++) {
        hmm = ngs->root_chan[i].next;

        while (hmm) {
            sibling = hmm->alt;
            reinit_search_subtree(hmm);
            hmm = sibling;
        }

        ngs->root_chan[i].penult_phn_wid = -1;
        ngs->root_chan[i].next = NULL;
    }
    ngs->n_nonroot_chan = 0;
}

void
ngram_fwdtree_init(ngram_search_t *ngs)
{
    init_search_tree(ngs);
    create_search_tree(ngs);
}

void
ngram_fwdtree_deinit(ngram_search_t *ngs)
{
    int i, w, n_words;

    n_words = ngs->dict->dict_entry_count;
    /* Reset non-root channels. */
    reinit_search_tree(ngs);

    /* Now deallocate all the root channels too. */
    for (i = 0; i < ngs->n_root_chan_alloc; i++) {
        hmm_deinit(&ngs->root_chan[i].hmm);
    }
    if (ngs->all_rhmm) {
        for (i = w = 0; w < n_words; ++w) {
            if (ngs->dict->dict_list[w]->len != 1)
                continue;
            hmm_deinit(&ngs->all_rhmm[i].hmm);
            ++i;
        }
        ckd_free(ngs->all_rhmm);
    }
    ngs->n_nonroot_chan = 0;
    ckd_free(ngs->first_phone_rchan_map);
    ckd_free(ngs->root_chan);
    ckd_free(ngs->homophone_set);
    ckd_free(ngs->single_phone_wid);
    ngs->max_nonroot_chan = 0;
    ckd_free_2d(ngs->active_chan_list);
}

void
ngram_fwdtree_start(ngram_search_t *ngs)
{
    int32 i, w, n_words;
    root_chan_t *rhmm;

    n_words = ngs->dict->dict_entry_count;

    /* Reset utterance statistics. */
    memset(&ngs->st, 0, sizeof(ngs->st));

    /* Reset backpointer table. */
    ngs->bpidx = 0;
    ngs->bss_head = 0;

    /* Reset word lattice. */
    for (i = 0; i < n_words; ++i)
        ngs->word_lat_idx[i] = NO_BP;

    /* Reset active HMM and word lists. */
    ngs->n_active_chan[0] = ngs->n_active_chan[1] = 0;
    ngs->n_active_word[0] = ngs->n_active_word[1] = 0;

    /* Reset scores. */
    ngs->best_score = 0;
    ngs->renormalized = 0;

    /* Reset other stuff. */
    for (i = 0; i < n_words; i++)
        ngs->last_ltrans[i].sf = -1;

    /* FIXME: Clear the hypothesis here. */

    /* Reset the permanently allocated single-phone words, since they
     * may have junk left over in them from FWDFLAT. */
    for (i = 0; i < ngs->n_1ph_words; i++) {
        w = ngs->single_phone_wid[i];
        rhmm = (root_chan_t *) ngs->word_chan[w];
        hmm_clear(&rhmm->hmm);
    }

    /* Start search with <s>; word_chan[<s>] is permanently allocated */
    rhmm = (root_chan_t *) ngs->word_chan[dict_to_id(ngs->dict, "<s>")];
    hmm_clear(&rhmm->hmm);
    hmm_enter(&rhmm->hmm, 0, NO_BP, 0);
}

void
ngram_fwdtree_search(ngram_search_t *ngs)
{
}

void
ngram_fwdtree_finish(ngram_search_t *ngs)
{
}
