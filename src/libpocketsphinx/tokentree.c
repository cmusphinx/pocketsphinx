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

#include <ckd_alloc.h>

#include "tokentree.h"

tokentree_t *
tokentree_init(void)
{
    tokentree_t *ttree = ckd_calloc(1, sizeof(*ttree));
    ttree->token_alloc = listelem_alloc_init(sizeof(token_t));
    ttree->leaves = heap_new();
    return ttree;
}

tokentree_t *
tokentree_retain(tokentree_t *tree)
{
    if (tree == NULL)
        return tree;
    --tree->refcount;
    return tree;
}

int
tokentree_free(tokentree_t *tree)
{
    if (tree == NULL)
        return 0;
    if (--tree->refcount > 0)
        return tree->refcount;
    listelem_alloc_free(tree->token_alloc);
    heap_destroy(tree->leaves);
    ckd_free(tree);
    return 0;
}

token_t *
tokentree_add(tokentree_t *tree, int32 pathcost, int32 arcid, token_t *prev)
{
    token_t *token = listelem_malloc(tree->token_alloc);

    token->pathcost = pathcost;
    token->arcid = arcid;
    token->prev = prev;

    if (prev != NULL) {
        heap_remove(tree->leaves, prev);
    }
    heap_insert(tree->leaves, token, pathcost);

    return token;
}

int
tokentree_prune(tokentree_t *tree, int32 maxcost)
{
    heap_t *newheap = heap_new();
    void *data;
    int32 val;

    while (heap_pop(tree->leaves, &data, &val)) {
        if (val < maxcost)
            heap_insert(newheap, data, val);
    }
    heap_destroy(tree->leaves);
    tree->leaves = newheap;
    return 0;
}

int
tokentree_prune_topn(tokentree_t *tree, int32 n)
{
    heap_t *newheap = heap_new();
    void *data;
    int32 val, i;

    i = 0;
    while (heap_pop(tree->leaves, &data, &val)) {
        if (i == n)
            break;
        heap_insert(newheap, data, val);
        ++i;
    }
    heap_destroy(tree->leaves);
    tree->leaves = newheap;
    return 0;
}

int
tokentree_clear(tokentree_t *tree)
{
    listelem_alloc_free(tree->token_alloc);
    heap_destroy(tree->leaves);
    tree->token_alloc = listelem_alloc_init(sizeof(token_t));
    tree->leaves = heap_new();
    return 0;
}
