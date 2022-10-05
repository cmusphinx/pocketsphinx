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
 * @file alignment.h
 * @brief Multi-level alignment structure
 *
 * Because doxygen is Bad Software, the actual documentation can only
 * exist in \ref ps_alignment_t.  Sorry about that.
 */

#ifndef __PS_ALIGNMENT_H__
#define __PS_ALIGNMENT_H__

#include <pocketsphinx/prim_type.h>

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

/**
 * Value indicating no parent or child for an entry.
 * @related ps_alignment_t
 */
#define PS_ALIGNMENT_NONE -1

/**
 * @struct ps_alignment_t pocketsphinx/alignment.h
 * @brief Multi-level alignment (words, phones, states) over an utterance.
 */
typedef struct ps_alignment_s ps_alignment_t;

/**
 * @struct ps_alignment_iter_t pocketsphinx/alignment.h
 * @brief Iterator over entries in an alignment.
 */
typedef struct ps_alignment_iter_s ps_alignment_iter_t;

/**
 * Retain an alighment
 * @memberof ps_alignment_t
 */
POCKETSPHINX_EXPORT
ps_alignment_t *ps_alignment_retain(ps_alignment_t *al);

/**
 * Release an alignment
 * @memberof ps_alignment_t
 */
POCKETSPHINX_EXPORT
int ps_alignment_free(ps_alignment_t *al);

/**
 * Iterate over the alignment starting at the first word.
 * @memberof ps_alignment_t
 */
POCKETSPHINX_EXPORT
ps_alignment_iter_t *ps_alignment_words(ps_alignment_t *al);

/**
 * Iterate over the alignment starting at the first phone.
 * @memberof ps_alignment_t
 */
POCKETSPHINX_EXPORT
ps_alignment_iter_t *ps_alignment_phones(ps_alignment_t *al);

/**
 * Iterate over the alignment starting at the first state.
 * @memberof ps_alignment_t
 */
POCKETSPHINX_EXPORT
ps_alignment_iter_t *ps_alignment_states(ps_alignment_t *al);

/**
 * Get the human-readable name of the current segment for an alignment.
 *
 * @memberof ps_alignment_iter_t
 * @return Name of this segment as a string (word, phone, or state
 * number).  This pointer is owned by the iterator, do not free it
 * yourself.
 */
POCKETSPHINX_EXPORT
const char *ps_alignment_iter_name(ps_alignment_iter_t *itor);

/**
 * Get the timing and score information for the current segment of an aligment.
 *
 * @memberof ps_alignment_iter_t
 * @arg start Output pointer for start frame
 * @arg duration Output pointer for duration
 * @return Acoustic score for this segment
 */
POCKETSPHINX_EXPORT
int ps_alignment_iter_seg(ps_alignment_iter_t *itor, int *start, int *duration);

/**
 * Move an alignment iterator forward.
 *
 * If the end of the alignment is reached, this will free the iterator
 * and return NULL.
 *
 * @memberof ps_alignment_iter_t
 */
POCKETSPHINX_EXPORT
ps_alignment_iter_t *ps_alignment_iter_next(ps_alignment_iter_t *itor);

/**
 * Iterate over the children of the current alignment entry.
 *
 * If there are no child nodes, NULL is returned.
 *
 * @memberof ps_alignment_iter_t
 */
POCKETSPHINX_EXPORT
ps_alignment_iter_t *ps_alignment_iter_children(ps_alignment_iter_t *itor);

/**
 * Release an iterator before completing all iterations.
 *
 * @memberof ps_alignment_iter_t
 */
POCKETSPHINX_EXPORT
int ps_alignment_iter_free(ps_alignment_iter_t *itor);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __PS_ALIGNMENT_H__ */
