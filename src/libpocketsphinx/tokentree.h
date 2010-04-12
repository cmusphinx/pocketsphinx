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
 * @name tokentree.h
 * @brief Token-passing search algorithm
 * @author David Huggins-Daines <dhuggins@cs.cmu.edu>
 */

#ifndef __TOKENTREE_H__
#define __TOKENTREE_H__

/* System includes. */

/* SphinxBase includes. */
#include <listelem_alloc.h>
#include <heap.h>

/* Local includes. */

/**
 * Token, which represents a particular path through the decoding graph.
 */
typedef struct token_s token_t;
struct token_s {
    int32 pathcost ;  /**< Cost (-score) of the path ending with this token. */
    int32 arcid;      /**< Head arc (or word) represented by this token. */
    int16 frame;      /**< Last frame in which this token was active. */
    int16 misc;       /**< Miscellaneous data. */
    token_t *prev;    /**< Previous token in this path. */
};

/**
 * Tree (lattice) of tokens, representing all paths explored so far.
 */
typedef struct tokentree_s tokentree_t;
struct tokentree_s {
    int refcount;
    heap_t *leaves;                 /**< Leaves (active tokens). */
    listelem_alloc_t *token_alloc;  /**< Allocator for tokens. */
};

/**
 * Create a new token tree.
 */
tokentree_t *tokentree_init(void);

/**
 * Retain a pointer to a token tree.
 */
tokentree_t *tokentree_retain(tokentree_t *tree);

/**
 * Release a pointer to a token tree.
 */
int tokentree_free(tokentree_t *tree);

/**
 * Create a new token and add it to the tree as a leaf.
 *
 * This has the side effect of removing prev from the set of leaves.
 */
token_t *tokentree_add(tokentree_t *tree, int32 pathcost,
                       int32 arcid, token_t *prev);

/**
 * Prune leaves from the tree with costs greather than maximum.
 */
int tokentree_prune(tokentree_t *tree, int32 maxcost);

/**
 * Prune leaves from the tree leaving only the top N.
 */
int tokentree_prune_topn(tokentree_t *tree, int32 n);

/**
 * Reset tokentree, removing all history.
 */
int tokentree_clear(tokentree_t *tree);

#endif /* __TOKENTREE_H__ */
