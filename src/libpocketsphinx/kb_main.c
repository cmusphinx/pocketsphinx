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

/* KB.C - for compile_kb
 * 
 * 02-Dec-2004	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Added acoustic score weight (applied only to S3 continuous
 * 		acoustic models).
 * 
 * 22-Nov-2004	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Incorporated continuous acoustic model handling.
 * 
 * 06-Aug-2004	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Added phonetp (phone transition probs matrix) for use in
 * 		allphone search.
 * 
 * 27-May-97  M K Ravishankar (rkm@cs.cmu.edu) at Carnegie-Mellon University
 * 		Included Bob Brennan's personaldic handling (similar to 
 *              oovdic).
 * 
 * 09-Dec-94	M K Ravishankar (rkm@cs) at Carnegie Mellon University
 * 		Cleaned up kb() interface; got rid of fwd-only, astar-mode
 * 		etc.
 * 
 * Revision 8.6  94/10/11  12:33:49  rkm
 * Minor changes.
 * 
 * Revision 8.5  94/07/29  11:52:10  rkm
 * Removed lmSetParameters() call; that is now part of lm_3g.c.
 * Added lm_init_oov() call to LM module.
 * Added ilm_init() call to ILM module.
 * 
 * Revision 8.4  94/05/19  14:19:12  rkm
 * Commented out computePhraseLMProbs().
 * 
 * Revision 8.3  94/04/14  14:38:01  rkm
 * Added OOV words sub-dictionary.
 * 
 * Revision 8.1  94/02/15  15:08:13  rkm
 * Derived from v7.  Includes multiple start symbols for the LISTEN
 * project.  Includes multiple LMs for grammar switching.
 * 
 * Revision 6.15  94/02/11  13:15:18  rkm
 * Initial revision (going into v7).
 * Added multiple start symbols for the LISTEN project.
 * 
 * Revision 6.14  94/01/31  16:35:17  rkm
 * Moved check for use of 8/16-bit senones on HPs to after pconf().
 * 
 * Revision 6.13  94/01/07  17:48:13  rkm
 * Added option to use trigrams in forward pass (simple implementation).
 * 
 * Revision 6.12  94/01/05  16:04:20  rkm
 * *** empty log message ***
 * 
 * Revision 6.11  94/01/05  16:02:17  rkm
 * Placed senone probs compression under conditional compilation.
 * 
 * Revision 6.10  93/12/05  17:25:46  rkm
 * Added -8bsen option and necessary datastructures.
 * 
 * Revision 6.9  93/12/04  16:24:11  rkm
 * Added check for use of -16bsen if compiled with _HPUX_SOURCE.
 * 
 * Revision 6.8  93/11/15  12:20:39  rkm
 * Added -ilmusesdarpalm flag.
 * 
 * Revision 6.7  93/11/03  12:42:35  rkm
 * Added -16bsen option to compress senone probs to 16 bits.
 * 
 * Revision 6.6  93/10/27  17:29:45  rkm
 * *** empty log message ***
 * 
 * Revision 6.5  93/10/15  14:59:13  rkm
 * Bug-fix in call to create_ilmwid_map.
 * 
 * Revision 6.4  93/10/13  16:48:16  rkm
 * Added ilm_init call to Roni's ILM and whatever else is needed.
 * Added option to process A* only (in which case phoneme-level
 * files are not loaded).  Added bigram_only argument to lm_read
 * call.
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <err.h>
#include <ckd_alloc.h>
#include <cmd_ln.h>
#include <pio.h>
#include <ngram_model.h>
#include <logmath.h>

#include "strfuncs.h"
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
dictT *word_dict;

/* Transition matrices. */
tmat_t *tmat;

/* S3 model definition */
bin_mdef_t *mdef;

/* S2 fast SCGMM computation object */
s2_semi_mgau_t *semi_mgau;

/* Slow CDGMM computation object */
ms_mgau_model_t *ms_mgau;

/* Model file names */
char *hmmdir, *mdeffn, *meanfn, *varfn, *mixwfn, *tmatfn, *kdtreefn, *sendumpfn, *logaddfn;

/* Language model set */
ngram_model_t *lmset;

static void
lm_init(void)
{
    char *lm_ctl_filename = cmd_ln_str("-lmctlfn");
    char *lm_file_name = cmd_ln_str("-lm");
    const char **words;
    int32 i, n_words;

    /* Do nothing if there is no language model. */
    if (lm_ctl_filename == NULL && lm_file_name == NULL)
        return;

    /* Read one or more language models. */
    if (lm_ctl_filename) {
        lmset = ngram_model_set_read(cmd_ln_get(),
                                     lm_ctl_filename,
                                     lmath);
        if (lmset == NULL) {
            E_FATAL("Failed to read language model control file: %s\n",
                    lm_ctl_filename);
        }
    }
    else {
        ngram_model_t *lm;
        static const char *name = "default";

        lm = ngram_model_read(cmd_ln_get(), lm_file_name,
                              NGRAM_AUTO, lmath);
        if (lm == NULL) {
            E_FATAL("Failed to read language model file: %s\n",
                    lm_file_name);
        }
        lmset = ngram_model_set_init(cmd_ln_get(),
                                     &lm, (char **)&name,
                                     NULL, 1);
        if (lmset == NULL) {
            E_FATAL("Failed to initialize language model set\n");
        }
    }

    /* Set the language model parameters. */
    ngram_model_apply_weights(lmset,
                              cmd_ln_float32("-lw"),
                              cmd_ln_float32("-wip"),
                              cmd_ln_float32("-uw"));

    /* Select a language model if requested. */
    if (cmd_ln_str("-lmname")) {
        if (ngram_model_set_select(lmset, cmd_ln_str("-lmname")) == NULL)
            E_FATAL("No such language model %s\n", cmd_ln_str("-lmname"));
    }

    /* Now create the dictionary to LM word ID mapping. */
    /* It's okay to include fillers since they won't be in the LM */
    n_words = word_dict->dict_entry_count;
    words = ckd_calloc(n_words, sizeof(*words));
    for (i = 0; i < n_words; ++i)
        words[i] = word_dict->dict_list[i]->word;
    ngram_model_set_map_words(lmset, words, n_words);
    ckd_free(words);
}

static void
dict_init(void)
{
    char *fdictfn = NULL;

    E_INFO("Reading dict file [%s]\n", cmd_ln_str("-dict"));
    word_dict = dict_new();
    /* Look for noise word dictionary in the HMM directory if not given */
    if (cmd_ln_str("-hmm") && !cmd_ln_str("-fdict")) {
        FILE *tmp;
        fdictfn = string_join(hmmdir, "/noisedict", NULL);
        if ((tmp = fopen(fdictfn, "r")) == NULL) {
            ckd_free(fdictfn);
            fdictfn = NULL;
        }
        else {
            fclose(tmp);
        }
    }

    if (dict_read(word_dict, cmd_ln_str("-dict"),
                  (fdictfn != NULL) ? fdictfn : cmd_ln_str("-fdict"),
                  !cmd_ln_boolean("-usewdphones"))) {
        ckd_free(fdictfn);
        E_FATAL("Failed to read dictionaries\n");
    }
    ckd_free(fdictfn);
}

void
kb_init(void)
{
    int32 num_phones, num_ci_phones;

    /* Get acoustic model filenames */
    if ((hmmdir = cmd_ln_str("-hmm")) != NULL) {
        FILE *tmp;

        mdeffn = string_join(hmmdir, "/mdef", NULL);
        meanfn = string_join(hmmdir, "/means", NULL);
        varfn = string_join(hmmdir, "/variances", NULL);
        mixwfn = string_join(hmmdir, "/mixture_weights", NULL);
        tmatfn = string_join(hmmdir, "/transition_matrices", NULL);

        /* These ones are optional, so make sure they exist. */
        sendumpfn = string_join(hmmdir, "/sendump", NULL);
        if ((tmp = fopen(sendumpfn, "rb")) == NULL) {
            ckd_free(sendumpfn);
            sendumpfn = NULL;
        }
        else {
            fclose(tmp);
        }
        kdtreefn = string_join(hmmdir, "/kdtrees", NULL);
        if ((tmp = fopen(kdtreefn, "rb")) == NULL) {
            ckd_free(kdtreefn);
            kdtreefn = NULL;
        }
        else {
            fclose(tmp);
        }
        logaddfn = string_join(hmmdir, "/logadd", NULL);
        if ((tmp = fopen(logaddfn, "rb")) == NULL) {
            ckd_free(logaddfn);
            logaddfn = NULL;
        }
        else {
            fclose(tmp);
        }
    }
    /* Allow overrides from the command line */
    if (cmd_ln_str("-mdef")) {
        ckd_free(mdeffn);
        mdeffn = ckd_salloc(cmd_ln_str("-mdef"));
    }
    if (cmd_ln_str("-mean")) {
        ckd_free(meanfn);
        meanfn = ckd_salloc(cmd_ln_str("-mean"));
    }
    if (cmd_ln_str("-var")) {
        ckd_free(varfn);
        varfn = ckd_salloc(cmd_ln_str("-var"));
    }
    if (cmd_ln_str("-mixw")) {
        ckd_free(mixwfn);
        mixwfn = ckd_salloc(cmd_ln_str("-mixw"));
    }
    if (cmd_ln_str("-tmat")) {
        ckd_free(tmatfn);
        tmatfn = ckd_salloc(cmd_ln_str("-tmat"));
    }
    if (cmd_ln_str("-sendump")) {
        ckd_free(sendumpfn);
        sendumpfn = ckd_salloc(cmd_ln_str("-sendump"));
    }
    if (cmd_ln_str("-kdtree")) {
        ckd_free(kdtreefn);
        kdtreefn = ckd_salloc(cmd_ln_str("-kdtree"));
    }
    if (cmd_ln_str("-logadd")) {
        ckd_free(logaddfn);
        logaddfn = ckd_salloc(cmd_ln_str("-logadd"));
    }

    /* Initialize log tables. */
    if (logaddfn) {
        lmath = logmath_read(logaddfn);
        if (lmath == NULL) {
            E_FATAL("Failed to read log-add table from %s\n", logaddfn);
        }
    }
    else {
        lmath = logmath_init((float64)cmd_ln_float32("-logbase"), 0, FALSE);
    }

    /* Read model definition. */
    if (mdeffn == NULL)
        E_FATAL("Must specify -mdeffn or -hmm\n");
    if ((mdef = bin_mdef_read(mdeffn)) == NULL)
        E_FATAL("Failed to read model definition from %s\n", mdeffn);
    num_ci_phones = bin_mdef_n_ciphone(mdef);
    num_phones = bin_mdef_n_phone(mdef);

    dict_init();
    lm_init();

    /* Read transition matrices. */
    if (tmatfn == NULL)
        E_FATAL("No tmat file specified\n");
    tmat = tmat_init(tmatfn, cmd_ln_float32("-tmatfloor"), TRUE);

    /* Read the acoustic models. */
    if ((meanfn == NULL)
        || (varfn == NULL)
        || (tmatfn == NULL))
        E_FATAL("No mean/var/tmat files specified\n");

    E_INFO("Attempting to use SCGMM computation module\n");
    semi_mgau = s2_semi_mgau_init(meanfn,
                                  varfn,
                                  cmd_ln_float32("-varfloor"),
                                  mixwfn,
                                  cmd_ln_float32("-mixwfloor"),
                                  cmd_ln_int32("-topn"));
    if (semi_mgau) {
        if (kdtreefn)
            s2_semi_mgau_load_kdtree(semi_mgau,
                                     kdtreefn,
                                     cmd_ln_int32("-kdmaxdepth"),
                                     cmd_ln_int32("-kdmaxbbi"));
        semi_mgau->ds_ratio = cmd_ln_int32("-dsratio");
    }
    else {
        E_INFO("Falling back to general multi-stream GMM computation\n");
        ms_mgau = ms_mgau_init(meanfn, varfn,
                               cmd_ln_float32("-varfloor"),
                               mixwfn, 
                               cmd_ln_float32("-mixwfloor"),
                               cmd_ln_int32("-topn"));
    }
}

void
kb_close(void)
{
    ckd_free(mdeffn);
    ckd_free(meanfn);
    ckd_free(varfn);
    ckd_free(mixwfn);
    ckd_free(tmatfn);
    mdeffn = meanfn = varfn = mixwfn = tmatfn = NULL;
    ckd_free(sendumpfn);
    sendumpfn = NULL;
    ckd_free(kdtreefn);
    kdtreefn = NULL;

    bin_mdef_free(mdef);
    mdef = NULL;

    dict_free(word_dict);
    word_dict = NULL;
    dict_cleanup();

    ngram_model_free(lmset);
    tmat_free(tmat);

    if (semi_mgau)
        s2_semi_mgau_free(semi_mgau);

    if (ms_mgau)
        ms_mgau_free(ms_mgau);
}

char *
kb_get_senprob_dump_file(void)
{
    return sendumpfn;
}

int32
kb_get_word_id(char const *word)
{
    return (dict_to_id(word_dict, word));
}

char *
kb_get_word_str(int32 wid)
{
    return (word_dict->dict_list[wid]->word);
}

int32
kb_dict_maxsize(void)
{
    return (hash_table_size(word_dict->dict));
}
