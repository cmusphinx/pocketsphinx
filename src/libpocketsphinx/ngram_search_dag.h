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
typedef struct ngram_dag_s {
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
    listelem_alloc_t *latpath_alloc;     /**< Path allocator for this DAG. */
} ngram_dag_t;

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
 * @return 0 for success, <0 on error.
 */
int ngram_dag_start_nbest(ngram_dag_t *dag);

/**
 * Find next best hypothesis of N-best on a word graph.
 *
 * @return a complete path, or NULL if no more hypotheses exist.
 */

latpath_t *ngram_dag_next_best(ngram_dag_t *dag);

#endif /* __NGRAM_SEARCH_DAG_H__ */
