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
#ifndef __LM_TRIE_QUANT_H__
#define __LM_TRIE_QUANT_H__

#include <sphinxbase/bitarr.h>

#include "ngrams_raw.h"

typedef struct lm_trie_quant_s lm_trie_quant_t;

/**
 * Create qunatizing
 */
lm_trie_quant_t *lm_trie_quant_create(int order);

/**
 * Write quant data to binary file
 */
lm_trie_quant_t *lm_trie_quant_read_bin(FILE * fp, int order);

/**
 * Write quant data to binary file
 */
void lm_trie_quant_write_bin(lm_trie_quant_t * quant, FILE * fp);

/**
 * Free quant
 */
void lm_trie_quant_free(lm_trie_quant_t * quant);

/**
 * Memory required for storing weights of middle-order ngrams.
 * Both backoff and probability should be stored
 */
uint8 lm_trie_quant_msize(lm_trie_quant_t * quant);

/**
 * Memory required for storing weights of largest-order ngrams.
 * Only probability should be stored
 */
uint8 lm_trie_quant_lsize(lm_trie_quant_t * quant);

/**
 * Trains prob and backoff quantizer for specified ngram order on provided raw ngram list
 */
void lm_trie_quant_train(lm_trie_quant_t * quant, int order, uint32 counts,
                         ngram_raw_t * raw_ngrams);

/**
 * Trains only prob quantizer for specified ngram order on provided raw ngram list
 */
void lm_trie_quant_train_prob(lm_trie_quant_t * quant, int order,
                              uint32 counts, ngram_raw_t * raw_ngrams);

/**
 * Writes specified weight for middle-order ngram. Quantize it if needed
 */
void lm_trie_quant_mwrite(lm_trie_quant_t * quant,
                          bitarr_address_t address, int order_minus_2,
                          float prob, float backoff);

/**
 * Writes specified weight for largest-order ngram. Quantize it if needed
 */
void lm_trie_quant_lwrite(lm_trie_quant_t * quant,
                          bitarr_address_t address, float prob);

/**
 * Reads and decodes if needed backoff for middle-order ngram
 */
float lm_trie_quant_mboread(lm_trie_quant_t * quant,
                            bitarr_address_t address, int order_minus_2);

/**
 * Reads and decodes if needed prob for middle-order ngram
 */
float lm_trie_quant_mpread(lm_trie_quant_t * quant,
                           bitarr_address_t address, int order_minus_2);

/**
 * Reads and decodes if needed prob for largest-order ngram
 */
float lm_trie_quant_lpread(lm_trie_quant_t * quant,
                           bitarr_address_t address);

#endif                          /* __LM_TRIE_QUANT_H__ */
