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

#include <pocketsphinx/err.h>

#include "util/pio.h"
#include "util/strfuncs.h"
#include "util/ckd_alloc.h"
#include "util/byteorder.h"
#include "lm/ngram_model_internal.h"
#include "lm/ngrams_raw.h"

int
ngram_ord_comparator(const void *a_raw, const void *b_raw)
{
    ngram_raw_t *a = (ngram_raw_t *) a_raw;
    ngram_raw_t *b = (ngram_raw_t *) b_raw;
    int a_w_ptr = 0;
    int b_w_ptr = 0;
    while ((uint32)a_w_ptr < a->order && (uint32)b_w_ptr < b->order) {
        if (a->words[a_w_ptr] == b->words[b_w_ptr]) {
            a_w_ptr++;
            b_w_ptr++;
            continue;
        }
        if (a->words[a_w_ptr] < b->words[b_w_ptr])
            return -1;
        else
            return 1;
    }
    return a->order - b->order;
}

static int
ngrams_raw_read_line(lineiter_t *li, hash_table_t *wid,
                    logmath_t *lmath, int order, int order_max,
                    ngram_raw_t *raw_ngram)
{
    int n, i;
    int words_expected;
    char *wptr[NGRAM_MAX_ORDER + 1];
    uint32 *word_out;

    words_expected = order + 1;
    if ((n =
         str2words(li->buf, wptr,
                   NGRAM_MAX_ORDER + 1)) < words_expected) {
        E_ERROR("Format error; %d-gram ignored at line %d\n", order, li->lineno);
        return -1;
    }

    raw_ngram->order = order;

    if (order == order_max) {
        raw_ngram->prob = atof_c(wptr[0]);
        if (raw_ngram->prob > 0) {
            E_WARN("%d-gram '%s' has positive probability\n", order, wptr[1]);
            raw_ngram->prob = 0.0f;
        }
        raw_ngram->prob =
            logmath_log10_to_log_float(lmath, raw_ngram->prob);
    }
    else {
        float weight, backoff;

        weight = atof_c(wptr[0]);
        if (weight > 0) {
            E_WARN("%d-gram '%s' has positive probability\n", order, wptr[1]);
            raw_ngram->prob = 0.0f;
        }
        else {
            raw_ngram->prob =
                logmath_log10_to_log_float(lmath, weight);
        }

        if (n == order + 1) {
            raw_ngram->backoff = 0.0f;
        }
        else {
            backoff = atof_c(wptr[order + 1]);
            raw_ngram->backoff =
                logmath_log10_to_log_float(lmath, backoff);
        }
    }
    raw_ngram->words =
        (uint32 *) ckd_calloc(order, sizeof(*raw_ngram->words));
    for (word_out = raw_ngram->words + order - 1, i = 1;
         word_out >= raw_ngram->words; --word_out, i++) {
        hash_table_lookup_int32(wid, wptr[i], (int32 *) word_out);
    }
    return 0;
}

static int
ngrams_raw_read_section(ngram_raw_t ** raw_ngrams, lineiter_t ** li,
                      hash_table_t * wid, logmath_t * lmath, uint32 *count,
                      int order, int order_max)
{
    char expected_header[20];
    uint32 i, cur;

    sprintf(expected_header, "\\%d-grams:", order);
    while (*li && strcmp((*li)->buf, expected_header) != 0) {
	*li = lineiter_next(*li);
    }
    
    if (*li == NULL) {
	E_ERROR("Failed to find '%s', language model file truncated\n", expected_header);
	return -1;
    }
    
    *raw_ngrams = (ngram_raw_t *) ckd_calloc(*count, sizeof(ngram_raw_t));
    for (i = 0, cur = 0; i < *count && *li != NULL; i++) {
	*li = lineiter_next(*li);
        if (*li == NULL) {
	    E_ERROR("Unexpected end of ARPA file. Failed to read %d-gram\n",
                order);
    	    return -1;
	}
        if (ngrams_raw_read_line(*li, wid, lmath, order, order_max,
                                *raw_ngrams + cur) == 0) {
            cur++;
        }
    }
    *count = cur;
    qsort(*raw_ngrams, *count, sizeof(ngram_raw_t), &ngram_ord_comparator);
    return 0;
}

ngram_raw_t **
ngrams_raw_read_arpa(lineiter_t ** li, logmath_t * lmath, uint32 * counts,
                     int order, hash_table_t * wid)
{
    ngram_raw_t **raw_ngrams;
    int order_it;

    raw_ngrams =
        (ngram_raw_t **) ckd_calloc(order - 1, sizeof(*raw_ngrams));

    for (order_it = 2; order_it <= order; order_it++) {
        if (ngrams_raw_read_section(&raw_ngrams[order_it - 2], li, wid, lmath,
                              counts + order_it - 1, order_it, order) < 0)
        break;
    }

    /* Check if we found ARPA end-mark */
    if (*li == NULL) {
        E_ERROR("ARPA file ends without end-mark\n");
	ngrams_raw_free(raw_ngrams, counts, order);
        return NULL;
    } else {
        *li = lineiter_next(*li);
	if (strcmp((*li)->buf, "\\end\\") != 0) {
    	    E_WARN
        	("Finished reading ARPA file. Expecting end mark but found '%s'\n",
        	 (*li)->buf);
        }
    }

    return raw_ngrams;
}

static void
read_dmp_weight_array(FILE * fp, logmath_t * lmath, uint8 do_swap,
                      int32 counts, ngram_raw_t * raw_ngrams,
                      int weight_idx)
{
    int32 i, k;
    dmp_weight_t *tmp_weight_arr;

    fread(&k, sizeof(k), 1, fp);
    if (do_swap)
        SWAP_INT32(&k);
    tmp_weight_arr =
        (dmp_weight_t *) ckd_calloc(k, sizeof(*tmp_weight_arr));
    fread(tmp_weight_arr, sizeof(*tmp_weight_arr), k, fp);
    for (i = 0; i < k; i++) {
        if (do_swap)
            SWAP_INT32(&tmp_weight_arr[i].l);
        /* Convert values to log. */
        tmp_weight_arr[i].f =
            logmath_log10_to_log_float(lmath, tmp_weight_arr[i].f);
    }
    /* replace indexes with real probs in raw bigrams */
    for (i = 0; i < counts; i++) {
	if (weight_idx == 0) {
	    raw_ngrams[i].prob =
                tmp_weight_arr[(int) raw_ngrams[i].prob].f;
        } else {
	    raw_ngrams[i].backoff =
                tmp_weight_arr[(int) raw_ngrams[i].backoff].f;
        }
    }
    ckd_free(tmp_weight_arr);
}

#define BIGRAM_SEGMENT_SIZE 9

ngram_raw_t **
ngrams_raw_read_dmp(FILE * fp, logmath_t * lmath, uint32 * counts,
                    int order, uint32 * unigram_next, uint8 do_swap)
{
    uint32 j, ngram_idx;
    uint16 *bigrams_next;
    ngram_raw_t **raw_ngrams =
        (ngram_raw_t **) ckd_calloc(order - 1, sizeof(*raw_ngrams));

    /* read bigrams */
    raw_ngrams[0] =
        (ngram_raw_t *) ckd_calloc((size_t) (counts[1] + 1),
                                   sizeof(*raw_ngrams[0]));
    bigrams_next =
        (uint16 *) ckd_calloc((size_t) (counts[1] + 1),
                              sizeof(*bigrams_next));
    ngram_idx = 1;
    for (j = 0; j <= counts[1]; j++) {
        uint16 wid, prob_idx, bo_idx;
        ngram_raw_t *raw_ngram = &raw_ngrams[0][j];

        fread(&wid, sizeof(wid), 1, fp);
        if (do_swap)
            SWAP_INT16(&wid);
        raw_ngram->order = 2;
        while (ngram_idx < counts[0] && j == unigram_next[ngram_idx]) {
            ngram_idx++;
        }
	
	if (j != counts[1]) {
            raw_ngram->words =
    		(uint32 *) ckd_calloc(2, sizeof(*raw_ngram->words));
    	    raw_ngram->words[0] = (uint32) wid;
	    raw_ngram->words[1] = (uint32) ngram_idx - 1;
	}

        fread(&prob_idx, sizeof(prob_idx), 1, fp);
        fread(&bo_idx, sizeof(bo_idx), 1, fp);
        fread(&bigrams_next[j], sizeof(bigrams_next[j]), 1, fp);
        if (do_swap) {
            SWAP_INT16(&prob_idx);
            SWAP_INT16(&bo_idx);
            SWAP_INT16(&bigrams_next[j]);
        }

	if (j != counts[1]) {
            raw_ngram->prob = prob_idx + 0.5f; /* keep index in float. ugly but avoiding using extra memory */
	    raw_ngram->backoff = bo_idx + 0.5f;
	}
    }

    if (ngram_idx < counts[0]) {
        E_ERROR("Corrupted model, not enough unigrams %d %d\n", ngram_idx, counts[0]);
        ckd_free(bigrams_next);
        ngrams_raw_free(raw_ngrams, counts, order);
        return NULL;
    }

    /* read trigrams */
    if (order > 2) {
        raw_ngrams[1] =
            (ngram_raw_t *) ckd_calloc((size_t) counts[2],
                                       sizeof(*raw_ngrams[1]));
        for (j = 0; j < counts[2]; j++) {
            uint16 wid, prob_idx;
            ngram_raw_t *raw_ngram = &raw_ngrams[1][j];

            fread(&wid, sizeof(wid), 1, fp);
            fread(&prob_idx, sizeof(prob_idx), 1, fp);
            if (do_swap) {
                SWAP_INT16(&wid);
                SWAP_INT16(&prob_idx);
            }
            
    	    raw_ngram->order = 3;
            raw_ngram->words =
                (uint32 *) ckd_calloc(3, sizeof(*raw_ngram->words));
            raw_ngram->words[0] = (uint32) wid;
            raw_ngram->prob = prob_idx + 0.5f; /* keep index in float. ugly but avoiding using extra memory */
        }
    }

    /* read prob2 */
    read_dmp_weight_array(fp, lmath, do_swap, (int32) counts[1],
                          raw_ngrams[0], 0);
    /* read bo2 */
    if (order > 2) {
        int32 k;
        int32 *tseg_base;
        read_dmp_weight_array(fp, lmath, do_swap, (int32) counts[1],
                              raw_ngrams[0], 1);
        /* read prob3 */
        read_dmp_weight_array(fp, lmath, do_swap, (int32) counts[2],
                              raw_ngrams[1], 0);
        /* Read tseg_base size and tseg_base to fill trigram's first words */
        fread(&k, sizeof(k), 1, fp);
        if (do_swap)
            SWAP_INT32(&k);
        tseg_base = (int32 *) ckd_calloc(k, sizeof(int32));
        fread(tseg_base, sizeof(int32), k, fp);
        if (do_swap) {
            for (j = 0; j < (uint32) k; j++) {
                SWAP_INT32(&tseg_base[j]);
            }
        }
        ngram_idx = 0;
        for (j = 1; j <= counts[1]; j++) {
            uint32 next_ngram_idx =
                (uint32) (tseg_base[j >> BIGRAM_SEGMENT_SIZE] +
                          bigrams_next[j]);
            while (ngram_idx < next_ngram_idx) {
                raw_ngrams[1][ngram_idx].words[1] =
                    raw_ngrams[0][j - 1].words[0];
                raw_ngrams[1][ngram_idx].words[2] =
                    raw_ngrams[0][j - 1].words[1];
                ngram_idx++;
            }
        }
        ckd_free(tseg_base);
        
        if (ngram_idx < counts[2]) {
      	    E_ERROR("Corrupted model, some trigrams have no corresponding bigram\n");
    	    ckd_free(bigrams_next);
    	    ngrams_raw_free(raw_ngrams, counts, order);
    	    return NULL;
        }
    }
    ckd_free(bigrams_next);

    /* sort raw ngrams for reverse trie */
    qsort(raw_ngrams[0], (size_t) counts[1], sizeof(*raw_ngrams[0]),
          &ngram_ord_comparator);
    if (order > 2) {
        qsort(raw_ngrams[1], (size_t) counts[2], sizeof(*raw_ngrams[1]),
              &ngram_ord_comparator);
    }
    return raw_ngrams;
}

void
ngrams_raw_free(ngram_raw_t ** raw_ngrams, uint32 * counts, int order)
{
    uint32 num;
    int order_it;

    for (order_it = 0; order_it < order - 1; order_it++) {
        for (num = 0; num < counts[order_it + 1]; num++) {
            ckd_free(raw_ngrams[order_it][num].words);
        }
        ckd_free(raw_ngrams[order_it]);
    }
    ckd_free(raw_ngrams);
}
