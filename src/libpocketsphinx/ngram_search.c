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

/* Local headers. */
#include "ngram_search.h"

ngram_search_t *
ngram_search_init(cmd_ln_t *config,
		  acmod_t *acmod,
		  dict_t *dict)
{
	ngram_search_t *ngs;

	ngs = ckd_calloc(1, sizeof(*ngs));
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

        /* Now build the search tree (hooray) */

        /* Allocate active word list arrays */

        /* Allocate bestbp_rc, lastphn_cand, last_ltrans */

	return ngs;
}

void
ngram_search_free(ngram_search_t *ngs)
{
	ckd_free(ngs);
}
