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
 * @file state_align_search.h State (and phone and word) alignment search.
 */

#ifndef __STATE_ALIGN_SEARCH_H__
#define __STATE_ALIGN_SEARCH_H__

#include <pocketsphinx.h>
#include "pocketsphinx_internal.h"
#include "ps_alignment_internal.h"
#include "hmm.h"

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

/**
 * History structure
 */
struct state_align_hist_s {
    int32 id;
    int32 score;
};
typedef struct state_align_hist_s state_align_hist_t;

/**
 * Forced alignment search structure.
 */
struct state_align_search_s {
    ps_search_t base;       /**< Base search structure. */
    hmm_context_t *hmmctx;  /**< HMM context structure. */
    ps_alignment_t *al;     /**< Alignment structure being operated on. */
    hmm_t *hmms;            /**< Vector of HMMs corresponding to phone level. */
    int *sf;                /**< Vector of minimum start frames for HMMs. */
    int *ef;                /**< Vector of maximum exit frames for HMMs.
                                 (note that exit frame = end frame + 1) */
    int n_phones;	    /**< Number of HMMs (phones). */

    int frame;              /**< Current frame being processed. */
    int32 best_score;       /**< Best score in current frame. */

    int n_emit_state;       /**< Number of emitting states (tokens per frame) */
    state_align_hist_t *tokens;         /**< Tokens (backpointers) for state alignment. */
    int n_fr_alloc;         /**< Number of frames of tokens allocated. */
};
typedef struct state_align_search_s state_align_search_t;

ps_search_t *state_align_search_init(const char *name,
				     cmd_ln_t *config,
                                     acmod_t *acmod,
                                     ps_alignment_t *al);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __STATE_ALIGN_SEARCH_H__ */
