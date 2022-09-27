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

#ifndef __PS_ALIGNMENT_INTERNAL_H__
#define __PS_ALIGNMENT_INTERNAL_H__

#include <pocketsphinx.h>

#include "dict2pid.h"
#include "hmm.h"

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

/**
 * @struct ps_alignment_entry_t
 * @brief Entry (phone, word, or state) in an alignment
 */
typedef struct ps_alignment_entry_s {
    int32 start;  /**< Start frame index. */
    int32 duration; /**< Duration in frames. */
    int32 score;  /**< Alignment score (fairly meaningless). */
    /**
     * Index of parent node.
     *
     * You can use this to determine if you have crossed a parent
     * boundary.  For example if you wish to iterate only over phones
     * inside a word, you can store this for the first phone and stop
     * iterating once it changes. */
    int parent;
    int child;  /**< Index of child node. */
    /**
     * ID or IDs for this entry.
     *
     * This is complex, though perhaps not needlessly so.  We need all
     * this information to do state alignment.
     */
    union {
        int32 wid;  /**< Word ID (for words) */
        struct {
            int16 cipid;  /**< Phone ID, which you care about. */
            uint16 ssid;  /**< Senone sequence ID, which you don't. */
            int32 tmatid; /**< Transition matrix ID, almost certainly
                             the same as cipid. */
        } pid;
        uint16 senid;
    } id;
} ps_alignment_entry_t;

struct ps_alignment_vector_s {
    ps_alignment_entry_t *seq;
    uint16 n_ent, n_alloc;
};
typedef struct ps_alignment_vector_s ps_alignment_vector_t;

struct ps_alignment_s {
    int refcount;
    dict2pid_t *d2p;
    ps_alignment_vector_t word;
    ps_alignment_vector_t sseq;
    ps_alignment_vector_t state;
};

struct ps_alignment_iter_s {
    ps_alignment_t *al;
    ps_alignment_vector_t *vec;
    int pos;
    int parent;
    char *name;
};

#define ps_alignment_n_words(al) (int)(al)->word.n_ent
#define ps_alignment_n_phones(al) (int)(al)->sseq.n_ent
#define ps_alignment_n_states(al) (int)(al)->state.n_ent

/**
 * Create a new, empty alignment.
 */
ps_alignment_t *ps_alignment_init(dict2pid_t *d2p);

/**
 * Append a word.
 */
int ps_alignment_add_word(ps_alignment_t *al,
                          int32 wid, int start, int duration);

/**
 * Populate lower layers using available word information.
 */
int ps_alignment_populate(ps_alignment_t *al);

/**
 * Populate lower layers using context-independent phones.
 */
int ps_alignment_populate_ci(ps_alignment_t *al);

/**
 * Propagate timing information up from state sequence.
 */
int ps_alignment_propagate(ps_alignment_t *al);

/**
 * Get the alignment entry pointed to by an iterator.
 *
 * The iterator retains ownership of this so don't try to free it.
 */
ps_alignment_entry_t *ps_alignment_iter_get(ps_alignment_iter_t *itor);

/**
 * Move alignment iterator to given index.
 */
ps_alignment_iter_t *ps_alignment_iter_goto(ps_alignment_iter_t *itor, int pos);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __PS_ALIGNMENT_INTERNAL_H__ */
