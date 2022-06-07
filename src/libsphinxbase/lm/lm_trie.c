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

#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <sphinxbase/prim_type.h>
#include <sphinxbase/ckd_alloc.h>
#include <sphinxbase/err.h>
#include <sphinxbase/priority_queue.h>

#include "lm_trie.h"
#include "lm_trie_quant.h"

static void lm_trie_alloc_ngram(lm_trie_t * trie, uint32 * counts, int order);

static uint32
base_size(uint32 entries, uint32 max_vocab, uint8 remaining_bits)
{
    uint8 total_bits = bitarr_required_bits(max_vocab) + remaining_bits;
    /* Extra entry for next pointer at the end.  
     * +7 then / 8 to round up bits and convert to bytes
     * +sizeof(uint64) so that ReadInt57 etc don't go segfault.  
     * Note that this waste is O(order), not O(number of ngrams).*/
    return ((1 + entries) * total_bits + 7) / 8 + sizeof(uint64);
}

uint32
middle_size(uint8 quant_bits, uint32 entries, uint32 max_vocab,
            uint32 max_ptr)
{
    return base_size(entries, max_vocab,
                     quant_bits + bitarr_required_bits(max_ptr));
}

uint32
longest_size(uint8 quant_bits, uint32 entries, uint32 max_vocab)
{
    return base_size(entries, max_vocab, quant_bits);
}

static void
base_init(base_t * base, void *base_mem, uint32 max_vocab,
          uint8 remaining_bits)
{
    base->word_bits = bitarr_required_bits(max_vocab);
    base->word_mask = (1U << base->word_bits) - 1U;
    if (base->word_bits > 25)
        E_ERROR
            ("Sorry, word indices more than %d are not implemented.  Edit util/bit_packing.hh and fix the bit packing functions\n",
             (1U << 25));
    base->total_bits = base->word_bits + remaining_bits;

    base->base = (uint8 *) base_mem;
    base->insert_index = 0;
    base->max_vocab = max_vocab;
}

void
middle_init(middle_t * middle, void *base_mem, uint8 quant_bits,
            uint32 entries, uint32 max_vocab, uint32 max_next,
            void *next_source)
{
    middle->quant_bits = quant_bits;
    bitarr_mask_from_max(&middle->next_mask, max_next);
    middle->next_source = next_source;
    if (entries + 1 >= (1U << 25) || (max_next >= (1U << 25)))
        E_ERROR
            ("Sorry, this does not support more than %d n-grams of a particular order.  Edit util/bit_packing.hh and fix the bit packing functions\n",
             (1U << 25));
    base_init(&middle->base, base_mem, max_vocab,
              quant_bits + middle->next_mask.bits);
}

void
longest_init(longest_t * longest, void *base_mem, uint8 quant_bits,
             uint32 max_vocab)
{
    base_init(&longest->base, base_mem, max_vocab, quant_bits);
}

static bitarr_address_t
middle_insert(middle_t * middle, uint32 word, int order, int max_order)
{
    uint32 at_pointer;
    uint32 next;
    bitarr_address_t address;
    assert(word <= middle->base.word_mask);
    address.base = middle->base.base;
    address.offset = middle->base.insert_index * middle->base.total_bits;
    bitarr_write_int25(address, middle->base.word_bits, word);
    address.offset += middle->base.word_bits;
    at_pointer = address.offset;
    address.offset += middle->quant_bits;
    if (order == max_order - 1) {
        next = ((longest_t *) middle->next_source)->base.insert_index;
    }
    else {
        next = ((middle_t *) middle->next_source)->base.insert_index;
    }

    bitarr_write_int25(address, middle->next_mask.bits, next);
    middle->base.insert_index++;
    address.offset = at_pointer;
    return address;
}

static bitarr_address_t
longest_insert(longest_t * longest, uint32 index)
{
    bitarr_address_t address;
    assert(index <= longest->base.word_mask);
    address.base = longest->base.base;
    address.offset = longest->base.insert_index * longest->base.total_bits;
    bitarr_write_int25(address, longest->base.word_bits, index);
    address.offset += longest->base.word_bits;
    longest->base.insert_index++;
    return address;
}

static void
middle_finish_loading(middle_t * middle, uint32 next_end)
{
    bitarr_address_t address;
    address.base = middle->base.base;
    address.offset =
        (middle->base.insert_index + 1) * middle->base.total_bits -
        middle->next_mask.bits;
    bitarr_write_int25(address, middle->next_mask.bits, next_end);
}

static uint32
unigram_next(lm_trie_t * trie, int order)
{
    return order ==
        2 ? trie->longest->base.insert_index : trie->middle_begin->base.
        insert_index;
}

void
lm_trie_fix_counts(ngram_raw_t ** raw_ngrams, uint32 * counts,
                   uint32 * fixed_counts, int order)
{
    priority_queue_t *ngrams =
        priority_queue_create(order - 1, &ngram_ord_comparator);
    uint32 raw_ngram_ptrs[NGRAM_MAX_ORDER - 1];
    uint32 words[NGRAM_MAX_ORDER];
    int i;

    memset(words, -1, sizeof(words));
    memcpy(fixed_counts, counts, order * sizeof(*fixed_counts));
    for (i = 2; i <= order; i++) {
        ngram_raw_t *tmp_ngram;
        
        if (counts[i - 1] <= 0)
            continue;

        raw_ngram_ptrs[i - 2] = 0;

        tmp_ngram =
            (ngram_raw_t *) ckd_calloc(1, sizeof(*tmp_ngram));
        *tmp_ngram = raw_ngrams[i - 2][0];
        tmp_ngram->order = i;
        priority_queue_add(ngrams, tmp_ngram);
    }

    for (;;) {
        int32 to_increment = TRUE;
        ngram_raw_t *top;
        if (priority_queue_size(ngrams) == 0) {
            break;
        }
        top = (ngram_raw_t *) priority_queue_poll(ngrams);
        if (top->order == 2) {
            memcpy(words, top->words, 2 * sizeof(*words));
        }
        else {
            for (i = 0; i < top->order - 1; i++) {
                if (words[i] != top->words[i]) {
                    int num;
                    num = (i == 0) ? 1 : i;
                    memcpy(words, top->words,
                           (num + 1) * sizeof(*words));
                    fixed_counts[num]++;
                    to_increment = FALSE;
                    break;
                }
            }
            words[top->order - 1] = top->words[top->order - 1];
        }
        if (to_increment) {
            raw_ngram_ptrs[top->order - 2]++;
        }
        if (raw_ngram_ptrs[top->order - 2] < counts[top->order - 1]) {
            *top = raw_ngrams[top->order - 2][raw_ngram_ptrs[top->order - 2]];
            priority_queue_add(ngrams, top);
        }
        else {
            ckd_free(top);
        }
    }

    assert(priority_queue_size(ngrams) == 0);
    priority_queue_free(ngrams, NULL);
}


static void
recursive_insert(lm_trie_t * trie, ngram_raw_t ** raw_ngrams,
                 uint32 * counts, int order)
{
    uint32 unigram_idx = 0;
    uint32 *words;
    float *probs;
    const uint32 unigram_count = (uint32) counts[0];
    priority_queue_t *ngrams =
        priority_queue_create(order, &ngram_ord_comparator);
    ngram_raw_t *ngram;
    uint32 *raw_ngrams_ptr;
    int i;

    words = (uint32 *) ckd_calloc(order, sizeof(*words));
    probs = (float *) ckd_calloc(order - 1, sizeof(*probs));
    ngram = (ngram_raw_t *) ckd_calloc(1, sizeof(*ngram));
    ngram->order = 1;
    ngram->words = &unigram_idx;
    priority_queue_add(ngrams, ngram);
    raw_ngrams_ptr =
        (uint32 *) ckd_calloc(order - 1, sizeof(*raw_ngrams_ptr));
    for (i = 2; i <= order; ++i) {
        ngram_raw_t *tmp_ngram;
        
        if (counts[i - 1] <= 0)
            continue;

        raw_ngrams_ptr[i - 2] = 0;
        tmp_ngram =
            (ngram_raw_t *) ckd_calloc(1, sizeof(*tmp_ngram));
        *tmp_ngram = raw_ngrams[i - 2][0];
        tmp_ngram->order = i;

        priority_queue_add(ngrams, tmp_ngram);
    }

    for (;;) {
        ngram_raw_t *top =
            (ngram_raw_t *) priority_queue_poll(ngrams);

        if (top->order == 1) {
            trie->unigrams[unigram_idx].next = unigram_next(trie, order);
            words[0] = unigram_idx;
            probs[0] = trie->unigrams[unigram_idx].prob;
            if (++unigram_idx == unigram_count + 1) {
                ckd_free(top);
                break;
            }
            priority_queue_add(ngrams, top);
        }
        else {
            for (i = 0; i < top->order - 1; i++) {
                if (words[i] != top->words[i]) {
                    /* need to insert dummy suffixes to make ngram of higher order reachable */
                    int j;
                    assert(i > 0);  /* unigrams are not pruned without removing ngrams that contains them */
                    for (j = i; j < top->order - 1; j++) {
                        middle_t *middle = &trie->middle_begin[j - 1];
                        bitarr_address_t address =
                            middle_insert(middle, top->words[j],
                                          j + 1, order);
                        /* calculate prob for blank */
                        float calc_prob =
                            probs[j - 1] +
                            trie->unigrams[top->words[j]].bo;
                        probs[j] = calc_prob;
                        lm_trie_quant_mwrite(trie->quant, address, j - 1,
                                             calc_prob, 0.0f);
                    }
                }
            }
            memcpy(words, top->words,
                   top->order * sizeof(*words));
            if (top->order == order) {
                bitarr_address_t address =
                    longest_insert(trie->longest,
                                   top->words[top->order - 1]);
                lm_trie_quant_lwrite(trie->quant, address, top->prob);
            }
            else {
                middle_t *middle = &trie->middle_begin[top->order - 2];
                bitarr_address_t address =
                    middle_insert(middle,
                                  top->words[top->order - 1],
                                  top->order, order);
                /* write prob and backoff */
                probs[top->order - 1] = top->prob;
                lm_trie_quant_mwrite(trie->quant, address, top->order - 2,
                                     top->prob, top->backoff);
            }
            raw_ngrams_ptr[top->order - 2]++;
            if (raw_ngrams_ptr[top->order - 2] < counts[top->order - 1]) {
        	*top = raw_ngrams[top->order -
                               2][raw_ngrams_ptr[top->order - 2]];

                priority_queue_add(ngrams, top);
            }
            else {
                ckd_free(top);
            }
        }
    }
    assert(priority_queue_size(ngrams) == 0);
    priority_queue_free(ngrams, NULL);
    ckd_free(raw_ngrams_ptr);
    ckd_free(words);
    ckd_free(probs);
}

static lm_trie_t *
lm_trie_init(uint32 unigram_count)
{
    lm_trie_t *trie;

    trie = (lm_trie_t *) ckd_calloc(1, sizeof(*trie));
    memset(trie->hist_cache, -1, sizeof(trie->hist_cache)); /* prepare request history */
    memset(trie->backoff_cache, 0, sizeof(trie->backoff_cache));
    trie->unigrams =
        (unigram_t *) ckd_calloc((unigram_count + 1),
                                 sizeof(*trie->unigrams));
    trie->ngram_mem = NULL;
    return trie;
}

lm_trie_t *
lm_trie_create(uint32 unigram_count, int order)
{
    lm_trie_t *trie = lm_trie_init(unigram_count);
    trie->quant =
        (order > 1) ? lm_trie_quant_create(order) : 0;
    return trie;
}

lm_trie_t *
lm_trie_read_bin(uint32 * counts, int order, FILE * fp)
{
    lm_trie_t *trie = lm_trie_init(counts[0]);
    trie->quant = (order > 1) ? lm_trie_quant_read_bin(fp, order) : NULL;
    fread(trie->unigrams, sizeof(*trie->unigrams), (counts[0] + 1), fp);
    if (order > 1) {
        lm_trie_alloc_ngram(trie, counts, order);
        fread(trie->ngram_mem, 1, trie->ngram_mem_size, fp);
    }
    return trie;
}

void
lm_trie_write_bin(lm_trie_t * trie, uint32 unigram_count, FILE * fp)
{

    if (trie->quant)
        lm_trie_quant_write_bin(trie->quant, fp);
    fwrite(trie->unigrams, sizeof(*trie->unigrams), (unigram_count + 1),
           fp);
    if (trie->ngram_mem)
        fwrite(trie->ngram_mem, 1, trie->ngram_mem_size, fp);
}

void
lm_trie_free(lm_trie_t * trie)
{
    if (trie->ngram_mem) {
        ckd_free(trie->ngram_mem);
        ckd_free(trie->middle_begin);
        ckd_free(trie->longest);
    }
    if (trie->quant)
        lm_trie_quant_free(trie->quant);
    ckd_free(trie->unigrams);
    ckd_free(trie);
}

static void
lm_trie_alloc_ngram(lm_trie_t * trie, uint32 * counts, int order)
{
    int i;
    uint8 *mem_ptr;
    uint8 **middle_starts;

    trie->ngram_mem_size = 0;
    for (i = 1; i < order - 1; i++) {
        trie->ngram_mem_size +=
            middle_size(lm_trie_quant_msize(trie->quant), counts[i],
                        counts[0], counts[i + 1]);
    }
    trie->ngram_mem_size +=
        longest_size(lm_trie_quant_lsize(trie->quant), counts[order - 1],
                     counts[0]);
    trie->ngram_mem =
        (uint8 *) ckd_calloc(trie->ngram_mem_size,
                             sizeof(*trie->ngram_mem));
    mem_ptr = trie->ngram_mem;
    trie->middle_begin =
        (middle_t *) ckd_calloc(order - 2, sizeof(*trie->middle_begin));
    trie->middle_end = trie->middle_begin + (order - 2);
    middle_starts =
        (uint8 **) ckd_calloc(order - 2, sizeof(*middle_starts));
    for (i = 2; i < order; i++) {
        middle_starts[i - 2] = mem_ptr;
        mem_ptr +=
            middle_size(lm_trie_quant_msize(trie->quant), counts[i - 1],
                        counts[0], counts[i]);
    }
    trie->longest = (longest_t *) ckd_calloc(1, sizeof(*trie->longest));
    /* Crazy backwards thing so we initialize using pointers to ones that have already been initialized */
    for (i = order - 1; i >= 2; --i) {
        middle_t *middle_ptr = &trie->middle_begin[i - 2];
        middle_init(middle_ptr, middle_starts[i - 2],
                    lm_trie_quant_msize(trie->quant), counts[i - 1],
                    counts[0], counts[i],
                    (i ==
                     order -
                     1) ? (void *) trie->longest : (void *) &trie->
                    middle_begin[i - 1]);
    }
    ckd_free(middle_starts);
    longest_init(trie->longest, mem_ptr, lm_trie_quant_lsize(trie->quant),
                 counts[0]);
}

void
lm_trie_build(lm_trie_t * trie, ngram_raw_t ** raw_ngrams, uint32 * counts, uint32 *out_counts,
              int order)
{
    int i;

    lm_trie_fix_counts(raw_ngrams, counts, out_counts, order);
    lm_trie_alloc_ngram(trie, out_counts, order);
    
    if (order > 1)
        E_INFO("Training quantizer\n");
    for (i = 2; i < order; i++) {
        lm_trie_quant_train(trie->quant, i, counts[i - 1],
                            raw_ngrams[i - 2]);
    }
    lm_trie_quant_train_prob(trie->quant, order, counts[order - 1],
                             raw_ngrams[order - 2]);

    E_INFO("Building LM trie\n");
    recursive_insert(trie, raw_ngrams, counts, order);
    /* Set ending offsets so the last entry will be sized properly */
    /* Last entry for unigrams was already set. */
    if (trie->middle_begin != trie->middle_end) {
        middle_t *middle_ptr;
        for (middle_ptr = trie->middle_begin;
             middle_ptr != trie->middle_end - 1; ++middle_ptr) {
            middle_t *next_middle_ptr = middle_ptr + 1;
            middle_finish_loading(middle_ptr,
                                  next_middle_ptr->base.insert_index);
        }
        middle_ptr = trie->middle_end - 1;
        middle_finish_loading(middle_ptr,
                              trie->longest->base.insert_index);
    }
}

unigram_t *
unigram_find(unigram_t * u, uint32 word, node_range_t * next)
{
    unigram_t *ptr = &u[word];
    next->begin = ptr->next;
    next->end = (ptr + 1)->next;
    return ptr;
}

static size_t
calc_pivot(uint32 off, uint32 range, uint32 width)
{
    return (size_t) ((off * width) / (range + 1));
}

static uint8
uniform_find(void *base, uint8 total_bits, uint8 key_bits, uint32 key_mask,
             uint32 before_it, uint32 before_v,
             uint32 after_it, uint32 after_v, uint32 key, uint32 * out)
{
    bitarr_address_t address;
    address.base = base;

    /* If we look for unigram added later */
    if (key > after_v)
	return FALSE;

    while (after_it - before_it > 1) {
        uint32 mid;
        uint32 pivot =
            before_it + (1 +
                         calc_pivot(key - before_v, after_v - before_v,
                                    after_it - before_it - 1));
        /* access by pivot */
        address.offset = pivot * (uint32) total_bits;
        mid = bitarr_read_int25(address, key_bits, key_mask);
        if (mid < key) {
            before_it = pivot;
            before_v = mid;
        }
        else if (mid > key) {
            after_it = pivot;
            after_v = mid;
        }
        else {
            *out = pivot;
            return TRUE;
        }
    }
    return FALSE;
}

static bitarr_address_t
middle_find(middle_t * middle, uint32 word, node_range_t * range)
{
    uint32 at_pointer;
    bitarr_address_t address;

    /* finding BitPacked with uniform find */
    if (!uniform_find
        ((void *) middle->base.base, middle->base.total_bits,
         middle->base.word_bits, middle->base.word_mask, range->begin - 1,
         0, range->end, middle->base.max_vocab, word, &at_pointer)) {
        address.base = NULL;
        address.offset = 0;
        return address;
    }

    address.base = middle->base.base;
    at_pointer *= middle->base.total_bits;
    at_pointer += middle->base.word_bits;
    address.offset = at_pointer + middle->quant_bits;
    range->begin =
        bitarr_read_int25(address, middle->next_mask.bits,
                          middle->next_mask.mask);
    address.offset += middle->base.total_bits;
    range->end =
        bitarr_read_int25(address, middle->next_mask.bits,
                          middle->next_mask.mask);
    address.offset = at_pointer;

    return address;
}

static bitarr_address_t
longest_find(longest_t * longest, uint32 word, node_range_t * range)
{
    uint32 at_pointer;
    bitarr_address_t address;

    /* finding BitPacked with uniform find */
    if (!uniform_find
        ((void *) longest->base.base, longest->base.total_bits,
         longest->base.word_bits, longest->base.word_mask,
         range->begin - 1, 0, range->end, longest->base.max_vocab, word,
         &at_pointer)) {
        address.base = NULL;
        address.offset = 0;
        return address;
    }
    address.base = longest->base.base;
    address.offset =
        at_pointer * longest->base.total_bits + longest->base.word_bits;
    return address;
}

static float
get_available_prob(lm_trie_t * trie, int32 wid, int32 * hist,
                   int max_order, int32 n_hist, int32 * n_used)
{
    float prob;
    node_range_t node;
    bitarr_address_t address;
    int order_minus_2;
    uint8 independent_left;
    int32 *hist_iter, *hist_end;

    *n_used = 1;
    prob = unigram_find(trie->unigrams, wid, &node)->prob;
    if (n_hist == 0) {
        return prob;
    }

    /* find ngrams of higher order if any */
    order_minus_2 = 0;
    independent_left = (node.begin == node.end);
    hist_iter = hist;
    hist_end = hist + n_hist;
    for (;; order_minus_2++, hist_iter++) {
        if (hist_iter == hist_end)
            return prob;
        if (independent_left)
            return prob;
        if (order_minus_2 == max_order - 2)
            break;

        address =
            middle_find(&trie->middle_begin[order_minus_2], *hist_iter,
                        &node);
        independent_left = (address.base == NULL)
            || (node.begin == node.end);

        /* didn't find entry */
        if (address.base == NULL)
            return prob;
        prob = lm_trie_quant_mpread(trie->quant, address, order_minus_2);
        *n_used = order_minus_2 + 2;
    }

    address = longest_find(trie->longest, *hist_iter, &node);
    if (address.base != NULL) {
        prob = lm_trie_quant_lpread(trie->quant, address);
        *n_used = max_order;
    }
    return prob;
}

static float
get_available_backoff(lm_trie_t * trie, int32 start, int32 * hist,
                      int32 n_hist)
{
    float backoff = 0.0f;
    int order_minus_2;
    int32 *hist_iter;
    node_range_t node;
    unigram_t *first_hist = unigram_find(trie->unigrams, hist[0], &node);
    if (start <= 1) {
        backoff += first_hist->bo;
        start = 2;
    }
    order_minus_2 = start - 2;
    for (hist_iter = hist + start - 1; hist_iter < hist + n_hist;
         hist_iter++, order_minus_2++) {
        bitarr_address_t address =
            middle_find(&trie->middle_begin[order_minus_2], *hist_iter,
                        &node);
        if (address.base == NULL)
            break;
        backoff +=
            lm_trie_quant_mboread(trie->quant, address, order_minus_2);
    }
    return backoff;
}

static float
lm_trie_nobo_score(lm_trie_t * trie, int32 wid, int32 * hist,
                   int max_order, int32 n_hist, int32 * n_used)
{
    float prob =
        get_available_prob(trie, wid, hist, max_order, n_hist, n_used);
    if (n_hist < *n_used)
        return prob;
    return prob + get_available_backoff(trie, *n_used, hist, n_hist);
}

static float
lm_trie_hist_score(lm_trie_t * trie, int32 wid, int32 * hist, int32 n_hist,
                   int32 * n_used)
{
    float prob;
    int i, j;
    node_range_t node;
    bitarr_address_t address;

    *n_used = 1;
    prob = unigram_find(trie->unigrams, wid, &node)->prob;
    if (n_hist == 0)
        return prob;
    for (i = 0; i < n_hist - 1; i++) {
        address = middle_find(&trie->middle_begin[i], hist[i], &node);
        if (address.base == NULL) {
            for (j = i; j < n_hist; j++) {
                prob += trie->backoff_cache[j];
            }
            return prob;
        }
        else {
            (*n_used)++;
            prob = lm_trie_quant_mpread(trie->quant, address, i);
        }
    }
    address = longest_find(trie->longest, hist[n_hist - 1], &node);
    if (address.base == NULL) {
        return prob + trie->backoff_cache[n_hist - 1];
    }
    else {
        (*n_used)++;
        return lm_trie_quant_lpread(trie->quant, address);
    }
}

static uint8
history_matches(int32 * hist, int32 * prev_hist, int32 n_hist)
{
    int i;
    for (i = 0; i < n_hist; i++) {
        if (hist[i] != prev_hist[i]) {
            return FALSE;
        }
    }
    return TRUE;
}

static void
update_backoff(lm_trie_t * trie, int32 * hist, int32 n_hist)
{
    int i;
    node_range_t node;
    bitarr_address_t address;

    memset(trie->backoff_cache, 0, sizeof(trie->backoff_cache));
    trie->backoff_cache[0] = unigram_find(trie->unigrams, hist[0], &node)->bo;
    for (i = 1; i < n_hist; i++) {
        address = middle_find(&trie->middle_begin[i - 1], hist[i], &node);
        if (address.base == NULL) {
            break;
        }
        trie->backoff_cache[i] =
            lm_trie_quant_mboread(trie->quant, address, i - 1);
    }
    memcpy(trie->hist_cache, hist, n_hist * sizeof(*hist));
}

float
lm_trie_score(lm_trie_t * trie, int order, int32 wid, int32 * hist,
              int32 n_hist, int32 * n_used)
{
    if (n_hist < order - 1) {
        return lm_trie_nobo_score(trie, wid, hist, order, n_hist, n_used);
    }
    else {
        assert(n_hist == order - 1);
        if (!history_matches(hist, (int32 *) trie->hist_cache, n_hist)) {
            update_backoff(trie, hist, n_hist);
        }
        return lm_trie_hist_score(trie, wid, hist, n_hist, n_used);
    }
}

void
lm_trie_fill_raw_ngram(lm_trie_t * trie,
    		       ngram_raw_t * raw_ngrams, uint32 * raw_ngram_idx,
            	       uint32 * counts, node_range_t range, uint32 * hist,
    	               int n_hist, int order, int max_order)
{
    if (n_hist > 0 && range.begin == range.end) {
        return;
    }
    if (n_hist == 0) {
        uint32 i;
        for (i = 0; i < counts[0]; i++) {
            node_range_t node;
            unigram_find(trie->unigrams, i, &node);
            hist[0] = i;
            lm_trie_fill_raw_ngram(trie, raw_ngrams, raw_ngram_idx, counts,
                           node, hist, 1, order, max_order);
        }
    }
    else if (n_hist < order - 1) {
        uint32 ptr;
        node_range_t node;
        bitarr_address_t address;
        uint32 new_word;
        middle_t *middle = &trie->middle_begin[n_hist - 1];
        for (ptr = range.begin; ptr < range.end; ptr++) {
            address.base = middle->base.base;
            address.offset = ptr * middle->base.total_bits;
            new_word =
                bitarr_read_int25(address, middle->base.word_bits,
                                  middle->base.word_mask);
            hist[n_hist] = new_word;
            address.offset += middle->base.word_bits + middle->quant_bits;
            node.begin =
                bitarr_read_int25(address, middle->next_mask.bits,
                                  middle->next_mask.mask);
            address.offset =
                (ptr + 1) * middle->base.total_bits +
                middle->base.word_bits + middle->quant_bits;
            node.end =
                bitarr_read_int25(address, middle->next_mask.bits,
                                  middle->next_mask.mask);
            lm_trie_fill_raw_ngram(trie, raw_ngrams, raw_ngram_idx, counts,
                           node, hist, n_hist + 1, order, max_order);
        }
    }
    else {
        bitarr_address_t address;
        uint32 ptr;
        float prob, backoff;
        int i;
        assert(n_hist == order - 1);
        for (ptr = range.begin; ptr < range.end; ptr++) {
            ngram_raw_t *raw_ngram = &raw_ngrams[*raw_ngram_idx];
            if (order == max_order) {
                longest_t *longest = trie->longest;
                address.base = longest->base.base;
                address.offset = ptr * longest->base.total_bits;
                hist[n_hist] =
                    bitarr_read_int25(address, longest->base.word_bits,
                                      longest->base.word_mask);
                address.offset += longest->base.word_bits;
                prob = lm_trie_quant_lpread(trie->quant, address);
            }
            else {
                middle_t *middle = &trie->middle_begin[n_hist - 1];
                address.base = middle->base.base;
                address.offset = ptr * middle->base.total_bits;
                hist[n_hist] =
                    bitarr_read_int25(address, middle->base.word_bits,
                                      middle->base.word_mask);
                address.offset += middle->base.word_bits;
                prob =
                    lm_trie_quant_mpread(trie->quant, address, n_hist - 1);
                backoff =
                    lm_trie_quant_mboread(trie->quant, address,
                                          n_hist - 1);
                raw_ngram->backoff = backoff;
            }
            raw_ngram->prob = prob;
            raw_ngram->words =
                (uint32 *) ckd_calloc(order, sizeof(*raw_ngram->words));
            for (i = 0; i <= n_hist; i++) {
                raw_ngram->words[i] = hist[n_hist - i];
            }
            (*raw_ngram_idx)++;
        }
    }
}
