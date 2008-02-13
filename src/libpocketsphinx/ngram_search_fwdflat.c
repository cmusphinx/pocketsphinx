/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 2008 Carnegie Mellon University.  All rights
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

/**
 * @file ngram_search_fwdflat.c Flat lexicon search.
 */

/* System headers. */

/* SphinxBase headers. */
#include <ckd_alloc.h>
#include <linklist.h>

/* Local headers. */
#include "ngram_search.h"

void
ngram_fwdflat_init(ngram_search_t *ngs)
{
    int n_words;

    n_words = ngs->dict->dict_entry_count;
    ngs->fwdflat_wordlist = ckd_calloc(n_words + 1, sizeof(*ngs->fwdflat_wordlist));
    /* FIXME: This should be a bitvector. */
    ngs->expand_word_flag = ckd_calloc(n_words, sizeof(*ngs->expand_word_flag));
    ngs->expand_word_list = ckd_calloc(n_words + 1, sizeof(*ngs->expand_word_list));

    ngs->min_ef_width = cmd_ln_int32_r(ngs->config, "-fwdflatefwid");
    ngs->max_sf_win = cmd_ln_int32_r(ngs->config, "-fwdflatsfwin");
    E_INFO("fwdflat: min_ef_width = %d, max_sf_win = %d\n",
           ngs->min_ef_width, ngs->max_sf_win);
}

void
ngram_fwdflat_deinit(ngram_search_t *ngs)
{
    ckd_free(ngs->fwdflat_wordlist);
    ckd_free(ngs->expand_word_flag);
    ckd_free(ngs->expand_word_list);
}
