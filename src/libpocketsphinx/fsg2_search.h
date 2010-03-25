/* -*- c-basic-offset:4; indent-tabs-mode: nil -*- */
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
 * @file fsg2_search.h
 * @brief New FSG search
 * @author David Huggins-Daines <dhuggins@cs.cmu.edu>
 */


#ifndef __FSG2_SEARCH_H__
#define __FSG2_SEARCH_H__


/* SphinxBase headers. */
#include <fsg_model.h>

/* Local headers. */
#include "pocketsphinx_internal.h"
#include "hmm.h"
#include "glextree.h"
#include "tokentree.h"

/**
 * Implementation of FSG search (and "FSG set") structure.
 */
typedef struct fsg2_search_s {
    ps_search_t base;

    hmm_context_t *hmmctx;      /**< HMM context. */

    fsg_model_t *fsg;		/**< Currently active FSG */
    jsgf_t *jsgf;               /**< Active JSGF grammar file. */

    glextree_t *lextree;        /**< Tree lexicon for search */
    tokentree_t *toktree;       /**< Token tree (lattice/history) */
  
    int16 frame;		/**< Current frame. */
    uint8 final;		/**< Decoding is finished for this utterance. */
    uint8 bestpath;		/**< Whether to run bestpath search
                                   and confidence annotation at end. */
} fsg_search_t;


/**
 * Create, initialize and return a search module.
 */
ps_search_t *fsg2_search_init(cmd_ln_t *config,
                              acmod_t *acmod,
                              dict_t *dict,
                              dict2pid_t *d2p);

/**
 * Deallocate search structure.
 */
void fsg2_search_free(ps_search_t *search);

/**
 * Update FSG search module for new or updated FSGs.
 */
int fsg2_search_reinit(ps_search_t *fsgs, dict_t *dict, dict2pid_t *d2p);

/**
 * Prepare the FSG search structure for beginning decoding of the next
 * utterance.
 */
int fsg2_search_start(ps_search_t *search);

/**
 * Step one frame forward through the Viterbi search.
 */
int fsg2_search_step(ps_search_t *search, int frame_idx);

/**
 * Windup and clean the FSG search structure after utterance.
 */
int fsg2_search_finish(ps_search_t *search);

/**
 * Get hypothesis string from the FSG search.
 */
char const *fsg2_search_hyp(ps_search_t *search, int32 *out_score);

#endif
