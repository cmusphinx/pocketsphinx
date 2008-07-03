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

/* SphinxBase headers. */
#include <err.h>
#include <strfuncs.h>
#include <filename.h>
#include <pio.h>

/* Local headers. */
#include "cmdln_macro.h"
#include "pocketsphinx_internal.h"
#include "ps_lattice_internal.h"
#include "fsg_search_internal.h"
#include "ngram_search.h"
#include "ngram_search_fwdtree.h"
#include "ngram_search_fwdflat.h"

static const arg_t ps_args_def[] = {
    POCKETSPHINX_OPTIONS,
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
ps_add_file(ps_decoder_t *ps, const char *arg,
            const char *hmmdir, const char *file)
{
    char *tmp = string_join(hmmdir, "/", file, NULL);

    if (cmd_ln_str_r(ps->config, arg) == NULL && file_exists(tmp))
        cmd_ln_set_str_r(ps->config, arg, tmp);
    ckd_free(tmp);
}

static void
ps_init_defaults(ps_decoder_t *ps)
{
    char const *hmmdir;

    /* Disable memory mapping on Blackfin (FIXME: should be uClinux in general). */
#ifdef __ADSPBLACKFIN__
    E_INFO("Will not use mmap() on uClinux/Blackfin.");
    cmd_ln_set_boolean_r(ps->config, "-mmap", FALSE);
#endif
    /* Get acoustic model filenames and add them to the command-line */
    if ((hmmdir = cmd_ln_str_r(ps->config, "-hmm")) != NULL) {
        ps_add_file(ps, "-mdef", hmmdir, "mdef");
        ps_add_file(ps, "-mean", hmmdir, "means");
        ps_add_file(ps, "-var", hmmdir, "variances");
        ps_add_file(ps, "-tmat", hmmdir, "transition_matrices");
        ps_add_file(ps, "-mixw", hmmdir, "mixture_weights");
        ps_add_file(ps, "-sendump", hmmdir, "sendump");
        ps_add_file(ps, "-kdtree", hmmdir, "kdtrees");
        ps_add_file(ps, "-fdict", hmmdir, "noisedict");
        ps_add_file(ps, "-lda", hmmdir, "feature_transform");
        ps_add_file(ps, "-featparams", hmmdir, "feat.params");
    }
}

int
ps_reinit(ps_decoder_t *ps, cmd_ln_t *config)
{
    char const *fsgfile = NULL;
    char const *lmfile, *lmctl = NULL;
    gnode_t *gn;

    if (config && config != ps->config) {
        cmd_ln_free_r(ps->config);
        ps->config = config;
    }
    /* Fill in some default arguments. */
    ps_init_defaults(ps);

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

        if ((fsgs = fsg_search_init(config, ps->acmod, ps->dict)) == NULL)
            return -1;
        ps->searches = glist_add_ptr(ps->searches, fsgs);
        ps->search = fsgs;
    }
    else if ((lmfile = cmd_ln_str_r(config, "-lm"))
             || (lmctl = cmd_ln_str_r(config, "-lmctl"))) {
        ps_search_t *ngs;

        if ((ngs = ngram_search_init(config, ps->acmod, ps->dict)) == NULL)
            return -1;
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

ps_decoder_t *
ps_init(cmd_ln_t *config)
{
    ps_decoder_t *ps;

    ps = ckd_calloc(1, sizeof(*ps));
    ps->refcount = 1;
    if (ps_reinit(ps, config) < 0) {
        ps_free(ps);
        return NULL;
    }
    return ps;
}

arg_t const *
ps_args(void)
{
    return ps_args_def;
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
    gnode_t *gn;

    if (ps == NULL)
        return 0;
    if (--ps->refcount > 0)
        return ps->refcount;
    for (gn = ps->searches; gn; gn = gnode_next(gn))
        ps_search_free(gnode_ptr(gn));
    glist_free(ps->searches);
    dict_free(ps->dict);
    acmod_free(ps->acmod);
    logmath_free(ps->lmath);
    cmd_ln_free_r(ps->config);
    ckd_free(ps->uttid);
    ckd_free(ps);
    return 0;
}

cmd_ln_t *
ps_get_config(ps_decoder_t *ps)
{
    return ps->config;
}

logmath_t *
ps_get_logmath(ps_decoder_t *ps)
{
    return ps->lmath;
}

ngram_model_t *
ps_get_lmset(ps_decoder_t *ps)
{
    if (ps->search == NULL
        || 0 != strcmp(ps_search_name(ps->search), "ngram"))
        return NULL;
    return ((ngram_search_t *)ps->search)->lmset;
}

ngram_model_t *
ps_update_lmset(ps_decoder_t *ps, ngram_model_t *lmset)
{
    ngram_search_t *ngs;
    gnode_t *gn;

    /* Look for N-Gram search. */
    for (gn = ps->searches; gn; gn = gnode_next(gn)) {
        if (0 == strcmp(ps_search_name(gnode_ptr(gn)), "ngram"))
            break;
    }
    if (gn == NULL) {
        /* Initialize N-Gram search. */
        ngs = (ngram_search_t *)ngram_search_init(ps->config,
                                                  ps->acmod, ps->dict);
        if (ngs == NULL)
            return NULL;
        ps->searches = glist_add_ptr(ps->searches, ngs);
    }
    else {
        ngs = gnode_ptr(gn);
        /* Free any previous lmset if this is a new one. */
        if (ngs->lmset != NULL && ngs->lmset != lmset)
            ngram_model_free(ngs->lmset);
        ngs->lmset = lmset;
        /* Tell N-Gram search to update its view of the world. */
        if (ps_search_reinit(ps_search_base(ngs)) < 0)
            return NULL;
    }
    ps->search = ps_search_base(ngs);
    return ngs->lmset;
}

fsg_set_t *
ps_get_fsgset(ps_decoder_t *ps)
{
    if (ps->search == NULL
        || 0 != strcmp(ps_search_name(ps->search), "fsg"))
        return NULL;
    return (fsg_set_t *)ps->search;
}

fsg_set_t *
ps_update_fsgset(ps_decoder_t *ps)
{
    gnode_t *gn;
    fsg_search_t *fsgs;

    /* Look for FSG search. */
    for (gn = ps->searches; gn; gn = gnode_next(gn)) {
        if (0 == strcmp(ps_search_name(gnode_ptr(gn)), "fsg"))
            break;
    }
    if (gn == NULL) {
        /* Initialize FSG search. */
        fsgs = (fsg_search_t *)fsg_search_init(ps->config,
                                               ps->acmod, ps->dict);
        ps->searches = glist_add_ptr(ps->searches, fsgs);
    }
    else {
        /* Tell FSG search to update its view of the world. */
        fsgs = gnode_ptr(gn);
        if (ps_search_reinit(ps_search_base(fsgs)) < 0)
            return NULL;
    }
    ps->search = ps_search_base(fsgs);
    return (fsg_set_t *)fsgs;
}

int
ps_add_word(ps_decoder_t *ps,
            char const *word,
            char const *phones,
            int update)
{
    int32 wid, lmwid;
    ngram_model_t *lmset;
    char *pron;
    int rv;

    pron = ckd_salloc(phones);
    if ((wid = dict_add_word(ps->dict, word, pron)) == -1) {
        ckd_free(pron);
        return -1;
    }
    ckd_free(pron);

    if ((lmset = ps_get_lmset(ps)) != NULL) {
        /* FIXME: There is a way more efficient way to do this, since all
         * we did was replace a placeholder string with the new word
         * string - therefore what we ought to do is add it directly to
         * the current LM, then update the mapping without reallocating
         * everything. */
        /* Add it to the LM set (meaning, the current LM).  In a perfect
         * world, this would result in the same WID, but because of the
         * weird way that word IDs are handled, it doesn't. */
        if ((lmwid = ngram_model_add_word(lmset, word, 1.0))
            == NGRAM_INVALID_WID)
            return -1;
    }
 
    /* Rebuild the widmap and search tree if requested. */
    if (update) {
        if ((rv = ps_search_reinit(ps->search) < 0))
            return rv;
    }
    return wid;
}

int
ps_decode_raw(ps_decoder_t *ps, FILE *rawfh,
              char const *uttid, long maxsamps)
{
    long total, pos;

    ps_start_utt(ps, uttid);
    /* If this file is seekable or maxsamps is specified, then decode
     * the whole thing at once. */
    if (maxsamps != -1 || (pos = ftell(rawfh)) >= 0) {
        int16 *data;

        if (maxsamps == -1) {
            long endpos;
            fseek(rawfh, 0, SEEK_END);
            endpos = ftell(rawfh);
            fseek(rawfh, pos, SEEK_SET);
            maxsamps = endpos - pos;
        }
        data = ckd_calloc(maxsamps, sizeof(*data));
        total = fread(data, sizeof(*data), maxsamps, rawfh);
        ps_process_raw(ps, data, total, FALSE, TRUE);
        ckd_free(data);
    }
    else {
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
ps_start_utt(ps_decoder_t *ps, char const *uttid)
{
    int rv;

    if (ps->search == NULL) {
        E_ERROR("No search module is selected, did you forget to "
                "specify a language model or grammar?\n");
        return -1;
    }

    ptmr_reset(&ps->perf);
    ptmr_start(&ps->perf);

    if (uttid) {
        ckd_free(ps->uttid);
        ps->uttid = ckd_salloc(uttid);
    }
    else {
        char nuttid[16];
        ckd_free(ps->uttid);
        sprintf(nuttid, "%09u", ps->uttno);
        ps->uttid = ckd_salloc(nuttid);
        ++ps->uttno;
    }
    /* Remove any residual word lattice and hypothesis. */
    ps_lattice_free(ps->search->dag);
    ps->search->dag = NULL;
    ckd_free(ps->search->hyp_str);
    ps->search->hyp_str = NULL;

    if ((rv = acmod_start_utt(ps->acmod)) < 0)
        return rv;

    return ps_search_start(ps->search);
}

int
ps_process_raw(ps_decoder_t *ps,
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
ps_process_cep(ps_decoder_t *ps,
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
ps_end_utt(ps_decoder_t *ps)
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

    /* Log a backtrace if requested. */
    if (cmd_ln_boolean_r(ps->config, "-backtrace")) {
        char const *uttid, *hyp;
        ps_seg_t *seg;
        int32 score;

        hyp = ps_get_hyp(ps, &score, &uttid);
        E_INFO("%s: %s\n", uttid, hyp, score);
        E_INFO_NOFN("%-20s %-5s %-5s %-5s %-10s %-10s %-3s\n",
                    "word", "start", "end", "pprob", "ascr", "lscr", "lback");
        for (seg = ps_seg_iter(ps, &score); seg;
             seg = ps_seg_next(seg)) {
            char const *word;
            int sf, ef;
            int32 post, lscr, ascr, lback;

            word = ps_seg_word(seg);
            ps_seg_frames(seg, &sf, &ef);
            post = ps_seg_prob(seg, &ascr, &lscr, &lback);
            E_INFO_NOFN("%-20s %-5d %-5d %-1.3f %-10d %-10d %-3d\n",
                        word, sf, ef, logmath_exp(ps_get_logmath(ps), post), ascr, lscr, lback);
        }
    }
    return rv;
}

char const *
ps_get_hyp(ps_decoder_t *ps, int32 *out_best_score, char const **uttid)
{
    char const *hyp;

    ptmr_start(&ps->perf);
    hyp = ps_search_hyp(ps->search, out_best_score);
    if (uttid)
        *uttid = ps->uttid;
    ptmr_stop(&ps->perf);
    return hyp;
}

ps_seg_t *
ps_seg_iter(ps_decoder_t *ps, int32 *out_best_score)
{
    ps_seg_t *itor;

    ptmr_start(&ps->perf);
    itor = ps_search_seg_iter(ps->search, out_best_score);
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
    return seg->word;
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
    return ps_search_lattice(ps->search);
}

ps_nbest_t *
ps_nbest(ps_decoder_t *ps, int sf, int ef,
         char const *ctx1, char const *ctx2)
{
    ps_lattice_t *dag;
    ngram_model_t *lmset;
    ps_astar_t *nbest;
    float32 lwf;
    int32 w1, w2;

    if (ps->search == NULL)
        return NULL;
    if ((dag = ps_get_lattice(ps)) == NULL)
        return NULL;

    /* FIXME: This is all quite specific to N-Gram search.  Either we
     * should make N-best a method for each search module or it needs
     * to be abstracted to work for N-Gram and FSG. */
    if (0 != strcmp(ps_search_name(ps->search), "ngram")) {
        lmset = NULL;
        lwf = 1.0f;
    }
    else {
        lmset = ((ngram_search_t *)ps->search)->lmset;
        lwf = ((ngram_search_t *)ps->search)->bestpath_fwdtree_lw_ratio;
    }

    w1 = ctx1 ? dict_to_id(ps_search_dict(ps->search), ctx1) : -1;
    w2 = ctx2 ? dict_to_id(ps_search_dict(ps->search), ctx2) : -1;
    nbest = ps_astar_start(dag, lmset, lwf, sf, ef, w1, w2);

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
    if (nbest->paths_done == NULL)
        return NULL;
    if (out_score) *out_score = nbest->paths_done->score;
    return ps_astar_hyp(nbest, nbest->paths_done);
}

ps_seg_t *
ps_nbest_seg(ps_nbest_t *nbest, int32 *out_score)
{
    if (nbest->paths_done == NULL)
        return NULL;
    if (out_score) *out_score = nbest->paths_done->score;
    return ps_astar_seg_iter(nbest, nbest->paths_done, 1.0);
}

void
ps_get_utt_time(ps_decoder_t *ps, double *out_nspeech,
                double *out_ncpu, double *out_nwall)
{
    int32 frate;

    frate = cmd_ln_int32_r(ps->config, "-frate");
    *out_nspeech = (double)ps->acmod->output_frame / frate;
    *out_ncpu = ps->perf.t_cpu;
    *out_nwall = ps->perf.t_elapsed;
}

void
ps_get_all_time(ps_decoder_t *ps, double *out_nspeech,
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
    search->start_wid = dict_to_id(dict, "<s>");
    search->finish_wid = dict_to_id(dict, "</s>");
    search->silence_wid = dict_to_id(dict, "<sil>");
}

void
ps_search_deinit(ps_search_t *search)
{
    /* FIXME: We will have refcounting on acmod, config, etc, at which
     * point we will free them here too. */
    ckd_free(search->hyp_str);
    ps_lattice_free(search->dag);
}
