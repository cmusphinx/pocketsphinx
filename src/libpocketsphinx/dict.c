/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 1999-2004 Carnegie Mellon University.  All rights
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

/* System headers. */
#include <string.h>

/* SphinxBase headers. */
#include <sphinxbase/pio.h>
#include <sphinxbase/strfuncs.h>

/* Local headers. */
#include "dict.h"
#include <sphinxbase/ngram_model.h>
#include <sphinxbase/array_heap.h>


#define DELIM	" \t\n"         /* Set of field separator characters */
#define DEFAULT_NUM_PHONE	(MAX_S3CIPID+1)

#if WIN32
#define snprintf sprintf_s
#endif 

extern const char *const cmu6_lts_phone_table[];

static s3cipid_t
dict_ciphone_id(dict_t * d, const char *str)
{
    if (d->nocase)
        return bin_mdef_ciphone_id_nocase(d->mdef, str);
    else
        return bin_mdef_ciphone_id(d->mdef, str);
}


const char *
dict_ciphone_str(dict_t * d, s3wid_t wid, int32 pos)
{
    assert(d != NULL);
    assert((wid >= 0) && (wid < d->n_word));
    assert((pos >= 0) && (pos < d->word[wid].pronlen));

    return bin_mdef_ciphone_str(d->mdef, d->word[wid].ciphone[pos]);
}


s3wid_t
dict_add_word(dict_t * d, char const *word, s3cipid_t const * p, int32 np)
{
    int32 len;
    dictword_t *wordp;
    s3wid_t newwid;
    char *wword;

    if (d->n_word >= d->max_words) {
        E_INFO("Reallocating to %d KiB for word entries\n",
               (d->max_words + S3DICT_INC_SZ) * sizeof(dictword_t) / 1024);
        d->word =
            (dictword_t *) ckd_realloc(d->word,
                                       (d->max_words +
                                        S3DICT_INC_SZ) * sizeof(dictword_t));
        d->max_words = d->max_words + S3DICT_INC_SZ;
    }

    wordp = d->word + d->n_word;
    wordp->word = (char *) ckd_salloc(word);    /* Freed in dict_free */

    /* Determine base/alt wids */
    wword = ckd_salloc(word);
    if ((len = dict_word2basestr(wword)) > 0) {
        int32 w;

        /* Truncated to a baseword string; find its ID */
        if (hash_table_lookup_int32(d->ht, wword, &w) < 0) {
            E_ERROR("Missing base word for: %s\n", word);
            ckd_free(wword);
            ckd_free(wordp->word);
            wordp->word = NULL;
            return BAD_S3WID;
        }

        /* Link into alt list */
        wordp->basewid = w;
        wordp->alt = d->word[w].alt;
        d->word[w].alt = d->n_word;
    } else {
        wordp->alt = BAD_S3WID;
        wordp->basewid = d->n_word;
    }
    ckd_free(wword);

    /* Associate word string with d->n_word in hash table */
    if (hash_table_enter_int32(d->ht, wordp->word, d->n_word) != d->n_word) {
        ckd_free(wordp->word);
        wordp->word = NULL;
        return BAD_S3WID;
    }

    /* Fill in word entry, and set defaults */
    if (p && (np > 0)) {
        wordp->ciphone = (s3cipid_t *) ckd_malloc(np * sizeof(s3cipid_t));      /* Freed in dict_free */
        memcpy(wordp->ciphone, p, np * sizeof(s3cipid_t));
        wordp->pronlen = np;
    }
    else {
        wordp->ciphone = NULL;
        wordp->pronlen = 0;
    }

    newwid = d->n_word++;

    return newwid;
}


static int32
dict_read(FILE * fp, dict_t * d)
{
    lineiter_t *li;
    char **wptr;
    s3cipid_t *p;
    int32 lineno, nwd;
    s3wid_t w;
    int32 i, maxwd;
    size_t stralloc, phnalloc;

    maxwd = 512;
    p = (s3cipid_t *) ckd_calloc(maxwd + 4, sizeof(*p));
    wptr = (char **) ckd_calloc(maxwd, sizeof(char *)); /* Freed below */

    lineno = 0;
    stralloc = phnalloc = 0;
    for (li = lineiter_start(fp); li; li = lineiter_next(li)) {
        lineno++;
        if (0 == strncmp(li->buf, "##", 2)
            || 0 == strncmp(li->buf, ";;", 2))
            continue;

        if ((nwd = str2words(li->buf, wptr, maxwd)) < 0) {
            /* Increase size of p, wptr. */
            nwd = str2words(li->buf, NULL, 0);
            assert(nwd > maxwd); /* why else would it fail? */
            maxwd = nwd;
            p = (s3cipid_t *) ckd_realloc(p, (maxwd + 4) * sizeof(*p));
            wptr = (char **) ckd_realloc(wptr, maxwd * sizeof(*wptr));
        }

        if (nwd == 0)           /* Empty line */
            continue;
        /* wptr[0] is the word-string and wptr[1..nwd-1] the pronunciation sequence */
        if (nwd == 1) {
            E_ERROR("Line %d: No pronunciation for word '%s'; ignored\n",
                    lineno, wptr[0]);
            continue;
        }


        /* Convert pronunciation string to CI-phone-ids */
        for (i = 1; i < nwd; i++) {
            p[i - 1] = dict_ciphone_id(d, wptr[i]);
            if (NOT_S3CIPID(p[i - 1])) {
                E_ERROR("Line %d: Phone '%s' is mising in the acoustic model; word '%s' ignored\n",
                        lineno, wptr[i], wptr[0]);
                break;
            }
        }

        if (i == nwd) {         /* All CI-phones successfully converted to IDs */
            w = dict_add_word(d, wptr[0], p, nwd - 1);
            if (NOT_S3WID(w))
                E_ERROR
                    ("Line %d: Failed to add the word '%s' (duplicate?); ignored\n",
                     lineno, wptr[0]);
            else {
                stralloc += strlen(d->word[w].word);
                phnalloc += d->word[w].pronlen * sizeof(s3cipid_t);
            }
        }
    }
    E_INFO("Allocated %d KiB for strings, %d KiB for phones\n",
           (int)stralloc / 1024, (int)phnalloc / 1024);
    ckd_free(p);
    ckd_free(wptr);

    return 0;
}

int
dict_write(dict_t *dict, char const *filename, char const *format)
{
    FILE *fh;
    int i;

    if ((fh = fopen(filename, "w")) == NULL) {
        E_ERROR_SYSTEM("Failed to open '%s'", filename);
        return -1;
    }
    for (i = 0; i < dict->n_word; ++i) {
        char *phones;
        int j, phlen;
        if (!dict_real_word(dict, i))
            continue;
        for (phlen = j = 0; j < dict_pronlen(dict, i); ++j)
            phlen += strlen(dict_ciphone_str(dict, i, j)) + 1;
        phones = ckd_calloc(1, phlen);
        for (j = 0; j < dict_pronlen(dict, i); ++j) {
            strcat(phones, dict_ciphone_str(dict, i, j));
            if (j != dict_pronlen(dict, i) - 1)
                strcat(phones, " ");
        }
        fprintf(fh, "%-30s %s\n", dict_wordstr(dict, i), phones);
        ckd_free(phones);
    }
    fclose(fh);
    return 0;
}


dict_t *
dict_init(cmd_ln_t *config, bin_mdef_t * mdef, logmath_t *logmath)
{
    FILE *fp, *fp2, *fp3;
    int32 n;
    lineiter_t *li;
    dict_t *d;
    s3cipid_t sil;
    char const *dictfile = NULL, *fillerfile = NULL;
    char *arpafile = NULL;

    if (config) {
        dictfile = cmd_ln_str_r(config, "-dict");
        fillerfile = cmd_ln_str_r(config, "-fdict");
        arpafile = string_join(dictfile, ".dmp",  NULL);
    }

    /*
     * First obtain #words in dictionary (for hash table allocation).
     * Reason: The PC NT system doesn't like to grow memory gradually.  Better to allocate
     * all the required memory in one go.
     */
    fp = NULL;
    n = 0;
    if (dictfile) {
        if ((fp = fopen(dictfile, "r")) == NULL) {
            E_ERROR_SYSTEM("Failed to open dictionary file '%s' for reading", dictfile);
            return NULL;
        }
        for (li = lineiter_start(fp); li; li = lineiter_next(li)) {
            if (0 != strncmp(li->buf, "##", 2)
                && 0 != strncmp(li->buf, ";;", 2))
                n++;
        }
	fseek(fp, 0L, SEEK_SET);
    }

    fp2 = NULL;
    if (fillerfile) {
        if ((fp2 = fopen(fillerfile, "r")) == NULL) {
            E_ERROR_SYSTEM("Failed to open filler dictionary file '%s' for reading", fillerfile);
            fclose(fp);
            return NULL;
	}
        for (li = lineiter_start(fp2); li; li = lineiter_next(li)) {
	    if (0 != strncmp(li->buf, "##", 2)
    	        && 0 != strncmp(li->buf, ";;", 2))
                n++;
        }
        fseek(fp2, 0L, SEEK_SET);
    }

    /*
     * Allocate dict entries.  HACK!!  Allow some extra entries for words not in file.
     * Also check for type size restrictions.
     */
    d = (dict_t *) ckd_calloc(1, sizeof(dict_t));       /* freed in dict_free() */

    if (arpafile) {
        fp3 = NULL;
        if ((fp3 = fopen(arpafile, "r")) == NULL) {
            E_INFO("No arpa model found.\n");
            d->ngram_g2p_model = NULL;
        } else {
            fclose(fp3);
            ngram_model_t *ngram_g2p_model = ngram_model_read(NULL,arpafile,NGRAM_AUTO,logmath);
            if (!ngram_g2p_model) {
                E_ERROR("Arpa model is broken\n");
                return NULL;
            }
            d->ngram_g2p_model = ngram_g2p_model;
        }
        ckd_free(arpafile);
    }

    d->refcnt = 1;
    d->max_words =
        (n + S3DICT_INC_SZ < MAX_S3WID) ? n + S3DICT_INC_SZ : MAX_S3WID;
    if (n >= MAX_S3WID) {
        E_ERROR("Number of words in dictionaries (%d) exceeds limit (%d)\n", n,
                MAX_S3WID);
        fclose(fp);
        fclose(fp2);
        ckd_free(d);
        return NULL;
    }

    E_INFO("Allocating %d * %d bytes (%d KiB) for word entries\n",
           d->max_words, sizeof(dictword_t),
           d->max_words * sizeof(dictword_t) / 1024);
    d->word = (dictword_t *) ckd_calloc(d->max_words, sizeof(dictword_t));      /* freed in dict_free() */
    d->n_word = 0;
    if (mdef)
        d->mdef = bin_mdef_retain(mdef);

    /* Create new hash table for word strings; case-insensitive word strings */
    if (config && cmd_ln_exists_r(config, "-dictcase"))
        d->nocase = cmd_ln_boolean_r(config, "-dictcase");
    d->ht = hash_table_new(d->max_words, d->nocase);

    /* Digest main dictionary file */
    if (fp) {
        E_INFO("Reading main dictionary: %s\n", dictfile);
        dict_read(fp, d);
        fclose(fp);
        E_INFO("%d words read\n", d->n_word);
    }

    if (dict_wordid(d, S3_START_WORD) != BAD_S3WID) {
	E_ERROR("Remove sentence start word '<s>' from the dictionary\n");
	dict_free(d);
	return NULL;
    }
    if (dict_wordid(d, S3_FINISH_WORD) != BAD_S3WID) {
	E_ERROR("Remove sentence start word '</s>' from the dictionary\n");
	dict_free(d);
	return NULL;
    }
    if (dict_wordid(d, S3_SILENCE_WORD) != BAD_S3WID) {
	E_ERROR("Remove silence word '<sil>' from the dictionary\n");
	dict_free(d);
	return NULL;
    }

    /* Now the filler dictionary file, if it exists */
    d->filler_start = d->n_word;
    if (fillerfile) {
        E_INFO("Reading filler dictionary: %s\n", fillerfile);
        dict_read(fp2, d);
        fclose(fp2);
        E_INFO("%d words read\n", d->n_word - d->filler_start);
    }
    if (mdef)
        sil = bin_mdef_silphone(mdef);
    else
        sil = 0;
    if (dict_wordid(d, S3_START_WORD) == BAD_S3WID) {
        dict_add_word(d, S3_START_WORD, &sil, 1);
    }
    if (dict_wordid(d, S3_FINISH_WORD) == BAD_S3WID) {
        dict_add_word(d, S3_FINISH_WORD, &sil, 1);
    }
    if (dict_wordid(d, S3_SILENCE_WORD) == BAD_S3WID) {
        dict_add_word(d, S3_SILENCE_WORD, &sil, 1);
    }

    d->filler_end = d->n_word - 1;

    /* Initialize distinguished word-ids */
    d->startwid = dict_wordid(d, S3_START_WORD);
    d->finishwid = dict_wordid(d, S3_FINISH_WORD);
    d->silwid = dict_wordid(d, S3_SILENCE_WORD);

    if ((d->filler_start > d->filler_end)
        || (!dict_filler_word(d, d->silwid))) {
        E_ERROR("Word '%s' must occur (only) in filler dictionary\n",
                S3_SILENCE_WORD);
        dict_free(d);
        return NULL;
    }

    /* No check that alternative pronunciations for filler words are in filler range!! */

    return d;
}


s3wid_t
dict_wordid(dict_t *d, const char *word)
{
    int32 w;

    assert(d);
    assert(word);

    if (hash_table_lookup_int32(d->ht, word, &w) < 0)
        return (BAD_S3WID);
    return w;
}


int
dict_filler_word(dict_t *d, s3wid_t w)
{
    assert(d);
    assert((w >= 0) && (w < d->n_word));

    w = dict_basewid(d, w);
    if ((w == d->startwid) || (w == d->finishwid))
        return 0;
    if ((w >= d->filler_start) && (w <= d->filler_end))
        return 1;
    return 0;
}

int
dict_real_word(dict_t *d, s3wid_t w)
{
    assert(d);
    assert((w >= 0) && (w < d->n_word));

    w = dict_basewid(d, w);
    if ((w == d->startwid) || (w == d->finishwid))
        return 0;
    if ((w >= d->filler_start) && (w <= d->filler_end))
        return 0;
    return 1;
}


int32
dict_word2basestr(char *word)
{
    int32 i, len;

    len = strlen(word);
    if (word[len - 1] == ')') {
        for (i = len - 2; (i > 0) && (word[i] != '('); --i);

        if (i > 0) {
            /* The word is of the form <baseword>(...); strip from left-paren */
            word[i] = '\0';
            return i;
        }
    }

    return -1;
}

dict_t *
dict_retain(dict_t *d)
{
    ++d->refcnt;
    return d;
}

int
dict_free(dict_t * d)
{
    int i;
    dictword_t *word;

    if (d == NULL)
        return 0;
    if (--d->refcnt > 0)
        return d->refcnt;

    /* First Step, free all memory allocated for each word */
    for (i = 0; i < d->n_word; i++) {
        word = (dictword_t *) & (d->word[i]);
        if (word->word)
            ckd_free((void *) word->word);
        if (word->ciphone)
            ckd_free((void *) word->ciphone);
    }

    if (d->word)
        ckd_free((void *) d->word);
    if (d->ht)
        hash_table_free(d->ht);
    if (d->mdef)
        bin_mdef_free(d->mdef);
    if (d->ngram_g2p_model)
        ngram_model_free(d->ngram_g2p_model);

    ckd_free((void *) d);

    return 0;
}

void
dict_report(dict_t * d)
{
    E_INFO_NOFN("Initialization of dict_t, report:\n");
    E_INFO_NOFN("Max word: %d\n", d->max_words);
    E_INFO_NOFN("No of word: %d\n", d->n_word);
    E_INFO_NOFN("\n");
}

typedef struct dict_g2p_tree_element_s {
    int32 wid;
    int32 probability;
    struct dict_g2p_tree_element_s *parent;
} dict_g2p_tree_element_t;

#define U8_IS_LEAD(c) ((uint8_t)((c)-0xc0)<0x3e)

#define U8_IS_SINGLE(c) (((c)&0x80)==0)

#define U8_IS_TRAIL(c) (((c)&0xc0)==0x80)

uint32 dict_utf8_length(const char* string) {
    uint32 length = 0;
    while (*string) {
        if (U8_IS_LEAD(*string) || U8_IS_SINGLE(*string)) {
            length++;
        }
        string++;
    }
    return length;
}

dict_g2p_tree_element_t *dict_g2p_tree_element_new(int32 wid, int32 probability, dict_g2p_tree_element_t* parent) {
    dict_g2p_tree_element_t *tree_element = ckd_malloc(sizeof(dict_g2p_tree_element_t));
    tree_element->wid = wid;
    tree_element->parent = parent;
    tree_element->probability = probability;
    return tree_element;
}

int
dict_graphemes_fit_count(const char *word, const char* unigram_text) {
    int32 count = 0;

    while (*word && *unigram_text && *unigram_text != '<' && *unigram_text != '}') {
        if (*unigram_text == '|') {
            unigram_text++;
        }
        if (*word != *unigram_text) {
            return 0;
        }
        if (U8_IS_SINGLE(*word) || U8_IS_LEAD(*word)) {
            count++;
        }
        unigram_text++;
        word++;
    }
    return count;
}

char *
dict_unwind_phoneme(ngram_model_t *model, dict_g2p_tree_element_t *tree_element) {
    int32 i, j, size = 0;
    char* phoneme;
    const char* unigram_phoneme;
    dict_g2p_tree_element_t *element = tree_element;

    while (element) {
        unigram_phoneme = strchr(ngram_word(model, element->wid), '}') + 1;
        if (strcmp(unigram_phoneme, "_") != 0) {
            size += strlen(unigram_phoneme) + 1;
        }
        element = element->parent;
    }

    phoneme = ckd_malloc(size);
    phoneme[size - 1] = '\0';
    i = size - 2;

    element = tree_element;
    while (element) {
        unigram_phoneme = strchr(ngram_word(model, element->wid), '}') + 1;
        if (strcmp(unigram_phoneme, "_") != 0) {
            i -= strlen(unigram_phoneme);
            j = i + 1;
            while (*unigram_phoneme) {
                phoneme[j] = *unigram_phoneme == '|' ? ' ' : *unigram_phoneme;
                j++;
                unigram_phoneme++;
            }
            if (i >= 0) {
                phoneme[i] = ' ';
            }
            i--;
        }
        element = element->parent;
    }
    return phoneme;
}

void
dict_try_add_tree_element(ngram_model_t *model, int32 wid, int32 *history, int32 history_size,
        dict_g2p_tree_element_t *tree_element_from, array_heap_t *heap) {
    int32 nused;
    int32 probability = ngram_ng_prob(model, wid, history, history_size, &nused);
    if (tree_element_from) {
        probability += tree_element_from->probability;
    }
    if (!array_heap_full(heap)) {
        array_heap_add(heap, probability, dict_g2p_tree_element_new(wid, probability, tree_element_from));
    } else if (array_heap_min_key(heap) < probability) {
        ckd_free(array_heap_pop(heap));
        array_heap_add(heap, probability, dict_g2p_tree_element_new(wid, probability, tree_element_from));
    }
}

int32
dict_unwind_history(int32 *history, dict_g2p_tree_element_t *tree_element, int32 start_wid) {
    int32 i = 0;

    while (tree_element) {
        history[i] = tree_element->wid;
        tree_element = tree_element->parent;
        i++;
    }
    history[i] = start_wid;
    return i + 1;
}

void
dict_try_add_tree_elements(ngram_model_t *model, int32 wid, array_heap_t *previous, array_heap_t *heap_to_fill,
        int32 *history_buffer, int32 start_wid) {
    int32 i, history_size;

    if (previous == NULL) {
        history_buffer[0] = start_wid;
        dict_try_add_tree_element(model, wid, history_buffer, 1, NULL, heap_to_fill);
    } else {
        for (i = 0; i < previous->size; i++) {
            dict_g2p_tree_element_t *tree_element = array_heap_element(previous, i);
            history_size = dict_unwind_history(history_buffer, tree_element, start_wid);
            dict_try_add_tree_element(model, wid, history_buffer, history_size, tree_element, heap_to_fill);
        }
    }
}

char *
dict_g2p(ngram_model_t *model, const char *grapheme, uint32 search_width) {
    int32 i, j, n, wid, fit_count;
    array_heap_t **tree_table;
    const char* unigram_text;
    char *phoneme;
    const char *current_char_p;
    int32 *history_buffer;
    int32 start_wid, end_wid;
    const uint32 total_unigrams = *ngram_model_get_counts(model);

    n = dict_utf8_length(grapheme);
    tree_table = ckd_calloc(n + 1, sizeof(array_heap_t *));
    for (i = 0; i < n; i++) {
        tree_table[i] = array_heap_new(search_width);
    }
    tree_table[n] = array_heap_new(1);
    history_buffer = ckd_calloc(n + 1, sizeof(int32));
    start_wid = ngram_wid(model, "<s>");
    end_wid = ngram_wid(model, "</s>");

    current_char_p = grapheme;
    for (i = 0; i < n; i++) {
        for (wid = 0; wid < total_unigrams; wid++) {
            unigram_text = ngram_word(model, wid);
            fit_count = dict_graphemes_fit_count(current_char_p, unigram_text);
            if (fit_count != 0) {
                dict_try_add_tree_elements(model, wid, i == 0 ? NULL : tree_table[i - 1], tree_table[i + fit_count - 1],
                        history_buffer, start_wid);
            }
        }
        current_char_p++;
        while (*current_char_p && U8_IS_TRAIL(*current_char_p)) {
            current_char_p++;
        }
    }

    dict_try_add_tree_elements(model, end_wid, tree_table[n - 1], tree_table[n], history_buffer, start_wid);

    phoneme = (tree_table[n]->size == 0) ? NULL : dict_unwind_phoneme(model,
            ((dict_g2p_tree_element_t*) array_heap_element(tree_table[n], 0))->parent);

    for (i = 0; i <= n; i++) {
        for (j = 0; j < tree_table[i]->size; j++) {
            ckd_free(array_heap_element(tree_table[i], j));
        }
        array_heap_free(tree_table[i]);
    }
    ckd_free(tree_table);
    ckd_free(history_buffer);
    return phoneme;
}

/**
 * This functions just receives the dict lacking word from fsg_search, call the main function dict_g2p, and then add the word to the memory dict.
 * The second part of this function is the same as pocketsphinx.c: https://github.com/cmusphinx/pocketsphinx/blob/ba6bd21b3601339646d2db6d2297d02a8a6b7029/src/libpocketsphinx/pocketsphinx.c#L816
 */
int
dict_add_g2p_word(dict_t *dict, char const *word)
{
    int32 wid = 0;
    s3cipid_t *pron;
    char **phonestr, *tmp;
    int np, i;
    char *phones;

    phones = dict_g2p(dict->ngram_g2p_model, word, G2P_SEARCH_WIDTH);
    if (phones == NULL)
        return 0;

    E_INFO("Adding phone %s for word %s \n",  phones, word);
    tmp = ckd_salloc(phones);
    np = str2words(tmp, NULL, 0);
    phonestr = ckd_calloc(np, sizeof(*phonestr));
    str2words(tmp, phonestr, np);
    pron = ckd_calloc(np, sizeof(*pron));
    for (i = 0; i < np; ++i) {
        pron[i] = bin_mdef_ciphone_id(dict->mdef, phonestr[i]);
        if (pron[i] == -1) {
            E_ERROR("Unknown phone %s in phone string %s\n",
                    phonestr[i], tmp);
            ckd_free(phonestr);
            ckd_free(tmp);
            ckd_free(pron);
            ckd_free(phones);
            return -1;
        }
    }
    ckd_free(phonestr);
    ckd_free(tmp);
    ckd_free(phones);
    if ((wid = dict_add_word(dict, word, pron, np)) == -1) {
        ckd_free(pron);
        return -1;
    }
    ckd_free(pron);

    return wid;
}
