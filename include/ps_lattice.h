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
 * @file ps_lattice.h Word graph search
 */

#ifndef __PS_LATTICE_H__
#define __PS_LATTICE_H__

/* SphinxBase headers. */
#include <prim_type.h>
#include <ngram_model.h>

/* PocketSphinx headers. */
#include <pocketsphinx_export.h>

/**
 * Word graph structure used in bestpath/nbest search.
 */
typedef struct ps_lattice_s ps_lattice_t;

/**
 * A* search structure (an iterator over N-best results).
 */
typedef struct ps_astar_s ps_astar_t;

/**
 * DAG nodes.
 *
 * A node corresponds to a number of hypothesized instances of a word
 * which all share the same starting point.
 */
typedef struct ps_latnode_s ps_latnode_t;

/**
 * Links between DAG nodes.
 *
 * A link corresponds to a single hypothesized instance of a word with
 * a given start and end point.
 */
typedef struct ps_latlink_s ps_latlink_t;

/**
 * Partial path structure used in N-best (A*) search.
 *
 * Each partial path (latpath_t) is constructed by extending another
 * partial path--parent--by one node.
 */
typedef struct ps_latpath_s ps_latpath_t;

/**
 * Retain a lattice.
 *
 * This function retains ownership of a lattice for the caller,
 * preventing it from being freed automatically.  You must call
 * ps_lattice_free() to free it after having called this function.
 *
 * @return pointer to the retained lattice.
 */
POCKETSPHINX_EXPORT
ps_lattice_t *ps_lattice_retain(ps_lattice_t *dag);

/**
 * Free a lattice.
 *
 * @return new reference count (0 if dag was freed)
 */
POCKETSPHINX_EXPORT
int ps_lattice_free(ps_lattice_t *dag);

/**
 * Write a lattice to disk.
 */
POCKETSPHINX_EXPORT
int32 ps_lattice_write(ps_lattice_t *dag, char const *filename);

/**
 * Create a directed link between "from" and "to" nodes, but if a link already exists,
 * choose one with the best link_scr.
 */
POCKETSPHINX_EXPORT
void ps_lattice_link(ps_lattice_t *dag, ps_latnode_t *from, ps_latnode_t *to,
                     int32 score, int32 ef);

/**
 * Bypass filler words.
 */
POCKETSPHINX_EXPORT
void ps_lattice_bypass_fillers(ps_lattice_t *dag, int32 silpen, int32 fillpen);

/**
 * Remove nodes marked as unreachable.
 */
POCKETSPHINX_EXPORT
void ps_lattice_delete_unreachable(ps_lattice_t *dag);

/**
 * Add an edge to the traversal queue.
 */
POCKETSPHINX_EXPORT
void ps_lattice_pushq(ps_lattice_t *dag, ps_latlink_t *link);

/**
 * Remove an edge from the traversal queue.
 */
POCKETSPHINX_EXPORT
ps_latlink_t *ps_lattice_popq(ps_lattice_t *dag);

/**
 * Clear and reset the traversal queue.
 */
POCKETSPHINX_EXPORT
void ps_lattice_delq(ps_lattice_t *dag);

/**
 * Start a forward traversal of edges in a word graph.
 */
POCKETSPHINX_EXPORT
ps_latlink_t *ps_lattice_traverse_edges(ps_lattice_t *dag, ps_latnode_t *start, ps_latnode_t *end);

/**
 * Get the next link in forward traversal.
 */
POCKETSPHINX_EXPORT
ps_latlink_t *ps_lattice_traverse_next(ps_lattice_t *dag, ps_latnode_t *end);

/**
 * Start a reverse traversal of edges in a word graph.
 */
POCKETSPHINX_EXPORT
ps_latlink_t *ps_lattice_reverse_edges(ps_lattice_t *dag, ps_latnode_t *start, ps_latnode_t *end);

/**
 * Get the next link in forward traversal.
 */
POCKETSPHINX_EXPORT
ps_latlink_t *ps_lattice_reverse_next(ps_lattice_t *dag, ps_latnode_t *start);

/**
 * Do N-Gram based best-path search on a word graph.
 *
 * This function calculates both the best path as well as the forward
 * probability used in confidence estimation.
 *
 * @return Final link in best path, NULL on error.
 */
POCKETSPHINX_EXPORT
ps_latlink_t *ps_lattice_bestpath(ps_lattice_t *dag, ngram_model_t *lmset,
                               float32 lwf, float32 ascale);

/**
 * Calculate link posterior probabilities on a word graph.
 *
 * This function assumes that bestpath search has already been done.
 *
 * @return Posterior probability of the utterance as a whole.
 */
POCKETSPHINX_EXPORT
int32 ps_lattice_posterior(ps_lattice_t *dag, ngram_model_t *lmset,
                           float32 ascale);

/**
 * Begin N-Gram based A* search on a word graph.
 *
 * @param sf Starting frame for N-best search.
 * @param ef Ending frame for N-best search, or -1 for last frame.
 * @param w1 First context word, or -1 for none.
 * @param w2 Second context word, or -1 for none.
 * @return 0 for success, <0 on error.
 */
POCKETSPHINX_EXPORT
ps_astar_t *ps_astar_start(ps_lattice_t *dag,
                           ngram_model_t *lmset,
                           float32 lwf,
                           int sf, int ef,
                           int w1, int w2);

/**
 * Find next best hypothesis of A* on a word graph.
 *
 * @return a complete path, or NULL if no more hypotheses exist.
 */
POCKETSPHINX_EXPORT
ps_latpath_t *ps_astar_next(ps_astar_t *nbest);

/**
 * Finish N-best search, releasing resources associated with it.
 */
POCKETSPHINX_EXPORT
void ps_astar_finish(ps_astar_t *nbest);

#endif /* __PS_LATTICE_H__ */
