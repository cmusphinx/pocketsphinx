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

#include <math.h>

#include <sphinxbase/prim_type.h>
#include <sphinxbase/ckd_alloc.h>
#include <sphinxbase/err.h>

#include "ngram_model_internal.h"
#include "lm_trie_quant.h"

#define FLOAT_INF (0x7f800000)

typedef struct bins_s {
    float *begin;
    const float *end;
} bins_t;

struct lm_trie_quant_s {
    bins_t tables[NGRAM_MAX_ORDER - 1][2];
    bins_t *longest;
    uint8 *mem;
    size_t mem_size;
    uint8 prob_bits;
    uint8 bo_bits;
    uint32 prob_mask;
    uint32 bo_mask;
};

static void
bins_create(bins_t * bins, uint8 bits, float *begin)
{
    bins->begin = begin;
    bins->end = bins->begin + (1ULL << bits);
}

static float *
lower_bound(float *first, const float *last, float val)
{
    int count, step;
    float *it;

    count = last - first;
    while (count > 0) {
        it = first;
        step = count / 2;
        it += step;
        if (*it < val) {
            first = ++it;
            count -= step + 1;
        }
        else {
            count = step;
        }
    }
    return first;
}

static uint64
bins_encode(bins_t * bins, float value)
{
    float *above = lower_bound(bins->begin, bins->end, value);
    if (above == bins->begin)
        return 0;
    if (above == bins->end)
        return bins->end - bins->begin - 1;
    return above - bins->begin - (value - *(above - 1) < *above - value);
}

static float
bins_decode(bins_t * bins, size_t off)
{
    return bins->begin[off];
}

static size_t
quant_size(int order)
{
    int prob_bits = 16;
    int bo_bits = 16;
    size_t longest_table = (1U << prob_bits) * sizeof(float);
    size_t middle_table = (1U << bo_bits) * sizeof(float) + longest_table;
    /* unigrams are currently not quantized so no need for a table. */
    return (order - 2) * middle_table + longest_table;
}

lm_trie_quant_t *
lm_trie_quant_create(int order)
{
    float *start;
    int i;
    lm_trie_quant_t *quant =
        (lm_trie_quant_t *) ckd_calloc(1, sizeof(*quant));
    quant->mem_size = quant_size(order);
    quant->mem =
        (uint8 *) ckd_calloc(quant->mem_size, sizeof(*quant->mem));

    quant->prob_bits = 16;
    quant->bo_bits = 16;
    quant->prob_mask = (1U << quant->prob_bits) - 1;
    quant->bo_mask = (1U << quant->bo_bits) - 1;

    start = (float *) (quant->mem);
    for (i = 0; i < order - 2; i++) {
        bins_create(&quant->tables[i][0], quant->prob_bits, start);
        start += (1ULL << quant->prob_bits);
        bins_create(&quant->tables[i][1], quant->bo_bits, start);
        start += (1ULL << quant->bo_bits);
    }
    bins_create(&quant->tables[order - 2][0], quant->prob_bits, start);
    quant->longest = &quant->tables[order - 2][0];
    return quant;
}


lm_trie_quant_t *
lm_trie_quant_read_bin(FILE * fp, int order)
{
    int dummy;
    lm_trie_quant_t *quant;

    fread(&dummy, sizeof(dummy), 1, fp);
    quant = lm_trie_quant_create(order);
    fread(quant->mem, sizeof(*quant->mem), quant->mem_size, fp);

    return quant;
}

void
lm_trie_quant_write_bin(lm_trie_quant_t * quant, FILE * fp)
{
    /* Before it was quantization type */
    int dummy = 1;
    fwrite(&dummy, sizeof(dummy), 1, fp);
    fwrite(quant->mem, sizeof(*quant->mem), quant->mem_size, fp);
}

void
lm_trie_quant_free(lm_trie_quant_t * quant)
{
    if (quant->mem)
        ckd_free(quant->mem);
    ckd_free(quant);
}

uint8
lm_trie_quant_msize(lm_trie_quant_t * quant)
{
    return 32;
}

uint8
lm_trie_quant_lsize(lm_trie_quant_t * quant)
{
    return 16;
}

static int
weights_comparator(const void *a, const void *b)
{
    return (int) (*(float *) a - *(float *) b);
}

static void
make_bins(float *values, uint32 values_num, float *centers, uint32 bins)
{
    float *finish, *start;
    uint32 i;

    qsort(values, values_num, sizeof(*values), &weights_comparator);
    start = values;
    for (i = 0; i < bins; i++, centers++, start = finish) {
        finish = values + (size_t) ((uint64) values_num * (i + 1) / bins);
        if (finish == start) {
            /* zero length bucket. */
            *centers = i ? *(centers - 1) : -FLOAT_INF;
        }
        else {
            float sum = 0.0f;
            float *ptr;
            for (ptr = start; ptr != finish; ptr++) {
                sum += *ptr;
            }
            *centers = sum / (float) (finish - start);
        }
    }
}

void
lm_trie_quant_train(lm_trie_quant_t * quant, int order, uint32 counts,
                    ngram_raw_t * raw_ngrams)
{
    float *probs;
    float *backoffs;
    float *centers;
    uint32 backoff_num;
    uint32 prob_num;
    ngram_raw_t *raw_ngrams_end;

    probs = (float *) ckd_calloc(counts, sizeof(*probs));
    backoffs = (float *) ckd_calloc(counts, sizeof(*backoffs));
    raw_ngrams_end = raw_ngrams + counts;

    for (backoff_num = 0, prob_num = 0; raw_ngrams != raw_ngrams_end;
         raw_ngrams++) {
        probs[prob_num++] = raw_ngrams->prob;
        backoffs[backoff_num++] = raw_ngrams->backoff;
    }

    make_bins(probs, prob_num, quant->tables[order - 2][0].begin,
              1ULL << quant->prob_bits);
    centers = quant->tables[order - 2][1].begin;
    make_bins(backoffs, backoff_num, centers, (1ULL << quant->bo_bits));
    ckd_free(probs);
    ckd_free(backoffs);
}

void
lm_trie_quant_train_prob(lm_trie_quant_t * quant, int order, uint32 counts,
                         ngram_raw_t * raw_ngrams)
{
    float *probs;
    uint32 prob_num;
    ngram_raw_t *raw_ngrams_end;

    probs = (float *) ckd_calloc(counts, sizeof(*probs));
    raw_ngrams_end = raw_ngrams + counts;

    for (prob_num = 0; raw_ngrams != raw_ngrams_end; raw_ngrams++) {
        probs[prob_num++] = raw_ngrams->prob;
    }

    make_bins(probs, prob_num, quant->tables[order - 2][0].begin,
              1ULL << quant->prob_bits);
    ckd_free(probs);
}

void
lm_trie_quant_mwrite(lm_trie_quant_t * quant, bitarr_address_t address,
                     int order_minus_2, float prob, float backoff)
{
    bitarr_write_int57(address, quant->prob_bits + quant->bo_bits,
                       (uint64) ((bins_encode
                                  (&quant->tables[order_minus_2][0],
                                   prob) << quant->
                                  bo_bits) | bins_encode(&quant->
                                                         tables
                                                         [order_minus_2]
                                                         [1],
                                                         backoff)));
}

void
lm_trie_quant_lwrite(lm_trie_quant_t * quant, bitarr_address_t address,
                     float prob)
{
    bitarr_write_int25(address, quant->prob_bits,
                       (uint32) bins_encode(quant->longest, prob));
}

float
lm_trie_quant_mboread(lm_trie_quant_t * quant, bitarr_address_t address,
                      int order_minus_2)
{
    return bins_decode(&quant->tables[order_minus_2][1],
                       bitarr_read_int25(address, quant->bo_bits,
                                         quant->bo_mask));
}

float
lm_trie_quant_mpread(lm_trie_quant_t * quant, bitarr_address_t address,
                     int order_minus_2)
{
    address.offset += quant->bo_bits;
    return bins_decode(&quant->tables[order_minus_2][0],
                       bitarr_read_int25(address, quant->prob_bits,
                                         quant->prob_mask));
}

float
lm_trie_quant_lpread(lm_trie_quant_t * quant, bitarr_address_t address)
{
    return bins_decode(quant->longest,
                       bitarr_read_int25(address, quant->prob_bits,
                                         quant->prob_mask));
}
