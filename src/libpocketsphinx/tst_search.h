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
 * @author David Huggins-Daines and Krisztian Loki
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
#include "lextree.h"
#include "vithist.h"
#include "hmm.h"

/**
 *  \struct histprune_t 
 * \brief Structure containing various histogram pruning parameters and internal storage  All in integers.
 * 
 */

typedef struct {
    int32 maxwpf;          /**< Max words per frame*/
    int32 maxhistpf;       /**< Max histories per frame*/
    int32 maxhmmpf;        /**< Max active HMMs per frame*/
    int32 hmm_hist_binsize;/**< Hmm histogram bin size */
    int32 hmm_hist_bins;   /**< Number of histogram bins*/
    int32 *hmm_hist;	   /**< Histogram: #frames in which a given no. of HMMs are active */
} histprune_t;

/** 
 *  \struct beam_t fast_algo_struct.h "fast_algo_struct.h"
 *  \brief Structure that contains all beam parameters for beam pruning in Viterbi algorithm. 
 *  
 *  This function include the definition of beam in multiple level of pruning in Viterbi
 *  algorithm.  That includes hmm (state-level), ptrans (phone-level), word (word-level). 
 *  ptranskip is used to specify how often in the Viterbi algorithm that phoneme level word 
 *  beam will be replaced by a word-level beam. 
 */

/**
 * Structure containing various beamwidth parameters.  All logs3 values; -infinite is widest,
 * 0 is narrowest.
 */

typedef struct {
    int32 hmm;		   /**< For selecting active HMMs, relative to best */
    int32 ptrans;	   /**< For determining which HMMs transition to their successors */
    int32 word;		   /**< For selecting words exited, relative to best HMM score */
    int32 ptranskip;       /**< Intervals at which wbeam is used for phone transitions */
    int32 wordend;         /**< For selecting the number of word ends  */
    int32 n_ciphone;       /**< No. of ci phone used to initialized the word best and exits list*/
    
    int32 bestscore;	   /**< Temporary variable: Best HMM state score in current frame */
    int32 bestwordscore;   /**< Temporary variable: Best wordexit HMM state score in current frame. */
    int32 thres;           /**< Temporary variable: The current frame general threshold */
    int32 phone_thres;     /**< Temporary variable: The current frame phone threshold */
    int32 word_thres;      /**< Temporary variable: The current frame phone threshold */

    int32 *wordbestscores; /**< The word best score list */
    int32 *wordbestexits;  /**< The word best exits list */

} beam_t;

/**
 * Time-switch tree search module structure.
 */
struct tst_search_s {
    ps_search_t base;
    ngram_model_t *lmset;  /**< Set of language models. */
    s3dict_t *dict;        /**< Sphinx3 dictionary. */
    dict2pid_t *dict2pid;  /**< Sphinx3 cross-word phone mapping. */
    fillpen_t *fillpen;    /**< Sphinx3 filler penalty structure. */

    /**
     * There can be several unigram lextrees.  If we're at the end of frame f, we can only
     * transition into the roots of lextree[(f+1) % n_lextree]; same for fillertree[].  This
     * alleviates the problem of excessive Viterbi pruning in lextrees.
     */

    int32 n_lextree;        /**< Number of lexical tree for time switching: n_lextree */
    lextree_t **curugtree;  /**< The current unigram tree that used in the search for this utterance. */

    hash_table_t *ugtree;  /**< The pool of trees that stores all word trees. */
    lextree_t **fillertree; /**< The pool of trees that stores all filler trees. */
    int32 n_lextrans;	    /**< #Transitions to lextree (root) made so far */
    int32 epl;              /**< The number of entry per lexical tree */

    int32 isLMLA;  /**< Is LM lookahead used?*/

    histprune_t *histprune; /**< Structure that wraps up parameters related to absolute pruning. */
    beam_t *beam;           /**< Structure that wraps up beam pruning parameters. */
    vithist_t *vithist;     /**< Viterbi history (backpointer) table */

    bitvec_t *ssid_active;    /**< Bitvector of active senone sequences. */
    bitvec_t *comssid_active; /**< Bitvector of active composite senone sequences. */
    int16 *composite_senone_scores; /**< Composite senone scores. */

    FILE *hmmdumpfp; /**< File for dumping HMM debugging information. */
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
void tst_search_free(ps_search_t *search);


#endif /* __TST_SEARCH_H__ */
