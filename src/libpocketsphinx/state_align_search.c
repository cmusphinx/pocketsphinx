/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 2010 Carnegie Mellon University.  All rights
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
 * @file state_align_search.c State (and phone and word) alignment search.
 */

#include "state_align_search.h"

static int
state_align_search_start(ps_search_t *search)
{
    state_align_search_t *sas = (state_align_search_t *)search;
    /* Create the initial token. */
    /* Activate the initial state. */
    return 0;
}

static int
state_align_search_step(ps_search_t *search, int frame_idx)
{
    state_align_search_t *sas = (state_align_search_t *)search;

    /* Calculate senone scores. */
    /* Viterbi step. */
    /* Transition out of non-emitting states. */
    /* Generate new tokens from best path results. */
    return 0;
}

static int
state_align_search_finish(ps_search_t *search)
{
    state_align_search_t *sas = (state_align_search_t *)search;
    /* Nothing to do here? */
    return 0;
}

static int
state_align_search_reinit(ps_search_t *search, dict_t *dict, dict2pid_t *d2p)
{
    /* This does nothing. */
    return 0;
}

static void
state_align_search_free(ps_search_t *search)
{
    state_align_search_t *sas = (state_align_search_t *)search;
    ps_search_deinit(search);
    ckd_free(sas->hmms);
    hmm_context_free(sas->hmmctx);
    ckd_free(sas);
}

static ps_searchfuncs_t state_align_search_funcs = {
    /* name: */   "state_align",
    /* start: */  state_align_search_start,
    /* step: */   state_align_search_step,
    /* finish: */ state_align_search_finish,
    /* reinit: */ state_align_search_reinit,
    /* free: */   state_align_search_free,
    /* lattice: */  NULL,
    /* hyp: */      NULL,
    /* prob: */     NULL,
    /* seg_iter: */ NULL,
};

ps_search_t *
state_align_search_init(cmd_ln_t *config,
                        acmod_t *acmod,
                        ps_alignment_t *al)
{
    state_align_search_t *sas;
    ps_alignment_iter_t *itor;
    hmm_t *hmm;

    sas = ckd_calloc(1, sizeof(*sas));
    ps_search_init(ps_search_base(sas), &state_align_search_funcs,
                   config, acmod, al->d2p->dict, al->d2p);
    sas->hmmctx = hmm_context_init(bin_mdef_n_emit_state(acmod->mdef),
                                   acmod->tmat->tp, NULL, acmod->mdef->sseq);
    if (sas->hmmctx == NULL) {
        ckd_free(sas);
        return NULL;
    }
    sas->al = al;

    /* Generate HMM vector from phone level of alignment. */
    sas->hmms = ckd_calloc(ps_alignment_n_phones(al), sizeof(*sas->hmms));
    for (hmm = sas->hmms, itor = ps_alignment_phones(al); itor;
         ++hmm, itor = ps_alignment_iter_next(itor)) {
        ps_alignment_entry_t *ent = ps_alignment_iter_get(itor);
        hmm_init(sas->hmmctx, hmm, FALSE,
                 ent->id.pid.ssid, ent->id.pid.cipid);
    }
    return ps_search_base(sas);
}

