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
 * @file glextree.h
 * @brief Generic lexicon trees for token-fsg search (and maybe elsewhere)
 * @author David Huggins-Daines <dhuggins@cs.cmu.edu>
 */

#ifndef __GLEXTREE_H__
#define __GLEXTREE_H__

/* System includes. */

/* SphinxBase includes. */
#include <listelem_alloc.h>

/* Local includes. */
#include "hmm.h"
#include "dict2pid.h"
#include "dict.h"

/**
 * Generic lexicon tree node structure.
 */
typedef struct glexnode_s glexnode_t;
struct glexnode_s {
    hmm_t hmm;         /**< Base HMM structure. */
    glexnode_t *sibs;  /**< Siblings of this node. */
    glexnode_t *kids;  /**< Children of this node, NULL for leaves. */
    int32 rc;          /**< Right context information (for leaf nodes). */
};

/**
 * Lexicon tree root node structure.
 *
 * Root nodes are organized by right context and base phone, with all
 * roots sharing the same (lc, ci) pair contiguous.  To make it easier
 * to dynamically add words, they are stored in a trie.  Although the
 * code currently assumes three levels, extending this to 5-phones or
 * 7-phones or some more exotic configuration shouldn't be difficult.
 */
typedef struct glexroot_s glexroot_t;
struct glexroot_s {
    int16 phone;      /**< Phone for this level of the trie (lc, ci, rc). */
    int16 n_down;     /**< Number of children (0 for a leafnode). */
    glexroot_t *sibs; /**< Next tree entry at this level. */
    union {
        glexroot_t *kids; /**< First tree entry at the next level. */
        glexnode_t *node; /**< Actual root node. */
    } down;
};

/**
 * Generic lexicon tree structure.
 */
typedef struct glextree_s glextree_t;
struct glextree_s {
    hmm_context_t *ctx;  /**< HMM context for this tree. */
    dict_t *dict;        /**< Dictionary used to build this tree. */
    dict2pid_t *d2p;     /**< Context-dependent mappings used. */
    int refcount;        /**< Reference count. */
    glexroot_t root;     /**< Root of trie holding root nodes. */
    listelem_alloc_t *root_alloc;
    listelem_alloc_t *node_alloc;
};

/**
 * Filter for dictionary words when building the lextree.
 *
 * Frequently we wish to build the lexicon tree over only a subset of
 * the dictionary, such as that part which intersects with a language
 * model or grammar.  In this case you can provide a function of this
 * type, which returns TRUE for words to be included and FALSE for
 * words to be skipped.
 */
typedef int (*glextree_wfilter_t)(glextree_t *tree, int32 wid,
				  void *udata);

/**
 * Build a lexicon tree from a dictionary and context mapping.
 */
glextree_t *glextree_build(hmm_context_t *ctx, dict_t *dict, dict2pid_t *d2p,
			   glextree_wfilter_t filter, void *udata);

/**
 * Add a word to the lexicon tree.
 *
 * Presumes that it's already been added to the dictionary and
 * dict2pid.
 */
void glextree_add_word(glextree_t *tree, int32 wid);

/**
 * Retain a pointer to a lexicon tree.
 */
glextree_t *glextree_retain(glextree_t *tree);

/**
 * Release a pointer to a lexicon tree.
 */
int glextree_free(glextree_t *tree);

#endif /* __GLEXTREE_H__ */
