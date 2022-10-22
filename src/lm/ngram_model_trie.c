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
#include <assert.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <pocketsphinx/err.h>

#include "util/pio.h"
#include "util/strfuncs.h"
#include "util/ckd_alloc.h"
#include "util/byteorder.h"
#include "lm/ngram_model_trie.h"

static const char trie_hdr[] = "Trie Language Model";
static const char dmp_hdr[] = "Darpa Trigram LM";
static ngram_funcs_t ngram_model_trie_funcs;

/*
 * Read and return #unigrams, #bigrams, #trigrams as stated in input file.
 */
static int
read_counts_arpa(lineiter_t ** li, uint32 * counts, int *order)
{
    int32 ngram, prev_ngram;
    uint32 ngram_cnt;

    /* skip file until past the '\data\' marker */
    while (*li) {
        if (strcmp((*li)->buf, "\\data\\") == 0)
            break;
        *li = lineiter_next(*li);
    }

    if (*li == NULL || strcmp((*li)->buf, "\\data\\") != 0) {
        E_INFO("No \\data\\ mark in LM file\n");
        return -1;
    }

    prev_ngram = 0;
    *order = 0;
    while ((*li = lineiter_next(*li))) {
        if (sscanf((*li)->buf, "ngram %d=%d", &ngram, &ngram_cnt) != 2)
            break;
        if (ngram != prev_ngram + 1) {
            E_ERROR
                ("Ngram counts in LM file is not in order. %d goes after %d\n",
                 ngram, prev_ngram);
            return -1;
        }
        prev_ngram = ngram;
        counts[*order] = ngram_cnt;
        (*order)++;
    }

    if (*li == NULL) {
        E_ERROR("EOF while reading ngram counts\n");
        return -1;
    }

    return 0;
}

static int
read_1grams_arpa(lineiter_t ** li, uint32 count, ngram_model_t * base,
                 unigram_t * unigrams)
{
    uint32 i;
    int n;
    int n_parts;
    char *wptr[3];

    while (*li && strcmp((*li)->buf, "\\1-grams:") != 0) {
	*li = lineiter_next(*li);
    }
    if (*li == NULL) {
        E_ERROR_SYSTEM("Failed to read \\1-grams: mark");
        return -1;
    }

    n_parts = 2;
    for (i = 0; i < count; i++) {
        unigram_t *unigram;
        
        *li = lineiter_next(*li);
        if (*li == NULL) {
            E_ERROR
                ("Unexpected end of ARPA file. Failed to read unigram %d\n",
                 i + 1);
            return -1;
        }
        if ((n = str2words((*li)->buf, wptr, 3)) < n_parts) {
            E_ERROR("Format error at line %d, Failed to read unigrams\n", (*li)->lineno);
            return -1;
        }

        unigram = &unigrams[i];
        unigram->prob =
            logmath_log10_to_log_float(base->lmath, atof_c(wptr[0]));
        if (unigram->prob > 0) {
            E_WARN("Unigram '%s' has positive probability\n", wptr[1]);
            unigram->prob = 0;
        }
        if (n == n_parts + 1) {
            unigram->bo =
                logmath_log10_to_log_float(base->lmath,
                                           atof_c(wptr[2]));
        }
        else {
            unigram->bo = 0.0f;
        }

        /* TODO: classify float with fpclassify and warn if bad value occurred */
        base->word_str[i] = ckd_salloc(wptr[1]);
    }

    /* fill hash-table that maps unigram names to their word ids */
    for (i = 0; i < count; i++) {
        if ((hash_table_enter
             (base->wid, base->word_str[i],
              (void *) (size_t) i)) != (void *) (size_t) i) {
            E_WARN("Duplicate word in dictionary: %s\n",
                   base->word_str[i]);
        }
    }
    return 0;
}

ngram_model_t *
ngram_model_trie_read_arpa(ps_config_t * config,
                           const char *path, logmath_t * lmath)
{
    FILE *fp;
    lineiter_t *li;
    ngram_model_trie_t *model;
    ngram_model_t *base;
    ngram_raw_t **raw_ngrams;
    int32 is_pipe;
    uint32 counts[NGRAM_MAX_ORDER];
    int order;
    int i;

    (void)config;
    E_INFO("Trying to read LM in arpa format\n");
    if ((fp = fopen_comp(path, "r", &is_pipe)) == NULL) {
        E_ERROR("File %s not found\n", path);
        return NULL;
    }

    model = (ngram_model_trie_t *) ckd_calloc(1, sizeof(*model));
    li = lineiter_start_clean(fp);
    /* Read n-gram counts from file */
    if (read_counts_arpa(&li, counts, &order) == -1) {
        ckd_free(model);
        lineiter_free(li);
        fclose_comp(fp, is_pipe);
        return NULL;
    }

    E_INFO("LM of order %d\n", order);
    for (i = 0; i < order; i++) {
        E_INFO("#%d-grams: %d\n", i + 1, counts[i]);
    }

    base = &model->base;
    ngram_model_init(base, &ngram_model_trie_funcs, lmath, order,
                     (int32) counts[0]);
    base->writable = TRUE;

    model->trie = lm_trie_create(counts[0], order);
    if (read_1grams_arpa(&li, counts[0], base, model->trie->unigrams) < 0) {
	ngram_model_free(base);
        lineiter_free(li);
        fclose_comp(fp, is_pipe);
        return NULL;
    }

    if (order > 1) {
        raw_ngrams =
            ngrams_raw_read_arpa(&li, base->lmath, counts, order,
                                 base->wid);
        if (raw_ngrams == NULL) {
            ngram_model_free(base);
            lineiter_free(li);
            fclose_comp(fp, is_pipe);
            return NULL;
        }
        lm_trie_build(model->trie, raw_ngrams, counts, base->n_counts, order);
        ngrams_raw_free(raw_ngrams, counts, order);
    }

    lineiter_free(li);
    fclose_comp(fp, is_pipe);

    return base;
}

int
ngram_model_trie_write_arpa(ngram_model_t * base, const char *path)
{
    int i;
    uint32 j;
    ngram_model_trie_t *model = (ngram_model_trie_t *) base;
    FILE *fp = fopen(path, "w");
    if (!fp) {
        E_ERROR("Unable to open %s to write arpa LM from trie\n", path);
        return -1;
    }
    fprintf(fp,
            "This is an ARPA-format language model file, generated by CMU Sphinx\n");
    /* Write N-gram counts. */
    fprintf(fp, "\\data\\\n");
    for (i = 0; i < base->n; ++i) {
        fprintf(fp, "ngram %d=%d\n", i + 1, base->n_counts[i]);
    }
    /* Write 1-grams */
    fprintf(fp, "\n\\1-grams:\n");
    for (j = 0; j < base->n_counts[0]; j++) {
        unigram_t *unigram = &model->trie->unigrams[j];
        fprintf(fp, "%.4f\t%s",
                logmath_log_float_to_log10(base->lmath, unigram->prob),
                base->word_str[j]);
        if (base->n > 1) {
            fprintf(fp, "\t%.4f",
                    logmath_log_float_to_log10(base->lmath, unigram->bo));
        }
        fprintf(fp, "\n");
    }
    /* Write ngrams */
    if (base->n > 1) {
        for (i = 2; i <= base->n; ++i) {
            ngram_raw_t *raw_ngrams =
                (ngram_raw_t *) ckd_calloc((size_t) base->n_counts[i - 1],
                                           sizeof(*raw_ngrams));
            uint32 raw_ngram_idx;
            uint32 j;
            uint32 hist[NGRAM_MAX_ORDER];
            node_range_t range;
            raw_ngram_idx = 0;
            range.begin = range.end = 0;  

            /* we need to iterate over a trie here. recursion should do the job */
            lm_trie_fill_raw_ngram(model->trie, raw_ngrams,
                           &raw_ngram_idx, base->n_counts, range, hist, 0,
                           i, base->n);
            assert(raw_ngram_idx == base->n_counts[i - 1]);
            qsort(raw_ngrams, (size_t) base->n_counts[i - 1],
                  sizeof(ngram_raw_t), &ngram_ord_comparator);

            fprintf(fp, "\n\\%d-grams:\n", i);
            for (j = 0; j < base->n_counts[i - 1]; j++) {
                int k;
                fprintf(fp, "%.4f", logmath_log_float_to_log10(base->lmath, raw_ngrams[j].prob));
                for (k = 0; k < i; k++) {
                    fprintf(fp, "\t%s",
                            base->word_str[raw_ngrams[j].words[k]]);
                }
                ckd_free(raw_ngrams[j].words);
                if (i < base->n) {
                    fprintf(fp, "\t%.4f", logmath_log_float_to_log10(base->lmath, raw_ngrams[j].backoff));
                }
                fprintf(fp, "\n");
            }
            ckd_free(raw_ngrams);
        }
    }
    fprintf(fp, "\n\\end\\\n");
    return fclose(fp);
}

static void
read_word_str(ngram_model_t * base, FILE * fp, int do_swap)
{
    int32 k;
    uint32 i, j;
    char *tmp_word_str;
    /* read ascii word strings */
    base->writable = TRUE;
    fread(&k, sizeof(k), 1, fp);
    if (do_swap)
        SWAP_INT32(&k);
    E_INFO("#word_str: %d\n", k);
    tmp_word_str = (char *) ckd_calloc((size_t) k, 1);
    fread(tmp_word_str, 1, (size_t) k, fp);

    /* First make sure string just read contains n_counts[0] words (PARANOIA!!) */
    for (i = 0, j = 0; i < (uint32) k; i++)
        if (tmp_word_str[i] == '\0')
            j++;
    if (j != base->n_counts[0]) {
        E_ERROR
            ("Error reading word strings (%d doesn't match n_unigrams %d)\n",
             j, base->n_counts[0]);
    }

    /* Break up string just read into words */
    j = 0;
    for (i = 0; i < base->n_counts[0]; i++) {
        base->word_str[i] = ckd_salloc(tmp_word_str + j);
        if (hash_table_enter(base->wid, base->word_str[i],
                             (void *) (size_t) i) != (void *) (size_t) i) {
            E_WARN("Duplicate word in dictionary: %s\n",
                   base->word_str[i]);
        }
        j += strlen(base->word_str[i]) + 1;
    }
    free(tmp_word_str);
}

ngram_model_t *
ngram_model_trie_read_bin(ps_config_t * config,
                          const char *path, logmath_t * lmath)
{
    int32 is_pipe;
    FILE *fp;
    size_t hdr_size;
    char *hdr;
    int cmp_res;
    uint8 i, order;
    uint32 counts[NGRAM_MAX_ORDER];
    ngram_model_trie_t *model;
    ngram_model_t *base;

    (void)config;
    E_INFO("Trying to read LM in trie binary format\n");
    if ((fp = fopen_comp(path, "rb", &is_pipe)) == NULL) {
        E_ERROR("File %s not found\n", path);
        return NULL;
    }
    hdr_size = strlen(trie_hdr);
    hdr = (char *) ckd_calloc(hdr_size + 1, sizeof(*hdr));
    fread(hdr, sizeof(*hdr), hdr_size, fp);
    cmp_res = strcmp(hdr, trie_hdr);
    ckd_free(hdr);
    if (cmp_res) {
        E_INFO("Header doesn't match\n");
        fclose_comp(fp, is_pipe);
        return NULL;
    }
    model = (ngram_model_trie_t *) ckd_calloc(1, sizeof(*model));
    base = &model->base;
    fread(&order, sizeof(order), 1, fp);
    for (i = 0; i < order; i++) {
        fread(&counts[i], sizeof(counts[i]), 1, fp);
        if (SWAP_LM_TRIE)
            SWAP_INT32(&counts[i]);
        E_INFO("#%d-grams: %d\n", i + 1, counts[i]);
    }
    ngram_model_init(base, &ngram_model_trie_funcs, lmath, order,
                     (int32) counts[0]);
    for (i = 0; i < order; i++) {
        base->n_counts[i] = counts[i];
    }

    model->trie = lm_trie_read_bin(counts, order, fp);
    read_word_str(base, fp, SWAP_LM_TRIE);
    fclose_comp(fp, is_pipe);

    return base;
}

static void
write_word_str(FILE * fp, ngram_model_t * model, int do_swap)
{
    int32 k;
    uint32 i;

    k = 0;
    for (i = 0; i < model->n_counts[0]; i++)
        k += strlen(model->word_str[i]) + 1;
    E_INFO("#word_str: %d\n", k);
    if (do_swap)
        SWAP_INT32(&k);
    fwrite(&k, sizeof(k), 1, fp);
    for (i = 0; i < model->n_counts[0]; i++)
        fwrite(model->word_str[i], 1, strlen(model->word_str[i]) + 1, fp);
}

int
ngram_model_trie_write_bin(ngram_model_t * base, const char *path)
{
    int i;
    int32 is_pipe;
    ngram_model_trie_t *model = (ngram_model_trie_t *) base;
    FILE *fp = fopen_comp(path, "wb", &is_pipe);
    if (!fp) {
        E_ERROR("Unable to open %s to write binary trie LM\n", path);
        return -1;
    }

    fwrite(trie_hdr, sizeof(*trie_hdr), strlen(trie_hdr), fp);
    fwrite(&model->base.n, sizeof(model->base.n), 1, fp);
    for (i = 0; i < model->base.n; i++) {
        uint32 count = model->base.n_counts[i];
        if (SWAP_LM_TRIE)
            SWAP_INT32(&count);
        fwrite(&count, sizeof(count), 1, fp);
    }
    lm_trie_write_bin(model->trie, base->n_counts[0], fp);
    write_word_str(fp, base, SWAP_LM_TRIE);
    fclose_comp(fp, is_pipe);
    return 0;
}

ngram_model_t *
ngram_model_trie_read_dmp(ps_config_t * config,
                          const char *file_name, logmath_t * lmath)
{
    uint8 do_swap;
    int32 is_pipe;
    int32 k;
    uint32 j;
    int32 vn, ts;
    int32 count;
    uint32 counts[3];
    uint32 *unigram_next;
    int order;
    char str[1024];
    FILE *fp;
    ngram_model_trie_t *model;
    ngram_model_t *base;
    ngram_raw_t **raw_ngrams;

    (void)config;
    E_INFO("Trying to read LM in dmp format\n");
    if ((fp = fopen_comp(file_name, "rb", &is_pipe)) == NULL) {
        E_ERROR("Dump file %s not found\n", file_name);
        return NULL;
    }

    do_swap = FALSE;
    fread(&k, sizeof(k), 1, fp);
    if (k != strlen(dmp_hdr) + 1) {
        SWAP_INT32(&k);
        if (k != strlen(dmp_hdr) + 1) {
            E_ERROR
                ("Wrong magic header size number %x: %s is not a dump file\n",
                 k, file_name);
            return NULL;
        }
        do_swap = 1;
    }
    if (fread(str, 1, k, fp) != (size_t) k) {
        E_ERROR("Cannot read header\n");
        return NULL;
    }
    if (strncmp(str, dmp_hdr, k) != 0) {
        E_ERROR("Wrong header %s: %s is not a dump file\n", dmp_hdr);
        return NULL;
    }

    if (fread(&k, sizeof(k), 1, fp) != 1)
        return NULL;
    if (do_swap)
        SWAP_INT32(&k);
    if (fread(str, 1, k, fp) != (size_t) k) {
        E_ERROR("Cannot read LM filename in header\n");
        return NULL;
    }

    /* read version#, if present (must be <= 0) */
    if (fread(&vn, sizeof(vn), 1, fp) != 1)
        return NULL;
    if (do_swap)
        SWAP_INT32(&vn);
    if (vn <= 0) {
        /* read and don't compare timestamps (we don't care) */
        if (fread(&ts, sizeof(ts), 1, fp) != 1)
            return NULL;
        if (do_swap)
            SWAP_INT32(&ts);

        /* read and skip format description */
        for (;;) {
            if (fread(&k, sizeof(k), 1, fp) != 1)
                return NULL;
            if (do_swap)
                SWAP_INT32(&k);
            if (k == 0)
                break;
            if (fread(str, 1, k, fp) != (size_t) k) {
                E_ERROR("Failed to read word\n");
                return NULL;
            }
        }
        /* read model->ucount */
        if (fread(&count, sizeof(count), 1, fp) != 1)
            return NULL;
        if (do_swap)
            SWAP_INT32(&count);
        counts[0] = count;
    }
    else {
        counts[0] = vn;
    }
    /* read model->bcount, tcount */
    if (fread(&count, sizeof(count), 1, fp) != 1)
        return NULL;
    if (do_swap)
        SWAP_INT32(&count);
    counts[1] = count;
    if (fread(&count, sizeof(count), 1, fp) != 1)
        return NULL;
    if (do_swap)
        SWAP_INT32(&count);
    counts[2] = count;
    E_INFO("ngrams 1=%d, 2=%d, 3=%d\n", counts[0], counts[1], counts[2]);

    model = (ngram_model_trie_t *) ckd_calloc(1, sizeof(*model));
    base = &model->base;
    if (counts[2] > 0)
        order = 3;
    else if (counts[1] > 0)
        order = 2;
    else
        order = 1;
    ngram_model_init(base, &ngram_model_trie_funcs, lmath, order,
                     (int32) counts[0]);

    model->trie = lm_trie_create(counts[0], order);

    unigram_next =
        (uint32 *) ckd_calloc((int32) counts[0] + 1, sizeof(unigram_next));
    for (j = 0; j <= counts[0]; j++) {
        int32 bigrams;
        int32 mapid;
        dmp_weight_t weightp;
        dmp_weight_t weightb;

        /* Skip over the mapping ID, we don't care about it. */
        /* Read the weights from actual unigram structure. */
        fread(&mapid, sizeof(int32), 1, fp);
        fread(&weightp, sizeof(weightp), 1, fp);
        fread(&weightb, sizeof(weightb), 1, fp);
        fread(&bigrams, sizeof(int32), 1, fp);
        if (do_swap) {
            SWAP_INT32(&weightp.l);
            SWAP_INT32(&weightb.l);
            SWAP_INT32(&bigrams);
        }
        model->trie->unigrams[j].prob = logmath_log10_to_log_float(lmath, weightp.f);
        model->trie->unigrams[j].bo = logmath_log10_to_log_float(lmath, weightb.f);
        model->trie->unigrams[j].next = bigrams;
        unigram_next[j] = bigrams;
    }

    if (order > 1) {
        raw_ngrams =
            ngrams_raw_read_dmp(fp, lmath, counts, order, unigram_next,
                                do_swap);
        if (raw_ngrams == NULL) {
            ngram_model_free(base);
            ckd_free(unigram_next);
            fclose_comp(fp, is_pipe);
            return NULL;
        }
        lm_trie_build(model->trie, raw_ngrams, counts, base->n_counts, order);        
        ngrams_raw_free(raw_ngrams, counts, order);
    }
    
    /* Sentinel unigram and bigrams read before */
    ckd_free(unigram_next);

    /* read ascii word strings */
    read_word_str(base, fp, do_swap);

    fclose_comp(fp, is_pipe);
    return base;
}

static void
ngram_model_trie_free(ngram_model_t * base)
{
    ngram_model_trie_t *model = (ngram_model_trie_t *) base;
    lm_trie_free(model->trie);
}

static int
trie_apply_weights(ngram_model_t * base, float32 lw, float32 wip)
{
    /* just update weights that are going to be used on score calculation */
    base->lw = lw;
    base->log_wip = logmath_log(base->lmath, wip);
    return 0;
}

static int32
weight_score(ngram_model_t * base, int32 score)
{
    return (int32) (score * base->lw + base->log_wip);
}

static int32
ngram_model_trie_raw_score(ngram_model_t * base, int32 wid, int32 * hist,
                           int32 n_hist, int32 * n_used)
{
    int32 i;
    ngram_model_trie_t *model = (ngram_model_trie_t *) base;

    if (n_hist > model->base.n - 1)
        n_hist = model->base.n - 1;
    for (i = 0; i < n_hist; i++) {
        if (hist[i] < 0) {
            n_hist = i;
            break;
        }
    }

    return (int32) lm_trie_score(model->trie, model->base.n, wid, hist,
                                 n_hist, n_used);
}

static int32
ngram_model_trie_score(ngram_model_t * base, int32 wid, int32 * hist,
                       int32 n_hist, int32 * n_used)
{
    return weight_score(base,
                        ngram_model_trie_raw_score(base, wid, hist, n_hist,
                                                   n_used));
}

static int32
lm_trie_add_ug(ngram_model_t * base, int32 wid, int32 lweight)
{
    ngram_model_trie_t *model = (ngram_model_trie_t *) base;

    /* This would be very bad if this happened! */
    assert(!NGRAM_IS_CLASSWID(wid));

    /* Reallocate unigram array. */
    model->trie->unigrams =
        (unigram_t *) ckd_realloc(model->trie->unigrams,
                                  sizeof(*model->trie->unigrams) *
                                  (base->n_1g_alloc + 1));
    memset(model->trie->unigrams + (base->n_counts[0] + 1), 0,
           (size_t) (base->n_1g_alloc -
                     base->n_counts[0]) * sizeof(*model->trie->unigrams));
    ++base->n_counts[0];
    lweight += logmath_log(base->lmath, 1.0 / base->n_counts[0]);
    model->trie->unigrams[wid + 1].next = model->trie->unigrams[wid].next;
    model->trie->unigrams[wid].prob = (float) lweight;
    /* This unigram by definition doesn't participate in any bigrams,
     * so its backoff weight is undefined and next pointer same as in finish unigram*/
    model->trie->unigrams[wid].bo = 0;
    /* Finally, increase the unigram count */
    /* FIXME: Note that this can actually be quite bogus due to the
     * presence of class words.  If wid falls outside the unigram
     * count, increase it to compensate, at the cost of no longer
     * really knowing how many unigrams we have :( */
    if ((uint32) wid >= base->n_counts[0])
        base->n_counts[0] = wid + 1;

    return (int32) weight_score(base, lweight);
}

static void
lm_trie_flush(ngram_model_t * base)
{
    ngram_model_trie_t *model = (ngram_model_trie_t *) base;
    lm_trie_t *trie = model->trie;
    memset(trie->hist_cache, -1, sizeof(trie->hist_cache));
    memset(trie->backoff_cache, 0, sizeof(trie->backoff_cache));
    return;
}

static ngram_funcs_t ngram_model_trie_funcs = {
    ngram_model_trie_free,      /* free */
    trie_apply_weights,         /* apply_weights */
    ngram_model_trie_score,     /* score */
    ngram_model_trie_raw_score, /* raw_score */
    lm_trie_add_ug,             /* add_ug */
    lm_trie_flush               /* flush */
};
