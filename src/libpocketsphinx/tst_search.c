/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 2009 Carnegie Mellon University.  All rights
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
 * @file tst_search.c Time-conditioned lexicon tree search ("S3")
 * @author David Huggins-Daines and Krisztian Loki
 */

/* System headers. */
#include <string.h>
#include <assert.h>

/* SphinxBase headers. */
#include <ckd_alloc.h>
#include <listelem_alloc.h>
#include <profile.h>
#include <err.h>

/* Local headers. */
#include "pocketsphinx_internal.h"
#include "ps_lattice_internal.h"
#include "tst_search.h"

static int tst_search_start(ps_search_t *search);
static int tst_search_step(ps_search_t *search, int frame_idx);
static int tst_search_finish(ps_search_t *search);
static int tst_search_reinit(ps_search_t *search);
static ps_lattice_t *tst_search_lattice(ps_search_t *search);
static char const *tst_search_hyp(ps_search_t *search, int32 *out_score);
static int32 tst_search_prob(ps_search_t *search);
static ps_seg_t *tst_search_seg_iter(ps_search_t *search, int32 *out_score);

static ps_searchfuncs_t tst_funcs = {
    /* name: */   "tst",
    /* start: */  tst_search_start,
    /* step: */   tst_search_step,
    /* finish: */ tst_search_finish,
    /* reinit: */ tst_search_reinit,
    /* free: */   tst_search_free,
    /* lattice: */  tst_search_lattice,
    /* hyp: */      tst_search_hyp,
    /* prob: */     tst_search_prob,
    /* seg_iter: */ tst_search_seg_iter,
};

static histprune_t *
histprune_init(int32 maxhmm, int32 maxhist, int32 maxword,
               int32 hmmhistbinsize, int32 numNodes)
{
    histprune_t *h;
    int32 n;

    h = (histprune_t *) ckd_calloc(1, sizeof(histprune_t));
    h->maxwpf = maxword;
    h->maxhmmpf = maxhmm;
    h->maxhistpf = maxhist;
    h->hmm_hist_binsize = hmmhistbinsize;

    n = numNodes;
    n /= h->hmm_hist_binsize;

    h->hmm_hist_bins = n + 1;

    h->hmm_hist = (int32 *) ckd_calloc(h->hmm_hist_bins, sizeof(int32));


    return h;
}

static void
histprune_zero_histbin(histprune_t * h)
{
    int32 *hmm_hist;
    int32 numhistbins;          /* Local version of number of histogram bins, don't expect it to change */
    int32 i;

    hmm_hist = h->hmm_hist;
    numhistbins = h->hmm_hist_bins;

    for (i = 0; i < numhistbins; i++)
        hmm_hist[i] = 0;

}


static void
histprune_update_histbinsize(histprune_t * h,
                             int32 hmmhistbinsize, int32 numNodes)
{
    int32 n;
    h->hmm_hist_binsize = hmmhistbinsize;
    n = numNodes;
    n /= h->hmm_hist_binsize;

    h->hmm_hist_bins = n + 1;

    h->hmm_hist =
        (int32 *) ckd_realloc(h->hmm_hist,
                              h->hmm_hist_bins * sizeof(int32));
}

static void
histprune_free(histprune_t * h)
{
    if (h != NULL) {
        if (h->hmm_hist != NULL) {
            ckd_free(h->hmm_hist);
        }
        free(h);
    }
}

static void
histprune_showhistbin(histprune_t * hp, int32 nfr, char *uttid)
{
    int32 i, j, k;

    if (nfr == 0) {
        nfr = 1;
        E_WARN("Set number of frame to 1\n");
    }

    for (j = hp->hmm_hist_bins - 1; (j >= 0) && (hp->hmm_hist[j] == 0);
         --j);
    E_INFO("HMMHist[0..%d](%s):", j, uttid);
    for (i = 0, k = 0; i <= j; i++) {
        k += hp->hmm_hist[i];
        E_INFOCONT(" %d(%d)", hp->hmm_hist[i], (k * 100) / nfr);
    }
    E_INFOCONT("\n");
}


static void
histprune_report(histprune_t * h)
{
    E_INFO_NOFN("Initialization of histprune_t, report:\n");
    E_INFO_NOFN("Parameters used in histogram pruning:\n");
    E_INFO_NOFN("Max.     HMM per frame=%d\n", h->maxhmmpf);
    E_INFO_NOFN("Max. history per frame=%d\n", h->maxhistpf);
    E_INFO_NOFN("Max.    word per frame=%d\n", h->maxwpf);
    E_INFO_NOFN("Size of histogram  bin=%d\n", h->hmm_hist_binsize);
    E_INFO_NOFN("No.  of histogram  bin=%d\n", h->hmm_hist_bins);
    E_INFO_NOFN("\n");
}

static beam_t *
beam_init(float64 hmm, float64 ptr, float64 wd, float64 wdend,
          int32 ptranskip, int32 n_ciphone, logmath_t *logmath)
{
    beam_t *beam;

    beam = (beam_t *) ckd_calloc(1, sizeof(beam_t));

    beam->hmm = logmath_log(logmath, hmm);
    beam->ptrans = logmath_log(logmath, ptr);
    beam->word = logmath_log(logmath, wd);
    beam->wordend = logmath_log(logmath, wdend);
    beam->ptranskip = ptranskip;
    beam->bestscore = MAX_NEG_INT32;
    beam->bestwordscore = MAX_NEG_INT32;
    beam->n_ciphone = n_ciphone;

    beam->wordbestscores = (int32 *) ckd_calloc(n_ciphone, sizeof(int32));
    beam->wordbestexits = (int32 *) ckd_calloc(n_ciphone, sizeof(int32));

    return beam;
}

static void
beam_report(beam_t * b)
{
    E_INFO_NOFN("Initialization of beam_t, report:\n");
    E_INFO_NOFN("Parameters used in Beam Pruning of Viterbi Search:\n");
    E_INFO_NOFN("Beam=%d\n", b->hmm);
    E_INFO_NOFN("PBeam=%d\n", b->ptrans);
    E_INFO_NOFN("WBeam=%d (Skip=%d)\n", b->word, b->ptranskip);
    E_INFO_NOFN("WEndBeam=%d \n", b->wordend);
    E_INFO_NOFN("No of CI Phone assumed=%d \n", b->n_ciphone);
    E_INFO_NOFN("\n");
}

static void
beam_free(beam_t * b)
{
    if (b) {
        if (b->wordbestscores) {
            free(b->wordbestscores);
        }
        if (b->wordbestexits) {
            free(b->wordbestexits);
        }
        free(b);
    }
}


static lextree_t **
create_lextree(tst_search_t *tstg, const char *lmname, ptmr_t *tm_build)
{
    int j;
    lextree_t **lextree;

    lextree = (lextree_t **)ckd_calloc(tstg->n_lextree, sizeof(lextree_t *));
    hash_table_enter(tstg->ugtree, lmname, lextree);
    for (j = 0; j < tstg->n_lextree; ++j) {
        if (tm_build)
            ptmr_start(tm_build);

        lextree[j] = lextree_init(ps_search_acmod(tstg)->mdef,
                                  ps_search_acmod(tstg)->tmat,
                                  tstg->dict,
                                  tstg->dict2pid,
                                  tstg->lmset,
                                  lmname,
                                  TRUE, TRUE,
                                  LEXTREE_TYPE_UNIGRAM);

        if (tm_build)
            ptmr_stop(tm_build);

        if (lextree[j] == NULL) {
            int i;
            E_INFO
                ("Failed to allocate lexical tree for lm %s and lextree %d\n",
                 lmname, j);

            for (i = j - 1; i >= 0; --i)
                lextree_free(lextree[i]);
            ckd_free(lextree);

            return NULL;
        }
        E_INFO("Lextree (%d) for lm \"%s\" has %d nodes(ug)\n",
               j, lmname, lextree_n_node(lextree[j]));
    }

    lextree_report(lextree[0]);

    return lextree;
}

ps_search_t *
tst_search_init(cmd_ln_t *config,
                acmod_t *acmod,
                dict_t *dict)
{
    tst_search_t *tstg;
    char const *path;
    int32 n_ltree;
    int32 i, j;
    ptmr_t tm_build;
    ngram_model_set_iter_t *lmset_iter;
    lextree_t **lextree = NULL;

    tstg = ckd_calloc(1, sizeof(*tstg));
    ps_search_init(&tstg->base, &tst_funcs, config, acmod, dict);

    /* Need to build a Sphinx3 dictionary, fillpen_t, beam_t, and
     * dict2pid (for the time being) */
    tstg->dict = s3dict_init(acmod->mdef,
                             cmd_ln_str_r(config, "-dict"),
                             cmd_ln_str_r(config, "-fdict"),
                             FALSE, TRUE);
    tstg->dict2pid = dict2pid_build(acmod->mdef,
                                    tstg->dict, TRUE,
                                    acmod->lmath);
    tstg->fillpen = fillpen_init(tstg->dict, NULL,
                                 cmd_ln_float32("-silprob"),
                                 cmd_ln_float32("-fillprob"),
                                 cmd_ln_float32("-lw"),
                                 cmd_ln_float32("-wip"),
                                 acmod->lmath);
    tstg->beam = beam_init(cmd_ln_float64_r(config, "-beam"),
                           cmd_ln_float64_r(config, "-pbeam"),
                           cmd_ln_float64_r(config, "-wbeam"),
                           cmd_ln_float64_r(config, "-lpbeam"),
                           0, bin_mdef_n_ciphone(acmod->mdef),
                           acmod->lmath);
    beam_report(tstg->beam);

    ptmr_init(&(tm_build));

    tstg->epl = cmd_ln_int32_r(config, "-epl");
    tstg->n_lextree = cmd_ln_int32_r(config, "-Nlextree");
    n_ltree = tstg->n_lextree;

    /* Initialize language model set (this is done for us in Sphinx3,
     * but not in PocketSphinx) */
    if ((path = cmd_ln_str_r(config, "-lmctl"))) {
        tstg->lmset = ngram_model_set_read(config, path, acmod->lmath);
        if (tstg->lmset == NULL) {
            E_ERROR("Failed to read language model control file: %s\n",
                    path);
            goto error_out;
        }
        /* Set the default language model if needed. */
        if ((path = cmd_ln_str_r(config, "-lmname"))) {
            ngram_model_set_select(tstg->lmset, path);
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
        tstg->lmset = ngram_model_set_init(config,
                                          &lm, (char **)&name,
                                          NULL, 1);
        if (tstg->lmset == NULL) {
            E_ERROR("Failed to initialize language model set\n");
            goto error_out;
        }
    }

    /* Unlink silences (FIXME: wtf) */
#if 0
    for (lmset_iter = ngram_model_set_iter(kbcore_lmset(kbc)); lmset_iter; lmset_iter = ngram_model_set_iter_next(lmset_iter)) {
        const char *model_name;
        unlinksilences(ngram_model_set_iter_model(lmset_iter, &model_name), kbc, kbc->dict);
    }
#endif


    /* STRUCTURE and REPORT: Initialize lexical tree. Filler tree's
       initialization is followed.  */
    tstg->ugtree =
        hash_table_new(ngram_model_set_count(tstg->lmset), HASH_CASE_YES);

    /* curugtree is a pointer pointing the current unigram tree which
       were being used. */

    tstg->curugtree =
        (lextree_t **) ckd_calloc(n_ltree, sizeof(lextree_t *));

    /* Just allocate pointers */

    ptmr_reset(&(tm_build));
    /*
     * Only create the lextree for the entire language model set
     * specific ones will be created in srch_TST_set_lm()
     */
    if ((lextree = create_lextree(tstg, "lmset", &tm_build)) == NULL) {
        goto error_out;
    }
    for (j = 0; j < n_ltree; ++j)
        tstg->curugtree[j] = lextree[j];

    E_INFO("Time for building trees, %4.4f CPU %4.4f Clk\n",
           tm_build.t_cpu, tm_build.t_elapsed);

    /* STRUCTURE: Create filler lextrees */
    /* ARCHAN : only one filler tree is supposed to be built even for dynamic LMs */
    tstg->fillertree =
        (lextree_t **) ckd_calloc(n_ltree, sizeof(lextree_t *));

    for (i = 0; i < n_ltree; i++) {
        if ((tstg->fillertree[i] = fillertree_init(acmod->mdef, acmod->tmat,
                                                   tstg->dict, tstg->dict2pid,
                                                   tstg->fillpen)) == NULL) {
            E_INFO("Fail to allocate filler tree  %d\n", i);
            goto error_out;
        }
        E_INFO("Lextrees(%d), %d nodes(filler)\n",
               i, lextree_n_node(tstg->fillertree[0]));
    }

    if (cmd_ln_int32_r(config, "-lextreedump")) {
        for (lmset_iter = ngram_model_set_iter(tstg->lmset);
             lmset_iter; lmset_iter = ngram_model_set_iter_next(lmset_iter)) {
            const char *model_name;
            ngram_model_set_iter_model(lmset_iter, &model_name);
            hash_table_lookup(tstg->ugtree, model_name, (void*)&lextree);
            for (j = 0; j < n_ltree; j++) {
                E_INFO("LM name: \'%s\' UGTREE: %d\n",
                        model_name, j);
                lextree_dump(lextree[j], stderr,
                             cmd_ln_int32_r(config, "-lextreedump"));
            }
        }
        ngram_model_set_iter_free(lmset_iter);
        for (i = 0; i < n_ltree; i++) {
            E_INFO("FILLERTREE %d\n", i);
            lextree_dump(tstg->fillertree[i], stderr,
                         cmd_ln_int32_r(config, "-lextreedump"));
        }
    }

    tstg->histprune = histprune_init(cmd_ln_int32_r(config, "-maxhmmpf"),
                                     cmd_ln_int32_r(config, "-maxhistpf"),
                                     cmd_ln_int32_r(config, "-maxwpf"),
                                     cmd_ln_int32_r(config, "-hmmhistbinsize"),
                                     (tstg->curugtree[0]->n_node + tstg->fillertree[0]->n_node) *
                                     tstg->n_lextree);

    /* Viterbi history structure */
    tstg->vithist = vithist_init(ngram_model_get_counts(tstg->lmset)[0],
                                 bin_mdef_n_ciphone(acmod->mdef),
                                 tstg->beam->word,
                                 cmd_ln_int32_r(config, "-bghist"), TRUE);

    return (ps_search_t *)tstg;

error_out:
    tst_search_free(ps_search_base(tstg));
    return NULL;
}

void
tst_search_free(ps_search_t *search)
{
    ckd_free(search);
}

static int
tst_search_start(ps_search_t *search)
{

    return 0;
}

static int
tst_search_step(ps_search_t *search, int frame_idx)
{
    return -1;
}

static int
tst_search_finish(ps_search_t *search)
{
    return -1;
}

static int
tst_search_reinit(ps_search_t *search)
{
    return -1;
}

static ps_lattice_t *
tst_search_lattice(ps_search_t *search)
{
    return NULL;
}

static char const *
tst_search_hyp(ps_search_t *search, int32 *out_score)
{
    return NULL;
}

static int32
tst_search_prob(ps_search_t *search)
{
    return 1;
}

static ps_seg_t *
tst_search_seg_iter(ps_search_t *search, int32 *out_score)
{
    return NULL;
}
