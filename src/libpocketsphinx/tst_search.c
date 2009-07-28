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
                                  tstg->fillpen,
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

static void
tst_search_update_widmap(tst_search_t *tstg)
{
    const char **words;
    int32 i, n_words;

    /* It's okay to include fillers since they won't be in the LM */
    n_words = s3dict_size(tstg->dict);
    words = ckd_calloc(n_words, sizeof(*words));
    /* This will include alternates, again, that's okay since they aren't in the LM */
    for (i = 0; i < n_words; ++i)
        words[i] = (const char *)s3dict_wordstr(tstg->dict, i);
    ngram_model_set_map_words(tstg->lmset, words, n_words);
    ckd_free(words);
}

static int
tst_search_reinit(ps_search_t *search)
{
    tst_search_t *tstg = (tst_search_t *)search;

    /* set_lm() code goes here. */
    return -1;
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
                                 cmd_ln_float32_r(config, "-silprob"),
                                 cmd_ln_float32_r(config, "-fillprob"),
                                 cmd_ln_float32_r(config, "-lw"),
                                 cmd_ln_float32_r(config, "-wip"),
                                 acmod->lmath);
    tstg->beam = beam_init(cmd_ln_float64_r(config, "-beam"),
                           cmd_ln_float64_r(config, "-pbeam"),
                           cmd_ln_float64_r(config, "-wbeam"),
                           cmd_ln_float64_r(config, "-lpbeam"),
                           0, bin_mdef_n_ciphone(acmod->mdef),
                           acmod->lmath);
    beam_report(tstg->beam);

    tstg->ssid_active = bitvec_alloc(bin_mdef_n_sseq(acmod->mdef));
    tstg->comssid_active = bitvec_alloc(dict2pid_n_comsseq(tstg->dict2pid));
    tstg->composite_senone_scores = ckd_calloc(bin_mdef_n_sen(acmod->mdef),
                                               sizeof(*tstg->composite_senone_scores));

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
    tst_search_t *tstg = (tst_search_t *)search;

    if (tstg == NULL)
        return;

    if (tstg->ugtree) {
        hash_iter_t *hiter = hash_table_iter(tstg->ugtree);
        if (hiter)
            do {
                lextree_t **lextree = (lextree_t**)hash_entry_val(hiter->ent);
                if (lextree) {
                    int j;
                    for (j = 0; j < tstg->n_lextree; ++j)
                        lextree_free(lextree[j]);
                }
            } while ((hiter = hash_table_iter_next(hiter)));
        hash_table_free(tstg->ugtree);
    }

    ckd_free(tstg->curugtree);
    if (tstg->fillertree) {
        int32 j;
        for (j = 0; j < tstg->n_lextree; ++j)
            lextree_free(tstg->fillertree[j]);
        ckd_free(tstg->fillertree);
    }

    if (tstg->vithist)
        vithist_free(tstg->vithist);
    if (tstg->histprune)
        histprune_free(tstg->histprune);
    if (tstg->dict)
        s3dict_free(tstg->dict);
    if (tstg->dict2pid)
        dict2pid_free(tstg->dict2pid);
    if (tstg->fillpen)
        fillpen_free(tstg->fillpen);
    if (tstg->beam)
        beam_free(tstg->beam);
    bitvec_free(tstg->ssid_active);
    bitvec_free(tstg->comssid_active);
    ckd_free(tstg->composite_senone_scores);

    ckd_free(tstg);
}

static int
tst_search_start(ps_search_t *search)
{
    tst_search_t *tstg = (tst_search_t *)search;
    int32 n, pred;
    int32 i;

    /* Clean up any previous viterbi history */
    vithist_utt_reset(tstg->vithist);
    histprune_zero_histbin(tstg->histprune);

    /* Insert initial <s> into vithist structure */
    pred = vithist_utt_begin(tstg->vithist,
                             s3dict_startwid(tstg->dict),
                             ngram_wid(tstg->lmset, "<s>"));
    assert(pred == 0);          /* Vithist entry ID for <s> */

    /* Need to reinitialize GMMs, things like that? */

    /* Enter into unigram lextree[0] */
    n = lextree_n_next_active(tstg->curugtree[0]);
    assert(n == 0);
    lextree_enter(tstg->curugtree[0],
                  bin_mdef_silphone(ps_search_acmod(tstg)->mdef),
                  -1, 0, pred, tstg->beam->hmm);

    /* Enter into filler lextree */
    n = lextree_n_next_active(tstg->fillertree[0]);
    assert(n == 0);
    lextree_enter(tstg->fillertree[0], BAD_S3CIPID, -1, 0, pred, tstg->beam->hmm);

    tstg->n_lextrans = 1;

    tstg->exit_id = -1;

    for (i = 0; i < tstg->n_lextree; i++) {
        lextree_active_swap(tstg->curugtree[i]);
        lextree_active_swap(tstg->fillertree[i]);
    }

    return 0;
}

static int
tst_search_finish(ps_search_t *search)
{
    tst_search_t *tstg = (tst_search_t *)search;
    int32 i;

    /* Find the exit word and wrap up Viterbi history (but don't reset
     * it yet!) */
    tstg->exit_id = vithist_utt_end(tstg->vithist,
                                    tstg->lmset, tstg->dict,
                                    tstg->dict2pid, tstg->fillpen);

    /* Statistics update/report */
    /* st->utt_wd_exit = vithist_n_entry(tstg->vithist); */
    /* Not sure hwo to get the uttid. */
    /* histprune_showhistbin(tstg->histprune, st->nfr, s->uttid); */

    for (i = 0; i < tstg->n_lextree; i++) {
        lextree_utt_end(tstg->curugtree[i]);
        lextree_utt_end(tstg->fillertree[i]);
    }

    if (tstg->exit_id >= 0)
        return 0;
    else
        return -1;
}

static int
srch_TST_hmm_compute_lv2(tst_search_t *tstg, int32 frmno)
{
    /* This is local to this codebase */

    int32 i, j;
    lextree_t *lextree;
    beam_t *bm;
    histprune_t *hp;

    int32 besthmmscr, bestwordscr;
    int32 frm_nhmm, hb, pb, wb;
    int32 n_ltree;              /* Local version of number of lexical trees used */
    int32 maxwpf;               /* Local version of Max words per frame, don't expect it to change */
    int32 maxhistpf;            /* Local version of Max histories per frame, don't expect it to change */
    int32 maxhmmpf;             /* Local version of Max active HMMs per frame, don't expect it to change  */
    int32 histbinsize;          /* Local version of histogram bin size, don't expect it to change */
    int32 numhistbins;          /* Local version of number of histogram bins, don't expect it to change */
    int32 hmmbeam;              /* Local version of hmm beam, don't expect it to change. */
    int32 pbeam;                /* Local version of phone beam, don't expect it to change. */
    int32 wbeam;                /* Local version of word beam , don't expect it to change */
    int32 *hmm_hist;            /* local version of histogram bins. */

    n_ltree = tstg->n_lextree;
    hp = tstg->histprune;
    bm = tstg->beam;
    hmm_hist = hp->hmm_hist;

    maxwpf = hp->maxwpf;
    maxhistpf = hp->maxhistpf;
    maxhmmpf = hp->maxhmmpf;
    histbinsize = hp->hmm_hist_binsize;
    numhistbins = hp->hmm_hist_bins;

    hmmbeam = bm->hmm;
    pbeam = bm->ptrans;
    wbeam = bm->word;

    /* Evaluate active HMMs in each lextree; note best HMM state score */
    besthmmscr = MAX_NEG_INT32;
    bestwordscr = MAX_NEG_INT32;
    frm_nhmm = 0;

    for (i = 0; i < (n_ltree << 1); i++) {
        lextree =
            (i < n_ltree) ? tstg->curugtree[i] : tstg->fillertree[i - n_ltree];
        if (tstg->hmmdumpfp != NULL)
            fprintf(tstg->hmmdumpfp, "Fr %d Lextree %d #HMM %d\n", frmno, i,
                    lextree->n_active);
        lextree_hmm_eval(lextree,
                         ps_search_acmod(tstg)->senone_scores,
                         tstg->composite_senone_scores,
                         frmno, tstg->hmmdumpfp);

        if (besthmmscr < lextree->best)
            besthmmscr = lextree->best;
        if (bestwordscr < lextree->wbest)
            bestwordscr = lextree->wbest;

        /* st->utt_hmm_eval += lextree->n_active; */
        frm_nhmm += lextree->n_active;
    }
    if (besthmmscr > 0) {
        E_ERROR
            ("***ERROR*** Fr %d, best HMM score > 0 (%d); int32 wraparound?\n",
             frmno, besthmmscr);
    }


    /* Hack! similar to the one in mode 5. The reason though is because
       dynamic allocation of node cause the histroyt array need to be
       allocated too.  I skipped this step by just making simple
       assumption here.
     */
    if (frm_nhmm / histbinsize > hp->hmm_hist_bins - 1)
        hmm_hist[hp->hmm_hist_bins - 1]++;
    else
        hmm_hist[frm_nhmm / histbinsize]++;

    /* Set pruning threshold depending on whether number of active HMMs 
     * is within limit 
     */
    /* ARCHAN: MAGIC */
    if (maxhmmpf != -1 && frm_nhmm > (maxhmmpf + (maxhmmpf >> 1))) {
        int32 *bin, nbin, bw;

        /* Use histogram pruning */
        nbin = 1000;
        bw = -(hmmbeam) / nbin;
        bin = (int32 *) ckd_calloc(nbin, sizeof(int32));

        for (i = 0; i < (n_ltree << 1); i++) {
            lextree = (i < n_ltree) ?
                tstg->curugtree[i] : tstg->fillertree[i - n_ltree];

            lextree_hmm_histbin(lextree, besthmmscr, bin, nbin, bw);
        }

        for (i = 0, j = 0; (i < nbin) && (j < maxhmmpf); i++, j += bin[i]);
        ckd_free((void *) bin);

        /* Determine hmm, phone, word beams */
        hb = -(i * bw);
        pb = (hb > pbeam) ? hb : pbeam;
        wb = (hb > wbeam) ? hb : wbeam;
    }
    else {
        hb = hmmbeam;
        pb = pbeam;
        wb = wbeam;
    }

    bm->bestscore = besthmmscr;
    bm->bestwordscore = bestwordscr;

    bm->thres = bm->bestscore + hb;     /* HMM survival threshold */
    bm->phone_thres = bm->bestscore + pb;       /* Cross-HMM transition threshold */
    bm->word_thres = bm->bestwordscore + wb;    /* Word exit threshold */

    return 0;
}

static int
srch_TST_propagate_graph_ph_lv2(tst_search_t *tstg, int32 frmno)
{
    int32 i;
    int32 n_ltree;              /* Local version of number of lexical trees used */

    lextree_t *lextree;

    n_ltree = tstg->n_lextree;

    for (i = 0; i < (n_ltree << 1); i++) {
        lextree = (i < n_ltree)
            ? tstg->curugtree[i]
            : tstg->fillertree[i - tstg-> n_lextree];

        if (lextree_hmm_propagate_non_leaves(lextree, frmno,
                                             tstg->beam->thres,
                                             tstg->beam->phone_thres,
                                             tstg->beam->word_thres)
            != LEXTREE_OPERATION_SUCCESS) {
            E_ERROR
                ("Propagation Failed for lextree_hmm_propagate_non_leave at tree %d\n",
                 i);
            lextree_utt_end(lextree);
            return -1;
        }
    }

    return 0;
}

static void
mdef_sseq2sen_active(bin_mdef_t * mdef, bitvec_t * sseq, bitvec_t * sen)
{
    int32 ss, i;
    s3senid_t *sp;

    for (ss = 0; ss < bin_mdef_n_sseq(mdef); ss++) {
        if (bitvec_is_set(sseq,ss)) {
            sp = mdef->sseq[ss];
            for (i = 0; i < mdef_n_emit_state(mdef); i++)
                bitvec_set(sen, sp[i]);
        }
    }
}

static int
srch_TST_select_active_gmm(tst_search_t *tstg)
{
    int32 n_ltree;              /* Local version of number of lexical trees used */
    dict2pid_t *d2p;
    bin_mdef_t *mdef;
    lextree_t *lextree;
    int32 i;

    n_ltree = tstg->n_lextree;
    mdef = ps_search_acmod(tstg)->mdef;
    d2p = tstg->dict2pid;

    /* Find active senone-sequence IDs (including composite ones) */
    for (i = 0; i < (n_ltree << 1); i++) {
        lextree = (i < n_ltree) ? tstg->curugtree[i] :
            tstg->fillertree[i - n_ltree];
        lextree_ssid_active(lextree, tstg->ssid_active,
                            tstg->comssid_active);
    }

    /* Find active senones from active senone-sequences */
    acmod_clear_active(ps_search_acmod(tstg));
    mdef_sseq2sen_active(mdef, tstg->ssid_active,
                         ps_search_acmod(tstg)->senone_active_vec);

    /* Add in senones needed for active composite senone-sequences */
    dict2pid_comsseq2sen_active(d2p, mdef, tstg->comssid_active,
                                ps_search_acmod(tstg)->senone_active_vec);

    return 0;
}

static int
srch_TST_rescoring(tst_search_t *tstg, int32 frmno)
{
    int32 i;
    int32 n_ltree;              /* Local version of number of lexical trees used */
    lextree_t *lextree;
    vithist_t *vh;

    vh = tstg->vithist;

    n_ltree = tstg->n_lextree;

    for (i = 0; i < (n_ltree << 1); i++) {
        lextree = (i < n_ltree)
            ? tstg->curugtree[i]
            : tstg->fillertree[i - tstg->n_lextree];
        if (lextree_hmm_propagate_leaves
            (lextree, vh, frmno,
             tstg->beam->word_thres) != LEXTREE_OPERATION_SUCCESS) {
            E_ERROR
                ("Propagation Failed for lextree_hmm_propagate_leave at tree %d\n",
                 i);
            lextree_utt_end(lextree);
            return -1;
        }
    }

    return 0;
}

static void
srch_utt_word_trans(tst_search_t * tstg, int32 cf)
{
    int32 k, th;
    vithist_t *vh;
    vithist_entry_t *ve;
    int32 vhid, le, n_ci, score;
    int32 maxpscore;
    int32 *bs = NULL, *bv = NULL, epl;
    beam_t *bm;
    s3wid_t wid;
    int32 p;
    s3dict_t *dict;
    bin_mdef_t *mdef;

    /* Call the rescoring routines at all word end */
    maxpscore = MAX_NEG_INT32;
    bm = tstg->beam;
    vh = tstg->vithist;
    th = bm->bestscore + bm->hmm;       /* Pruning threshold */

    if (vh->bestvh[cf] < 0)
        return;

    dict = tstg->dict;
    mdef = ps_search_acmod(tstg)->mdef;
    n_ci = bin_mdef_n_ciphone(mdef);

    /* Initialize best exit for each distinct word-final CI phone to NONE */

    bs = bm->wordbestscores;
    bv = bm->wordbestexits;
    epl = tstg->epl;

    for (p = 0; p < n_ci; p++) {
        bs[p] = MAX_NEG_INT32;
        bv[p] = -1;
    }

    /* Find best word exit in this frame for each distinct word-final CI phone */
    vhid = vithist_first_entry(vh, cf);
    le = vithist_n_entry(vh) - 1;
    for (; vhid <= le; vhid++) {
        ve = vithist_id2entry(vh, vhid);
        if (!vithist_entry_valid(ve))
            continue;

        wid = vithist_entry_wid(ve);
        p = s3dict_last_phone(dict, wid);
        if (bin_mdef_is_fillerphone(mdef, p))
            p = bin_mdef_silphone(mdef);

        score = vithist_entry_score(ve);
        if (score > bs[p]) {
            bs[p] = score;
            bv[p] = vhid;
            if (maxpscore < score) {
                maxpscore = score;
                /*    E_INFO("maxscore = %d\n", maxpscore); */
            }
        }
    }

    /* Find lextree instance to be entered */
    k = tstg->n_lextrans++;
    k = (k % (tstg->n_lextree * epl)) / epl;

    /* Transition to unigram lextrees */
    for (p = 0; p < n_ci; p++) {
        if (bv[p] >= 0) {
            if (tstg->beam->wordend == 0
                || bs[p] > tstg->beam->wordend + maxpscore) {
                /* RAH, typecast p to (s3cipid_t) to make compiler happy */
                lextree_enter(tstg->curugtree[k], (s3cipid_t) p, cf, bs[p],
                              bv[p], th);
            }
        }

    }

    /* Transition to filler lextrees */
    lextree_enter(tstg->fillertree[k], BAD_S3CIPID, cf, vh->bestscore[cf],
                  vh->bestvh[cf], th);
}

static int
srch_TST_propagate_graph_wd_lv2(tst_search_t *tstg, int32 frmno)
{
    s3dict_t *dict;
    vithist_t *vh;
    histprune_t *hp;
    int32 maxwpf;               /* Local version of Max words per frame, don't expect it to change */
    int32 maxhistpf;            /* Local version of Max histories per frame, don't expect it to change */
    int32 maxhmmpf;             /* Local version of Max active HMMs per frame, don't expect it to change  */


    hp = tstg->histprune;
    vh = tstg->vithist;
    dict = tstg->dict;

    maxwpf = hp->maxwpf;
    maxhistpf = hp->maxhistpf;
    maxhmmpf = hp->maxhmmpf;

    srch_TST_rescoring(tstg, frmno);

    vithist_prune(vh, dict, frmno, maxwpf, maxhistpf,
                  tstg->beam->word_thres - tstg->beam->bestwordscore);

    srch_utt_word_trans(tstg, frmno);

    return 0;
}


static int
srch_TST_frame_windup(tst_search_t *tstg, int32 frmno)
{
    vithist_t *vh;
    int32 i;

    vh = tstg->vithist;

    /* Wind up this frame */
    vithist_frame_windup(vh, frmno, NULL, tstg->lmset, tstg->dict);

    for (i = 0; i < tstg->n_lextree; i++) {
        lextree_active_swap(tstg->curugtree[i]);
        lextree_active_swap(tstg->fillertree[i]);
    }
    return 0;
}

static int
tst_search_step(ps_search_t *search, int frame_idx)
{
    tst_search_t *tstg = (tst_search_t *)search;
    int16 const *senscr;

    /* Select active senones for the current frame. */
    srch_TST_select_active_gmm(tstg);

    /* Compute GMM scores for the current frame. */
    if ((senscr = acmod_score(ps_search_acmod(search), &frame_idx)) == NULL)
        return 0;
    /* Evaluate composite senone scores from senone scores */
    memset(tstg->composite_senone_scores, 0,
           bin_mdef_n_sen(ps_search_acmod(search)->mdef)
           * sizeof(*tstg->composite_senone_scores));
    dict2pid_comsenscr(tstg->dict2pid, senscr, tstg->composite_senone_scores);

    /* Compute HMMs, propagate phone and word exits, etc, etc. */
    srch_TST_hmm_compute_lv2(tstg, frame_idx);
    srch_TST_propagate_graph_ph_lv2(tstg, frame_idx);
    srch_TST_rescoring(tstg, frame_idx);
    srch_TST_propagate_graph_wd_lv2(tstg, frame_idx);
    srch_TST_frame_windup(tstg, frame_idx);

    /* FIXME: Renormalization? */

    /* Return the number of frames processed. */
    return 1;
}

static ps_lattice_t *
tst_search_lattice(ps_search_t *search)
{
    tst_search_t *tstg = (tst_search_t *)search;

    return NULL;
}

static char const *
tst_search_hyp(ps_search_t *base, int32 *out_score)
{
    tst_search_t *tstg = (tst_search_t *)base;
    vithist_entry_t *ve;
    char *c;
    size_t len;
    int32 exit_id, id;

    if (tstg->exit_id == -1) /* Search not finished */
	exit_id = vithist_partialutt_end(tstg->vithist, tstg->lmset, tstg->dict);
    else
        exit_id = tstg->exit_id;

    if (exit_id < 0) {
        E_WARN("Failed to retrieve viterbi history.\n");
        return NULL;
    }

    id = exit_id;
    len = 0;
    while (id >= 0) {
        s3wid_t wid;
        ve = vithist_id2entry(tstg->vithist, id);
        assert(ve);
        wid = vithist_entry_wid(ve);
        id = ve->path.pred;
        if (s3dict_filler_word(tstg->dict, wid)
            || wid == s3dict_startwid(tstg->dict)
            || wid == s3dict_finishwid(tstg->dict))
            continue;
        len += strlen(s3dict_wordstr(tstg->dict, s3dict_basewid(tstg->dict, wid))) + 1;
    }

    ckd_free(base->hyp_str);
    base->hyp_str = ckd_calloc(1, len);
    id = exit_id;
    c = base->hyp_str + len - 1;
    while (id >= 0) {
        s3wid_t wid;
        ve = vithist_id2entry(tstg->vithist, id);
        assert(ve);
        wid = vithist_entry_wid(ve);
        id = ve->path.pred;
        if (s3dict_filler_word(tstg->dict, wid)
            || wid == s3dict_startwid(tstg->dict)
            || wid == s3dict_finishwid(tstg->dict))
            continue;
        len = strlen(s3dict_wordstr(tstg->dict, s3dict_basewid(tstg->dict, wid)));
        c -= len;
        memcpy(c, s3dict_wordstr(tstg->dict, s3dict_basewid(tstg->dict, wid)), len);
        if (c > base->hyp_str) {
            --c;
            *c = ' ';
        }
    }

    return base->hyp_str;
}

static int32
tst_search_prob(ps_search_t *search)
{
    tst_search_t *tstg = (tst_search_t *)search;

    /* Bogus out-of-band value for now. */
    return 1;
}

/**
 * Segmentation "iterator" for vithist results.
 */
typedef struct tst_seg_s {
    ps_seg_t base;  /**< Base structure. */
    int32 *bpidx;   /**< Sequence of backpointer IDs. */
    int16 n_bpidx;  /**< Number of backpointer IDs. */
    int16 cur;      /**< Current position in bpidx. */
} tst_seg_t;

static void
tst_search_bp2itor(ps_seg_t *seg, int id)
{
    tst_search_t *tstg = (tst_search_t *)seg->search;
    vithist_entry_t *ve;

    ve = vithist_id2entry(tstg->vithist, id);
    assert(ve);
    seg->word = s3dict_wordstr(tstg->dict, vithist_entry_wid(ve));
    seg->ef = vithist_entry_ef(ve);
    seg->sf = vithist_entry_sf(ve);
    seg->prob = 0; /* Bogus value... */
    seg->ascr = vithist_entry_ascr(ve);
    seg->lscr = vithist_entry_lscr(ve);
    seg->lback = 0; /* FIXME: Compute this somewhere... */
}

static void
tst_seg_free(ps_seg_t *seg)
{
    tst_seg_t *itor = (tst_seg_t *)seg;
    
    ckd_free(itor->bpidx);
    ckd_free(itor);
}

static ps_seg_t *
tst_seg_next(ps_seg_t *seg)
{
    tst_seg_t *itor = (tst_seg_t *)seg;

    if (++itor->cur == itor->n_bpidx) {
        tst_seg_free(seg);
        return NULL;
    }

    tst_search_bp2itor(seg, itor->bpidx[itor->cur]);
    return seg;
}

static ps_segfuncs_t tst_segfuncs = {
    /* seg_next */ tst_seg_next,
    /* seg_free */ tst_seg_free
};

static ps_seg_t *
tst_search_seg_iter(ps_search_t *search, int32 *out_score)
{
    tst_search_t *tstg = (tst_search_t *)search;
    tst_seg_t *itor;
    int exit_id, id, cur;

    if (tstg->exit_id == -1) /* Search not finished */
	exit_id = vithist_partialutt_end(tstg->vithist, tstg->lmset, tstg->dict);
    else
        exit_id = tstg->exit_id;
    if (exit_id < 0) {
        E_WARN("Failed to retrieve viterbi history.\n");
        return NULL;
    }

    /* Calling this an "iterator" is a bit of a misnomer since we have
     * to get the entire backtrace in order to produce it.  On the
     * other hand, all we actually need is the vithist IDs, and we can
     * allocate a fixed-size array of them. */
    itor = ckd_calloc(1, sizeof(*itor));
    itor->base.vt = &tst_segfuncs;
    itor->base.search = search;
    itor->base.lwf = 1.0;
    itor->n_bpidx = 0;
    id = exit_id;
    while (id >= 0) {
        vithist_entry_t *ve = vithist_id2entry(tstg->vithist, id);
        assert(ve);
        id = vithist_entry_pred(ve);
        ++itor->n_bpidx;
    }
    if (itor->n_bpidx == 0) {
        ckd_free(itor);
        return NULL;
    }
    itor->bpidx = ckd_calloc(itor->n_bpidx, sizeof(*itor->bpidx));
    cur = itor->n_bpidx - 1;
    id = exit_id;
    while (id >= 0) {
        vithist_entry_t *ve = vithist_id2entry(tstg->vithist, id);
        assert(ve);
        itor->bpidx[cur] = id;
        id = vithist_entry_pred(ve);
        --cur;
    }

    /* Fill in relevant fields for first element. */
    tst_search_bp2itor((ps_seg_t *)itor, itor->bpidx[0]);

    return (ps_seg_t *)itor;
}
