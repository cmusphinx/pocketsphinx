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
 * @file phone_loop_search.h Fast and rough context-independent phoneme loop search.
 */

#include <err.h>

#include "phone_loop_search.h"

static int phone_loop_search_start(ps_search_t *search);
static int phone_loop_search_step(ps_search_t *search);
static int phone_loop_search_finish(ps_search_t *search);
static int phone_loop_search_reinit(ps_search_t *search);
static char const *phone_loop_search_hyp(ps_search_t *search, int32 *out_score);
static int32 phone_loop_search_prob(ps_search_t *search);
static ps_seg_t *phone_loop_search_seg_iter(ps_search_t *search, int32 *out_score);

static ps_searchfuncs_t phone_loop_search_funcs = {
    /* name: */   "phone_loop",
    /* start: */  phone_loop_search_start,
    /* step: */   phone_loop_search_step,
    /* finish: */ phone_loop_search_finish,
    /* reinit: */ phone_loop_search_reinit,
    /* free: */   phone_loop_search_free,
    /* lattice: */  NULL,
    /* hyp: */      phone_loop_search_hyp,
    /* prob: */     phone_loop_search_prob,
    /* seg_iter: */ phone_loop_search_seg_iter,
};

static int
phone_loop_search_reinit(ps_search_t *search)
{
    phone_loop_search_t *pls = (phone_loop_search_t *)search;
    acmod_t *acmod = ps_search_acmod(search);
    int i;

    /* Initialize HMM context. */
    if (pls->hmmctx)
        hmm_context_free(pls->hmmctx);
    pls->hmmctx = hmm_context_init(bin_mdef_n_emit_state(acmod->mdef),
                                   acmod->tmat->tp, NULL, acmod->mdef->sseq);

    /* Initialize phone HMMs. */
    if (pls->phones) {
        for (i = 0; i < pls->n_phones; ++i)
            hmm_deinit((hmm_t *)&pls->phones[i]);
        ckd_free(pls->phones);
    }
    pls->n_phones = bin_mdef_n_ciphone(acmod->mdef);
    pls->phones = ckd_calloc(pls->n_phones, sizeof(*pls->phones));
    for (i = 0; i < pls->n_phones; ++i) {
        pls->phones[i].ciphone = i;
        hmm_init(pls->hmmctx, (hmm_t *)&pls->phones[i],
                 FALSE,
                 bin_mdef_pid2ssid(acmod->mdef, i),
                 bin_mdef_pid2tmatid(acmod->mdef, i));
        /* All CI senones are active all the time. */
        acmod_activate_hmm(acmod, (hmm_t *)&pls->phones[i]);
    }

    return 0;
}

ps_search_t *
phone_loop_search_init(cmd_ln_t *config,
		       acmod_t *acmod,
		       dict_t *dict)
{
    phone_loop_search_t *pls;

    /* Allocate and initialize. */
    pls = ckd_calloc(1, sizeof(*pls));
    ps_search_init(ps_search_base(pls), &phone_loop_search_funcs,
                   config, acmod, dict);
    phone_loop_search_reinit(ps_search_base(pls));

    return ps_search_base(pls);
}

void
phone_loop_search_free(ps_search_t *search)
{
    phone_loop_search_t *pls = (phone_loop_search_t *)search;
    int i;

    ps_search_deinit(search);
    for (i = 0; i < pls->n_phones; ++i)
        hmm_deinit((hmm_t *)&pls->phones[i]);
    ckd_free(pls->phones);
    hmm_context_free(pls->hmmctx);
    ckd_free(pls);
}

static int
phone_loop_search_start(ps_search_t *search)
{
    phone_loop_search_t *pls = (phone_loop_search_t *)search;
    int i;

    /* Reset scores and backpointers in phone HMMs. */
    for (i = 0; i < pls->n_phones; ++i) {
        hmm_clear((hmm_t *)&pls->phones[i]);
        pls->phones[i].frame = -1;
    }

    return 0;
}

static int
phone_loop_search_step(ps_search_t *search)
{
    phone_loop_search_t *pls = (phone_loop_search_t *)search;
    acmod_t *acmod = ps_search_acmod(search);

    /* Calculate senone scores for current frame. */
    /* senscr = acmod_score(acmod, &frame_idx); */

    /* Evaluate phone HMMs for current frame. */

    /* Prune phone HMMs. */

    /* Do phone transitions. */

    /* Renormalize? */

    return -1;
}

static int
phone_loop_search_finish(ps_search_t *search)
{
    /* Actually nothing to do here really. */
    return 0;
}

static char const *
phone_loop_search_hyp(ps_search_t *search, int32 *out_score)
{
    E_WARN("Hypotheses are not returned from phone loop search");
    return NULL;
}

static int32
phone_loop_search_prob(ps_search_t *search)
{
    /* FIXME: Actually... they ought to be. */
    E_WARN("Posterior probabilities are not returned from phone loop search");
    return 0;
}

static ps_seg_t *
phone_loop_search_seg_iter(ps_search_t *search, int32 *out_score)
{
    E_WARN("Hypotheses are not returned from phone loop search");
    return NULL;
}
