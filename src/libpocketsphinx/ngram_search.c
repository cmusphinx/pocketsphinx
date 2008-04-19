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
 * @file ngram_search.c N-Gram based multi-pass search ("FBS")
 */

/* System headers. */
#include <string.h>
#include <assert.h>

/* SphinxBase headers. */
#include <ckd_alloc.h>
#include <listelem_alloc.h>

/* Local headers. */
#include "ngram_search.h"
#include "ngram_search_fwdtree.h"
#include "ngram_search_fwdflat.h"

static int ngram_search_start(ps_search_t *search);
static int ngram_search_step(ps_search_t *search);
static int ngram_search_finish(ps_search_t *search);
static int ngram_search_reinit(ps_search_t *search);
static char const *ngram_search_hyp(ps_search_t *search, int32 *out_score);
static ps_seg_t *ngram_search_seg_iter(ps_search_t *search, int32 *out_score);

static ps_searchfuncs_t ngram_funcs = {
    /* name: */   "ngram",
    /* start: */  ngram_search_start,
    /* step: */   ngram_search_step,
    /* finish: */ ngram_search_finish,
    /* reinit: */ ngram_search_reinit,
    /* free: */   ngram_search_free,
    /* lattice: */  ngram_search_lattice,
    /* hyp: */      ngram_search_hyp,
    /* seg_iter: */ ngram_search_seg_iter,
};

static void
ngram_search_update_widmap(ngram_search_t *ngs)
{
    const char **words;
    int32 i, n_words;

    /* It's okay to include fillers since they won't be in the LM */
    n_words = ps_search_n_words(ngs);
    words = ckd_calloc(n_words, sizeof(*words));
    /* This will include alternates, again, that's okay since they aren't in the LM */
    for (i = 0; i < n_words; ++i)
        words[i] = dict_word_str(ps_search_dict(ngs), i);
    ngram_model_set_map_words(ngs->lmset, words, n_words);
    ckd_free(words);
}

static void
ngram_search_calc_beams(ngram_search_t *ngs)
{
    cmd_ln_t *config;
    acmod_t *acmod;

    config = ps_search_config(ngs);
    acmod = ps_search_acmod(ngs);

    /* Log beam widths. */
    ngs->beam = logmath_log(acmod->lmath, cmd_ln_float64_r(config, "-beam"));
    ngs->wbeam = logmath_log(acmod->lmath, cmd_ln_float64_r(config, "-wbeam"));
    ngs->pbeam = logmath_log(acmod->lmath, cmd_ln_float64_r(config, "-pbeam"));
    ngs->lpbeam = logmath_log(acmod->lmath, cmd_ln_float64_r(config, "-lpbeam"));
    ngs->lponlybeam = logmath_log(acmod->lmath, cmd_ln_float64_r(config, "-lponlybeam"));
    ngs->fwdflatbeam = logmath_log(acmod->lmath, cmd_ln_float64_r(config, "-fwdflatbeam"));
    ngs->fwdflatwbeam = logmath_log(acmod->lmath, cmd_ln_float64_r(config, "-fwdflatwbeam"));

    /* Absolute pruning parameters. */
    ngs->maxwpf = cmd_ln_int32_r(config, "-maxwpf");
    ngs->maxhmmpf = cmd_ln_int32_r(config, "-maxhmmpf");

    /* Various penalties which may or may not be useful. */
    ngs->wip = logmath_log(acmod->lmath, cmd_ln_float32_r(config, "-wip"));
    ngs->nwpen = logmath_log(acmod->lmath, cmd_ln_float32_r(config, "-nwpen"));
    ngs->pip = logmath_log(acmod->lmath, cmd_ln_float32_r(config, "-pip"));
    ngs->silpen = ngs->pip
        + logmath_log(acmod->lmath, cmd_ln_float32_r(config, "-silprob"));
    ngs->fillpen = ngs->pip
        + logmath_log(acmod->lmath, cmd_ln_float32_r(config, "-fillprob"));

    /* Language weight ratios for fwdflat and bestpath search. */
    ngs->fwdflat_fwdtree_lw_ratio =
        cmd_ln_float32_r(config, "-fwdflatlw")
        / cmd_ln_float32_r(config, "-lw");
    ngs->bestpath_fwdtree_lw_ratio =
        cmd_ln_float32_r(config, "-bestpathlw")
        / cmd_ln_float32_r(config, "-lw");
}

ps_search_t *
ngram_search_init(cmd_ln_t *config,
		  acmod_t *acmod,
		  dict_t *dict)
{
    ngram_search_t *ngs;
    const char *path;

    ngs = ckd_calloc(1, sizeof(*ngs));
    ps_search_init(&ngs->base, &ngram_funcs, config, acmod, dict);
    ngs->hmmctx = hmm_context_init(bin_mdef_n_emit_state(acmod->mdef),
                                   acmod->tmat->tp, NULL, acmod->mdef->sseq);
    ngs->chan_alloc = listelem_alloc_init(sizeof(chan_t));
    ngs->root_chan_alloc = listelem_alloc_init(sizeof(root_chan_t));
    ngs->latnode_alloc = listelem_alloc_init(sizeof(latnode_t));

    /* Calculate various beam widths and such. */
    ngram_search_calc_beams(ngs);

    /* Allocate a billion different tables for stuff. */
    ngs->word_chan = ckd_calloc(dict->dict_entry_count,
                                sizeof(*ngs->word_chan));
    ngs->word_lat_idx = ckd_calloc(dict->dict_entry_count,
                                   sizeof(*ngs->word_lat_idx));
    ngs->zeroPermTab = ckd_calloc(bin_mdef_n_ciphone(acmod->mdef),
                                  sizeof(*ngs->zeroPermTab));
    ngs->word_active = bitvec_alloc(dict->dict_entry_count);
    ngs->last_ltrans = ckd_calloc(dict->dict_entry_count,
                                  sizeof(*ngs->last_ltrans));

    /* FIXME: All these structures need to be made dynamic with
     * garbage collection. */
    ngs->bp_table_size = cmd_ln_int32_r(config, "-latsize");
    ngs->bp_table = ckd_calloc(ngs->bp_table_size,
                               sizeof(*ngs->bp_table));
    /* FIXME: This thing is frickin' huge. */
    ngs->bscore_stack_size = ngs->bp_table_size * 20;
    ngs->bscore_stack = ckd_calloc(ngs->bscore_stack_size,
                                   sizeof(*ngs->bscore_stack));
    ngs->n_frame_alloc = 256;
    ngs->bp_table_idx = ckd_calloc(ngs->n_frame_alloc + 1,
                                   sizeof(*ngs->bp_table_idx));
    ++ngs->bp_table_idx; /* Make bptableidx[-1] valid */

    /* Allocate active word list array */
    ngs->active_word_list = ckd_calloc_2d(2, dict->dict_entry_count,
                                          sizeof(**ngs->active_word_list));

    /* Load language model(s) */
    if ((path = cmd_ln_str_r(config, "-lmctlfn"))) {
        ngs->lmset = ngram_model_set_read(config, path, acmod->lmath);
        if (ngs->lmset == NULL) {
            E_ERROR("Failed to read language model control file: %s\n",
                    path);
            goto error_out;
        }
        /* Set the default language model if needed. */
        if ((path = cmd_ln_str_r(config, "-lmname"))) {
            ngram_model_set_select(ngs->lmset, path);
        }
    }
    else if ((path = cmd_ln_str_r(config, "-lm"))) {
        static const char *name = "default";
        ngram_model_t *lm;

        lm = ngram_model_read(config, path, NGRAM_AUTO, acmod->lmath);
        if (lm == NULL) {
            E_ERROR("Failed to read language model file: %s\n", path);
            goto error_out;
        }
        ngs->lmset = ngram_model_set_init(config,
                                          &lm, (char **)&name,
                                          NULL, 1);
        if (ngs->lmset == NULL) {
            E_ERROR("Failed to initialize language model set\n");
            goto error_out;
        }
    }

    /* Create word mappings. */
    ngram_search_update_widmap(ngs);

    /* Initialize fwdtree, fwdflat, bestpath modules if necessary. */
    if (cmd_ln_boolean_r(config, "-fwdtree")) {
        ngram_fwdtree_init(ngs);
        ngs->fwdtree = TRUE;
    }
    if (cmd_ln_boolean_r(config, "-fwdflat")) {
        ngram_fwdflat_init(ngs);
        ngs->fwdflat = TRUE;
    }
    if (cmd_ln_boolean_r(config, "-bestpath")) {
        ngs->bestpath = TRUE;
    }
    return (ps_search_t *)ngs;

error_out:
    ngram_search_free((ps_search_t *)ngs);
    return NULL;
}

static int
ngram_search_reinit(ps_search_t *search)
{
    ngram_search_t *ngs = (ngram_search_t *)search;
    int rv = 0;

    /*
     * NOTE!!! This is not a general-purpose reinit function.  It only
     * deals with updates to the language model set and beam widths.
     */

    /* Update beam widths. */
    ngram_search_calc_beams(ngs);

    /* Update word mappings. */
    ngram_search_update_widmap(ngs);

    /* Now rebuild lextrees or what have you. */
    if (ngs->fwdtree) {
        if ((rv = ngram_fwdtree_reinit(ngs)) < 0)
            return rv;
    }
    if (ngs->fwdflat) {
        if ((rv = ngram_fwdflat_reinit(ngs)) < 0)
            return rv;
    }

    return rv;
}

void
ngram_search_free(ps_search_t *search)
{
    ngram_search_t *ngs = (ngram_search_t *)search;

    ps_search_deinit(search);
    if (ngs->fwdtree)
        ngram_fwdtree_deinit(ngs);
    if (ngs->fwdflat)
        ngram_fwdflat_deinit(ngs);
    ps_lattice_free(ngs->dag);

    hmm_context_free(ngs->hmmctx);
    listelem_alloc_free(ngs->chan_alloc);
    listelem_alloc_free(ngs->root_chan_alloc);
    listelem_alloc_free(ngs->latnode_alloc);
    ngram_model_free(ngs->lmset);

    ckd_free(ngs->word_chan);
    ckd_free(ngs->word_lat_idx);
    ckd_free(ngs->zeroPermTab);
    bitvec_free(ngs->word_active);
    ckd_free(ngs->bp_table);
    ckd_free(ngs->bscore_stack);
    ckd_free(ngs->bp_table_idx - 1);
    ckd_free_2d(ngs->active_word_list);
    ckd_free(ngs->last_ltrans);
    ckd_free(ngs);
}

int
ngram_search_mark_bptable(ngram_search_t *ngs, int frame_idx)
{
    if (frame_idx >= ngs->n_frame_alloc) {
        ngs->n_frame_alloc *= 2;
        ngs->bp_table_idx = ckd_realloc(ngs->bp_table_idx - 1,
                                        (ngs->n_frame_alloc + 1)
                                        * sizeof(*ngs->bp_table_idx));
        if (ngs->frm_wordlist) {
            ngs->frm_wordlist = ckd_realloc(ngs->frm_wordlist,
                                            ngs->n_frame_alloc
                                            * sizeof(*ngs->frm_wordlist));
        }
        ++ngs->bp_table_idx; /* Make bptableidx[-1] valid */
    }
    ngs->bp_table_idx[frame_idx] = ngs->bpidx;
    return ngs->bpidx;
}

/**
 * Find trigram predecessors for a backpointer table entry.
 */
static void
cache_bptable_paths(ngram_search_t *ngs, int32 bp)
{
    int32 w, prev_bp;
    bptbl_t *bpe;

    bpe = &(ngs->bp_table[bp]);
    prev_bp = bp;
    w = bpe->wid;
    /* FIXME: This isn't the ideal way to tell if it's a filler. */
    while (w >= ps_search_silence_wid(ngs)) {
        prev_bp = ngs->bp_table[prev_bp].bp;
        w = ngs->bp_table[prev_bp].wid;
    }
    bpe->real_wid = dict_base_wid(ps_search_dict(ngs), w);

    prev_bp = ngs->bp_table[prev_bp].bp;
    bpe->prev_real_wid =
        (prev_bp != NO_BP) ? ngs->bp_table[prev_bp].real_wid : -1;
}

void
ngram_search_save_bp(ngram_search_t *ngs, int frame_idx,
                     int32 w, int32 score, int32 path, int32 rc)
{
    int32 _bp_;

    _bp_ = ngs->word_lat_idx[w];
    if (_bp_ != NO_BP) {
        if (ngs->bp_table[_bp_].score < score) {
            if (ngs->bp_table[_bp_].bp != path) {
                ngs->bp_table[_bp_].bp = path;
                cache_bptable_paths(ngs, _bp_);
            }
            ngs->bp_table[_bp_].score = score;
        }
        ngs->bscore_stack[ngs->bp_table[_bp_].s_idx + rc] = score;
    }
    else {
        int32 i, rcsize, *bss;
        dict_entry_t *de;
        bptbl_t *bpe;

        /* Expand the backpointer tables if necessary. */
        if (ngs->bpidx >= ngs->bp_table_size) {
            ngs->bp_table_size *= 2;
            ngs->bp_table = ckd_realloc(ngs->bp_table,
                                        ngs->bp_table_size
                                        * sizeof(*ngs->bp_table));
            E_INFO("Resized backpointer table to %d entries\n", ngs->bp_table_size);
        }
        if (ngs->bss_head >= ngs->bscore_stack_size
            - bin_mdef_n_ciphone(ps_search_acmod(ngs)->mdef)) {
            ngs->bscore_stack_size *= 2;
            ngs->bscore_stack = ckd_realloc(ngs->bscore_stack,
                                            ngs->bscore_stack_size
                                            * sizeof(*ngs->bscore_stack));
            E_INFO("Resized score stack to %d entries\n", ngs->bscore_stack_size);
        }

        de = ps_search_dict(ngs)->dict_list[w];
        ngs->word_lat_idx[w] = ngs->bpidx;
        bpe = &(ngs->bp_table[ngs->bpidx]);
        bpe->wid = w;
        bpe->frame = frame_idx;
        bpe->bp = path;
        bpe->score = score;
        bpe->s_idx = ngs->bss_head;
        bpe->valid = TRUE;

        if ((de->len != 1) && (de->mpx)) {
            bpe->r_diph = de->phone_ids[de->len - 1];
            rcsize = ps_search_dict(ngs)->rcFwdSizeTable[bpe->r_diph];
        }
        else {
            bpe->r_diph = -1;
            rcsize = 1;
        }
        for (i = rcsize, bss = ngs->bscore_stack + ngs->bss_head; i > 0; --i, bss++)
            *bss = WORST_SCORE;
        ngs->bscore_stack[ngs->bss_head + rc] = score;
        cache_bptable_paths(ngs, ngs->bpidx);

        ngs->bpidx++;
        ngs->bss_head += rcsize;
    }
}

int
ngram_search_find_exit(ngram_search_t *ngs, int frame_idx, int32 *out_best_score)
{
    /* End of backpointers for this frame. */
    int end_bpidx;
    int best_exit, bp;
    int32 best_score;

    if (frame_idx == -1)
        frame_idx = acmod_frame_idx(ps_search_acmod(ngs));
    end_bpidx = ngs->bp_table_idx[frame_idx];

    /* FIXME: WORST_SCORE has to go away and be replaced with a log-zero number. */
    best_score = WORST_SCORE;
    best_exit = NO_BP;

    /* Scan back to find a frame with some backpointers in it. */
    while (frame_idx >= 0 && ngs->bp_table_idx[frame_idx] == end_bpidx)
        --frame_idx;
    if (frame_idx < 0) {
        E_ERROR("ngram_search_find_exit() called with empty backpointer table\n");
        return NO_BP;
    }

    /* Now find the entry for </s> OR the best scoring entry. */
    for (bp = ngs->bp_table_idx[frame_idx]; bp < end_bpidx; ++bp) {
        if (ngs->bp_table[bp].wid == ps_search_finish_wid(ngs)
            || ngs->bp_table[bp].score > best_score) {
            best_score = ngs->bp_table[bp].score;
            best_exit = bp;
        }
        if (ngs->bp_table[bp].wid == ps_search_finish_wid(ngs))
            break;
    }

    if (out_best_score) *out_best_score = best_score;
    return best_exit;
}

char const *
ngram_search_bp_hyp(ngram_search_t *ngs, int bpidx)
{
    ps_search_t *base = ps_search_base(ngs);
    char *c;
    size_t len;
    int bp;

    if (bpidx == NO_BP)
        return NULL;

    bp = bpidx;
    len = 0;
    while (bp != NO_BP) {
        bptbl_t *be = &ngs->bp_table[bp];
        bp = be->bp;
        if (ISA_REAL_WORD(ngs, be->wid))
            len += strlen(dict_base_str(ps_search_dict(ngs), be->wid)) + 1;
    }

    ckd_free(base->hyp_str);
    base->hyp_str = ckd_calloc(1, len);
    bp = bpidx;
    c = base->hyp_str + len - 1;
    while (bp != NO_BP) {
        bptbl_t *be = &ngs->bp_table[bp];
        size_t len;

        bp = be->bp;
        if (ISA_REAL_WORD(ngs, be->wid)) {
            len = strlen(dict_base_str(ps_search_dict(ngs), be->wid));
            c -= len;
            memcpy(c, dict_base_str(ps_search_dict(ngs), be->wid), len);
            if (c > base->hyp_str) {
                --c;
                *c = ' ';
            }
        }
    }

    return base->hyp_str;
}

void
ngram_search_alloc_all_rc(ngram_search_t *ngs, int32 w)
{
    dict_entry_t *de;
    chan_t *hmm, *thmm;
    int32 *sseq_rc;             /* list of sseqid for all possible right context for w */
    int32 i;

    de = ps_search_dict(ngs)->dict_list[w];

    assert(de->mpx);

    sseq_rc = ps_search_dict(ngs)->rcFwdTable[de->phone_ids[de->len - 1]];

    hmm = ngs->word_chan[w];
    if ((hmm == NULL) || (hmm->hmm.s.ssid != *sseq_rc)) {
        hmm = listelem_malloc(ngs->chan_alloc);
        hmm->next = ngs->word_chan[w];
        ngs->word_chan[w] = hmm;

        hmm->info.rc_id = 0;
        hmm->ciphone = de->ci_phone_ids[de->len - 1];
        hmm_init(ngs->hmmctx, &hmm->hmm, FALSE, *sseq_rc, hmm->ciphone);
    }
    for (i = 1, sseq_rc++; *sseq_rc >= 0; sseq_rc++, i++) {
        if ((hmm->next == NULL) || (hmm->next->hmm.s.ssid != *sseq_rc)) {
            thmm = listelem_malloc(ngs->chan_alloc);
            thmm->next = hmm->next;
            hmm->next = thmm;
            hmm = thmm;

            hmm->info.rc_id = i;
            hmm->ciphone = de->ci_phone_ids[de->len - 1];
            hmm_init(ngs->hmmctx, &hmm->hmm, FALSE, *sseq_rc, hmm->ciphone);
        }
        else
            hmm = hmm->next;
    }
}

void
ngram_search_free_all_rc(ngram_search_t *ngs, int32 w)
{
    chan_t *hmm, *thmm;

    for (hmm = ngs->word_chan[w]; hmm; hmm = thmm) {
        thmm = hmm->next;
        hmm_deinit(&hmm->hmm);
        listelem_free(ngs->chan_alloc, hmm);
    }
    ngs->word_chan[w] = NULL;
}

/*
 * Compute acoustic and LM scores for each BPTable entry (segment).
 */
void
ngram_compute_seg_scores(ngram_search_t *ngs, float32 lwf)
{
    int32 bp, start_score;
    bptbl_t *bpe, *p_bpe;
    int32 *rcpermtab;
    dict_entry_t *de;

    for (bp = 0; bp < ngs->bpidx; bp++) {
        bpe = ngs->bp_table + bp;

        /* Start of utterance. */
        if (bpe->bp == NO_BP) {
            bpe->ascr = bpe->score;
            bpe->lscr = 0;
            continue;
        }

        /* Otherwise, calculate lscr and ascr. */
        de = ps_search_dict(ngs)->dict_list[bpe->wid];
        p_bpe = ngs->bp_table + bpe->bp;
        rcpermtab = (p_bpe->r_diph >= 0) ?
            ps_search_dict(ngs)->rcFwdPermTable[p_bpe->r_diph] : ngs->zeroPermTab;
        start_score =
            ngs->bscore_stack[p_bpe->s_idx + rcpermtab[de->ci_phone_ids[0]]];

        if (bpe->wid == ps_search_silence_wid(ngs)) {
            bpe->lscr = ngs->silpen;
        }
        else if (ISA_FILLER_WORD(ngs, bpe->wid)) {
            bpe->lscr = ngs->fillpen;
        }
        else {
            int32 n_used;
            bpe->lscr = ngram_tg_score(ngs->lmset, de->wid,
                                       p_bpe->real_wid,
                                       p_bpe->prev_real_wid, &n_used);
            bpe->lscr = bpe->lscr * lwf;
        }
        bpe->ascr = bpe->score - start_score - bpe->lscr;
    }
}

static int
ngram_search_start(ps_search_t *search)
{
    ngram_search_t *ngs = (ngram_search_t *)search;

    if (ngs->fwdtree)
        ngram_fwdtree_start(ngs);
    else if (ngs->fwdflat)
        ngram_fwdflat_start(ngs);
    else
        return -1;
    return 0;
}

static int
ngram_search_step(ps_search_t *search)
{
    ngram_search_t *ngs = (ngram_search_t *)search;

    if (ngs->fwdtree)
        return ngram_fwdtree_search(ngs);
    else if (ngs->fwdflat)
        return ngram_fwdflat_search(ngs);
    else
        return -1;
}

static int
ngram_search_finish(ps_search_t *search)
{
    ngram_search_t *ngs = (ngram_search_t *)search;

    if (ngs->fwdtree) {
        ngram_fwdtree_finish(ngs);

        /* Now do fwdflat search in its entirety, if requested. */
        if (ngs->fwdflat) {
            int nfr;

            /* Rewind the acoustic model. */
            acmod_rewind(ps_search_acmod(ngs));
            /* Now redo search. */
            ngram_fwdflat_start(ngs);
            while ((nfr = ngram_fwdflat_search(ngs)) > 0) {
                /* Do nothing! */
            }
            if (nfr < 0)
                return nfr;
            ngram_fwdflat_finish(ngs);
            /* And now, we should have a result... */
        }
    }
    else if (ngs->fwdflat) {
        ngram_fwdflat_finish(ngs);
    }

    /* Build a DAG if necessary. */
    if (ngs->bestpath) {
        /* Compute these such that they agree with the fwdtree language weight. */
        ngram_compute_seg_scores(ngs,
                                 ngs->fwdflat
                                 ? ngs->fwdflat_fwdtree_lw_ratio : 1.0);
        ngram_search_lattice(search);
    }

    return 0;
}

static char const *
ngram_search_hyp(ps_search_t *search, int32 *out_score)
{
    ngram_search_t *ngs = (ngram_search_t *)search;

    /* Use the DAG if it exists (means that the utt is done, for now
     * at least...) */
    if (ngs->bestpath && ngs->dag) {
        latlink_t *link;

        link = ps_lattice_bestpath(ngs->dag, ngs->lmset,
                                   ngs->bestpath_fwdtree_lw_ratio);
        if (out_score) *out_score = link->path_scr;
        return ps_lattice_hyp(ngs->dag, link);
    }
    else {
        int32 bpidx;

        /* fwdtree and fwdflat use same backpointer table. */
        bpidx = ngram_search_find_exit(ngs, -1, out_score);
        if (bpidx != NO_BP)
            return ngram_search_bp_hyp(ngs, bpidx);
    }

    return NULL;
}

static void
ngram_search_bp2itor(ps_seg_t *seg, int bp)
{
    ngram_search_t *ngs = (ngram_search_t *)seg->search;
    bptbl_t *be, *pbe;

    be = &ngs->bp_table[bp];
    pbe = be->bp == -1 ? NULL : &ngs->bp_table[be->bp];
    seg->word = dict_word_str(ps_search_dict(ngs), be->wid);
    seg->ef = be->frame;
    seg->sf = pbe ? pbe->frame + 1 : 0;
    seg->prob = 0; /* Bogus value... */
}

static void
ngram_bp_seg_free(ps_seg_t *seg)
{
    bptbl_seg_t *itor = (bptbl_seg_t *)seg;
    
    ckd_free(itor->bpidx);
    ckd_free(itor);
}

static ps_seg_t *
ngram_bp_seg_next(ps_seg_t *seg)
{
    bptbl_seg_t *itor = (bptbl_seg_t *)seg;

    if (++itor->cur == itor->n_bpidx) {
        ngram_bp_seg_free(seg);
        return NULL;
    }

    ngram_search_bp2itor(seg, itor->bpidx[itor->cur]);
    return seg;
}

static ps_segfuncs_t ngram_bp_segfuncs = {
    /* seg_next */ ngram_bp_seg_next,
    /* seg_free */ ngram_bp_seg_free
};

static ps_seg_t *
ngram_search_bp_iter(ngram_search_t *ngs, int bpidx)
{
    bptbl_seg_t *itor;
    int bp, cur;

    /* Calling this an "iterator" is a bit of a misnomer since we have
     * to get the entire backtrace in order to produce it.  On the
     * other hand, all we actually need is the bptbl IDs, and we can
     * allocate a fixed-size array of them. */
    itor = ckd_calloc(1, sizeof(*itor));
    itor->base.vt = &ngram_bp_segfuncs;
    itor->base.search = ps_search_base(ngs);
    itor->n_bpidx = 0;
    bp = bpidx;
    while (bp != NO_BP) {
        bptbl_t *be = &ngs->bp_table[bp];
        bp = be->bp;
        ++itor->n_bpidx;
    }
    itor->bpidx = ckd_calloc(itor->n_bpidx, sizeof(*itor->bpidx));
    cur = itor->n_bpidx - 1;
    bp = bpidx;
    while (bp != NO_BP) {
        bptbl_t *be = &ngs->bp_table[bp];
        itor->bpidx[cur] = bp;
        bp = be->bp;
        --cur;
    }

    /* Fill in relevant fields for first element. */
    ngram_search_bp2itor((ps_seg_t *)itor, itor->bpidx[0]);

    return (ps_seg_t *)itor;
}

static ps_seg_t *
ngram_search_seg_iter(ps_search_t *search, int32 *out_score)
{
    ngram_search_t *ngs = (ngram_search_t *)search;

    /* Use the DAG if it exists (means that the utt is done, for now
     * at least...) */
    if (ngs->bestpath && ngs->dag) {
        latlink_t *last;

        last = ps_lattice_bestpath(ngs->dag, ngs->lmset,
                                   ngs->bestpath_fwdtree_lw_ratio);
        return ps_lattice_iter(ngs->dag, last);
    }
    else {
        int32 bpidx;

        /* fwdtree and fwdflat use same backpointer table. */
        bpidx = ngram_search_find_exit(ngs, -1, out_score);
        return ngram_search_bp_iter(ngs, bpidx);
    }

    return NULL;
}

static void
create_dag_nodes(ngram_search_t *ngs, ps_lattice_t *dag)
{
    bptbl_t *bp_ptr;
    int32 i;

    for (i = 0, bp_ptr = ngs->bp_table; i < ngs->bpidx; ++i, ++bp_ptr) {
        int32 sf, ef, wid;
        latnode_t *node;

        if (!bp_ptr->valid)
            continue;

        sf = (bp_ptr->bp < 0) ? 0 : ngs->bp_table[bp_ptr->bp].frame + 1;
        ef = bp_ptr->frame;
        wid = bp_ptr->wid;

        assert(ef < dag->n_frames);
        /* Skip non-final </s> entries. */
        if ((wid == ps_search_finish_wid(ngs)) && (ef < dag->n_frames - 1))
            continue;

        /* Skip if word not in LM */
        if ((!ISA_FILLER_WORD(ngs, wid))
            && (!ngram_model_set_known_wid(ngs->lmset,
                                           dict_base_wid(ps_search_dict(ngs), wid))))
            continue;

        /* See if bptbl entry <wid,sf> already in lattice */
        for (node = dag->nodes; node; node = node->next) {
            if ((node->wid == wid) && (node->sf == sf))
                break;
        }

        /* For the moment, store bptbl indices in node.{fef,lef} */
        if (node)
            node->lef = i;
        else {
            /* New node; link to head of list */
            node = listelem_malloc(dag->latnode_alloc);
            node->wid = wid;
            node->sf = sf; /* This is a frame index. */
            node->fef = node->lef = i; /* These are backpointer indices (argh) */
            node->reachable = FALSE;
            node->links = NULL;
            node->revlinks = NULL;

            node->next = dag->nodes;
            dag->nodes = node;
        }
    }
}

static latnode_t *
find_start_node(ngram_search_t *ngs, ps_lattice_t *dag)
{
    latnode_t *node;

    /* Find start node <s>.0 */
    for (node = dag->nodes; node; node = node->next) {
        if ((node->wid == ps_search_start_wid(ngs)) && (node->sf == 0))
            break;
    }
    if (!node) {
        /* This is probably impossible. */
        E_ERROR("Couldn't find <s> in first frame\n");
        return NULL;
    }
    return node;
}

static latnode_t *
find_end_node(ngram_search_t *ngs, ps_lattice_t *dag, float32 lwf)
{
    latnode_t *node;
    int32 ef, bestbp, bp, bestscore;

    /* Find final node </s>.last_frame; nothing can follow this node */
    for (node = dag->nodes; node; node = node->next) {
        int32 lef = ngs->bp_table[node->lef].frame;
        if ((node->wid == ps_search_finish_wid(ngs))
            && (lef == dag->n_frames - 1))
            break;
    }
    if (node != NULL)
        return node;

    /* It is quite likely that no </s> exited in the last frame.  So,
     * find the node corresponding to the best exit. */
    /* Find the last frame containing a word exit. */
    for (ef = dag->n_frames - 1;
         ef >= 0 && ngs->bp_table_idx[ef] == ngs->bpidx;
         --ef);
    if (ef < 0) {
        E_ERROR("Empty backpointer table: can not build DAG.\n");
        return NULL;
    }

    /* Find best word exit in that frame. */
    bestscore = WORST_SCORE;
    bestbp = NO_BP;
    for (bp = ngs->bp_table_idx[ef]; bp < ngs->bp_table_idx[ef + 1]; ++bp) {
        int32 n_used, l_scr;
        l_scr = ngram_tg_score(ngs->lmset, ps_search_finish_wid(ngs),
                               ngs->bp_table[bp].real_wid,
                               ngs->bp_table[bp].prev_real_wid,
                               &n_used);
        l_scr = l_scr * lwf;

        if (ngs->bp_table[bp].score + l_scr > bestscore) {
            bestscore = ngs->bp_table[bp].score + l_scr;
            bestbp = bp;
        }
    }
    E_WARN("</s> not found in last frame, using %s.%d instead\n",
           dict_base_str(ps_search_dict(ngs), ngs->bp_table[bestbp].real_wid), ef);

    /* Now find the node that corresponds to it. */
    for (node = dag->nodes; node; node = node->next) {
        if (node->lef == bestbp)
            return node;
    }

    E_ERROR("Failed to find DAG node corresponding to %s.%d\n",
           dict_base_str(ps_search_dict(ngs), ngs->bp_table[bestbp].real_wid), ef);
    return NULL;
}

/*
 * Build lattice from bptable.
 */
ps_lattice_t *
ngram_search_lattice(ps_search_t *search)
{
    int32 i, ef, lef, score, bss_offset;
    latnode_t *node, *from, *to;
    ngram_search_t *ngs;
    ps_lattice_t *dag;

    ngs = (ngram_search_t *)search;
    /* Remove previous lattice and cache this one. */
    if (ngs->dag) {
        ps_lattice_free(ngs->dag);
        ngs->dag = NULL;
    }

    dag = ps_lattice_init(search, ngs->n_frame);
    create_dag_nodes(ngs, dag);
    if ((dag->start = find_start_node(ngs, dag)) == NULL)
        goto error_out;
    if ((dag->end = find_end_node(ngs, dag, ngs->bestpath_fwdtree_lw_ratio)) == NULL)
        goto error_out;
    dag->final_node_ascr = ngs->bp_table[dag->end->lef].ascr;

    /*
     * At this point, dag->nodes is ordered such that nodes earlier in
     * the list can follow (in time) those later in the list, but not
     * vice versa.  Now create precedence links and simultanesously
     * mark all nodes that can reach dag->end.  (All nodes are reached
     * from dag->start; no problem there.)
     */
    dag->end->reachable = TRUE;
    for (to = dag->end; to; to = to->next) {
        /* Skip if not reachable; it will never be reachable from dag->end */
        if (!to->reachable)
            continue;

        /* Find predecessors of to : from->fef+1 <= to->sf <= from->lef+1 */
        for (from = to->next; from; from = from->next) {
            bptbl_t *bp_ptr;

            ef = ngs->bp_table[from->fef].frame;
            lef = ngs->bp_table[from->lef].frame;

            if ((to->sf <= ef) || (to->sf > lef + 1))
                continue;

            /* Find bptable entry for "from" that exactly precedes "to" */
            i = from->fef;
            bp_ptr = ngs->bp_table + i;
            for (; i <= from->lef; i++, bp_ptr++) {
                if (bp_ptr->wid != from->wid)
                    continue;
                if (bp_ptr->frame >= to->sf - 1)
                    break;
            }

            if ((i > from->lef) || (bp_ptr->frame != to->sf - 1))
                continue;

            /* Find acoustic score from.sf->to.sf-1 with right context = to */
            if (bp_ptr->r_diph >= 0)
                bss_offset =
                    search->dict->rcFwdPermTable[bp_ptr->r_diph]
                    [search->dict->dict_list[to->wid]->ci_phone_ids[0]];
            else
                bss_offset = 0;
            score =
                (ngs->bscore_stack[bp_ptr->s_idx + bss_offset] - bp_ptr->score) +
                bp_ptr->ascr;
            if (score > WORST_SCORE) {
                link_latnodes(dag, from, to, score, bp_ptr->frame);
                from->reachable = TRUE;
            }
        }
    }

    /* There must be at least one path between dag->start and dag->end */
    if (!dag->start->reachable) {
        E_ERROR("<s> isolated; unreachable\n");
        goto error_out;
    }

    /* Now change node->{fef,lef} from bptbl indices to frames. */
    for (node = dag->nodes; node; node = node->next) {
        node->fef = ngs->bp_table[node->fef].frame;
        node->lef = ngs->bp_table[node->lef].frame;
    }

    /* Find base wid for nodes. */
    for (node = dag->nodes; node; node = node->next) {
        node->basewid = dict_base_wid(search->dict, node->wid);
    }

    /* Remove SIL and noise nodes from DAG; extend links through them */
    ps_lattice_bypass_fillers(dag, ngs->silpen, ngs->fillpen);

    /* Free nodes unreachable from dag->start and their links */
    ps_lattice_delete_unreachable(dag);

    ngs->dag = dag;
    return dag;

error_out:
    ps_lattice_free(dag);
    return NULL;
}

