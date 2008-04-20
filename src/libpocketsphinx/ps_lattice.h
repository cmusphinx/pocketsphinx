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

/* Local headers. */
#include "pocketsphinx_internal.h"

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
    int32 basewid;		/**< Dictionary base word id */
    int16 fef;			/**< First end frame */
    int16 lef;			/**< Last end frame */
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
 * Word graph structure used in bestpath/nbest search.
 */
struct ps_lattice_s {
    ps_search_t *search; /**< Search object which produced this DAG. */

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
 * Segmentation "iterator" for backpointer table results.
 */
typedef struct dag_seg_s {
    ps_seg_t base;       /**< Base structure. */
    latlink_t **links;   /**< Sequence of lattice links. */
    int16 n_links;  /**< Number of lattice links. */
    int16 cur;      /**< Current position in bpidx. */
} dag_seg_t;

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
 * A* search structure.
 */
typedef struct ps_astar_s {
    ps_lattice_t *dag;
    ngram_model_t *lmset;
    float32 lwf;

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
} ps_astar_t;

/**
 * Construct an empty word graph.
 */
ps_lattice_t *ps_lattice_init(ps_search_t *search, int n_frame);

/**
 * Destruct a word graph.
 */
void ps_lattice_free(ps_lattice_t *dag);

/**
 * Create a directed link between "from" and "to" nodes, but if a link already exists,
 * choose one with the best link_scr.
 */
void link_latnodes(ps_lattice_t *dag, latnode_t *from, latnode_t *to,
                   int32 score, int32 ef);

/**
 * Add bypass links around filler words.
 */
void ps_lattice_bypass_fillers(ps_lattice_t *dag, int32 silpen, int32 fillpen);

/**
 * Remove nodes not marked as reachable.
 */
void ps_lattice_delete_unreachable(ps_lattice_t *dag);

/**
 * Do N-Gram based best-path search on a word graph.
 *
 * @return Final link in best path, NULL on error.
 */
latlink_t *ps_lattice_bestpath(ps_lattice_t *dag, ngram_model_t *lmset, float32 lwf);

/**
 * Get hypothesis string after bestpath search.
 */
char const *ps_lattice_hyp(ps_lattice_t *dag, latlink_t *link);

/**
 * Get hypothesis segmentation iterator after bestpath search.
 */
ps_seg_t *ps_lattice_iter(ps_lattice_t *dag, latlink_t *link);

/**
 * Begin N-Gram based A* search on a word graph.
 *
 * @param sf Starting frame for N-best search.
 * @param ef Ending frame for N-best search, or -1 for last frame.
 * @param w1 First context word, or -1 for none.
 * @param w2 Second context word, or -1 for none.
 * @return 0 for success, <0 on error.
 */
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
latpath_t *ps_astar_next(ps_astar_t *nbest);

/**
 * Get hypothesis string from A* search.
 */
char const *ps_astar_hyp(ps_astar_t *nbest, latpath_t *path);

/**
 * Finish N-best search, releasing resources associated with it.
 */
void ps_astar_finish(ps_astar_t *nbest);

#endif /* __PS_LATTICE_H__ */
