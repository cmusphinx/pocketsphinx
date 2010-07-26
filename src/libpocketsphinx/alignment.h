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
 * @file alignment.h Multi-level alignment structure
 */

#ifndef __ALIGNMENT_H__
#define __ALIGNMENT_H__

/* System headers. */

/* SphinxBase headers. */
#include <sphinxbase/prim_type.h>

/* Local headers. */
#include "dict2pid.h"
#include "hmm.h"

struct alignment_entry_s {
    union {
        int32 wid;
        struct {
            uint16 ssid;
            uint16 cipid;
        } pid;
        uint16 senid;
    } id;
    int16 start;
    int16 duration;
};
typedef struct alignment_entry_s alignment_entry_t;

struct alignment_s {
    dict2pid_t *d2p;
    alignment_entry_t *word;
    uint16 n_word, n_word_alloc;
    alignment_entry_t *sseq;
    uint16 n_sseq, n_sseq_alloc;
    alignment_entry_t *state;
    uint16 n_state, n_state_alloc;
};
typedef struct alignment_s alignment_t;

struct alignment_iter_s {
    alignment_t *al;
    uint16 ent, parent, child;
};
typedef struct alignment_iter_s alignment_iter_t;

/**
 * Create a new, empty alignment.
 */
alignment_t *alignment_init(dict2pid_t *d2p);

/**
 * Release an alignment
 */
int alignment_free(alignment_t *al);

/**
 * Append a word.
 */
int alignment_add_word(alignment_t *al,
                       int32 wid, int duration);

/**
 * Populate lower layers using available word information.
 */
int alignment_populate(alignment_t *al);

/**
 * Propagate timing information up from state sequence.
 */
int alignment_propagate(alignment_t *al);

/**
 * Iterate over the alignment starting at the first word.
 */
alignment_iter_t *alignment_words(alignment_t *al);

/**
 * Iterate over the alignment starting at the first phone.
 */
alignment_iter_t *alignment_phones(alignment_t *al);

/**
 * Iterate over the alignment starting at the first state.
 */
alignment_iter_t *alignment_states(alignment_t *al);

alignment_iter_t *alignment_iter_next(alignment_iter_t *itor);
alignment_iter_t *alignment_iter_prev(alignment_iter_t *itor);
alignment_iter_t *alignment_iter_up(alignment_iter_t *itor);
alignment_iter_t *alignment_iter_down(alignment_iter_t *itor);

#endif /* __ALIGNMENT_H__ */
