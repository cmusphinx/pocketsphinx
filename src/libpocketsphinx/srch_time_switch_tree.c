/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 1999-2004 Carnegie Mellon University.  All rights
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

/* srch_time_switch_tree.c
 * HISTORY
 * 
 * $Log$
 * Revision 1.5  2006/04/17  20:14:12  dhdfu
 * Fix a class-based LM segfault
 * 
 * Revision 1.4  2006/04/05 20:27:34  dhdfu
 * A Great Reorganzation of header files and executables
 *
 * Revision 1.3  2006/02/23 16:46:50  arthchan2003
 * Merged from branch SPHINX3_5_2_RCI_IRII_BRANCH:
 * 1, Used the corresponding lextree interface.
 * 2, 2nd-stage search logic are all commented.
 *
 * Revision 1.2.4.16  2006/01/16 20:14:02  arthchan2003
 * Remove the unlink silences part because that could affect the performance of the 1st pass search when -bestpath is specified.
 *
 * Revision 1.2.4.15  2005/11/17 06:43:25  arthchan2003
 * Removed senone scale in lextree_hmm_propagate.
 *
 * Revision 1.2.4.14  2005/10/17 04:54:49  arthchan2003
 * Freed the tree set correctly.
 *
 * Revision 1.2.4.13  2005/10/07 20:04:50  arthchan2003
 * When rescoring in full triphone expansion, the code should use the score for the word end with corret right context.
 *
 * Revision 1.2.4.12  2005/09/25 19:32:11  arthchan2003
 * (Change for comments) Allow lexical tree to be dumped optionally.
 *
 * Revision 1.2.4.11  2005/09/25 19:23:55  arthchan2003
 * 1, Added arguments for turning on/off LTS rules. 2, Added arguments for turning on/off composite triphones. 3, Moved dict2pid deallocation back to dict2pid. 4, Tidying up the clean up code.
 *
 * Revision 1.2.4.10  2005/09/18 01:46:20  arthchan2003
 * Moved unlinkSilences to mode 4 and mode 5 search-specific implementation.
 *
 * Revision 1.2.4.9  2005/09/11 03:01:01  arthchan2003
 * Bug fix on the size of hmmpf and histpf
 *
 * Revision 1.2.4.8  2005/08/03 18:54:32  dhdfu
 * Fix the support for multi-stream / semi-continuous models.  It is
 * still kind of a hack, but it now works.
 *
 * Revision 1.2.4.7  2005/07/27 23:19:59  arthchan2003
 * Added assert for make sure lmname is valid.
 *
 * Revision 1.2.4.6  2005/07/24 01:41:52  arthchan2003
 * Use ascr provided clearing function instead of directly clearing the array.
 *
 * Revision 1.2.4.5  2005/07/13 02:00:33  arthchan2003
 * Add ptmr_init for lexical tree timer. The program will not cause invalid write on the timer structure.
 *
 * Revision 1.2.4.4  2005/07/07 02:38:35  arthchan2003
 * 1, Remove -lminsearch, 2 Remove rescoring interface in the header.
 *
 * Revision 1.2.4.3  2005/07/04 07:20:48  arthchan2003
 * 1, Ignored -lmsearch, 2, cleaned up memory, 3 added documentation of TST search.
 *
 * Revision 1.2.4.2  2005/07/03 23:19:16  arthchan2003
 * Added free code for srch_time_switch_tree.c
 *
 * Revision 1.2.4.1  2005/06/28 07:03:37  arthchan2003
 * Set lmset correctly in srch_time_switch_tree.
 *
 * Revision 1.2  2005/06/22 02:45:52  arthchan2003
 * Log. Implementation of word-switching tree. Currently only work for a
 * very small test case and it's deliberately fend-off from users. Detail
 * omitted.
 *
 * Revision 1.15  2005/06/17 23:44:40  archan
 * Sphinx3 to s3.generic, 1, Support -lmname in decode and livepretend.  2, Wrap up the initialization of dict2lmwid to lm initialization. 3, add Dave's trick in LM switching in mode 4 of the search.
 *
 * Revision 1.14  2005/06/16 04:59:10  archan
 * Sphinx3 to s3.generic, a gentle-refactored version of Dave's change in senone scale.
 *
 * Revision 1.13  2005/06/11 06:58:35  archan
 * In both mode 4 and mode 5 searches, fixed a serious bug that cause the beam computation screwed up.
 *
 * Revision 1.12  2005/06/03 05:22:52  archan
 * Commented variables after refactor both mode 4 and mode 5 search to use reg_result_dump.
 *
 * Revision 1.11  2005/05/11 06:10:39  archan
 * Code for lattice and back track pointer table dumping is now wrapped in reg_result_dump.  The function is shared across mode 4 and mode 5.  Possibly later for mode 3 and mode 6 as well.
 *
 * Revision 1.10  2005/05/04 04:02:25  archan
 * Implementation of lm addition, deletion in (mode 4) time-switching tree implementation of search.  Not yet tested. Just want to keep up my own momentum.
 *
 * Revision 1.9  2005/05/03 06:57:44  archan
 * Finally. The word switching tree code is completed. Of course, the reporting routine largely duplicate with time switching tree code.  Also, there has to be some bugs in the filler treatment.  But, hey! These stuffs we can work on it.
 *
 * Revision 1.8  2005/05/03 04:09:10  archan
 * Implemented the heart of word copy search. For every ci-phone, every word end, a tree will be allocated to preserve its pathscore.  This is different from 3.5 or below, only the best score for a particular ci-phone, regardless of the word-ends will be preserved at every frame.  The graph propagation will not collect unused word tree at this point. srch_WST_propagate_wd_lv2 is also as the most stupid in the century.  But well, after all, everything needs a start.  I will then really get the results from the search and see how it looks.
 *
 * Revision 1.7  2005/04/25 23:53:35  archan
 * 1, Some minor modification of vithist_t, vithist_rescore can now support optional LM rescoring, vithist also has its own reporting routine. A new argument -lmrescore is also added in decode and livepretend.  This can switch on and off the rescoring procedure. 2, I am reaching the final difficulty of mode 5 implementation.  That is, to implement an algorithm which dynamically decide which tree copies should be entered.  However, stuffs like score propagation in the leave nodes and non-leaves nodes are already done. 3, As briefly mentioned in 2, implementation of rescoring , which used to happened at leave nodes are now separated. The current implementation is not the most clever one. Wish I have time to change it before check-in to the canonical.
 *
 * Revision 1.6  2005/04/25 19:22:48  archan
 * Refactor out the code of rescoring from lexical tree. Potentially we want to turn off the rescoring if we need.
 *
 * 17-Mar-2005 A. Chan (archan@cs.cmu.edu) at Carnegie Mellon University
 *             Started. This replaced utt.c starting from Sphinx 3.6. 
 */

#include "srch.h"
#include "gmm_wrap.h"
#include "astar.h"
#include "dict.h"
#include "cmd_ln.h"
#include "corpus.h"

#define REPORT_SRCH_TST 1

#include "s3types.h"
#include "kb.h"
#include "lextree.h"
#include "lm.h"
#include "vithist.h"

/** 
    \file srch_time_switch_tree.h 
    \brief implementation of Sphinx 3.5 search (aka s3 fast). 

    ARCHAN 20050324: The time condition tree search structure.  The
    structure is reserved for legacy reason and it is explained as
    follows. During the development of Sphinx 2 (pre-1994), we (folks
    in CMU) already realized that using single unigram lexical tree
    will cause so called word segmentation error.  I.e. In bigram
    search, words with different word begins will be collected and
    treated as one.

    At the time of Sphinx 3.1 (~1998-1999) (the first attempt to build
    a fast CDHMM decoder in CMU Sphinx group), Ravi Mosur considered
    this problem and would like to solve this by introducing copies of
    tree.  The standard method to do it was to introduce
    word-conditioned tree copies which probably first introduced by
    the RWTH group. Without careful adjustment of the allocation of
    tree, this implementation method will requires (w+1) * lexical
    trees in search for a bigram implementation.  Where w is the
    number of words in the search architecture.

    Therefore, he decided to use an interesting way to deal with the
    word segmentation problem (and I personally feel that it is very
    innovative).  What he did was instead of making word-conditioned
    tree copies.  He used time-conditioned tree copies. That is to
    say, at every certain frame, he will switch lexical tree in the
    search.  This results in higher survival chance of tokens with an
    early low score and allows Ravi to create a recognizer which has a
    difference only relatively 10% from the s3.0 decode_anytopo. 

    I usually refer this method as "lucky wheel" search because 1)
    there is no principled reason why the switching of tree should be
    done in this way. 2) The switching is done without considering the
    maximum scores. 3) It might well be possible that some word ends
    are active but they are just not "lucky" enough to enter next
    tree.  So, it might amaze you that this search was used and it only
    give 10% relative degradation from s3 slow. 

    To understand why this search works, one could think in this
    way. (i.e.  this is my opinion.).  When it boils down to the
    highest scoring paths in a search. People observed that the time
    information and word information each gives a relative good guess
    to tell whether a path should be preserved or not.  This is
    probably the reason in one version of IBM stack decoder, they
    actually rescoring the priority queue by considering whether a
    word has earlier word-begin.  RWTH's group also has one journal
    paper on making use of the time information.  

    The "lucky wheel" search is one type of search that tries to
    tackling word segmentation problem using time information. It did
    a reasonably good job. Of course, it will still be limited by the
    fact time information is still poorer than the word information as 
    a heuristic measure. 

    This method is remained in sphinx 3.6 for three reasons.  First, I
    personally think that there might be ways to trade-off his method
    and conventional tree copies method. This could result in a better
    memory/accuracy/speed trade-off. Second, I actually don't know
    whether mode 5 (word copies searrch ) will work well in
    general. Last but not least, I preserved the search because of my
    respect to Ravi. :-)
*/

/**
   A note by ARCHAN at 20050703

   In general, what made a LVCSR search to be unique has three major
   factors.  They are 1) how high level knowledge source (LM, FSG) is
   applied, 2) how the lexicon is organized, 3, how cross word
   triphones is realized and implemented.  It is important to realize
   the interplay between these 3 components to realize how a search
   actually really works. 
   
*/

typedef struct {

    /**
     * There can be several unigram lextrees.  If we're at the end of frame f, we can only
     * transition into the roots of lextree[(f+1) % n_lextree]; same for fillertree[].  This
     * alleviates the problem of excessive Viterbi pruning in lextrees.
     */

    int32 n_lextree;		/**< Number of lexical tree for time switching: n_lextree */
    lextree_t **curugtree;        /**< The current unigram tree that used in the search for this utterance. */

    lextree_t **ugtree;           /**< The pool of trees that stores all word trees. */
    lextree_t **fillertree;       /**< The pool of trees that stores all filler trees. */
    int32 n_lextrans;		/**< #Transitions to lextree (root) made so far */
    int32 epl ;                   /**< The number of entry per lexical tree */

    lmset_t* lmset;               /**< The LM set */
    int32 isLMLA;  /**< Is LM lookahead used?*/

    histprune_t *histprune; /**< Structure that wraps up parameters related to  */
  
    vithist_t *vithist;     /**< Viterbi history (backpointer) table */

} srch_TST_graph_t ;

int
srch_TST_init(kb_t * kb, void *srch)
{

    int32 n_ltree;
    kbcore_t *kbc;
    srch_TST_graph_t *tstg;
    int32 i, j;
    srch_t *s;
    ptmr_t tm_build;

    kbc = kb->kbcore;
    s = (srch_t *) srch;

    ptmr_init(&(tm_build));

    if (kbc->lmset == NULL) {
        E_ERROR("TST search requires a language model, please specify one with -lm or -lmctl\n");
        return SRCH_FAILURE;
    }

    /* Unlink silences */
    for (i = 0; i < kbc->lmset->n_lm; i++)
        unlinksilences(kbc->lmset->lmarray[i], kbc, kbc->dict);

    if (cmd_ln_int32_r(kbcore_config(kbc), "-Nstalextree"))
        E_WARN("-Nstalextree is omitted in TST search.\n");


  /** STRUCTURE : allocation of the srch graphs */
    tstg = ckd_calloc(1, sizeof(srch_TST_graph_t));

    tstg->epl = cmd_ln_int32_r(kbcore_config(kbc), "-epl");
    tstg->n_lextree = cmd_ln_int32_r(kbcore_config(kbc), "-Nlextree");
    tstg->isLMLA = cmd_ln_int32_r(kbcore_config(kbc), "-treeugprob");

    /* CHECK: make sure the number of lexical tree is at least one. */

    if (tstg->n_lextree < 1) {
        E_WARN("No. of ugtrees specified: %d; will instantiate 1 ugtree\n",
               tstg->n_lextree);
        tstg->n_lextree = 1;
    }

    n_ltree = tstg->n_lextree;

    /* STRUCTURE and REPORT: Initialize lexical tree. Filler tree's
       initialization is followed.  */

    tstg->ugtree =
        (lextree_t **) ckd_calloc(kbc->lmset->n_lm * n_ltree,
                                  sizeof(lextree_t *));

    /* curugtree is a pointer pointing the current unigram tree which
       were being used. */

    tstg->curugtree =
        (lextree_t **) ckd_calloc(n_ltree, sizeof(lextree_t *));

    /* Just allocate pointers */

    ptmr_reset(&(tm_build));
    for (i = 0; i < kbc->lmset->n_lm; i++) {
        for (j = 0; j < n_ltree; j++) {
            /*     ptmr_reset(&(tm_build)); */
            ptmr_start(&tm_build);

            tstg->ugtree[i * n_ltree + j] =
                lextree_init(kbc, kbc->lmset->lmarray[i],
                             lmset_idx_to_name(kbc->lmset, i),
                             tstg->isLMLA, REPORT_SRCH_TST,
                             LEXTREE_TYPE_UNIGRAM);

            ptmr_stop(&tm_build);

            /* Just report the lexical tree parameters for the first tree */
            lextree_report(tstg->ugtree[0]);

            if (tstg->ugtree[i * n_ltree + j] == NULL) {
                E_INFO
                    ("Fail to allocate lexical tree for lm %d and lextree %d\n",
                     i, j);
                return SRCH_FAILURE;
            }

            if (REPORT_SRCH_TST) {
                E_INFO
                    ("Lextrees (%d) for lm %d, its name is %s, it has %d nodes(ug)\n",
                     j, i, lmset_idx_to_name(kbc->lmset, i),
                     lextree_n_node(tstg->ugtree[i * n_ltree + j]));
            }
        }
    }
    E_INFO("Time for building trees, %4.4f CPU %4.4f Clk\n",
           tm_build.t_cpu, tm_build.t_elapsed);



    /* By default, curugtree will be pointed to the first sets of tree */
    for (j = 0; j < n_ltree; j++)
        tstg->curugtree[j] = tstg->ugtree[j];


    /* STRUCTURE: Create filler lextrees */
    /* ARCHAN : only one filler tree is supposed to be built even for dynamic LMs */
    /* Get the number of lexical tree */
    tstg->fillertree =
        (lextree_t **) ckd_calloc(n_ltree, sizeof(lextree_t *));

    for (i = 0; i < n_ltree; i++) {
        if ((tstg->fillertree[i] = fillertree_init(kbc)) == NULL) {
            E_INFO("Fail to allocate filler tree  %d\n", i);
            return SRCH_FAILURE;
        }
        if (REPORT_SRCH_TST) {
            E_INFO("Lextrees(%d), %d nodes(filler)\n",
                   i, lextree_n_node(tstg->fillertree[0]));
        }
    }

    if (cmd_ln_int32_r(kbcore_config(kbc), "-lextreedump")) {
        for (i = 0; i < kbc->lmset->n_lm; i++) {
            for (j = 0; j < n_ltree; j++) {
                E_INFO("LM %d name %s UGTREE %d\n", i,
                        lmset_idx_to_name(kbc->lmset, i), j);
                lextree_dump(tstg->ugtree[i * n_ltree + j], kbc->dict,
                             kbc->mdef, stderr,
                             cmd_ln_int32_r(kbcore_config(kbc), "-lextreedump"));
            }
        }
        for (i = 0; i < n_ltree; i++) {
            E_INFO("FILLERTREE %d\n", i);
            lextree_dump(tstg->fillertree[i], kbc->dict, kbc->mdef, stderr,
                         cmd_ln_int32_r(kbcore_config(kbc), "-lextreedump"));
        }
    }

    tstg->histprune = histprune_init(cmd_ln_int32_r(kbcore_config(kbc), "-maxhmmpf"),
                                     cmd_ln_int32_r(kbcore_config(kbc), "-maxhistpf"),
                                     cmd_ln_int32_r(kbcore_config(kbc), "-maxwpf"),
                                     cmd_ln_int32_r(kbcore_config(kbc), "-hmmhistbinsize"),
                                     (tstg->curugtree[0]->n_node +
                                      tstg->fillertree[0]->n_node) *
                                     tstg->n_lextree);

    /* Viterbi history structure */
    tstg->vithist = vithist_init(kb->kbcore, kb->beam->word,
                                 cmd_ln_int32_r(kbcore_config(kbc), "-bghist"), TRUE);


    /* Glue the graph structure */
    s->grh->graph_struct = tstg;
    s->grh->graph_type = GRAPH_STRUCT_TST;


    tstg->lmset = kbc->lmset;

    return SRCH_SUCCESS;

}

int
srch_TST_uninit(void *srch)
{
    srch_TST_graph_t *tstg;
    srch_t *s;
    int32 i, j;
    kbcore_t *kbc;

    s = (srch_t *) srch;
    kbc = s->kbc;

    tstg = (srch_TST_graph_t *) s->grh->graph_struct;

    for (i = 0; i < kbc->lmset->n_lm; i++) {
        for (j = 0; j < tstg->n_lextree; j++) {
            lextree_free(tstg->ugtree[i * tstg->n_lextree + j]);
            lextree_free(tstg->fillertree[i * tstg->n_lextree + j]);
        }
    }

    ckd_free(tstg->ugtree);
    ckd_free(tstg->curugtree);
    ckd_free(tstg->fillertree);

    if (tstg->vithist)
	vithist_free(tstg->vithist);

    if (tstg->histprune != NULL) {
        histprune_free((void *) tstg->histprune);
    }

    ckd_free(tstg);

    return SRCH_SUCCESS;
}

int
srch_TST_begin(void *srch)
{
    kbcore_t *kbc;
    int32 n, pred;
    int32 i;
    mgau_model_t *g;
    srch_t *s;
    srch_TST_graph_t *tstg;

    s = (srch_t *) srch;
    assert(s);
    assert(s->op_mode == 4);
    assert(s->grh);
    tstg = (srch_TST_graph_t *) s->grh->graph_struct;
    assert(tstg);

    kbc = s->kbc;
    g = kbc->mgau;

    /* Clean up any previous viterbi history */
    vithist_utt_reset(tstg->vithist);
    histprune_zero_histbin(tstg->histprune);

    /* Insert initial <s> into vithist structure */
    pred = vithist_utt_begin(tstg->vithist, kbc);
    assert(pred == 0);          /* Vithist entry ID for <s> */

    /* This reinitialize the cont_mgau routine in a GMM.  */
    if (g) {
        for (i = 0; i < g->n_mgau; i++) {
            g->mgau[i].bstidx = NO_BSTIDX;
            g->mgau[i].updatetime = NOT_UPDATED;
        }
    }

    /* Enter into unigram lextree[0] */
    n = lextree_n_next_active(tstg->curugtree[0]);
    assert(n == 0);
    lextree_enter(tstg->curugtree[0], mdef_silphone(kbc->mdef), -1, 0,
                  pred, s->beam->hmm, s->kbc);

    /* Enter into filler lextree */
    n = lextree_n_next_active(tstg->fillertree[0]);
    assert(n == 0);
    lextree_enter(tstg->fillertree[0], BAD_S3CIPID, -1, 0, pred,
                  s->beam->hmm, s->kbc);

    tstg->n_lextrans = 1;

    for (i = 0; i < tstg->n_lextree; i++) {
        lextree_active_swap(tstg->curugtree[i]);
        lextree_active_swap(tstg->fillertree[i]);
    }

    return SRCH_SUCCESS;
}


int
srch_TST_end(void *srch)
{
    int32 i;
    srch_t *s;
    srch_TST_graph_t *tstg;
    stat_t *st;

    s = (srch_t *) srch;
    assert(s);
    assert(s->op_mode == 4);
    assert(s->grh);
    tstg = (srch_TST_graph_t *) s->grh->graph_struct;
    assert(tstg);

    st = s->stat;

    /* Find the exit word and wrap up Viterbi history (but don't reset
     * it yet!) */
    s->exit_id = vithist_utt_end(tstg->vithist, s->kbc);

    /* Statistics update/report */
    st->utt_wd_exit = vithist_n_entry(tstg->vithist);
    histprune_showhistbin(tstg->histprune, st->nfr, s->uttid);

    for (i = 0; i < tstg->n_lextree; i++) {
        lextree_utt_end(tstg->curugtree[i], s->kbc);
        lextree_utt_end(tstg->fillertree[i], s->kbc);
    }

    lm_cache_stats_dump(kbcore_lm(s->kbc));
    lm_cache_reset(kbcore_lm(s->kbc));

    if (s->exit_id >= 0)
        return SRCH_SUCCESS;
    else
        return SRCH_FAILURE;
}

int
srch_TST_add_lm(void *srch, lm_t * lm, const char *lmname)
{
    lmset_t *lms;
    srch_t *s;
    srch_TST_graph_t *tstg;
    kbcore_t *kbc;
    int32 n_ltree;
    int32 j;
    int32 idx;

    s = (srch_t *) srch;
    tstg = (srch_TST_graph_t *) s->grh->graph_struct;

    kbc = s->kbc;
    lms = kbc->lmset;

    n_ltree = tstg->n_lextree;

    /* 1, Add a new LM */
    lmset_add_lm(lms, lm, lmname);      /* No. of lm will be increased by 1 */

    /* 2, Create a new set of trees for this LM. */

    tstg->ugtree =
        (lextree_t **) ckd_realloc(tstg->ugtree,
                                   (lms->n_lm * n_ltree) *
                                   sizeof(lextree_t *));

    idx = lms->n_lm - 1;
    for (j = 0; j < n_ltree; j++) {
        tstg->ugtree[(idx) * n_ltree + j] = lextree_init(kbc,
                                                         lms->lmarray[idx],
                                                         lmset_idx_to_name
                                                         (lms, idx),
                                                         tstg->isLMLA,
                                                         REPORT_SRCH_TST,
                                                         LEXTREE_TYPE_UNIGRAM);

        if (tstg->ugtree[idx * n_ltree + j] == NULL) {
            E_INFO
                ("Fail to allocate lexical tree for lm %d and lextree %d\n",
                 idx, j);
            return SRCH_FAILURE;
        }

        if (REPORT_SRCH_TST) {
            E_INFO
                ("Lextrees (%d) for lm %d, its name is %s, it has %d nodes(ug)\n",
                 j, idx, lmset_idx_to_name(kbc->lmset, idx),
                 lextree_n_node(tstg->ugtree[idx * n_ltree + j]));
        }
    }

    return SRCH_SUCCESS;
}

int
srch_TST_delete_lm(void *srch, const char *lmname)
{
    lmset_t *lms;
    kbcore_t *kbc = NULL;
    srch_t *s;
    srch_TST_graph_t *tstg;
    int32 lmidx;
    int32 n_ltree;
    int i, j;

    s = (srch_t *) srch;
    tstg = (srch_TST_graph_t *) s->grh->graph_struct;
    kbc = s->kbc;
    lms = kbc->lmset;
    n_ltree = tstg->n_lextree;

    /* Get the index of a the lm name */
    lmidx = lmset_name_to_idx(lms, lmname);

    /* Free the n_ltree copies of tree */
    for (j = 0; j < n_ltree; j++) {
        lextree_free(tstg->curugtree[lmidx * n_ltree + j]);
        tstg->curugtree[lmidx * n_ltree + j] = NULL;
    }

    /* Shift the pointer by one in the trees */
    for (i = lmidx; i < kbc->lmset->n_lm; i++) {
        for (j = 0; j < n_ltree; j++) {
            tstg->curugtree[i * n_ltree + j] =
                tstg->curugtree[(i + 1) * n_ltree + j];
        }
    }
    /* Tree is handled, now also handled the lmset */

    /* Delete the LM */
    /* Remember that the n_lm is minus by 1 at this point */
    lmset_delete_lm(lms, lmname);

    return SRCH_SUCCESS;
}

int
srch_TST_set_lm(void *srch, const char *lmname)
{
    lmset_t *lms;
    lm_t *lm;
    kbcore_t *kbc = NULL;
    int j;
    int idx;
    srch_t *s;
    srch_TST_graph_t *tstg;

    /*  s3wid_t dictid; */

    s = (srch_t *) srch;
    tstg = (srch_TST_graph_t *) s->grh->graph_struct;
    kbc = s->kbc;
    lms = kbc->lmset;

    kbc->lmset->cur_lm = NULL;

    for (j = 0; j < tstg->n_lextree; j++) {
        tstg->curugtree[j] = NULL;
    }

    assert(lms != NULL);
    assert(lms->lmarray != NULL);
    assert(lmname != NULL);

    idx = lmset_name_to_idx(lms, lmname);

    if (idx == LM_NOT_FOUND) {
        E_ERROR("LM name %s cannot be found, use the first language model",
                lmname);
        idx = 0;
    }

    /* Don't switch LM if we are already using this one. */
    if (lms->cur_lm == lms->lmarray[idx]) {     /* What if idx=0 and the first LM is not initialized? The previous assert safe-guard this. */
        return SRCH_SUCCESS;
    }

    lmset_set_curlm_widx(lms, idx);

    for (j = 0; j < tstg->n_lextree; j++) {
        tstg->curugtree[j] = tstg->ugtree[idx * tstg->n_lextree + j];
    }

    lm = kbc->lmset->cur_lm;

    if ((tstg->vithist->lms2vh_root =
         (vh_lms2vh_t **) ckd_realloc(tstg->vithist->lms2vh_root,
                                      lm_n_ug(lm) * sizeof(vh_lms2vh_t *)
         )) == NULL) {
        E_FATAL("failed to allocate memory for vithist\n");
    }
    memset(tstg->vithist->lms2vh_root, 0,
           lm_n_ug(lm) * sizeof(vh_lms2vh_t *));

    histprune_update_histbinsize(tstg->histprune,
                                 tstg->histprune->hmm_hist_binsize,
                                 ((tstg->curugtree[0]->n_node) +
                                  (tstg->fillertree[0]->n_node)) *
                                 tstg->n_lextree);


    if (REPORT_SRCH_TST) {
        E_INFO("Current LM name %s\n", lmset_idx_to_name(kbc->lmset, idx));
        E_INFO("LM ug size %d\n", lm->n_ug);
        E_INFO("LM bg size %d\n", lm->n_bg);
        E_INFO("LM tg size %d\n", lm->n_tg);
        E_INFO("HMM history bin size %d\n",
               tstg->histprune->hmm_hist_bins + 1);

        for (j = 0; j < tstg->n_lextree; j++) {
            E_INFO("Lextrees(%d), %d nodes(ug)\n",
                   j, lextree_n_node(tstg->curugtree[j]));
        }
    }
    return SRCH_SUCCESS;
}

int
srch_TST_eval_beams_lv1(void *srch)
{
    return SRCH_SUCCESS;
}

int
srch_TST_eval_beams_lv2(void *srch)
{
    return SRCH_SUCCESS;
}

int
srch_TST_hmm_compute_lv1(void *srch)
{
    return SRCH_SUCCESS;
}

int
srch_TST_compute_heuristic(void *srch, int32 win_efv)
{
    srch_t *s;
    srch_TST_graph_t *tstg;
    pl_t *pl;
    ascr_t *ascr;
    mdef_t *mdef;

    s = (srch_t *) srch;
    tstg = (srch_TST_graph_t *) s->grh->graph_struct;
    mdef = kbcore_mdef(s->kbc);

    ascr = s->ascr;
    pl = s->pl;

    if (pl->pheurtype != 0)
        pl_computePhnHeur(mdef, ascr, pl, pl->pheurtype, s->cache_win_strt,
                          win_efv);

    return SRCH_SUCCESS;
}


int
srch_TST_hmm_compute_lv2(void *srch, int32 frmno)
{
    /* This is local to this codebase */

    int32 i, j;
    lextree_t *lextree;
    srch_t *s;
    srch_TST_graph_t *tstg;
    kbcore_t *kbcore;
    beam_t *bm;
    ascr_t *ascr;
    histprune_t *hp;
    stat_t *st;
    pl_t *pl;

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

    s = (srch_t *) srch;
    tstg = (srch_TST_graph_t *) s->grh->graph_struct;

    n_ltree = tstg->n_lextree;
    kbcore = s->kbc;
    hp = tstg->histprune;
    bm = s->beam;
    ascr = s->ascr;
    hmm_hist = hp->hmm_hist;
    st = s->stat;
    pl = s->pl;


    maxwpf = hp->maxwpf;
    maxhistpf = hp->maxhistpf;
    maxhmmpf = hp->maxhmmpf;
    histbinsize = hp->hmm_hist_binsize;
    numhistbins = hp->hmm_hist_bins;
    hmmbeam = s->beam->hmm;
    pbeam = s->beam->ptrans;
    wbeam = s->beam->word;

    /* Evaluate active HMMs in each lextree; note best HMM state score */
    besthmmscr = MAX_NEG_INT32;
    bestwordscr = MAX_NEG_INT32;
    frm_nhmm = 0;

    for (i = 0; i < (n_ltree << 1); i++) {
        lextree =
            (i <
             n_ltree) ? tstg->curugtree[i] : tstg->fillertree[i - n_ltree];
        if (s->hmmdumpfp != NULL)
            fprintf(s->hmmdumpfp, "Fr %d Lextree %d #HMM %d\n", frmno, i,
                    lextree->n_active);
        lextree_hmm_eval(lextree, kbcore, ascr, frmno, s->hmmdumpfp);

        if (besthmmscr < lextree->best)
            besthmmscr = lextree->best;
        if (bestwordscr < lextree->wbest)
            bestwordscr = lextree->wbest;

        st->utt_hmm_eval += lextree->n_active;
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
    if (frm_nhmm > (maxhmmpf + (maxhmmpf >> 1))) {
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

    return SRCH_SUCCESS;
}

int
srch_TST_propagate_graph_ph_lv1(void *srch)
{

    return SRCH_SUCCESS;
}

int
srch_TST_propagate_graph_wd_lv1(void *srch)
{

    return SRCH_SUCCESS;
}

int
srch_TST_propagate_graph_ph_lv2(void *srch, int32 frmno)
{
    int32 i;
    srch_t *s;
    srch_TST_graph_t *tstg;
    int32 n_ltree;              /* Local version of number of lexical trees used */
    int32 ptranskip;            /* intervals at which wbeam is used for phone transitions, don't expect it to change */
    int32 pheurtype;            /* Local version of pheurtype, don't expect it to change */

    lextree_t *lextree;
    kbcore_t *kbcore;
    pl_t *pl;

    s = (srch_t *) srch;
    tstg = (srch_TST_graph_t *) s->grh->graph_struct;
    kbcore = s->kbc;
    pl = s->pl;
    pheurtype = pl->pheurtype;

    n_ltree = tstg->n_lextree;
    ptranskip = s->beam->ptranskip;

    /*  This stupid is the legacy when we try to make the intel compiler happy. */
    if (ptranskip == 0) {
        for (i = 0; i < (n_ltree << 1); i++) {
            lextree =
                (i <
                 n_ltree) ? tstg->curugtree[i] : tstg->fillertree[i -
                                                                  tstg->
                                                                  n_lextree];

            if (lextree_hmm_propagate_non_leaves(lextree, kbcore, frmno,
                                                 s->beam->thres,
                                                 s->beam->phone_thres,
                                                 s->beam->word_thres,
                                                 pl) !=
                LEXTREE_OPERATION_SUCCESS) {
                E_ERROR
                    ("Propagation Failed for lextree_hmm_propagate_non_leave at tree %d\n",
                     i);
                lextree_utt_end(lextree, kbcore);
                return SRCH_FAILURE;
            }

        }
    }
    else {
        for (i = 0; i < (n_ltree << 1); i++) {
            lextree =
                (i <
                 n_ltree) ? tstg->curugtree[i] : tstg->fillertree[i -
                                                                  n_ltree];

            if ((frmno % ptranskip) != 0) {
                if (lextree_hmm_propagate_non_leaves
                    (lextree, kbcore, frmno, s->beam->thres,
                     s->beam->phone_thres, s->beam->word_thres,
                     pl) != LEXTREE_OPERATION_SUCCESS) {
                    E_ERROR
                        ("Propagation Failed for lextree_hmm_propagate_non_leave at tree %d\n",
                         i);
                    lextree_utt_end(lextree, kbcore);
                    return SRCH_FAILURE;
                }

            }
            else {

                if (lextree_hmm_propagate_non_leaves
                    (lextree, kbcore, frmno, s->beam->thres,
                     s->beam->word_thres, s->beam->word_thres,
                     pl) != LEXTREE_OPERATION_SUCCESS) {
                    E_ERROR
                        ("Propagation Failed for lextree_hmm_propagate_non_leave at tree %d\n",
                         i);
                    lextree_utt_end(lextree, kbcore);
                    return SRCH_FAILURE;
                }
            }
        }
    }

    return SRCH_SUCCESS;
}

int
srch_TST_rescoring(void *srch, int32 frmno)
{
    int32 i;
    srch_t *s;
    srch_TST_graph_t *tstg;
    int32 n_ltree;              /* Local version of number of lexical trees used */
    int32 ptranskip;            /* intervals at which wbeam is used for phone transitions, don't expect it to change */
    lextree_t *lextree;
    kbcore_t *kbcore;
    vithist_t *vh;

    s = (srch_t *) srch;
    tstg = (srch_TST_graph_t *) s->grh->graph_struct;
    kbcore = s->kbc;
    vh = tstg->vithist;

    n_ltree = tstg->n_lextree;
    ptranskip = s->beam->ptranskip;

    if (ptranskip == 0) {
        for (i = 0; i < (n_ltree << 1); i++) {
            lextree =
                (i <
                 n_ltree) ? tstg->curugtree[i] : tstg->fillertree[i -
                                                                  tstg->
                                                                  n_lextree];
            if (lextree_hmm_propagate_leaves
                (lextree, kbcore, vh, frmno,
                 s->beam->word_thres) != LEXTREE_OPERATION_SUCCESS) {
                E_ERROR
                    ("Propagation Failed for lextree_hmm_propagate_leave at tree %d\n",
                     i);
                lextree_utt_end(lextree, kbcore);
                return SRCH_FAILURE;
            }

        }
    }
    else {
        for (i = 0; i < (n_ltree << 1); i++) {
            lextree =
                (i <
                 n_ltree) ? tstg->curugtree[i] : tstg->fillertree[i -
                                                                  n_ltree];

            if ((frmno % ptranskip) != 0) {
                if (lextree_hmm_propagate_leaves
                    (lextree, kbcore, vh, frmno,
                     s->beam->word_thres) != LEXTREE_OPERATION_SUCCESS) {
                    E_ERROR
                        ("Propagation Failed for lextree_hmm_propagate_leave at tree %d\n",
                         i);
                    lextree_utt_end(lextree, kbcore);
                    return SRCH_FAILURE;
                }

            }
            else {
                if (lextree_hmm_propagate_leaves
                    (lextree, kbcore, vh, frmno,
                     s->beam->word_thres) != LEXTREE_OPERATION_SUCCESS) {
                    E_ERROR
                        ("Propagation Failed for lextree_hmm_propagate_leave at tree %d\n",
                         i);
                    lextree_utt_end(lextree, kbcore);
                    lextree_utt_end(lextree, kbcore);
                    return SRCH_FAILURE;
                }
            }
        }
    }

    return SRCH_SUCCESS;
}


static void
srch_utt_word_trans(srch_t * s, int32 cf)
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
    dict_t *dict;
    mdef_t *mdef;
    srch_TST_graph_t *tstg;


    /* Call the rescoring routines at all word end */




    maxpscore = MAX_NEG_INT32;
    bm = s->beam;

    tstg = (srch_TST_graph_t *) s->grh->graph_struct;

    vh = tstg->vithist;
    th = bm->bestscore + bm->hmm;       /* Pruning threshold */

    if (vh->bestvh[cf] < 0)
        return;

    dict = kbcore_dict(s->kbc);
    mdef = kbcore_mdef(s->kbc);
    n_ci = mdef_n_ciphone(mdef);

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
        p = dict_last_phone(dict, wid);
        if (mdef_is_fillerphone(mdef, p))
            p = mdef_silphone(mdef);

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
            if (s->beam->wordend == 0
                || bs[p] > s->beam->wordend + maxpscore) {
                /* RAH, typecast p to (s3cipid_t) to make compiler happy */
                lextree_enter(tstg->curugtree[k], (s3cipid_t) p, cf, bs[p],
                              bv[p], th, s->kbc);
            }
        }

    }

    /* Transition to filler lextrees */
    lextree_enter(tstg->fillertree[k], BAD_S3CIPID, cf, vh->bestscore[cf],
                  vh->bestvh[cf], th, s->kbc);
}

int
srch_TST_propagate_graph_wd_lv2(void *srch, int32 frmno)
{
    dict_t *dict;
    vithist_t *vh;
    histprune_t *hp;
    kbcore_t *kbcore;

    srch_t *s;
    srch_TST_graph_t *tstg;
    int32 maxwpf;               /* Local version of Max words per frame, don't expect it to change */
    int32 maxhistpf;            /* Local version of Max histories per frame, don't expect it to change */
    int32 maxhmmpf;             /* Local version of Max active HMMs per frame, don't expect it to change  */


    s = (srch_t *) srch;
    tstg = (srch_TST_graph_t *) s->grh->graph_struct;
    kbcore = s->kbc;

    hp = tstg->histprune;
    vh = tstg->vithist;
    dict = kbcore_dict(kbcore);

    maxwpf = hp->maxwpf;
    maxhistpf = hp->maxhistpf;
    maxhmmpf = hp->maxhmmpf;


    s = (srch_t *) srch;
    tstg = (srch_TST_graph_t *) s->grh->graph_struct;

    srch_TST_rescoring((void *) s, frmno);

    vithist_prune(vh, dict, frmno, maxwpf, maxhistpf,
                  s->beam->word_thres - s->beam->bestwordscore);

    srch_utt_word_trans(s, frmno);
    return SRCH_SUCCESS;
}

int
srch_TST_frame_windup(void *srch, int32 frmno)
{
    vithist_t *vh;
    kbcore_t *kbcore;
    int32 i;

    srch_t *s;
    srch_TST_graph_t *tstg;

    s = (srch_t *) srch;
    tstg = (srch_TST_graph_t *) s->grh->graph_struct;
    kbcore = s->kbc;

    vh = tstg->vithist;

    /* Wind up this frame */
    vithist_frame_windup(vh, frmno, NULL, kbcore);


    for (i = 0; i < tstg->n_lextree; i++) {
        lextree_active_swap(tstg->curugtree[i]);
        lextree_active_swap(tstg->fillertree[i]);
    }
    return SRCH_SUCCESS;
}

int
srch_TST_shift_one_cache_frame(void *srch, int32 win_efv)
{
    ascr_t *ascr;
    srch_t *s;

    s = (srch_t *) srch;

    ascr = s->ascr;

    ascr_shift_one_cache_frame(ascr, win_efv);
    return SRCH_SUCCESS;
}

int
srch_TST_select_active_gmm(void *srch)
{

    ascr_t *ascr;
    int32 n_ltree;              /* Local version of number of lexical trees used */
    srch_t *s;
    srch_TST_graph_t *tstg;
    dict2pid_t *d2p;
    mdef_t *mdef;
    fast_gmm_t *fgmm;
    lextree_t *lextree;
    pl_t *pl;
    stat_t *st;
    mgau_model_t *mgau;

    kbcore_t *kbcore;
    int32 i;

    s = (srch_t *) srch;
    tstg = (srch_TST_graph_t *) s->grh->graph_struct;
    kbcore = s->kbc;

    ascr = s->ascr;
    n_ltree = tstg->n_lextree;

    mdef = kbcore_mdef(kbcore);
    d2p = kbcore_dict2pid(kbcore);
    mgau = kbcore_mgau(kbcore);
    pl = s->pl;
    fgmm = s->fastgmm;
    st = s->stat;

    if (ascr->sen_active) {
        /*    E_INFO("Decide whether senone is active\n"); */


        ascr_clear_ssid_active(ascr);
        ascr_clear_comssid_active(ascr);


        /* Find active senone-sequence IDs (including composite ones) */
        for (i = 0; i < (n_ltree << 1); i++) {
            lextree = (i < n_ltree) ? tstg->curugtree[i] :
                tstg->fillertree[i - n_ltree];
            lextree_ssid_active(lextree, ascr->ssid_active,
                                ascr->comssid_active);
        }

        /* Find active senones from active senone-sequences */

        ascr_clear_sen_active(ascr);


        mdef_sseq2sen_active(mdef, ascr->ssid_active, ascr->sen_active);

        /* Add in senones needed for active composite senone-sequences */
        dict2pid_comsseq2sen_active(d2p, mdef, ascr->comssid_active,
                                    ascr->sen_active);
    }

    return SRCH_SUCCESS;
}

glist_t
srch_TST_gen_hyp(void *srch)
{
    srch_t *s;
    srch_TST_graph_t *tstg;
    int32 id;

    s = (srch_t *) srch;
    tstg = (srch_TST_graph_t *) s->grh->graph_struct;
    assert(tstg->vithist);

    if (s->exit_id == -1) /* Search not finished */
	id = vithist_partialutt_end(tstg->vithist, s->kbc);
    else
        id = s->exit_id;

    if (id < 0) {
        E_WARN("Failed to retrieve viterbi history.\n");
        return NULL;
    }

    return vithist_backtrace(tstg->vithist, id, kbcore_dict(s->kbc));
}

int
srch_TST_dump_vithist(void *srch)
{
    srch_t *s;
    srch_TST_graph_t *tstg;
    FILE *bptfp;
    char *file;

    s = (srch_t *) srch;
    tstg = (srch_TST_graph_t *) s->grh->graph_struct;
    assert(tstg->vithist);

    file = ckd_calloc(strlen(cmd_ln_str_r(kbcore_config(s->kbc), "-bptbldir")) + strlen(s->uttid) + 5, 1);
    sprintf(file, "%s/%s.bpt", cmd_ln_str_r(kbcore_config(s->kbc), "-bptbldir"), s->uttid);

    if ((bptfp = fopen(file, "w")) == NULL) {
        E_ERROR("fopen(%s,w) failed; using stdout\n", file);
        bptfp = stdout;
    }
    ckd_free(file);

    vithist_dump(tstg->vithist, -1, s->kbc, bptfp);

    if (bptfp != stdout)
        fclose(bptfp);

    return SRCH_SUCCESS;
}


dag_t *
srch_TST_gen_dag(void *srch,         /**< a pointer of srch_t */
                 glist_t hyp)
{
    srch_t *s = (srch_t *)srch;
    srch_TST_graph_t *tstg = (srch_TST_graph_t *) s->grh->graph_struct;

    return vithist_dag_build(tstg->vithist, hyp, kbcore_dict(s->kbc),
                             s->exit_id, kbcore_config(s->kbc), kbcore_logmath(s->kbc));
}

glist_t
srch_TST_bestpath_impl(void *srch,          /**< A void pointer to a search structure */
                       dag_t * dag)
{
    glist_t ghyp, rhyp;
    float32 bestpathlw;
    float64 lwf;
    srch_hyp_t *tmph, *bph;
    srch_t *s = (srch_t *) srch;

    bestpathlw = cmd_ln_float32_r(kbcore_config(s->kbc), "-bestpathlw");
    lwf = bestpathlw ? (bestpathlw / cmd_ln_float32_r(kbcore_config(s->kbc), "-lw")) : 1.0;

    /* Bypass filler nodes */
    if (!dag->filler_removed) {
        /* If Viterbi search terminated in filler word coerce final DAG node to FINISH_WORD */
        if (dict_filler_word(s->kbc->dict, dag->end->wid))
            dag->end->wid = s->kbc->dict->finishwid;
        if (dag_bypass_filler_nodes(dag, lwf, s->kbc->dict, s->kbc->fillpen) < 0)
            E_ERROR("maxedge limit (%d) exceeded\n", dag->maxedge);
        else
            dag->filler_removed = 1;
    }

    /* FIXME: This is some bogus crap to do with the different
     * treatment of <s> and </s> in the flat vs. the tree decoder.  If
     * we don't do this then bestpath search will fail because
     * trigrams ending in </s> can't be scored. */
    linksilences(kbcore_lm(s->kbc), s->kbc, kbcore_dict(s->kbc));
    bph = dag_search(dag, s->uttid,
                     lwf,
                     dag->end,
                     kbcore_dict(s->kbc),
                     kbcore_lm(s->kbc), kbcore_fillpen(s->kbc)
        );
    unlinksilences(kbcore_lm(s->kbc), s->kbc, kbcore_dict(s->kbc));

    if (bph != NULL) {
        ghyp = NULL;
        for (tmph = bph; tmph; tmph = tmph->next)
            ghyp = glist_add_ptr(ghyp, (void *) tmph);

        rhyp = glist_reverse(ghyp);
        return rhyp;
    }
    else {
        return NULL;
    }

}

glist_t
srch_TST_nbest_impl(void *srch,          /**< A void pointer to a search structure */
                    dag_t * dag)
{
    float32 bestpathlw;
    float64 lwf;
    srch_t *s = (srch_t *) srch;
    char str[2000];

    if (!(cmd_ln_exists_r(kbcore_config(s->kbc), "-nbestdir") && cmd_ln_str_r(kbcore_config(s->kbc), "-nbestdir")))
        return NULL;
    ctl_outfile(str, cmd_ln_str_r(kbcore_config(s->kbc), "-nbestdir"), cmd_ln_str_r(kbcore_config(s->kbc), "-nbestext"),
                (s->uttfile ? s->uttfile : s->uttid), s->uttid);

    bestpathlw = cmd_ln_float32_r(kbcore_config(s->kbc), "-bestpathlw");
    lwf = bestpathlw ? (bestpathlw / cmd_ln_float32_r(kbcore_config(s->kbc), "-lw")) : 1.0;

    /* FIXME: This is some bogus crap to do with the different
     * treatment of <s> and </s> in the flat vs. the tree decoder.  If
     * we don't do this then bestpath search will fail because
     * trigrams ending in </s> can't be scored. */
    linksilences(kbcore_lm(s->kbc), s->kbc, kbcore_dict(s->kbc));

    /* Bypass filler nodes */
    if (!dag->filler_removed) {
        /* If Viterbi search terminated in filler word coerce final DAG node to FINISH_WORD */
        if (dict_filler_word(s->kbc->dict, dag->end->wid))
            dag->end->wid = s->kbc->dict->finishwid;
        dag_remove_unreachable(dag);
        if (dag_bypass_filler_nodes(dag, lwf, s->kbc->dict, s->kbc->fillpen) < 0) {
            E_ERROR("maxedge limit (%d) exceeded\n", dag->maxedge);
            return NULL;
        }
    }

    dag_compute_hscr(dag, kbcore_dict(s->kbc), kbcore_lm(s->kbc), lwf);
    dag_remove_bypass_links(dag);
    dag->filler_removed = 0;

    nbest_search(dag, str, s->uttid, lwf,
                 kbcore_dict(s->kbc),
                 kbcore_lm(s->kbc), kbcore_fillpen(s->kbc)
        );
    unlinksilences(kbcore_lm(s->kbc), s->kbc, kbcore_dict(s->kbc));

    return NULL;
}

/* Pointers to all functions */
srch_funcs_t srch_TST_funcs = {
	/* init */			srch_TST_init,
	/* uninit */			srch_TST_uninit,
	/* utt_begin */ 		srch_TST_begin,
	/* utt_end */   		srch_TST_end,
	/* decode */			NULL,
	/* set_lm */			srch_TST_set_lm,
	/* add_lm */			srch_TST_add_lm,
	/* delete_lm */ 		srch_TST_delete_lm,

	/* gmm_compute_lv1 */		approx_ci_gmm_compute,
	/* one_srch_frame_lv1 */	NULL,
	/* hmm_compute_lv1 */		srch_debug_hmm_compute_lv1,
	/* eval_beams_lv1 */		srch_debug_eval_beams_lv1,
	/* propagate_graph_ph_lv1 */	srch_debug_propagate_graph_ph_lv1,
	/* propagate_graph_wd_lv1 */	srch_debug_propagate_graph_wd_lv1,

	/* gmm_compute_lv2 */		s3_cd_gmm_compute_sen_comp,
	/* one_srch_frame_lv2 */	NULL,
	/* hmm_compute_lv2 */		srch_TST_hmm_compute_lv2,
	/* eval_beams_lv2 */		srch_debug_eval_beams_lv2,
	/* propagate_graph_ph_lv2 */	srch_TST_propagate_graph_ph_lv2,
	/* propagate_graph_wd_lv2 */	srch_TST_propagate_graph_wd_lv2,

	/* rescoring */			NULL,
	/* frame_windup */		srch_TST_frame_windup,
	/* compute_heuristic */		srch_TST_compute_heuristic,
	/* shift_one_cache_frame */	srch_TST_shift_one_cache_frame,
	/* select_active_gmm */		srch_TST_select_active_gmm,

	/* gen_hyp */			srch_TST_gen_hyp,
	/* gen_dag */			srch_TST_gen_dag,
	/* dump_vithist */		srch_TST_dump_vithist,
	/* bestpath_impl */		srch_TST_bestpath_impl,
	/* dag_dump */			NULL,
	/* nbest_impl */		srch_TST_nbest_impl,
	NULL
};
