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
 * @file ngram_search_dag.h Word graph search
 */

#ifndef __NGRAM_SEARCH_DAG_H__
#define __NGRAM_SEARCH_DAG_H__

/* SphinxBase headers. */

/* Local headers. */
#include "ngram_search.h"

/**
 * Word graph structure used in bestpath/nbest search.
 */
struct ngram_dag_s {
    ngram_search_t *ngs; /**< Search object which produced this DAG. */

    latnode_t *nodes;  /**< List of all nodes. */
    latnode_t *start;  /**< Starting node. */
    latnode_t *end;    /**< Ending node. */

    int32 n_frames;    /**< Number of frames for this utterance. */
    int32 final_node_ascr;
    char *hyp_str;     /**< Current hypothesis string. */

    listelem_alloc_t *latnode_alloc;     /**< Node allocator for this DAG. */
    listelem_alloc_t *latlink_alloc;     /**< Link allocator for this DAG. */
    listelem_alloc_t *rev_latlink_alloc; /**< Reverse link allocator for this DAG. */
};

/**
 * Partial path structure used in N-best (A*) search.
 *
 * Each partial path (latpath_t) is constructed by extending another
 * partial path--parent--by one node.
 */
typedef struct latpath_s {
    latnode_t *node;            /**< Node ending this path. */
    struct latpath_s *parent;   /**< Previous element in this path. */
    struct latpath_s *next;     /**< Pointer to next path in list of paths. */
    int32 score;                /**< Exact score from start node up to node->sf. */
} latpath_t;

/**
 * N-best search structure.
 */
typedef struct ngram_nbest_s {
    ngram_dag_t *dag;
    int16 sf;
    int16 ef;
    int32 w1;
    int32 w2;

    int32 n_hyp_tried;
    int32 n_hyp_insert;
    int32 n_hyp_reject;
    int32 insert_depth;
    int32 n_path;

    latpath_t *path_list;
    latpath_t *path_tail;
    latpath_t *paths_done;

    glist_t hyps;	             /**< List of hypothesis strings. */
    listelem_alloc_t *latpath_alloc; /**< Path allocator for N-best search. */
} ngram_nbest_t;

/**
 * Construct a word graph from the current hypothesis.
 */
ngram_dag_t *ngram_dag_build(ngram_search_t *ngs);

/**
 * Destruct a word graph.
 */
void ngram_dag_free(ngram_dag_t *dag);

/**
 * Write a lattice to disk.
 */
int32 ngram_dag_write(ngram_dag_t *dag, char *filename);

/**
 * Do best-path search on a word graph.
 *
 * @return First link in best path, NULL on error.
 */
latlink_t *ngram_dag_bestpath(ngram_dag_t *dag);

/**
 * Get hypothesis string after bestpath search.
 */
char const *ngram_dag_hyp(ngram_dag_t *dag, latlink_t *link);

/**
 * Begin N-best search on a word graph.
 *
 * @param sf Starting frame for N-best search.
 * @param ef Ending frame for N-best search, or -1 for last frame.
 * @param w1 First context word, or -1 for none.
 * @param w2 Second context word, or -1 for none.
 * @return 0 for success, <0 on error.
 */
ngram_nbest_t *ngram_nbest_start(ngram_dag_t *dag,
                                 int sf, int ef,
                                 int w1, int w2);


/**
 * Find next best hypothesis of N-best on a word graph.
 *
 * @return a complete path, or NULL if no more hypotheses exist.
 */
latpath_t *ngram_nbest_next(ngram_nbest_t *nbest);

/**
 * Get hypothesis string from N-best search.
 */
char const *ngram_nbest_hyp(ngram_nbest_t *nbest, latpath_t *path);

/**
 * Finish N-best search, releasing resources associated with it.
 */
void ngram_nbest_finish(ngram_nbest_t *nbest);

#endif /* __NGRAM_SEARCH_DAG_H__ */
