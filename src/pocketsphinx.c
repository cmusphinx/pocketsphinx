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

/* System headers. */
#include <stdio.h>
#include <assert.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <pocketsphinx.h>

#include "util/strfuncs.h"
#include "util/filename.h"
#include "util/pio.h"
#include "lm/jsgf.h"
#include "util/hash_table.h"
#include "pocketsphinx_internal.h"
#include "ps_lattice_internal.h"
#include "ps_alignment_internal.h"
#include "phone_loop_search.h"
#include "kws_search.h"
#include "fsg_search_internal.h"
#include "ngram_search.h"
#include "ngram_search_fwdtree.h"
#include "ngram_search_fwdflat.h"
#include "allphone_search.h"
#include "state_align_search.h"
#include "fe/fe_internal.h"

/* I'm not sure what the portable way to do this is. */
static int
file_exists(const char *path)
{
    FILE *tmp;

    tmp = fopen(path, "rb");
    if (tmp) fclose(tmp);
    return (tmp != NULL);
}

static int
hmmdir_exists(const char *path)
{
    FILE *tmp;
    char *mdef = string_join(path, "/means", NULL);

    tmp = fopen(mdef, "rb");
    if (tmp) fclose(tmp);
    ckd_free(mdef);
    return (tmp != NULL);
}

static void
ps_expand_file_config(ps_config_t *config, const char *arg,
	              const char *hmmdir, const char *file)
{
    const char *val;
    if ((val = ps_config_str(config, arg)) == NULL) {
        char *tmp = string_join(hmmdir, "/", file, NULL);
        if (file_exists(tmp))
	    ps_config_set_str(config, arg, tmp);
	else
	    ps_config_set_str(config, arg, NULL);
        ckd_free(tmp);
    }
}

void
ps_expand_model_config(ps_config_t *config)
{
    char const *hmmdir, *featparams;

    /* Get acoustic model filenames and add them to the command-line */
    hmmdir = ps_config_str(config, "hmm");
    if (hmmdir) {
        ps_expand_file_config(config, "mdef", hmmdir, "mdef");
        ps_expand_file_config(config, "mean", hmmdir, "means");
        ps_expand_file_config(config, "var", hmmdir, "variances");
        ps_expand_file_config(config, "tmat", hmmdir, "transition_matrices");
        ps_expand_file_config(config, "mixw", hmmdir, "mixture_weights");
        ps_expand_file_config(config, "sendump", hmmdir, "sendump");
        ps_expand_file_config(config, "fdict", hmmdir, "noisedict");
        ps_expand_file_config(config, "lda", hmmdir, "feature_transform");
        ps_expand_file_config(config, "featparams", hmmdir, "feat.params");
        ps_expand_file_config(config, "senmgau", hmmdir, "senmgau");
    }

    /* Look for feat.params in acoustic model dir. */
    if ((featparams = ps_config_str(config, "featparams"))) {
        if (NULL !=
            cmd_ln_parse_file_r(config, ps_args(), featparams, FALSE))
            E_INFO("Parsed model-specific feature parameters from %s\n",
                    featparams);
    }
}

static void
ps_free_searches(ps_decoder_t *ps)
{
    if (ps->searches) {
        hash_iter_t *search_it;
        for (search_it = hash_table_iter(ps->searches); search_it;
             search_it = hash_table_iter_next(search_it)) {
            ps_search_free(hash_entry_val(search_it->ent));
        }
        hash_table_free(ps->searches);
    }

    ps->searches = NULL;
    ps->search = NULL;
}

static ps_search_t *
ps_find_search(ps_decoder_t *ps, char const *name)
{
    void *search = NULL;
    if (name == NULL)
        return ps->search;
    hash_table_lookup(ps->searches, name, &search);
    return (ps_search_t *) search;
}

const char *
ps_default_modeldir(void)
{
    const char *modeldir = getenv("POCKETSPHINX_PATH");
#ifdef MODELDIR
    if (modeldir == NULL)
        modeldir = MODELDIR;
#endif
    return modeldir;
}

/* Set default acoustic and language models if they are not defined in configuration. */
void
ps_default_search_args(ps_config_t *config)
{
    const char *modeldir = ps_default_modeldir();

    if (modeldir) {
        const char *hmmdir = ps_config_str(config, "hmm");
        const char *lmfile = ps_config_str(config, "lm");
        const char *dictfile = ps_config_str(config, "dict");
        int maxlen;
        char *path;
    
        maxlen = snprintf(NULL, 0, "%s/en-us/cmudict-en-us.dict", modeldir);
        if (maxlen < 0)
            E_FATAL_SYSTEM("snprintf() failed, giving up all hope");
        path = ckd_malloc(++maxlen);

        E_INFO("Looking for default model in %s\n", modeldir);
        snprintf(path, maxlen, "%s/en-us/en-us", modeldir);
        if (hmmdir == NULL && hmmdir_exists(path)) {
            hmmdir = path;
            E_INFO("Loading default acoustic model from %s\n", hmmdir);
            ps_config_set_str(config, "hmm", hmmdir);
        }

        snprintf(path, maxlen, "%s/en-us/en-us.lm.bin", modeldir);
        if (lmfile == NULL && !ps_config_str(config, "fsg")
            && !ps_config_str(config, "jsgf")
            && !ps_config_str(config, "lmctl")
            && !ps_config_str(config, "kws")
            && !ps_config_str(config, "keyphrase")
            && file_exists(path)) {
            lmfile = path;
            E_INFO("Loading default language model from %s\n", lmfile);
            ps_config_set_str(config, "lm", lmfile);
        }

        snprintf(path, maxlen, "%s/en-us/cmudict-en-us.dict", modeldir);
        if (dictfile == NULL && file_exists(path)) {
            dictfile = path;
            E_INFO("Loading default dictionary from %s\n", dictfile);
            ps_config_set_str(config, "dict", dictfile);
        }
        ckd_free(path);
    }
    else
        E_INFO("No system default model directory exists "
               "and POCKETSPHINX_PATH is not set."
               "(Python users can probably ignore this message)\n");
}

int
ps_reinit_feat(ps_decoder_t *ps, ps_config_t *config)
{
    if (config && config != ps->config) {
        ps_config_free(ps->config);
        ps->config = ps_config_retain(config);
    }
    return acmod_reinit_feat(ps->acmod, NULL, NULL);
}

int
ps_reinit(ps_decoder_t *ps, ps_config_t *config)
{
    const char *path;
    const char *keyphrase;
    int32 lw;

    /* Enforce only one of keyphrase, kws, fsg, jsgf, allphone, lm */
    if (config) {
        if (ps_config_validate(config) < 0)
            return -1;
    }
    else if (ps->config) {
        if (ps_config_validate(ps->config) < 0)
            return -1;
    }

    if (config && config != ps->config) {
        ps_config_free(ps->config);
        ps->config = ps_config_retain(config);
    }

    /* Set up logging. We need to do this earlier because we want to dump
     * the information to the configured log, not to the stderr. */
    if (config) {
        const char *logfn, *loglevel;
        logfn = ps_config_str(ps->config, "logfn");
        if (logfn) {
            if (err_set_logfile(logfn) < 0) {
                E_ERROR("Cannot redirect log output\n");
                return -1;
            }
        }
        loglevel = ps_config_str(ps->config, "loglevel");
        if (loglevel) {
            if (err_set_loglevel_str(loglevel) == NULL) {
                E_ERROR("Invalid log level: %s\n", loglevel);
                return -1;
            }
        }
    }
    
    ps->mfclogdir = ps_config_str(ps->config, "mfclogdir");
    ps->rawlogdir = ps_config_str(ps->config, "rawlogdir");
    ps->senlogdir = ps_config_str(ps->config, "senlogdir");

    /* Fill in some default arguments. */
    ps_expand_model_config(ps->config);

    /* Print out the config for logging. */
    cmd_ln_log_values_r(ps->config, ps_args());
    
    /* Free old searches (do this before other reinit) */
    ps_free_searches(ps);
    ps->searches = hash_table_new(3, HASH_CASE_YES);

    /* Free old acmod. */
    acmod_free(ps->acmod);
    ps->acmod = NULL;

    /* Free old dictionary (must be done after the two things above) */
    dict_free(ps->dict);
    ps->dict = NULL;

    /* Free d2p */
    dict2pid_free(ps->d2p);
    ps->d2p = NULL;

    /* Logmath computation (used in acmod and search) */
    if (ps->lmath == NULL
        || (logmath_get_base(ps->lmath) !=
            ps_config_float(ps->config, "logbase"))) {
        if (ps->lmath)
            logmath_free(ps->lmath);
        ps->lmath = logmath_init
            (ps_config_float(ps->config, "logbase"), 0, TRUE);
    }

    /* Acoustic model (this is basically everything that
     * uttproc.c, senscr.c, and others used to do) */
    if ((ps->acmod = acmod_init(ps->config, ps->lmath, NULL, NULL)) == NULL)
        return -1;

    if (ps_config_int(ps->config, "pl_window") > 0) {
        /* Initialize an auxiliary phone loop search, which will run in
         * "parallel" with FSG or N-Gram search. */
        if ((ps->phone_loop =
             phone_loop_search_init(ps->config, ps->acmod, ps->dict)) == NULL)
            return -1;
        hash_table_enter(ps->searches,
                         ps_search_name(ps->phone_loop),
                         ps->phone_loop);
    }

    /* Dictionary and triphone mappings (depends on acmod). */
    /* FIXME: pass config, change arguments, implement LTS, etc. */
    if ((ps->dict = dict_init(ps->config, ps->acmod->mdef)) == NULL)
        return -1;
    if ((ps->d2p = dict2pid_build(ps->acmod->mdef, ps->dict)) == NULL)
        return -1;

    lw = ps_config_float(ps->config, "lw");

    /* Determine whether we are starting out in FSG or N-Gram search mode.
     * If neither is used skip search initialization. */

    if ((keyphrase = ps_config_str(ps->config, "keyphrase"))) {
        if (ps_add_keyphrase(ps, PS_DEFAULT_SEARCH, keyphrase))
            return -1;
        ps_activate_search(ps, PS_DEFAULT_SEARCH);
    }
    else if ((path = ps_config_str(ps->config, "kws"))) {
        if (ps_add_kws(ps, PS_DEFAULT_SEARCH, path))
            return -1;
        ps_activate_search(ps, PS_DEFAULT_SEARCH);
    }
    else if ((path = ps_config_str(ps->config, "fsg"))) {
        fsg_model_t *fsg = fsg_model_readfile(path, ps->lmath, lw);
        if (!fsg)
            return -1;
        if (ps_add_fsg(ps, PS_DEFAULT_SEARCH, fsg)) {
            fsg_model_free(fsg);
            return -1;
        }
        fsg_model_free(fsg);
        ps_activate_search(ps, PS_DEFAULT_SEARCH);
    }
    else if ((path = ps_config_str(ps->config, "jsgf"))) {
        if (ps_add_jsgf_file(ps, PS_DEFAULT_SEARCH, path)
            || ps_activate_search(ps, PS_DEFAULT_SEARCH))
            return -1;
    }
    else if ((path = ps_config_str(ps->config, "allphone"))) {
        if (ps_add_allphone_file(ps, PS_DEFAULT_SEARCH, path)
            || ps_activate_search(ps, PS_DEFAULT_SEARCH))
            return -1;
    }
    else if ((path = ps_config_str(ps->config, "lm"))) {
        if (ps_add_lm_file(ps, PS_DEFAULT_SEARCH, path)
            || ps_activate_search(ps, PS_DEFAULT_SEARCH))
            return -1;
    }
    else if ((path = ps_config_str(ps->config, "lmctl"))) {
        const char *name;
        ngram_model_t *lmset;
        ngram_model_set_iter_t *lmset_it;

        if (!(lmset = ngram_model_set_read(ps->config, path, ps->lmath))) {
            E_ERROR("Failed to read language model control file: %s\n", path);
            return -1;
        }

        for(lmset_it = ngram_model_set_iter(lmset);
            lmset_it; lmset_it = ngram_model_set_iter_next(lmset_it)) {    
            ngram_model_t *lm = ngram_model_set_iter_model(lmset_it, &name);            
            E_INFO("adding search %s\n", name);
            if (ps_add_lm(ps, name, lm)) {
                ngram_model_set_iter_free(lmset_it);
        	ngram_model_free(lmset);
                return -1;
            }
        }
        ngram_model_free(lmset);

        name = ps_config_str(ps->config, "lmname");
        if (name)
            ps_activate_search(ps, name);
        else {
            E_ERROR("No default LM name (-lmname) for `-lmctl'\n");
            return -1;
        }
    }

    /* Initialize performance timer. */
    ps->perf.name = "decode";
    ptmr_init(&ps->perf);

    return 0;
}

const char *
ps_get_cmn(ps_decoder_t *ps, int update)
{
    if (update)
        cmn_live_update(ps->acmod->fcb->cmn_struct);
    return cmn_repr(ps->acmod->fcb->cmn_struct);
}

int
ps_set_cmn(ps_decoder_t *ps, const char *cmn)
{
    return cmn_set_repr(ps->acmod->fcb->cmn_struct, cmn);
}


ps_decoder_t *
ps_init(ps_config_t *config)
{
    ps_decoder_t *ps;
    
    ps = ckd_calloc(1, sizeof(*ps));
    ps->refcount = 1;
    if (config) {
        if (ps_reinit(ps, config) < 0) {
            ps_free(ps);
            return NULL;
        }
    }
    return ps;
}

ps_decoder_t *
ps_retain(ps_decoder_t *ps)
{
    ++ps->refcount;
    return ps;
}

int
ps_free(ps_decoder_t *ps)
{
    if (ps == NULL)
        return 0;
    if (--ps->refcount > 0)
        return ps->refcount;
    ps_free_searches(ps);
    dict_free(ps->dict);
    dict2pid_free(ps->d2p);
    acmod_free(ps->acmod);
    logmath_free(ps->lmath);
    ps_config_free(ps->config);
    ckd_free(ps);
    return 0;
}

ps_config_t *
ps_get_config(ps_decoder_t *ps)
{
    return ps->config;
}

logmath_t *
ps_get_logmath(ps_decoder_t *ps)
{
    return ps->lmath;
}

fe_t *
ps_get_fe(ps_decoder_t *ps)
{
    return ps->acmod->fe;
}

feat_t *
ps_get_feat(ps_decoder_t *ps)
{
    return ps->acmod->fcb;
}

ps_mllr_t *
ps_update_mllr(ps_decoder_t *ps, ps_mllr_t *mllr)
{
    return acmod_update_mllr(ps->acmod, mllr);
}

int
ps_activate_search(ps_decoder_t *ps, const char *name)
{
    ps_search_t *search;

    if (ps->acmod->state != ACMOD_ENDED && ps->acmod->state != ACMOD_IDLE) {
        E_ERROR("Cannot change search while decoding, end utterance first\n");
        return -1;
    }

    if (name == NULL)
        name = PS_DEFAULT_SEARCH;

    if (!(search = ps_find_search(ps, name)))
        return -1;

    ps->search = search;
    /* Set pl window depending on the search */
    if (!strcmp(PS_SEARCH_TYPE_NGRAM, ps_search_type(search))) {
        ps->pl_window = ps_config_int(ps->config, "pl_window");
    } else {
        ps->pl_window = 0;
    }

    return 0;
}

const char*
ps_current_search(ps_decoder_t *ps)
{
    hash_iter_t *search_it;
    const char* name = NULL;
    for (search_it = hash_table_iter(ps->searches); search_it;
        search_it = hash_table_iter_next(search_it)) {
        if (hash_entry_val(search_it->ent) == ps->search) {
            name = hash_entry_key(search_it->ent);
            break;
        }
    }
    return name;
}

int 
ps_remove_search(ps_decoder_t *ps, const char *name)
{
    ps_search_t *search = hash_table_delete(ps->searches, name);
    if (!search)
        return -1;
    if (ps->search == search)
        ps->search = NULL;
    ps_search_free(search);
    return 0;
}

ps_search_iter_t *
ps_search_iter(ps_decoder_t *ps)
{
   return (ps_search_iter_t *)hash_table_iter(ps->searches);
}

ps_search_iter_t *
ps_search_iter_next(ps_search_iter_t *itor)
{
   return (ps_search_iter_t *)hash_table_iter_next((hash_iter_t *)itor);
}

const char* 
ps_search_iter_val(ps_search_iter_t *itor)
{
   return (const char*)(((hash_iter_t *)itor)->ent->key);
}

void 
ps_search_iter_free(ps_search_iter_t *itor)
{
    hash_table_iter_free((hash_iter_t *)itor);
}

ngram_model_t *
ps_get_lm(ps_decoder_t *ps, const char *name)
{
    ps_search_t *search =  ps_find_search(ps, name);
    if (search == NULL)
        return NULL;
    if (0 != strcmp(PS_SEARCH_TYPE_NGRAM, ps_search_type(search)))
        return NULL;
    return ((ngram_search_t *) search)->lmset;
}

fsg_model_t *
ps_get_fsg(ps_decoder_t *ps, const char *name)
{
    ps_search_t *search = ps_find_search(ps, name);
    if (search == NULL)
        return NULL;
    if (0 != strcmp(PS_SEARCH_TYPE_FSG, ps_search_type(search)))
        return NULL;
    return ((fsg_search_t *) search)->fsg;
}

const char*
ps_get_kws(ps_decoder_t *ps, const char *name)
{
    ps_search_t *search = ps_find_search(ps, name);
    if (search == NULL)
        return NULL;
    if (0 != strcmp(PS_SEARCH_TYPE_KWS, ps_search_type(search)))
        return NULL;
    return kws_search_get_keyphrases(search);
}

ps_alignment_t *
ps_get_alignment(ps_decoder_t *ps)
{
    if (ps->search == NULL)
        return NULL;
    if (0 != strcmp(PS_SEARCH_TYPE_STATE_ALIGN, ps_search_type(ps->search)))
        return NULL;
    return ((state_align_search_t *) ps->search)->al;
}

static int
set_search_internal(ps_decoder_t *ps, ps_search_t *search)
{
    ps_search_t *old_search;
    
    if (!search)
	return -1;

    search->pls = ps->phone_loop;
    old_search = (ps_search_t *) hash_table_replace(ps->searches, ps_search_name(search), search);
    if (old_search != search)
        ps_search_free(old_search);

    return 0;
}

int
ps_add_lm(ps_decoder_t *ps, const char *name, ngram_model_t *lm)
{
    ps_search_t *search;
    search = ngram_search_init(name, lm, ps->config, ps->acmod, ps->dict, ps->d2p);
    return set_search_internal(ps, search);
}

int
ps_add_lm_file(ps_decoder_t *ps, const char *name, const char *path)
{
  ngram_model_t *lm;
  int result;

  lm = ngram_model_read(ps->config, path, NGRAM_AUTO, ps->lmath);
  if (!lm)
      return -1;

  result = ps_add_lm(ps, name, lm);
  ngram_model_free(lm);
  return result;
}

int
ps_add_allphone(ps_decoder_t *ps, const char *name, ngram_model_t *lm)
{
    ps_search_t *search;
    search = allphone_search_init(name, lm, ps->config, ps->acmod, ps->dict, ps->d2p);
    return set_search_internal(ps, search);
}

int
ps_add_allphone_file(ps_decoder_t *ps, const char *name, const char *path)
{
  ngram_model_t *lm;
  int result;

  lm = NULL;
  if (path)
    lm = ngram_model_read(ps->config, path, NGRAM_AUTO, ps->lmath);
  result = ps_add_allphone(ps, name, lm);
  if (lm)
      ngram_model_free(lm);
  return result;
}

int
ps_set_align_text(ps_decoder_t *ps, const char *text)
{
    fsg_model_t *fsg;
    char *textbuf = ckd_salloc(text);
    char *ptr, *word, delimfound;
    int n, nwords;

    textbuf = string_trim(textbuf, STRING_BOTH);
    /* First pass: count and verify words */
    nwords = 0;
    ptr = textbuf;
    while ((n = nextword(ptr, " \t\n\r", &word, &delimfound)) >= 0) {
        int wid;
        if ((wid = dict_wordid(ps->dict, word)) == BAD_S3WID) {
            E_ERROR("Unknown word %s\n", word);
            ckd_free(textbuf);
            return -1;
        }
        ptr = word + n;
        *ptr = delimfound;
        ++nwords;
    }
    /* Second pass: make fsg */
    fsg = fsg_model_init("_align", ps->lmath,
                         ps_config_float(ps->config, "lw"),
                         nwords + 1);
    nwords = 0;
    ptr = textbuf;
    while ((n = nextword(ptr, " \t\n\r", &word, &delimfound)) >= 0) {
        int wid;
        if ((wid = dict_wordid(ps->dict, word)) == BAD_S3WID) {
            E_ERROR("Unknown word %s\n", word);
            ckd_free(textbuf);
            return -1;
        }
        wid = fsg_model_word_add(fsg, word);
        fsg_model_trans_add(fsg, nwords, nwords + 1, 0, wid);
        ptr = word + n;
        *ptr = delimfound;
        ++nwords;
    }
    ckd_free(textbuf);
    fsg->start_state = 0;
    fsg->final_state = nwords;
    if (ps_add_fsg(ps, PS_DEFAULT_SEARCH, fsg) < 0) {
        fsg_model_free(fsg);
        return -1;
    }
    /* FIXME: Should rethink ownership semantics, this is annoying. */
    fsg_model_free(fsg);
    return ps_activate_search(ps, PS_DEFAULT_SEARCH);
}

int
ps_set_alignment(ps_decoder_t *ps, ps_alignment_t *al)
{
    ps_search_t *search;
    int new_alignment = FALSE;
    
    if (al == NULL) {
        ps_seg_t *seg;
        seg = ps_seg_iter(ps);
        if (seg == NULL)
            return -1;
        al = ps_alignment_init(ps->d2p);
        new_alignment = TRUE;
        while (seg) {
            if (seg->wid == BAD_S3WID) {
                E_ERROR("No word ID for segment %s, cannot align\n",
                        seg->text);
                goto error_out;
            }
            ps_alignment_add_word(al, seg->wid, seg->sf, seg->ef - seg->sf + 1);
            seg = ps_seg_next(seg);
        }
        /* FIXME: Add cionly parameter as in SoundSwallower */
        if (ps_alignment_populate(al) < 0)
            goto error_out;
    }
    else
        al = ps_alignment_retain(al);
    search = state_align_search_init("_state_align", ps->config, ps->acmod, al);
    if (search == NULL)
        goto error_out;
    if (new_alignment)
        ps_alignment_free(al);
    if (set_search_internal(ps, search) < 0)
        goto error_out;
    return ps_activate_search(ps, "_state_align");
error_out:
    if (new_alignment)
        ps_alignment_free(al);
    return -1;
}

int
ps_add_kws(ps_decoder_t *ps, const char *name, const char *keyfile)
{
    ps_search_t *search;
    search = kws_search_init(name, NULL, keyfile, ps->config, ps->acmod, ps->dict, ps->d2p);
    return set_search_internal(ps, search);
}

int
ps_add_keyphrase(ps_decoder_t *ps, const char *name, const char *keyphrase)
{
    ps_search_t *search;
    search = kws_search_init(name, keyphrase, NULL, ps->config, ps->acmod, ps->dict, ps->d2p);
    return set_search_internal(ps, search);
}

int
ps_add_fsg(ps_decoder_t *ps, const char *name, fsg_model_t *fsg)
{
    ps_search_t *search;
    search = fsg_search_init(name, fsg, ps->config, ps->acmod, ps->dict, ps->d2p);
    return set_search_internal(ps, search);
}

int 
ps_add_jsgf_file(ps_decoder_t *ps, const char *name, const char *path)
{
  fsg_model_t *fsg;
  jsgf_rule_t *rule;
  char const *toprule;
  jsgf_t *jsgf = jsgf_parse_file(path, NULL);
  float lw;
  int result;

  if (!jsgf)
      return -1;

  rule = NULL;
  /* Take the -toprule if specified. */
  if ((toprule = ps_config_str(ps->config, "toprule"))) {
      rule = jsgf_get_rule(jsgf, toprule);
      if (rule == NULL) {
          E_ERROR("Start rule %s not found\n", toprule);
          jsgf_grammar_free(jsgf);
          return -1;
      }
  } else {
      rule = jsgf_get_public_rule(jsgf);
      if (rule == NULL) {
          E_ERROR("No public rules found in %s\n", path);
          jsgf_grammar_free(jsgf);
          return -1;
      }
  }

  lw = ps_config_float(ps->config, "lw");
  fsg = jsgf_build_fsg(jsgf, rule, ps->lmath, lw);
  result = ps_add_fsg(ps, name, fsg);
  fsg_model_free(fsg);
  jsgf_grammar_free(jsgf);
  return result;
}

int 
ps_add_jsgf_string(ps_decoder_t *ps, const char *name, const char *jsgf_string)
{
  fsg_model_t *fsg;
  jsgf_rule_t *rule;
  char const *toprule;
  jsgf_t *jsgf = jsgf_parse_string(jsgf_string, NULL);
  float lw;
  int result;

  if (!jsgf)
      return -1;

  rule = NULL;
  /* Take the -toprule if specified. */
  if ((toprule = ps_config_str(ps->config, "toprule"))) {
      rule = jsgf_get_rule(jsgf, toprule);
      if (rule == NULL) {
          E_ERROR("Start rule %s not found\n", toprule);
          jsgf_grammar_free(jsgf);
          return -1;
      }
  } else {
      rule = jsgf_get_public_rule(jsgf);
      if (rule == NULL) {
          E_ERROR("No public rules found in input string\n");
          jsgf_grammar_free(jsgf);
          return -1;
      }
  }

  lw = ps_config_float(ps->config, "lw");
  fsg = jsgf_build_fsg(jsgf, rule, ps->lmath, lw);
  result = ps_add_fsg(ps, name, fsg);
  fsg_model_free(fsg);
  jsgf_grammar_free(jsgf);
  return result;
}


int
ps_load_dict(ps_decoder_t *ps, char const *dictfile,
             char const *fdictfile, char const *format)
{
    dict2pid_t *d2p;
    dict_t *dict;
    hash_iter_t *search_it;
    ps_config_t *newconfig;

    (void)format;
    /* Create a new scratch config to load this dict (so existing one
     * won't be affected if it fails) */
    newconfig = ps_config_init(NULL);
    ps_config_set_bool(newconfig, "dictcase",
                       ps_config_bool(ps->config, "dictcase"));
    ps_config_set_str(newconfig, "dict", dictfile);
    if (fdictfile)
        ps_config_set_str(newconfig, "fdict", fdictfile);
    else
        ps_config_set_str(newconfig, "fdict",
                          ps_config_str(ps->config, "fdict"));

    /* Try to load it. */
    if ((dict = dict_init(newconfig, ps->acmod->mdef)) == NULL) {
        ps_config_free(newconfig);
        return -1;
    }

    /* Reinit the dict2pid. */
    if ((d2p = dict2pid_build(ps->acmod->mdef, dict)) == NULL) {
        ps_config_free(newconfig);
        return -1;
    }

    /* Success!  Update the existing config to reflect new dicts and
     * drop everything into place. */
    ps_config_free(newconfig);
    dict_free(ps->dict);
    ps->dict = dict;
    dict2pid_free(ps->d2p);
    ps->d2p = d2p;

    /* And tell all searches to reconfigure themselves. */
    for (search_it = hash_table_iter(ps->searches); search_it;
       search_it = hash_table_iter_next(search_it)) {
        if (ps_search_reinit(hash_entry_val(search_it->ent), dict, d2p) < 0) {
            hash_table_iter_free(search_it);
            return -1;
        }
    }

    return 0;
}

int
ps_save_dict(ps_decoder_t *ps, char const *dictfile,
             char const *format)
{
    return dict_write(ps->dict, dictfile, format);
}

int
ps_add_word(ps_decoder_t *ps,
            char const *word,
            char const *phones,
            int update)
{
    int32 wid;
    s3cipid_t *pron;
    hash_iter_t *search_it;
    char **phonestr, *tmp;
    int np, i, rv;

    /* Parse phones into an array of phone IDs. */
    tmp = ckd_salloc(phones);
    np = str2words(tmp, NULL, 0);
    phonestr = ckd_calloc(np, sizeof(*phonestr));
    str2words(tmp, phonestr, np);
    pron = ckd_calloc(np, sizeof(*pron));
    for (i = 0; i < np; ++i) {
        pron[i] = bin_mdef_ciphone_id(ps->acmod->mdef, phonestr[i]);
        if (pron[i] == -1) {
            E_ERROR("Unknown phone %s in phone string %s\n",
                    phonestr[i], tmp);
            ckd_free(phonestr);
            ckd_free(tmp);
            ckd_free(pron);
            return -1;
        }
    }
    /* No longer needed. */
    ckd_free(phonestr);
    ckd_free(tmp);

    /* Add it to the dictionary. */
    if ((wid = dict_add_word(ps->dict, word, pron, np)) == -1) {
        ckd_free(pron);
        return -1;
    }
    /* No longer needed. */
    ckd_free(pron);

    /* Now we also have to add it to dict2pid. */
    dict2pid_add_word(ps->d2p, wid);

    /* TODO: we definitely need to refactor this */
    for (search_it = hash_table_iter(ps->searches); search_it;
         search_it = hash_table_iter_next(search_it)) {
        ps_search_t *search = hash_entry_val(search_it->ent);
        if (!strcmp(PS_SEARCH_TYPE_NGRAM, ps_search_type(search))) {
            ngram_model_t *lmset = ((ngram_search_t *) search)->lmset;
            if (ngram_model_add_word(lmset, word, 1.0) == NGRAM_INVALID_WID) {
                hash_table_iter_free(search_it);
                return -1;
            }
        }

        if (update) {
            if ((rv = ps_search_reinit(search, ps->dict, ps->d2p)) < 0) {
                hash_table_iter_free(search_it);
                return rv;
            }
        }
    }

    /* Rebuild the widmap and search tree if requested. */
    return wid;
}

char *
ps_lookup_word(ps_decoder_t *ps, const char *word)
{
    s3wid_t wid;
    int32 phlen, j;
    char *phones;
    dict_t *dict = ps->dict;
    
    wid = dict_wordid(dict, word);
    if (wid == BAD_S3WID)
	return NULL;

    for (phlen = j = 0; j < dict_pronlen(dict, wid); ++j)
        phlen += strlen(dict_ciphone_str(dict, wid, j)) + 1;
    phones = ckd_calloc(1, phlen);
    for (j = 0; j < dict_pronlen(dict, wid); ++j) {
        strcat(phones, dict_ciphone_str(dict, wid, j));
        if (j != dict_pronlen(dict, wid) - 1)
            strcat(phones, " ");
    }
    return phones;
}

long
ps_decode_raw(ps_decoder_t *ps, FILE *rawfh,
              long maxsamps)
{
    int16 *data;
    long total, pos, endpos;

    ps_start_utt(ps);

    /* If this file is seekable or maxsamps is specified, then decode
     * the whole thing at once. */
    if (maxsamps != -1) {
        data = ckd_calloc(maxsamps, sizeof(*data));
        total = fread(data, sizeof(*data), maxsamps, rawfh);
        ps_process_raw(ps, data, total, FALSE, TRUE);
        ckd_free(data);
    } else if ((pos = ftell(rawfh)) >= 0) {
        fseek(rawfh, 0, SEEK_END);
        endpos = ftell(rawfh);
        fseek(rawfh, pos, SEEK_SET);
        maxsamps = endpos - pos;

        data = ckd_calloc(maxsamps, sizeof(*data));
        total = fread(data, sizeof(*data), maxsamps, rawfh);
        ps_process_raw(ps, data, total, FALSE, TRUE);
        ckd_free(data);
    } else {
        /* Otherwise decode it in a stream. */
        total = 0;
        while (!feof(rawfh)) {
            int16 data[256];
            size_t nread;

            nread = fread(data, sizeof(*data), sizeof(data)/sizeof(*data), rawfh);
            ps_process_raw(ps, data, nread, FALSE, FALSE);
            total += nread;
        }
    }
    ps_end_utt(ps);
    return total;
}

int
ps_start_stream(ps_decoder_t *ps)
{
    if (ps->acmod == NULL)
        return -1;
    if (ps->acmod->fe == NULL)
        return -1;
    if (ps->acmod->fe->noise_stats == NULL)
        return -1;
    fe_reset_noisestats(ps->acmod->fe->noise_stats);
    return 0;
}

int
ps_get_in_speech(ps_decoder_t *ps)
{
    return (ps->acmod->state == ACMOD_STARTED || ps->acmod->state == ACMOD_PROCESSING);
}

int
ps_start_utt(ps_decoder_t *ps)
{
    int rv;
    char uttid[16];
    
    if (ps->acmod->state == ACMOD_STARTED || ps->acmod->state == ACMOD_PROCESSING) {
	E_ERROR("Utterance already started\n");
	return -1;
    }

    if (ps->search == NULL) {
        E_ERROR("No search module is selected, did you forget to "
                "specify a language model or grammar?\n");
        return -1;
    }

    ptmr_reset(&ps->perf);
    ptmr_start(&ps->perf);

    sprintf(uttid, "%09u", ps->uttno);
    ++ps->uttno;

    /* Remove any residual word lattice and hypothesis. */
    ps_lattice_free(ps->search->dag);
    ps->search->dag = NULL;
    ps->search->last_link = NULL;
    ps->search->post = 0;
    ckd_free(ps->search->hyp_str);
    ps->search->hyp_str = NULL;
    if ((rv = acmod_start_utt(ps->acmod)) < 0)
        return rv;

    /* Start logging features and audio if requested. */
    if (ps->mfclogdir) {
        char *logfn = string_join(ps->mfclogdir, "/",
                                  uttid, ".mfc", NULL);
        FILE *mfcfh;
        E_INFO("Writing MFCC file: %s\n", logfn);
        if ((mfcfh = fopen(logfn, "wb")) == NULL) {
            E_ERROR_SYSTEM("Failed to open MFCC file %s", logfn);
            ckd_free(logfn);
            return -1;
        }
        ckd_free(logfn);
        acmod_set_mfcfh(ps->acmod, mfcfh);
    }
    if (ps->rawlogdir) {
        char *logfn = string_join(ps->rawlogdir, "/",
                                  uttid, ".raw", NULL);
        FILE *rawfh;
        E_INFO("Writing raw audio file: %s\n", logfn);
        if ((rawfh = fopen(logfn, "wb")) == NULL) {
            E_ERROR_SYSTEM("Failed to open raw audio file %s", logfn);
            ckd_free(logfn);
            return -1;
        }
        ckd_free(logfn);
        acmod_set_rawfh(ps->acmod, rawfh);
    }
    if (ps->senlogdir) {
        char *logfn = string_join(ps->senlogdir, "/",
                                  uttid, ".sen", NULL);
        FILE *senfh;
        E_INFO("Writing senone score file: %s\n", logfn);
        if ((senfh = fopen(logfn, "wb")) == NULL) {
            E_ERROR_SYSTEM("Failed to open senone score file %s", logfn);
            ckd_free(logfn);
            return -1;
        }
        ckd_free(logfn);
        acmod_set_senfh(ps->acmod, senfh);
    }

    /* Start auxiliary phone loop search. */
    if (ps->phone_loop)
        ps_search_start(ps->phone_loop);

    return ps_search_start(ps->search);
}

static int
ps_search_forward(ps_decoder_t *ps)
{
    int nfr;

    if (ps->search == NULL) {
        E_ERROR("No search module is selected, did you forget to "
                "specify a language model or grammar?\n");
        return -1;
    }
    nfr = 0;
    while (ps->acmod->n_feat_frame > 0) {
        int k;
        if (ps->pl_window > 0)
            if ((k = ps_search_step(ps->phone_loop, ps->acmod->output_frame)) < 0)
                return k;
        if (ps->acmod->output_frame >= ps->pl_window)
            if ((k = ps_search_step(ps->search,
                                    ps->acmod->output_frame - ps->pl_window)) < 0)
                return k;
        acmod_advance(ps->acmod);
        ++ps->n_frame;
        ++nfr;
    }
    return nfr;
}

int
ps_decode_senscr(ps_decoder_t *ps, FILE *senfh)
{
    int nfr, n_searchfr;

    ps_start_utt(ps);
    n_searchfr = 0;
    acmod_set_insenfh(ps->acmod, senfh);
    while ((nfr = acmod_read_scores(ps->acmod)) > 0) {
        if ((nfr = ps_search_forward(ps)) < 0) {
            ps_end_utt(ps);
            return nfr;
        }
        n_searchfr += nfr;
    }
    ps_end_utt(ps);
    acmod_set_insenfh(ps->acmod, NULL);

    return n_searchfr;
}

int
ps_process_raw(ps_decoder_t *ps,
               int16 const *data,
               size_t n_samples,
               int no_search,
               int full_utt)
{
    int n_searchfr = 0;

    if (ps->acmod->state == ACMOD_IDLE) {
	E_ERROR("Failed to process data, utterance is not started. Use start_utt to start it\n");
	return 0;
    }

    if (no_search)
        acmod_set_grow(ps->acmod, TRUE);

    while (n_samples) {
        int nfr;

        /* Process some data into features. */
        if ((nfr = acmod_process_raw(ps->acmod, &data,
                                     &n_samples, full_utt)) < 0)
            return nfr;

        /* Score and search as much data as possible */
        if (no_search)
            continue;
        if ((nfr = ps_search_forward(ps)) < 0)
            return nfr;
        n_searchfr += nfr;
    }

    return n_searchfr;
}

int
ps_process_cep(ps_decoder_t *ps,
               float32 **data,
               int32 n_frames,
               int no_search,
               int full_utt)
{
    int n_searchfr = 0;

#ifdef FIXED_POINT
    mfcc_t **idata, **ptr;
    ptr = idata = ckd_calloc_2d(n_frames,
                                fe_get_output_size(ps->acmod->fe),
                                sizeof(**idata));
    fe_float_to_mfcc(ps->acmod->fe, data, idata, n_frames);
#else
    mfcc_t **ptr = data;
#endif

    if (no_search)
        acmod_set_grow(ps->acmod, TRUE);

    while (n_frames) {
        int nfr;

        /* Process some data into features. */
        if ((nfr = acmod_process_cep(ps->acmod, &ptr,
                                     &n_frames, full_utt)) < 0)
            return nfr;

        /* Score and search as much data as possible */
        if (no_search)
            continue;
        if ((nfr = ps_search_forward(ps)) < 0)
            return nfr;
        n_searchfr += nfr;
    }
#ifdef FIXED_POINT
    ckd_free_2d(idata);
#endif

    return n_searchfr;
}

int
ps_end_utt(ps_decoder_t *ps)
{
    int rv, i;

    if (ps->search == NULL) {
        E_ERROR("No search module is selected, did you forget to "
                "specify a language model or grammar?\n");
        return -1;
    }
    if (ps->acmod->state == ACMOD_ENDED || ps->acmod->state == ACMOD_IDLE) {
	E_ERROR("Utterance is not started\n");
	return -1;
    }
    acmod_end_utt(ps->acmod);

    /* Search any remaining frames. */
    if ((rv = ps_search_forward(ps)) < 0) {
        ptmr_stop(&ps->perf);
        return rv;
    }
    /* Finish phone loop search. */
    if (ps->phone_loop) {
        if ((rv = ps_search_finish(ps->phone_loop)) < 0) {
            ptmr_stop(&ps->perf);
            return rv;
        }
    }
    /* Search any frames remaining in the lookahead window. */
    if (ps->acmod->output_frame >= ps->pl_window) {
        for (i = ps->acmod->output_frame - ps->pl_window;
             i < ps->acmod->output_frame; ++i)
            ps_search_step(ps->search, i);
    }
    /* Finish main search. */
    if ((rv = ps_search_finish(ps->search)) < 0) {
        ptmr_stop(&ps->perf);
        return rv;
    }
    ptmr_stop(&ps->perf);

    /* Log a backtrace if requested. */
    if (ps_config_bool(ps->config, "backtrace")) {
        const char* hyp;
        ps_seg_t *seg;
        int32 score;

        hyp = ps_get_hyp(ps, &score);
        
        if (hyp != NULL) {
    	    E_INFO("%s (%d)\n", hyp, score);
    	    E_INFO_NOFN("%-20s %-5s %-5s %-5s %-10s %-10s %-3s\n",
                    "word", "start", "end", "pprob", "ascr", "lscr", "lback");
    	    for (seg = ps_seg_iter(ps); seg;
        	 seg = ps_seg_next(seg)) {
    	        char const *word;
        	int sf, ef;
        	int32 post, lscr, ascr, lback;

        	word = ps_seg_word(seg);
        	ps_seg_frames(seg, &sf, &ef);
        	post = ps_seg_prob(seg, &ascr, &lscr, &lback);
        	E_INFO_NOFN("%-20s %-5d %-5d %-1.3f %-10d %-10d %-3d\n",
                    	    word, sf, ef, logmath_exp(ps_get_logmath(ps), post),
                    	ascr, lscr, lback);
    	    }
        }
    }
    return rv;
}

char const *
ps_get_hyp(ps_decoder_t *ps, int32 *out_best_score)
{
    char const *hyp;

    if (ps->search == NULL) {
        E_ERROR("No search module is selected, did you forget to "
                "specify a language model or grammar?\n");
        return NULL;
    }
    ptmr_start(&ps->perf);
    hyp = ps_search_hyp(ps->search, out_best_score);
    ptmr_stop(&ps->perf);
    return hyp;
}

int32
ps_get_prob(ps_decoder_t *ps)
{
    int32 prob;

    if (ps->search == NULL) {
        E_ERROR("No search module is selected, did you forget to "
                "specify a language model or grammar?\n");
        return -1;
    }
    ptmr_start(&ps->perf);
    prob = ps_search_prob(ps->search);
    ptmr_stop(&ps->perf);
    return prob;
}

ps_seg_t *
ps_seg_iter(ps_decoder_t *ps)
{
    ps_seg_t *itor;

    if (ps->search == NULL) {
        E_ERROR("No search module is selected, did you forget to "
                "specify a language model or grammar?\n");
        return NULL;
    }
    ptmr_start(&ps->perf);
    itor = ps_search_seg_iter(ps->search);
    ptmr_stop(&ps->perf);
    return itor;
}

ps_seg_t *
ps_seg_next(ps_seg_t *seg)
{
    return ps_search_seg_next(seg);
}

char const *
ps_seg_word(ps_seg_t *seg)
{
    return seg->text;
}

void
ps_seg_frames(ps_seg_t *seg, int *out_sf, int *out_ef)
{
    if (out_sf) *out_sf = seg->sf;
    if (out_ef) *out_ef = seg->ef;
}

int32
ps_seg_prob(ps_seg_t *seg, int32 *out_ascr, int32 *out_lscr, int32 *out_lback)
{
    if (out_ascr) *out_ascr = seg->ascr;
    if (out_lscr) *out_lscr = seg->lscr;
    if (out_lback) *out_lback = seg->lback;
    return seg->prob;
}

void
ps_seg_free(ps_seg_t *seg)
{
    ps_search_seg_free(seg);
}

ps_lattice_t *
ps_get_lattice(ps_decoder_t *ps)
{
    if (ps->search == NULL) {
        E_ERROR("No search module is selected, did you forget to "
                "specify a language model or grammar?\n");
        return NULL;
    }
    return ps_search_lattice(ps->search);
}

ps_nbest_t *
ps_nbest(ps_decoder_t *ps)
{
    ps_lattice_t *dag;
    ngram_model_t *lmset;
    ps_astar_t *nbest;
    float32 lwf;

    if (ps->search == NULL) {
        E_ERROR("No search module is selected, did you forget to "
                "specify a language model or grammar?\n");
        return NULL;
    }
    if ((dag = ps_get_lattice(ps)) == NULL)
        return NULL;

    /* FIXME: This is all quite specific to N-Gram search.  Either we
     * should make N-best a method for each search module or it needs
     * to be abstracted to work for N-Gram and FSG. */
    if (0 != strcmp(ps_search_type(ps->search), PS_SEARCH_TYPE_NGRAM)) {
        lmset = NULL;
        lwf = 1.0f;
    } else {
        lmset = ((ngram_search_t *)ps->search)->lmset;
        lwf = ((ngram_search_t *)ps->search)->bestpath_fwdtree_lw_ratio;
    }

    nbest = ps_astar_start(dag, lmset, lwf, 0, -1, -1, -1);

    nbest = ps_nbest_next(nbest);

    return (ps_nbest_t *)nbest;
}

void
ps_nbest_free(ps_nbest_t *nbest)
{
    ps_astar_finish(nbest);
}

ps_nbest_t *
ps_nbest_next(ps_nbest_t *nbest)
{
    ps_latpath_t *next;

    next = ps_astar_next(nbest);
    if (next == NULL) {
        ps_nbest_free(nbest);
        return NULL;
    }
    return nbest;
}

char const *
ps_nbest_hyp(ps_nbest_t *nbest, int32 *out_score)
{
    assert(nbest != NULL);

    if (nbest->top == NULL)
        return NULL;
    if (out_score) *out_score = nbest->top->score;
    return ps_astar_hyp(nbest, nbest->top);
}

ps_seg_t *
ps_nbest_seg(ps_nbest_t *nbest)
{
    if (nbest->top == NULL)
        return NULL;

    return ps_astar_seg_iter(nbest, nbest->top, 1.0);
}

int
ps_get_n_frames(ps_decoder_t *ps)
{
    return ps->acmod->output_frame + 1;
}

void
ps_get_utt_time(ps_decoder_t *ps, double *out_nspeech,
                double *out_ncpu, double *out_nwall)
{
    int32 frate;

    frate = ps_config_int(ps->config, "frate");
    *out_nspeech = (double)ps->acmod->output_frame / frate;
    *out_ncpu = ps->perf.t_cpu;
    *out_nwall = ps->perf.t_elapsed;
}

void
ps_get_all_time(ps_decoder_t *ps, double *out_nspeech,
                double *out_ncpu, double *out_nwall)
{
    int32 frate;

    frate = ps_config_int(ps->config, "frate");
    *out_nspeech = (double)ps->n_frame / frate;
    *out_ncpu = ps->perf.t_tot_cpu;
    *out_nwall = ps->perf.t_tot_elapsed;
}

void
ps_search_init(ps_search_t *search, ps_searchfuncs_t *vt,
	       const char *type,
	       const char *name,
               ps_config_t *config, acmod_t *acmod, dict_t *dict,
               dict2pid_t *d2p)
{
    search->vt = vt;
    search->name = ckd_salloc(name);
    search->type = ckd_salloc(type);

    search->config = config;
    search->acmod = acmod;
    if (d2p)
        search->d2p = dict2pid_retain(d2p);
    else
        search->d2p = NULL;
    if (dict) {
        search->dict = dict_retain(dict);
        search->start_wid = dict_startwid(dict);
        search->finish_wid = dict_finishwid(dict);
        search->silence_wid = dict_silwid(dict);
        search->n_words = dict_size(dict);
    }
    else {
        search->dict = NULL;
        search->start_wid = search->finish_wid = search->silence_wid = -1;
        search->n_words = 0;
    }
}

void
ps_search_base_free(ps_search_t *search)
{
    /* FIXME: We will have refcounting on acmod, config, etc, at which
     * point we will free them here too. */
    ckd_free(search->name);
    ckd_free(search->type);
    dict_free(search->dict);
    dict2pid_free(search->d2p);
    ckd_free(search->hyp_str);
    ps_lattice_free(search->dag);
}

void
ps_search_base_reinit(ps_search_t *search, dict_t *dict,
                      dict2pid_t *d2p)
{
    dict_free(search->dict);
    dict2pid_free(search->d2p);
    /* FIXME: _retain() should just return NULL if passed NULL. */
    if (dict) {
        search->dict = dict_retain(dict);
        search->start_wid = dict_startwid(dict);
        search->finish_wid = dict_finishwid(dict);
        search->silence_wid = dict_silwid(dict);
        search->n_words = dict_size(dict);
    }
    else {
        search->dict = NULL;
        search->start_wid = search->finish_wid = search->silence_wid = -1;
        search->n_words = 0;
    }
    if (d2p)
        search->d2p = dict2pid_retain(d2p);
    else
        search->d2p = NULL;
}
