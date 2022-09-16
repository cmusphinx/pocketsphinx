/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 2015 Carnegie Mellon University.  All rights
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

#ifndef __NGRAMS_RAW_H__
#define __NGRAMS_RAW_H__

#include <pocketsphinx.h>

#include "util/hash_table.h"
#include "util/pio.h"

typedef struct ngram_raw_s {
    uint32 *words;              /* array of word indexes, length corresponds to ngram order */
    float32 prob;
    float32 backoff;
    uint32 order;
} ngram_raw_t;

typedef union {
    float32 f;
    int32 l;
} dmp_weight_t;

/**
 * Raw ordered ngrams comparator
 */
int ngram_ord_comparator(const void *a_raw, const void *b_raw);

/**
 * Read ngrams of order > 1 from ARPA file
 * @param li     [in] sphinxbase file line iterator that point to bigram description in ARPA file
 * @param wid    [in] hashtable that maps string word representation to id
 * @param lmath  [in] log math used for log convertions
 * @param counts [in] amount of ngrams for each order
 * @param order  [in] maximum order of ngrams
 * @return            raw ngrams of order bigger than 1
 */
ngram_raw_t **ngrams_raw_read_arpa(lineiter_t ** li, logmath_t * lmath,
                                   uint32 * counts, int order,
                                   hash_table_t * wid);

/**
 * Reads ngrams of order > 1 from DMP file.
 * @param fp           [in] file to read from. Position in file corresponds to start of bigram description
 * @param lmath        [in] log math used for log convertions
 * @param counts       [in] amount of ngrams for each order
 * @param order        [in] maximum order of ngrams
 * @param unigram_next [in] array of next word pointers for unigrams. Needed to define forst word of bigrams
 * @param do_swap      [in] wether to do swap of bits
 * @return                  raw ngrams of order bigger than 1
 */
ngram_raw_t **ngrams_raw_read_dmp(FILE * fp, logmath_t * lmath,
                                  uint32 * counts, int order,
                                  uint32 * unigram_next, uint8 do_swap);

void ngrams_raw_free(ngram_raw_t ** raw_ngrams, uint32 * counts,
                     int order);

#endif                          /* __LM_NGRAMS_RAW_H__ */
