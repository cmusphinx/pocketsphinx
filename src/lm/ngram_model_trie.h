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
#ifndef __NGRAM_MODEL_TRIE_H__
#define __NGRAM_MODEL_TRIE_H__

#include "lm/ngram_model_internal.h"
#include "lm/lm_trie.h"

typedef struct ngram_model_trie_s {
    ngram_model_t base;  /**< Base ngram_model_t structure */
    lm_trie_t *trie;     /**< Trie structure that stores ngram relations and weights */
} ngram_model_trie_t;

/**
 * Read N-Gram model from and ARPABO text file and arrange it in trie structure
 */
ngram_model_t *ngram_model_trie_read_arpa(ps_config_t * config,
                                          const char *path,
                                          logmath_t * lmath);

/**
 * Write N-Gram model stored in trie structure in ARPABO format
 */
int ngram_model_trie_write_arpa(ngram_model_t * base, const char *path);

/**
 * Read N-Gram model from the binary file and arrange it in a trie structure
 */
ngram_model_t *ngram_model_trie_read_bin(ps_config_t * config,
                                         const char *path,
                                         logmath_t * lmath);

/**
 * Write trie to binary file
 */
int ngram_model_trie_write_bin(ngram_model_t * model, const char *path);

/**
 * Read N-Gram model from DMP file and arrange it in trie structure
 */
ngram_model_t *ngram_model_trie_read_dmp(ps_config_t * config,
                                         const char *file_name,
                                         logmath_t * lmath);

#endif                          /* __NGRAM_MODEL_TRIE_H__ */
