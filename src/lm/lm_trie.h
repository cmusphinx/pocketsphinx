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

#ifndef __LM_TRIE_H__
#define __LM_TRIE_H__

#include <sphinxbase/pio.h>
#include <sphinxbase/bitarr.h>

#include "ngram_model_internal.h"
#include "lm_trie_quant.h"

typedef struct unigram_s {
    float prob;
    float bo;
    uint32 next;
} unigram_t;

typedef struct node_range_s {
    uint32 begin;
    uint32 end;
} node_range_t;

typedef struct base_s {
    uint8 word_bits;
    uint8 total_bits;
    uint32 word_mask;
    uint8 *base;
    uint32 insert_index;
    uint32 max_vocab;
} base_t;

typedef struct middle_s {
    base_t base;
    bitarr_mask_t next_mask;
    uint8 quant_bits;
    void *next_source;
} middle_t;

typedef struct longest_s {
    base_t base;
    uint8 quant_bits;
} longest_t;

typedef struct lm_trie_s {
    uint8 *ngram_mem;
    size_t ngram_mem_size;
    unigram_t *unigrams;
    middle_t *middle_begin;
    middle_t *middle_end;
    longest_t *longest;
    lm_trie_quant_t *quant;

    float backoff_cache[NGRAM_MAX_ORDER];
    uint32 hist_cache[NGRAM_MAX_ORDER - 1];
} lm_trie_t;

/**
 * Creates lm_trie structure. Fills it if binary file with correspondent data is provided
 */
lm_trie_t *lm_trie_create(uint32 unigram_count, int order);

lm_trie_t *lm_trie_read_bin(uint32 * counts, int order, FILE * fp);

void lm_trie_write_bin(lm_trie_t * trie, uint32 unigram_count, FILE * fp);

void lm_trie_free(lm_trie_t * trie);

void lm_trie_build(lm_trie_t * trie, ngram_raw_t ** raw_ngrams,
                   uint32 * counts, uint32 *out_counts, int order);

void lm_trie_fill_raw_ngram(lm_trie_t * trie,
			    ngram_raw_t * raw_ngrams, uint32 * raw_ngram_idx,
            	            uint32 * counts, node_range_t range, uint32 * hist,
    	                    int n_hist, int order, int max_order);

float lm_trie_score(lm_trie_t * trie, int order, int32 wid, int32 * hist,
                    int32 n_hist, int32 * n_used);

#endif                          /* __LM_TRIE_H__ */
