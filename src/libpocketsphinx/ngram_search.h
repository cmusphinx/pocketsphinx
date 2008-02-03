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
 * @file ngram_search.h N-Gram based multi-pass search ("FBS")
 */

#ifndef __NGRAM_SEARCH_H__
#define __NGRAM_SEARCH_H__

/* SphinxBase headers. */
#include <cmd_ln.h>
#include <logmath.h>
#include <ngram_model.h>

/* Local headers. */
#include "hmm.h"
#include "dict.h"

/**
 * Lexical tree node data type.
 *
 * Not the first HMM for words, which multiplex HMMs based on
 * different left contexts.  This structure is used both in the
 * dynamic HMM tree structure and in the per-word last-phone right
 * context fanout.
 */
typedef struct chan_s {
    hmm_t hmm;                  /**< Basic HMM structure.  This *must* be first in
                                   the structure because chan_t and root_chan_t are
                                   sometimes used interchangeably */
    struct chan_s *next;	/**< first descendant of this channel; or, in the
				   case of the last phone of a word, the next
				   alternative right context channel */
    struct chan_s *alt;		/**< sibling; i.e., next descendant of parent HMM */

    int32    ciphone;		/**< ciphone for this node */
    union {
	int32 penult_phn_wid;	/**< list of words whose last phone follows this one;
				   this field indicates the first of the list; the
				   rest must be built up in a separate array.  Used
				   only within HMM tree.  -1 if none */
	int32 rc_id;		/**< right-context id for last phone of words */
    } info;
} chan_t;

/**
 * Lexical tree node data type for the first phone (root) of each dynamic HMM tree
 * structure.
 *
 * Each state may have a different parent static HMM.  Most fields are
 * similar to those in chan_t.
 */
typedef struct root_chan_s {
    hmm_t hmm;                  /**< Basic HMM structure.  This *must* be first in
                                   the structure because chan_t and root_chan_t are
                                   sometimes used interchangeably. */
    chan_t *next;		/**< first descendant of this channel */

    int32  penult_phn_wid;
    int32  this_phn_wid;	/**< list of words consisting of this single phone;
				   actually the first of the list, like penult_phn_wid;
				   -1 if none */
    int32    diphone;		/**< first diphone of this node; all words rooted at this
				   node begin with this diphone */
    int32    ciphone;		/**< first ciphone of this node; all words rooted at this
				   node begin with this ciphone */
} root_chan_t;

/**
 * Back pointer table (forward pass lattice; actually a tree)
 */
typedef struct bptbl_s {
    int16    frame;		/**< start or end frame */
    uint8    valid;		/**< For absolute pruning */
    uint8    reserved;          /**< Not used */
    int32    wid;		/**< Word index */
    int32    bp;		/**< Back Pointer */
    int32    score;		/**< Score (best among all right contexts) */
    int32    s_idx;		/**< Start of BScoreStack for various right contexts*/
    int32    real_wid;		/**< wid of this or latest predecessor real word */
    int32    prev_real_wid;	/**< real word predecessor of real_wid */
    int32    r_diph;		/**< rightmost diphone of this word */
    int32    ascr;
    int32    lscr;
} bptbl_t;

#define NO_BP		-1

/**
 * Links between DAG nodes (see latnode_t below).
 * Also used to keep scores in a bestpath search.
 */
typedef struct latlink_s {
    struct latnode_s *from;	/**< From node */
    struct latnode_s *to;	/**< To node */
    struct latlink_s *next;	/**< Next link from the same "from" node */
    struct latlink_s *best_prev;
    struct latlink_s *q_next;
    int32 link_scr;		/**< Score for from->wid (from->sf to this->ef) */
    int32 path_scr;		/**< Best path score from root of DAG */
    int32 ef;			/**< end-frame for the "from" node */
} latlink_t;

typedef struct rev_latlink_s {
    latlink_t *link;
    struct rev_latlink_s *next;
} rev_latlink_t;

/**
 * DAG nodes.
 */
typedef struct latnode_s {
    int32 wid;			/**< Dictionary word id */
    int32 fef;			/**< First end frame */
    int32 lef;			/**< Last end frame */
    int16 sf;			/**< Start frame */
    int16 reachable;		/**< From </s> or <s> */
    union {
	int32 fanin;		/**< #nodes with links to this node */
	int32 rem_score;	/**< Estimated best score from node.sf to end */
    } info;
    latlink_t *links;		/**< Links out of this node */
    rev_latlink_t *revlinks;	/**< Reverse links (for housekeeping purposes only) */
    struct latnode_s *next;	/**< Next node (for housekeeping purposes only) */
} latnode_t;

/**
 * N-Gram search module structure.
 */
struct ngram_search_s {
    ngram_model_t *lmset; /**< Set of language models. */
};
typedef struct ngram_search_s ngram_search_t;

/**
 * Initialize the N-Gram search module.
 */
ngram_search_t *ngram_search_init(cmd_ln_t *config,
                                  logmath_t *lmath,
                                  dict_t *dict);

/**
 * Finalize the N-Gram search module.
 */
void ngram_search_free(ngram_search_t *ngs);

#endif /* __NGRAM_SEARCH_H__ */
