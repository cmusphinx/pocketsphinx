/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 1999-2001 Carnegie Mellon University.  All rights
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* SphinxBase headers. */
#include <err.h>
#include <ckd_alloc.h>
#include <cmd_ln.h>
#include <pio.h>
#include <ngram_model.h>
#include <logmath.h>
#include <strfuncs.h>

/* Local headers. */
#include "dict.h"
#include "s2_semi_mgau.h"
#include "ms_mgau.h"
#include "kb.h"
#include "phone.h"
#include "fbs.h"
#include "mdef.h"
#include "tmat.h"
#include "search.h"

/* Dictionary. */
dict_t *g_word_dict;

/* Transition matrices. */
tmat_t *g_tmat;

/* S3 model definition */
bin_mdef_t *g_mdef;

/* S2 fast SCGMM computation object */
s2_semi_mgau_t *g_semi_mgau;

/* Slow CDGMM computation object */
ms_mgau_model_t *g_ms_mgau;

/* Language model set */
ngram_model_t *g_lmset;

/* Log-math computation object */
logmath_t *g_lmath;

void
kb_update_widmap(void)
{
    const char **words;
    int32 i, n_words;

    /* It's okay to include fillers since they won't be in the LM */
    n_words = g_word_dict->dict_entry_count;
    words = ckd_calloc(n_words, sizeof(*words));
    for (i = 0; i < n_words; ++i)
        words[i] = g_word_dict->dict_list[i]->word;
    ngram_model_set_map_words(g_lmset, words, n_words);
    ckd_free(words);
}

static void
lm_init(void)
{
    char *lm_ctl_filename = cmd_ln_str("-lmctlfn");
    char *lm_file_name = cmd_ln_str("-lm");

    /* Do nothing if there is no language model. */
    if (lm_ctl_filename == NULL && lm_file_name == NULL)
        return;

    /* Read one or more language models. */
    if (lm_ctl_filename) {
        g_lmset = ngram_model_set_read(cmd_ln_get(),
                                       lm_ctl_filename,
                                       g_lmath);
        if (g_lmset == NULL) {
            E_FATAL("Failed to read language model control file: %s\n",
                    lm_ctl_filename);
        }
    }
    else {
        ngram_model_t *lm;
        static const char *name = "default";

        lm = ngram_model_read(cmd_ln_get(), lm_file_name,
                              NGRAM_AUTO, g_lmath);
        if (lm == NULL) {
            E_FATAL("Failed to read language model file: %s\n",
                    lm_file_name);
        }
        g_lmset = ngram_model_set_init(cmd_ln_get(),
                                       &lm, (char **)&name,
                                       NULL, 1);
        if (g_lmset == NULL) {
            E_FATAL("Failed to initialize language model set\n");
        }
    }

    /* Set the language model parameters. */
    ngram_model_apply_weights(g_lmset,
                              cmd_ln_float32("-lw"),
                              cmd_ln_float32("-wip"),
                              cmd_ln_float32("-uw"));

    /* Select a language model if requested. */
    if (cmd_ln_str("-lmname")) {
        if (ngram_model_set_select(g_lmset, cmd_ln_str("-lmname")) == NULL)
            E_FATAL("No such language model %s\n", cmd_ln_str("-lmname"));
    }

    /* Now create the dictionary to LM word ID mapping. */
    kb_update_widmap();
}


void
kb_init(void)
{
    int32 num_phones, num_ci_phones;
    char *mdeffn, *tmatfn;

    /* Initialize log computation. */
    g_lmath = logmath_init((float64)cmd_ln_float32("-logbase"), 0, FALSE);

    /* Read model definition. */
    if ((mdeffn = cmd_ln_str("-mdef")) == NULL)
        E_FATAL("Must specify -mdeffn or -hmm\n");
    if ((g_mdef = bin_mdef_read(mdeffn)) == NULL)
        E_FATAL("Failed to read model definition from %s\n", mdeffn);
    num_ci_phones = bin_mdef_n_ciphone(g_mdef);
    num_phones = bin_mdef_n_phone(g_mdef);

    if ((g_word_dict = dict_init(cmd_ln_get(), g_mdef)) == NULL)
        E_FATAL("Failed to read dictionaries\n");
    lm_init();

    /* Read transition matrices. */
    if ((tmatfn = cmd_ln_str("-tmat")) == NULL)
        E_FATAL("No tmat file specified\n");
    g_tmat = tmat_init(tmatfn, g_lmath, cmd_ln_float32("-tmatfloor"), TRUE);

    /* Read the acoustic models. */
    if ((cmd_ln_str("-mean") == NULL)
        || (cmd_ln_str("-var") == NULL)
        || (cmd_ln_str("-tmat") == NULL))
        E_FATAL("No mean/var/tmat files specified\n");

    E_INFO("Attempting to use SCGMM computation module\n");
    g_semi_mgau = s2_semi_mgau_init(cmd_ln_get(), g_lmath, g_mdef);
    if (g_semi_mgau) {
        char *kdtreefn = cmd_ln_str("-kdtree");
        if (kdtreefn)
            s2_semi_mgau_load_kdtree(g_semi_mgau,
                                     kdtreefn,
                                     cmd_ln_int32("-kdmaxdepth"),
                                     cmd_ln_int32("-kdmaxbbi"));
    }
    else {
        E_INFO("Falling back to general multi-stream GMM computation\n");
        g_ms_mgau = ms_mgau_init(cmd_ln_get(), g_lmath);
    }
}

void
kb_close(void)
{
    bin_mdef_free(g_mdef);
    g_mdef = NULL;

    dict_free(g_word_dict);
    g_word_dict = NULL;

    ngram_model_free(g_lmset);
    tmat_free(g_tmat);

    if (g_semi_mgau)
        s2_semi_mgau_free(g_semi_mgau);

    if (g_ms_mgau)
        ms_mgau_free(g_ms_mgau);

    logmath_free(g_lmath);
}

int32
kb_get_word_id(char const *word)
{
    return (dict_to_id(g_word_dict, word));
}

char *
kb_get_word_str(int32 wid)
{
    return (g_word_dict->dict_list[wid]->word);
}

int32
kb_dict_maxsize(void)
{
    return (hash_table_size(g_word_dict->dict));
}

int32
lm_read(char const *lmfile,
        char const *lmname,
        double lw, double uw, double wip)
{
    ngram_model_t *lm;

    if ((lm = ngram_model_read(cmd_ln_get(),
                               lmfile, NGRAM_AUTO, g_lmath)) == NULL)
        return -1;
    ngram_model_apply_weights(lm, lw, wip, uw);

    /* Add this LM while preserving word ID mapping (hooray) */
    if ((ngram_model_set_add(g_lmset, lm, lmname, 1.0, TRUE)) != lm) {
        ngram_model_free(lm);
        return -1;
    }
    return 0;
}

int32
lm_delete(char const *lmname)
{
    ngram_model_t *lm;

    if ((lm = ngram_model_set_remove(g_lmset, lmname, TRUE)) == NULL)
        return -1;
    ngram_model_free(lm);
    return 0;
}

ngram_model_t *
lm_get(char const *lmname)
{
    return ngram_model_set_lookup(g_lmset, lmname);
}

ngram_model_t *
lm_get_lmset(char const *lmname)
{
    return g_lmset;
}
