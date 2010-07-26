/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
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
 * @file alignment.c Multi-level alignment structure
 */

/* System headers. */

/* SphinxBase headers. */
#include <sphinxbase/ckd_alloc.h>

/* Local headers. */
#include "alignment.h"

alignment_t *
alignment_init(dict2pid_t *d2p)
{
    alignment_t *al = ckd_calloc(1, sizeof(*al));
    al->d2p = dict2pid_retain(d2p);
    return al;
}

int
alignment_free(alignment_t *al)
{
    if (al == NULL)
        return 0;
    dict2pid_free(al->d2p);
    ckd_free(al);
    return 0;
}

#define VECTOR_GROW 10
static void *
vector_grow_one(void *ptr, uint16 *n_alloc, uint16 *n, size_t item_size)
{
    int newsize = *n + 1;
    if (newsize < *n_alloc)
        return ptr;
    newsize += VECTOR_GROW;
    if (newsize > 0xffff)
        return NULL;
    ptr = ckd_realloc(ptr, newsize * item_size);
    *n += 1;
    *n_alloc = newsize;
    return ptr;
}

int
alignment_add_word(alignment_t *al,
		   int32 wid, int duration)
{
    void *ptr;

    if ((ptr = vector_grow_one(&al->word, &al->n_word_alloc,
                               &al->n_word, sizeof(*al))) == NULL)
        return 0;
    al->word = (alignment_entry_t *)ptr;
    al->word[al->n_word - 1].id.wid = wid;
    if (al->n_word > 1)
        al->word[al->n_word - 1].start = (al->word[al->n_word - 2].start
                                          + al->word[al->n_word - 2].duration);
    else
        al->word[al->n_word - 1].start = 0;
    al->word[al->n_word - 1].duration = duration;

    return al->n_word;
}

int
alignment_populate(alignment_t *al)
{
    /* For each word, expand to phones/senone sequences. */

    /* For each senone sequence, expand to senones. */
    return 0;
}

int
alignment_propagate(alignment_t *al)
{
    /* For each senone, if it is at a boundary, propagate duration up. */

    /* For each senone sequence, if it is at a boundary, propagate duration up. */
    return 0;
}

alignment_iter_t *
alignment_words(alignment_t *al)
{
    alignment_iter_t *itor = ckd_calloc(1, sizeof(*itor));
    return itor;
}

alignment_iter_t *
alignment_phones(alignment_t *al)
{
    alignment_iter_t *itor = ckd_calloc(1, sizeof(*itor));
    return itor;
}

alignment_iter_t *
alignment_states(alignment_t *al)
{
    alignment_iter_t *itor = ckd_calloc(1, sizeof(*itor));
    return itor;
}

alignment_iter_t *
alignment_iter_next(alignment_iter_t *itor)
{
    return itor;
}

alignment_iter_t *
alignment_iter_prev(alignment_iter_t *itor)
{
    return itor;
}

alignment_iter_t *
alignment_iter_up(alignment_iter_t *itor)
{
    return itor;
}

alignment_iter_t *
alignment_iter_down(alignment_iter_t *itor)
{
    return itor;
}
