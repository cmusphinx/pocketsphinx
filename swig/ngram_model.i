/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 2013 Carnegie Mellon University.  All rights
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


%extend NGramModel {

    static NGramModel *fromIter(void *itor) {
        const char *name;
	return ngram_model_set_iter_model((ngram_model_set_iter_t*)itor, &name);
    }

    NGramModel(const char *path) {
        logmath_t *lmath = logmath_init(1.0001, 0, 0);
        ngram_model_t * model = ngram_model_read(NULL, path, NGRAM_AUTO, lmath);
        logmath_free(lmath);
        return model;
    }

    NGramModel(Config *config, LogMath *logmath, const char *path) {
        return ngram_model_read(config, path, NGRAM_AUTO, logmath);
    }

    ~NGramModel() {
        ngram_model_free($self);
    }

    void write(const char *path, int ftype, int *errcode) {
        *errcode = ngram_model_write($self, path, (ngram_file_type_t)ftype);
    }

    int str_to_type(const char *str) {
        return ngram_str_to_type(str);
    }

    const char * type_to_str(int type) {
        return ngram_type_to_str(type);
    }

    void casefold(int kase, int *errcode) {
        *errcode = ngram_model_casefold($self, kase);
    }

    int size() {
        return ngram_model_get_size($self);
    }

    int add_word(const char *word, float weight) {
        return ngram_model_add_word($self, word, weight);
    }

    int prob(size_t n, char **ptr) {
        return ngram_prob($self, (const char * const *)ptr, n);
    }
}

// TODO: shares ptr type with NGramModel, docstrings are not generated
%extend NGramModelSet {
    NGramModelSet(Config *config, LogMath *logmath, const char *path) {
        return ngram_model_set_read(config, path, logmath);
    }

    ~NGramModelSet() {
        ngram_model_free($self);
    }

    int count() {
        return ngram_model_set_count($self);
    }

    NGramModel * add(
        NGramModel *model, const char *name, float weight, bool reuse_widmap) {
        return ngram_model_set_add($self, model, name, weight, reuse_widmap);
    }

    NGramModel * select(const char *name) {
        return ngram_model_set_select($self, name);
    }

    NGramModel * lookup(const char *name) {
        return ngram_model_set_lookup($self, name);
    }

    const char * current() {
        return ngram_model_set_current($self);
    }
}

