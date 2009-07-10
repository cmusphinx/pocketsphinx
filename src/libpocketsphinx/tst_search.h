/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 2009 Carnegie Mellon University.  All rights
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
 * @file tst_search.h Time-conditioned lexicon tree search ("S3")
 */

#ifndef __TST_SEARCH_H__
#define __TST_SEARCH_H__

/* SphinxBase headers. */
#include <cmd_ln.h>
#include <logmath.h>
#include <ngram_model.h>
#include <listelem_alloc.h>
#include <err.h>

/* Local headers. */
#include "pocketsphinx_internal.h"
/* #include "lextree.h" */
#include "hmm.h"

/* Forward-declare these for the time being so things will compile. */
typedef struct lextree_s lextree_t;
typedef struct vithist_s vithist_t;
typedef struct histprune_s histprune_t;

/**
 * Time-switch tree search module structure.
 */
struct tst_search_s {
    ps_search_t base;
    ngram_model_t *lmset;  /**< Set of language models. */
    hmm_context_t *hmmctx; /**< HMM context. */

    /**
     * There can be several unigram lextrees.  If we're at the end of frame f, we can only
     * transition into the roots of lextree[(f+1) % n_lextree]; same for fillertree[].  This
     * alleviates the problem of excessive Viterbi pruning in lextrees.
     */

    int32 n_lextree;        /**< Number of lexical tree for time switching: n_lextree */
    lextree_t **curugtree;  /**< The current unigram tree that used in the search for this utterance. */

    lextree_t **ugtree;     /**< The pool of trees that stores all word trees. */
    lextree_t **fillertree; /**< The pool of trees that stores all filler trees. */
    int32 n_lextrans;	    /**< #Transitions to lextree (root) made so far */
    int32 epl;              /**< The number of entry per lexical tree */

    histprune_t *histprune; /**< Structure that wraps up parameters related to  */
  
    vithist_t *vithist;     /**< Viterbi history (backpointer) table */
};
typedef struct tst_search_s tst_search_t;

/**
 * Initialize the N-Gram search module.
 */
ps_search_t *tst_search_init(cmd_ln_t *config,
                             acmod_t *acmod,
                             dict_t *dict);

/**
 * Finalize the N-Gram search module.
 */
void tst_search_free(ps_search_t *ngs);


#endif /* __TST_SEARCH_H__ */
