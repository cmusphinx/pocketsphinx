/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 2010 Carnegie Mellon University.  All rights
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
 * @file fsg2_search.h
 * @brief Rewritten FSG search
 * @author David Huggins-Daines <dhuggins@cs.cmu.edu>
 */

#include <strfuncs.h>

#include "fsg2_search.h"

static ps_searchfuncs_t fsg2_funcs = {
    /* name: */   "fsg2",
    /* start: */  fsg2_search_start,
    /* step: */   fsg2_search_step,
    /* finish: */ fsg2_search_finish,
    /* reinit: */ fsg2_search_reinit,
    /* free: */   fsg2_search_free,
    /* lattice: */  NULL,
    /* hyp: */      fsg2_search_hyp,
    /* prob: */     NULL,
    /* seg_iter: */ NULL
};

ps_search_t *
fsg2_search_init(cmd_ln_t *config,
		 acmod_t *acmod,
		 dict_t *dict,
		 dict2pid_t *d2p)
{
    fsg2_search_t *fs;
    char const *path;

    fs = ckd_calloc(1, sizeof(*fs));
    ps_search_init(ps_search_base(fs), &fsg2_funcs,
                   config, acmod, dict, d2p);

    /* Initialize HMM context. */
    fs->hmmctx = hmm_context_init(bin_mdef_n_emit_state(acmod->mdef),
                                  acmod->tmat->tp, NULL, acmod->mdef->sseq);
    if (fs->hmmctx == NULL) {
        ps_search_free(ps_search_base(fs));
        return NULL;
    }

    /* Load an FSG if one was specified in config */
    if ((path = cmd_ln_str_r(config, "-fsg"))) {
        if ((fs->fsg = fsg_model_readfile(path, acmod->lmath,
                                          cmd_ln_float32_r(config, "-lw"))) == NULL)
            goto error_out;
        if (fsg2_search_reinit(ps_search_base(fs),
                               ps_search_dict(fs),
                               ps_search_dict2pid(fs)) < 0)
            goto error_out;
    }
    /* Or load a JSGF grammar */
    else if ((path = cmd_ln_str_r(config, "-jsgf"))) {
        jsgf_rule_t *rule;
        char const *toprule;

        if ((fs->jsgf = jsgf_parse_file(path, NULL)) == NULL)
            goto error_out;

        rule = NULL;
        /* Take the -toprule if specified. */
        if ((toprule = cmd_ln_str_r(config, "-toprule"))) {
            char *anglerule;
            anglerule = string_join("<", toprule, ">", NULL);
            rule = jsgf_get_rule(fs->jsgf, anglerule);
            ckd_free(anglerule);
            if (rule == NULL) {
                E_ERROR("Start rule %s not found\n", toprule);
                goto error_out;
            }
        }
        /* Otherwise, take the first public rule. */
        else {
            jsgf_rule_iter_t *itor;

            for (itor = jsgf_rule_iter(fs->jsgf); itor;
                 itor = jsgf_rule_iter_next(itor)) {
                rule = jsgf_rule_iter_rule(itor);
                if (jsgf_rule_public(rule))
                    break;
            }
            if (rule == NULL) {
                E_ERROR("No public rules found in %s\n", path);
                goto error_out;
            }
        }
        fs->fsg = jsgf_build_fsg(fs->jsgf, rule, acmod->lmath,
                                 cmd_ln_float32_r(config, "-lw"));
        if (fsg2_search_reinit(ps_search_base(fs),
                               ps_search_dict(fs),
                               ps_search_dict2pid(fs)) < 0)
            goto error_out;
    }

    return ps_search_base(fs);

error_out:
    fsg2_search_free(ps_search_base(fs));
    return NULL;
}

void
fsg2_search_free(ps_search_t *search)
{
    fsg2_search_t *fs = (fsg2_search_t *)search;

    ps_search_deinit(search);
    glextree_free(fs->lextree);
    tokentree_free(fs->toktree);
    hmm_context_free(fs->hmmctx);
    if (fs->vocab)
        hash_table_free(fs->vocab);
    fsg_model_free(fs->fsg);
    jsgf_grammar_free(fs->jsgf);
    ckd_free(fs);
}

static int
fsg2_search_wfilter(glextree_t *tree, int32 wid, void *udata)
{
    fsg2_search_t *fs = (fsg2_search_t *)udata;
    int32 wid2;

    if (hash_table_lookup_int32(fs->vocab,
                                dict_basestr(ps_search_dict(fs), wid),
                                &wid2) < 0)
        return FALSE;
    return TRUE;
}

int
fsg2_search_reinit(ps_search_t *s, dict_t *dict, dict2pid_t *d2p)
{
    fsg2_search_t *fs = (fsg2_search_t *)s;
    int i;

    /* Build active vocabulary. */
    if (fs->vocab)
        hash_table_free(fs->vocab);
    fs->vocab = hash_table_new(fsg_model_n_word(fs->fsg), HASH_CASE_YES);
    for (i = 0; i < fsg_model_n_word(fs->fsg); ++i)
        (void)hash_table_enter_int32(fs->vocab, fsg_model_word_str(fs->fsg, i), i);

    /* (re-)Initialize lexicon tree. */
    fs->lextree = glextree_build(fs->hmmctx, dict, d2p,
                                 fsg2_search_wfilter, (void *)fs);

    /* Initialize token tree. */
    tokentree_free(fs->toktree);
    fs->toktree = tokentree_init();

    s->dict = dict;
    s->d2p = d2p;
    return 0;
}

int
fsg2_search_start(ps_search_t *s)
{
    fsg2_search_t *fs = (fsg2_search_t *)s;

    /* Reset HMMs and tokens. */
    glextree_clear(fs->lextree);
    tokentree_clear(fs->toktree);

    /* Reset score renormalization information. */

    /* Follow epsilon transitions out of start state to populate
     * initial active state list. */

    return 0;
}

int
fsg2_search_step(ps_search_t *search, int frame_idx)
{
    return 0;
}

int
fsg2_search_finish(ps_search_t *search)
{
    return 0;
}

char const *
fsg2_search_hyp(ps_search_t *search, int32 *out_score)
{
    return "go forward ten meters";
}
