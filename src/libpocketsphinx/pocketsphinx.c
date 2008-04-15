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

/* SphinxBase headers. */
#include <err.h>
#include <strfuncs.h>
#include <filename.h>
#include <pio.h>

/* Local headers. */
#include "pocketsphinx_internal.h"
#include "cmdln_macro.h"
#include "fsg_search_internal.h"
#include "ngram_search.h"
#include "ngram_search_fwdtree.h"
#include "ngram_search_fwdflat.h"
#include "ngram_search_dag.h"

static const arg_t ps_args_def[] = {
    input_cmdln_options(),
    waveform_to_cepstral_command_line_macro(),
    output_cmdln_options(),
    am_cmdln_options(),
    lm_cmdln_options(),
    dictionary_cmdln_options(),
    fsg_cmdln_options(),
    beam_cmdln_options(),
    search_cmdln_options(),
    CMDLN_EMPTY_OPTION
};

/* I'm not sure what the portable way to do this is. */
static int
file_exists(const char *path)
{
    FILE *tmp;

    tmp = fopen(path, "rb");
    if (tmp) fclose(tmp);
    return (tmp != NULL);
}

static void
pocketsphinx_add_file(pocketsphinx_t *ps, const char *arg,
                      const char *hmmdir, const char *file)
{
    char *tmp = string_join(hmmdir, "/", file, NULL);

    if (cmd_ln_str_r(ps->config, arg) == NULL && file_exists(tmp)) {
        cmd_ln_set_str_r(ps->config, arg, tmp);
        ps->strings = glist_add_ptr(ps->strings, tmp);
    }
    else {
        ckd_free(tmp);
    }
}

static void
pocketsphinx_init_defaults(pocketsphinx_t *ps)
{
    char *hmmdir;

    /* Disable memory mapping on Blackfin (FIXME: should be uClinux in general). */
#ifdef __ADSPBLACKFIN__
    E_INFO("Will not use mmap() on uClinux/Blackfin.");
    cmd_ln_set_boolean_r(ps->config, "-mmap", FALSE);
#endif
    /* Get acoustic model filenames and add them to the command-line */
    if ((hmmdir = cmd_ln_str_r(ps->config, "-hmm")) != NULL) {
        pocketsphinx_add_file(ps, "-mdef", hmmdir, "mdef");
        pocketsphinx_add_file(ps, "-mean", hmmdir, "means");
        pocketsphinx_add_file(ps, "-var", hmmdir, "variances");
        pocketsphinx_add_file(ps, "-tmat", hmmdir, "transition_matrices");
        pocketsphinx_add_file(ps, "-mixw", hmmdir, "mixture_weights");
        pocketsphinx_add_file(ps, "-sendump", hmmdir, "sendump");
        pocketsphinx_add_file(ps, "-kdtree", hmmdir, "kdtrees");
        pocketsphinx_add_file(ps, "-fdict", hmmdir, "noisedict");
        pocketsphinx_add_file(ps, "-featparams", hmmdir, "feat.params");
    }
}

int
pocketsphinx_reinit(pocketsphinx_t *ps, cmd_ln_t *config)
{
    char *fsgfile = NULL;
    char *lmfile, *lmctl = NULL;
    gnode_t *gn;

    if (config && config != ps->config) {
        if (ps->config) {
            for (gn = ps->strings; gn; gn = gnode_next(gn))
                ckd_free(gnode_ptr(gn));
            cmd_ln_free_r(ps->config);
        }
        ps->config = config;
    }
    /* Fill in some default arguments. */
    pocketsphinx_init_defaults(ps);

    /* Logmath computation (used in acmod and search) */
    if (ps->lmath == NULL
        || (logmath_get_base(ps->lmath) != 
            (float64)cmd_ln_float32_r(config, "-logbase"))) {
        if (ps->lmath)
            logmath_free(ps->lmath);
        ps->lmath = logmath_init
            ((float64)cmd_ln_float32_r(config, "-logbase"), 0, FALSE);
    }

    /* Acoustic model (this is basically everything that
     * uttproc.c, senscr.c, and others used to do) */
    if (ps->acmod)
        acmod_free(ps->acmod);
    if ((ps->acmod = acmod_init(config, ps->lmath, NULL, NULL)) == NULL)
        return -1;

    /* Make the acmod's feature buffer growable if we are doing two-pass search. */
    if (cmd_ln_boolean_r(config, "-fwdflat")
        && cmd_ln_boolean_r(config, "-fwdtree"))
        acmod_set_grow(ps->acmod, TRUE);

    /* Dictionary and triphone mappings (depends on acmod). */
    if (ps->dict)
        dict_free(ps->dict);
    if ((ps->dict = dict_init(config, ps->acmod->mdef)) == NULL)
        return -1;

    /* Determine whether we are starting out in FSG or N-Gram search mode. */
    if (ps->searches) {
        for (gn = ps->searches; gn; gn = gnode_next(gn))
            ps_search_free(gnode_ptr(gn));
        glist_free(ps->searches);
        ps->searches = NULL;
        ps->search = NULL;
    }
    if ((fsgfile = cmd_ln_str_r(config, "-fsg"))) {
        ps_search_t *fsgs;

        fsgs = fsg_search_init(config, ps->acmod, ps->dict);
        ps->searches = glist_add_ptr(ps->searches, fsgs);
        ps->search = fsgs;
    }
    else if ((lmfile = cmd_ln_str_r(config, "-lm"))
             || (lmctl = cmd_ln_str_r(config, "-lmctlfn"))) {
        ps_search_t *ngs;

        ngs = ngram_search_init(config, ps->acmod, ps->dict);
        ps->searches = glist_add_ptr(ps->searches, ngs);
        ps->search = ngs;
    }
    /* Otherwise, we will initialize the search whenever the user
     * decides to load an FSG or a language model. */

    /* Initialize performance timer. */
    ps->perf.name = "decode";
    ptmr_init(&ps->perf);

    return 0;
}

pocketsphinx_t *
pocketsphinx_init(cmd_ln_t *config)
{
    pocketsphinx_t *ps;

    ps = ckd_calloc(1, sizeof(*ps));
    if (pocketsphinx_reinit(ps, config) < 0) {
        pocketsphinx_free(ps);
        return NULL;
    }
    return ps;
}

arg_t const *
pocketsphinx_args(void)
{
    return ps_args_def;
}

void
pocketsphinx_free(pocketsphinx_t *ps)
{
    gnode_t *gn;

    for (gn = ps->searches; gn; gn = gnode_next(gn))
        ps_search_free(gnode_ptr(gn));
    glist_free(ps->searches);
    dict_free(ps->dict);
    acmod_free(ps->acmod);
    logmath_free(ps->lmath);
    cmd_ln_free_r(ps->config);
    for (gn = ps->strings; gn; gn = gnode_next(gn))
        ckd_free(gnode_ptr(gn));
    glist_free(ps->strings);
    ckd_free(ps);
}

cmd_ln_t *
pocketsphinx_get_config(pocketsphinx_t *ps)
{
    return ps->config;
}

logmath_t *
pocketsphinx_get_logmath(pocketsphinx_t *ps)
{
    return ps->lmath;
}

fsg_set_t *
pocketsphinx_get_fsgset(pocketsphinx_t *ps)
{
    return NULL;
}

int
pocketsphinx_run_ctl_file(pocketsphinx_t *ps,
			  char const *ctlfile,
                          char const *format)
{
    return -1;
}

int
pocketsphinx_start_utt(pocketsphinx_t *ps)
{
    int rv;

    ptmr_reset(&ps->perf);
    ptmr_start(&ps->perf);

    if ((rv = acmod_start_utt(ps->acmod)) < 0)
        return rv;

    return ps_search_start(ps->search);
}

int
pocketsphinx_process_raw(pocketsphinx_t *ps,
			 int16 const *data,
			 size_t n_samples,
			 int no_search,
			 int full_utt)
{
    int n_searchfr = 0;

    if (no_search)
        acmod_set_grow(ps->acmod, TRUE);

    while (n_samples) {
        int nfr;

        /* Process some data into features. */
        if ((nfr = acmod_process_raw(ps->acmod, &data,
                                    &n_samples, full_utt)) < 0)
            return nfr;

        /* Score and search as much data as possible */
        if (!no_search) {
            while ((nfr = ps_search_step(ps->search)) > 0) {
                n_searchfr += nfr;
            }
            if (nfr < 0)
                return nfr;
        }
    }

    ps->n_frame += n_searchfr;
    return n_searchfr;
}

int
pocketsphinx_process_cep(pocketsphinx_t *ps,
			 mfcc_t **data,
			 int32 n_frames,
			 int no_search,
			 int full_utt)
{
    int n_searchfr = 0;

    if (no_search)
        acmod_set_grow(ps->acmod, TRUE);

    while (n_frames) {
        int nfr;

        /* Process some data into features. */
        if ((nfr = acmod_process_cep(ps->acmod, &data,
                                     &n_frames, full_utt)) < 0)
            return nfr;

        /* Score and search as much data as possible */
        if (!no_search) {
            while ((nfr = ps_search_step(ps->search)) > 0) {
                n_searchfr += nfr;
            }
            if (nfr < 0)
                return nfr;
        }
    }

    ps->n_frame += n_searchfr;
    return n_searchfr;
}

int
pocketsphinx_end_utt(pocketsphinx_t *ps)
{
    int rv;

    acmod_end_utt(ps->acmod);
    while ((rv = ps_search_step(ps->search)) > 0) {
    }
    if (rv < 0) {
        ptmr_stop(&ps->perf);
        return rv;
    }
    rv = ps_search_finish(ps->search);
    ptmr_stop(&ps->perf);
    return rv;
}

char const *
pocketsphinx_get_hyp(pocketsphinx_t *ps, int32 *out_best_score)
{
    char const *hyp;

    ptmr_start(&ps->perf);
    hyp = ps_search_hyp(ps->search, out_best_score);
    ptmr_stop(&ps->perf);
    return hyp;
}

ps_seg_t *
pocketsphinx_seg_iter(pocketsphinx_t *ps, int32 *out_best_score)
{
    ps_seg_t *itor;

    ptmr_start(&ps->perf);
    itor = ps_search_seg_iter(ps->search, out_best_score);
    ptmr_stop(&ps->perf);
    return itor;
}

ps_seg_t *
pocketsphinx_seg_next(ps_seg_t *seg)
{
    return ps_search_seg_next(seg);
}

char const *
pocketsphinx_seg_word(ps_seg_t *seg)
{
    return seg->word;
}

void
pocketsphinx_seg_frames(ps_seg_t *seg, int *out_sf, int *out_ef)
{
    *out_sf = seg->sf;
    *out_ef = seg->ef;
}

void
pocketsphinx_seg_prob(ps_seg_t *seg, int32 *out_pprob)
{
    *out_pprob = seg->prob;
}

void
pocketsphinx_seg_free(ps_seg_t *seg)
{
    ps_search_seg_free(seg);
}

void
pocketsphinx_get_utt_time(pocketsphinx_t *ps, double *out_nspeech,
                          double *out_ncpu, double *out_nwall)
{
    int32 frate;

    frate = cmd_ln_int32_r(ps->config, "-frate");
    *out_nspeech = (double)ps->acmod->output_frame / frate;
    *out_ncpu = ps->perf.t_cpu;
    *out_nwall = ps->perf.t_elapsed;
}

void
pocketsphinx_get_all_time(pocketsphinx_t *ps, double *out_nspeech,
                          double *out_ncpu, double *out_nwall)
{
    int32 frate;

    frate = cmd_ln_int32_r(ps->config, "-frate");
    *out_nspeech = (double)ps->n_frame / frate;
    *out_ncpu = ps->perf.t_tot_cpu;
    *out_nwall = ps->perf.t_tot_elapsed;
}

void
ps_search_init(ps_search_t *search, ps_searchfuncs_t *vt,
               cmd_ln_t *config, acmod_t *acmod, dict_t *dict)
{
    search->vt = vt;
    search->config = config;
    search->acmod = acmod;
    search->dict = dict;
}

void
ps_search_deinit(ps_search_t *search)
{
    /* FIXME: We will have refcounting on acmod, config, etc, at which
     * point we will free them here too. */
    ckd_free(search->hyp_str);
}
