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
 * @file ngram_search.c N-Gram based multi-pass search ("FBS")
 */

/* System headers. */

/* SphinxBase headers. */
#include <ckd_alloc.h>
#include <linklist.h>

/* Local headers. */
#include "ngram_search.h"
#include "ngram_search_fwdtree.h"
#include "ngram_search_fwdflat.h"
#include "ngram_search_dag.h"

ngram_search_t *
ngram_search_init(cmd_ln_t *config,
		  acmod_t *acmod,
		  dict_t *dict)
{
	ngram_search_t *ngs;
        const char *path;

	ngs = ckd_calloc(1, sizeof(*ngs));
        ngs->config = config;
	ngs->acmod = acmod;
	ngs->dict = dict;
	ngs->hmmctx = hmm_context_init(bin_mdef_n_emit_state(acmod->mdef),
				       acmod->tmat->tp, NULL, acmod->mdef->sseq);

        /* Allocate a billion different tables for stuff. */
        ngs->word_chan = ckd_calloc(dict->dict_entry_count,
                                    sizeof(*ngs->word_chan));
        ngs->word_lat_idx = ckd_calloc(dict->dict_entry_count,
                                       sizeof(*ngs->word_lat_idx));
        ngs->zeroPermTab = ckd_calloc(bin_mdef_n_ciphone(acmod->mdef),
                                      sizeof(*ngs->zeroPermTab));
        ngs->word_active = ckd_calloc(dict->dict_entry_count,
                                      sizeof(*ngs->word_active));

        /* FIXME: All these structures need to be made dynamic with
         * garbage collection. */
        ngs->bp_table_size = cmd_ln_int32_r(config, "-latsize");
        ngs->bp_table = ckd_calloc(ngs->bp_table_size,
                                   sizeof(*ngs->bp_table));
        ngs->bscore_stack_size = ngs->bp_table_size * 20;
        ngs->bscore_stack = ckd_calloc(ngs->bscore_stack_size,
                                       sizeof(*ngs->bscore_stack));
        /* FIXME: Totally arbitrary since there is no MAX_FRAMES. */
        ngs->bp_table_idx = ckd_calloc(2000, sizeof(*ngs->bp_table_idx));
        ++ngs->bp_table_idx; /* Make bptableidx[-1] valid */

        /* Allocate active word list array */
        ngs->active_word_list = ckd_calloc_2d(dict->dict_entry_count, 2,
                                              sizeof(**ngs->active_word_list));

        /* Allocate bestbp_rc, lastphn_cand, last_ltrans */
        ngs->bestbp_rc = ckd_calloc(bin_mdef_n_ciphone(acmod->mdef),
                                    sizeof(*ngs->bestbp_rc));
        ngs->lastphn_cand = ckd_calloc(dict->dict_entry_count,
                                       sizeof(*ngs->lastphn_cand));
        ngs->last_ltrans = ckd_calloc(dict->dict_entry_count,
                                      sizeof(*ngs->last_ltrans));

        /* Load language model(s) */
        if ((path = cmd_ln_str_r(config, "-lmctlfn"))) {
            ngs->lmset = ngram_model_set_read(config, path, acmod->lmath);
            if (ngs->lmset == NULL) {
                E_ERROR("Failed to read language model control file: %s\n",
                        path);
                goto error_out;
            }
            /* Set the default language model if needed. */
            if ((path = cmd_ln_str_r(config, "-lmname"))) {
                ngram_model_set_select(ngs->lmset, path);
            }
        }
        else if ((path = cmd_ln_str_r(config, "-lm"))) {
            static const char *name = "default";
            ngram_model_t *lm;

            lm = ngram_model_read(config, path, NGRAM_AUTO, acmod->lmath);
            if (lm == NULL) {
                E_ERROR("Failed to read language model file: %s\n", path);
                goto error_out;
            }
            ngs->lmset = ngram_model_set_init(config,
                                              &lm, (char **)&name,
                                              NULL, 1);
            if (ngs->lmset == NULL) {
                E_ERROR("Failed to initialize language model set\n");
                goto error_out;
            }
        }

        /* Initialize fwdtree, fwdflat, bestpath modules if necessary. */
        if (cmd_ln_boolean_r(config, "-fwdtree"))
            ngram_fwdtree_init(ngs);
        if (cmd_ln_boolean_r(config, "-fwdflat"))
            ngram_fwdflat_init(ngs);
	return ngs;

error_out:
        ngram_search_free(ngs);
        return NULL;
}

void
ngram_search_free(ngram_search_t *ngs)
{
    ngram_fwdtree_deinit(ngs);
    ngram_fwdflat_deinit(ngs);

    hmm_context_free(ngs->hmmctx);
    ngram_model_free(ngs->lmset);

    ckd_free(ngs->word_chan);
    ckd_free(ngs->word_lat_idx);
    ckd_free(ngs->zeroPermTab);
    ckd_free(ngs->word_active);
    ckd_free(ngs->bp_table);
    ckd_free(ngs->bscore_stack);
    ckd_free(ngs->bp_table_idx - 1);
    ckd_free_2d(ngs->active_word_list);
    ckd_free(ngs->bestbp_rc);
    ckd_free(ngs->lastphn_cand);
    ckd_free(ngs->last_ltrans);
    ckd_free(ngs);
}
