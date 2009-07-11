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
/*
 * vithist.c -- Viterbi history
 * 
 * **********************************************
 * CMU ARPA Speech Project
 *
 * Copyright (c) 1999 Carnegie Mellon University.
 * ALL RIGHTS RESERVED.
 * **********************************************
 * 
 * HISTORY
 * 
 * $Log: vithist.c,v $
 * Revision 1.9  2006/02/23 16:56:12  arthchan2003
 * Merged from the branch SPHINX3_5_2_RCI_IRII_BRANCH
 * 1, Split latticehist_t from flat_fwd.c to  here.
 * 2, Introduced vithist_entry_cp.  This is much better than the direct
 * copy we have been using. (Which could cause memory problem easily)
 *
 * Revision 1.8.4.12  2006/01/16 18:11:39  arthchan2003
 * 1, Important Bug fixes, a local pointer is used when realloc is needed.  This causes invalid writing of the memory, 2, Acoustic scores of the last segment in IBM lattice generation couldn't be found in the past.  Now, this could be handled by the optional acoustic scores in the node of lattice.  Application code is not yet checked-in
 *
 * Revision 1.8.4.11  2005/11/17 06:46:02  arthchan2003
 * 3 changes. 1, Code was added for full triphone implementation, not yet working. 2, Senone scale is removed from vithist table. This was a bug introduced during some fixes in CALO.
 *
 * Revision 1.8.4.10  2005/10/17 04:58:30  arthchan2003
 * vithist.c is the true source of memory leaks in the past for full cwtp expansion.  There are two changes made to avoid this happen, 1, instead of using ve->rc_info as the indicator whether something should be done, used a flag bFullExpand to control it. 2, avoid doing direct C-struct copy (like *ve = *tve), it becomes the reason of why memory are leaked and why the code goes wrong.
 *
 * Revision 1.8.4.9  2005/10/07 20:05:05  arthchan2003
 * When rescoring in full triphone expansion, the code should use the score for the word end with corret right context.
 *
 * Revision 1.8.4.8  2005/09/26 07:23:06  arthchan2003
 * Also fixed a bug such SINGLE_RC_HISTORY=0 compiled.
 *
 * Revision 1.8.4.7  2005/09/26 02:28:00  arthchan2003
 * Remove a E_INFO in vithist.c
 *
 * Revision 1.8.4.6  2005/09/25 19:33:40  arthchan2003
 * (Change for comments) Added support for Viterbi history.
 *
 * Revision 1.8.4.5  2005/09/25 19:23:55  arthchan2003
 * 1, Added arguments for turning on/off LTS rules. 2, Added arguments for turning on/off composite triphones. 3, Moved dict2pid deallocation back to dict2pid. 4, Tidying up the clean up code.
 *
 * Revision 1.8.4.4  2005/09/11 03:00:15  arthchan2003
 * All lattice-related functions are not incorporated into vithist. So-called "lattice" is essentially the predecessor of vithist_t and fsg_history_t.  Later when vithist_t support by right context score and history.  It should replace both of them.
 *
 * Revision 1.8.4.3  2005/07/26 02:20:39  arthchan2003
 * merged hyp_t with srch_hyp_t.
 *
 * Revision 1.8.4.2  2005/07/17 05:55:45  arthchan2003
 * Removed vithist_dag_write_header
 *
 * Revision 1.8.4.1  2005/07/04 07:25:22  arthchan2003
 * Added vithist_entry_display and vh_lmstate_display in vithist.
 *
 * Revision 1.8  2005/06/22 02:47:35  arthchan2003
 * 1, Added reporting flag for vithist_init. 2, Added a flag to allow using words other than silence to be the last word for backtracing. 3, Fixed doxygen documentation. 4, Add  keyword.
 *
 * Revision 1.9  2005/06/16 04:59:10  archan
 * Sphinx3 to s3.generic, a gentle-refactored version of Dave's change in senone scale.
 *
 * Revision 1.8  2005/05/26 00:46:59  archan
 * Added functionalities that such that <sil> will not be inserted at the end of the utterance.
 *
 * Revision 1.7  2005/04/25 23:53:35  archan
 * 1, Some minor modification of vithist_t, vithist_rescore can now support optional LM rescoring, vithist also has its own reporting routine. A new argument -lmrescore is also added in decode and livepretend.  This can switch on and off the rescoring procedure. 2, I am reaching the final difficulty of mode 5 implementation.  That is, to implement an algorithm which dynamically decide which tree copies should be entered.  However, stuffs like score propagation in the leave nodes and non-leaves nodes are already done. 3, As briefly mentioned in 2, implementation of rescoring , which used to happened at leave nodes are now separated. The current implementation is not the most clever one. Wish I have time to change it before check-in to the canonical.
 *
 * Revision 1.6  2005/04/21 23:50:26  archan
 * Some more refactoring on the how reporting of structures inside kbcore_t is done, it is now 50% nice. Also added class-based LM test case into test-decode.sh.in.  At this moment, everything in search mode 5 is already done.  It is time to test the idea whether the search can really be used.
 *
 * Revision 1.5  2005/04/20 03:46:30  archan
 * factor dag header writer into vithist.[ch], do the corresponding change for lm_t
 *
 * Revision 1.4  2005/03/30 01:08:38  archan
 * codebase-wide update. Performed an age-old trick: Adding $Log into all .c and .h files. This will make sure the commit message be preprended into a file.
 *
 * 20.Apr.2001  RAH (rhoughton@mediasite.com, ricky.houghton@cs.cmu.edu)
 *              Added vithist_free() to free allocated memory
 * 
 * 30-Dec-2000  Rita Singh (rsingh@cs.cmu.edu) at Carnegie Mellon University
 *		Added vithist_partialutt_end() to allow backtracking in
 *		the middle of an utterance
 * 
 * 13-Aug-1999	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Added maxwpf handling.
 * 
 * 24-May-1999	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Started.
 */

#include <listelem_alloc.h>
#include <pio.h>
#include <heap.h>

#include "vithist.h"
#include "lextree.h"

void
vh_lmstate_display(vh_lmstate_t * vhl, dict_t * dict)
{
    /* TODO: Also translate wid to string if dict is not NULL */
    E_INFO("lwid[0] %d\n", vhl->lm3g.lwid[0]);
    E_INFO("lwid[1] %d\n", vhl->lm3g.lwid[1]);
    E_INFO("lwid[2] %d\n", vhl->lm3g.lwid[2]);
}

void
vithist_entry_display(vithist_entry_t * ve, dict_t * dict)
{
    E_INFO("Word ID %d \n", ve->wid);
    E_INFO("Sf %d Ef %d \n", ve->sf, ve->ef);
    E_INFO("Ascr %d Lscr %d \n", ve->ascr, ve->lscr);
    E_INFO("Score %d \n", ve->path.score);
    E_INFO("Type %d\n", ve->type);
    E_INFO("Valid for LM rescoring? %d\n", ve->valid);
    vh_lmstate_display(&(ve->lmstate), dict);
}


vithist_t *
vithist_init(kbcore_t * kbc, int32 wbeam, int32 bghist, int32 report)
{
    vithist_t *vh;
#ifdef OLD_LM_API
    lmset_t *lmset;
    int i;
#else
    ngram_model_t *lmset;
#endif
    dict_t *dict;

    int max = -1;

    if (report)
        E_INFO("Initializing Viterbi-history module\n");

    vh = (vithist_t *) ckd_calloc(1, sizeof(vithist_t));

    vh->entry =
        (vithist_entry_t **) ckd_calloc(VITHIST_MAXBLKS,
                                        sizeof(vithist_entry_t *));

    vh->n_entry = 0;

    vh->frame_start =
        (int32 *) ckd_calloc(S3_MAX_FRAMES + 1, sizeof(int32));

    vh->bestscore = (int32 *) ckd_calloc(S3_MAX_FRAMES + 1, sizeof(int32));
    vh->bestvh = (int32 *) ckd_calloc(S3_MAX_FRAMES + 1, sizeof(int32));

    vh->wbeam = wbeam;
    vh->bghist = bghist;

    lmset = kbcore_lmset(kbc);
    dict = kbcore_dict(kbc);

#ifdef OLD_LM_API
    for (i = 0; i < kbc->lmset->n_lm; i++) {
        if (lm_n_ug(lmset->lmarray[i]) > max) {
            max = lm_n_ug(lmset->lmarray[i]);
        }
    }
#else
    max = ngram_model_get_counts(lmset)[0];
#endif

    E_INFO("Allocation for Viterbi history, lmset-final size: %d\n", max);
    vh->lms2vh_root =
        (vh_lms2vh_t **) ckd_calloc(max, sizeof(vh_lms2vh_t *));
    vh->n_ci = mdef_n_ciphone(kbc->mdef);

    vh->lwidlist = NULL;

    return vh;
}

/**
   This function cleans up rc_info
 */
static void
clean_up_rc_info(backpointer_t * rc_info, int32 n_rc_info)
{
    int32 i;
    for (i = 0; i < n_rc_info; i++) {
        rc_info[i].score = S3_LOGPROB_ZERO;
        rc_info[i].pred = -1;
    }
}

/**
   Copy vithist entry from the source vb to to destination va without
   copying rc_info. This is a specific function and please don't use
   it for copying if you haven't traced it.
 */
static void
vithist_entry_dirty_cp(vithist_entry_t * va, vithist_entry_t * vb,
                       int32 n_rc_info)
{
    backpointer_t *tmpshp;
    assert(vb->rc == NULL);

    tmpshp = va->rc;
    /* Do a direct copy */
    *va = *vb;
    va->rc = tmpshp;
    va->n_rc = n_rc_info;
}

/**
   Whole thing copying. 
 */

static void
vithist_entry_cp(vithist_entry_t * va, vithist_entry_t * vb)
{
    int i;
    /* Do a direct copy */
    va->wid = vb->wid;
    va->sf = vb->sf;
    va->ef = vb->ef;
    va->ascr = vb->ascr;
    va->lscr = vb->lscr;
    va->path.score = vb->path.score;
    va->path.pred = vb->path.pred;
    va->type = vb->type;
    va->valid = vb->valid;
    va->lmstate.lm3g.lwid[0] = vb->lmstate.lm3g.lwid[0];
    va->lmstate.lm3g.lwid[1] = vb->lmstate.lm3g.lwid[1];
    va->n_rc = vb->n_rc;

    if (va->rc) {
        for (i = 0; i < vb->n_rc; i++)
            va->rc[i] = vb->rc[i];
    }
}


/*
 * Allocate a new entry at vh->n_entry if one doesn't exist and return ptr to it.
 */
static vithist_entry_t *
vithist_entry_alloc(vithist_t * vh)
{
    int32 b, l;
    vithist_entry_t *ve;

    b = VITHIST_ID2BLK(vh->n_entry);
    l = VITHIST_ID2BLKOFFSET(vh->n_entry);

    if (l == 0) {               /* Crossed block boundary; allocate a new block of vithist space */
        if (b >= VITHIST_MAXBLKS)
            E_FATAL
                ("Viterbi history array exhausted; increase VITHIST_MAXBLKS\n");

        assert(vh->entry[b] == NULL);

        ve = (vithist_entry_t *) ckd_calloc(VITHIST_BLKSIZE,
                                            sizeof(vithist_entry_t));
        vh->entry[b] = ve;
    }
    else
        ve = vh->entry[b] + l;

    vh->n_entry++;
    return ve;
}


int32
vithist_utt_begin(vithist_t * vh, kbcore_t * kbc)
{
    vithist_entry_t *ve;
    dict_t *dict;
#ifdef OLD_LM_API
    lm_t *lm;

    lm = kbcore_lm(kbc);
#endif
    dict = kbcore_dict(kbc);

    assert(vh->n_entry == 0);
    assert(vh->entry[0] == NULL);
    assert(vh->lwidlist == NULL);

    /* Create an initial dummy <s> entry.  This is the root for the utterance */
    ve = vithist_entry_alloc(vh);

    ve->wid = dict_startwid(dict);
    ve->sf = -1;
    ve->ef = -1;
    ve->ascr = 0;
    ve->lscr = 0;
    ve->path.score = 0;
    ve->path.pred = -1;
    ve->type = 0;
    ve->valid = 1;
#ifdef OLD_LM_API
    ve->lmstate.lm3g.lwid[0] = lm_startwid(lm);
    ve->lmstate.lm3g.lwid[1] = BAD_LMWID(lm);
#else
    ve->lmstate.lm3g.lwid[0] = ngram_wid(kbcore_lm(kbc), S3_START_WORD);
    ve->lmstate.lm3g.lwid[1] = NGRAM_INVALID_WID;
#endif
    vh->n_frm = 0;
    vh->frame_start[0] = 1;
    vh->bestscore[0] = MAX_NEG_INT32;
    vh->bestvh[0] = -1;

    return 0;
}


static int32
vh_lmstate_find(vithist_t * vh, vh_lmstate_t * lms)
{
    vh_lms2vh_t *lms2vh;
    s3lmwid32_t lwid;
    gnode_t *gn;

    lwid = lms->lm3g.lwid[0];
    if ((lms2vh = vh->lms2vh_root[lwid]) == NULL)
        return -1;

    assert(lms2vh->state == lwid);

    lwid = lms->lm3g.lwid[1];
    for (gn = lms2vh->children; gn; gn = gnode_next(gn)) {
        lms2vh = (vh_lms2vh_t *) gnode_ptr(gn);
        if (lms2vh->state == lwid)
            return lms2vh->vhid;
    }

    return -1;
}


/*
 * Enter a new LMstate into the current frame LMstates trees; called ONLY IF not already
 * present.
 */
static void
vithist_lmstate_enter(vithist_t * vh, int32 vhid, vithist_entry_t * ve)
{
    vh_lms2vh_t *lms2vh, *child;
    s3lmwid32_t lwid;

    lwid = ve->lmstate.lm3g.lwid[0];
    if ((lms2vh = vh->lms2vh_root[lwid]) == NULL) {
        lms2vh = (vh_lms2vh_t *) ckd_calloc(1, sizeof(vh_lms2vh_t));
        vh->lms2vh_root[lwid] = lms2vh;

        lms2vh->state = lwid;
        lms2vh->children = NULL;

        vh->lwidlist = glist_add_int32(vh->lwidlist, (int32) lwid);
    }
    else {
        assert(lms2vh->state == lwid);
    }

    child = (vh_lms2vh_t *) ckd_calloc(1, sizeof(vh_lms2vh_t));
    child->state = ve->lmstate.lm3g.lwid[1];
    child->children = NULL;
    child->vhid = vhid;
    child->ve = ve;

    lms2vh->children = glist_add_ptr(lms2vh->children, (void *) child);
}


/* Rclist is separate from tve because C structure copying is used in *ve = *tve 
 */
void
vithist_enter(vithist_t * vh,              /**< The history table */
              kbcore_t * kbc,              /**< a KB core */
              vithist_entry_t * tve,       /**< an input vithist element */
              int32 comp_rc                /**< a compressed rc. If it is the actual rc, it won't work. */
    )
{
    vithist_entry_t *ve;
    int32 vhid;
    int32 n_ci;
    dict_t *d;
    int32 n_rc_info;
    int32 old_n_rc_info;

    d = kbc->dict;
    n_ci = vh->n_ci;
    /* Check if an entry with this LM state already exists in current frame */
    vhid = vh_lmstate_find(vh, &(tve->lmstate));
    n_rc_info = 0;          /* Just fill in something if not using crossword triphon */


    assert(comp_rc < n_rc_info);

    if (vhid < 0) {             /* Not found; allocate new entry */
        vhid = vh->n_entry;
        ve = vithist_entry_alloc(vh);

        vithist_entry_dirty_cp(ve, tve, n_rc_info);
        vithist_lmstate_enter(vh, vhid, ve);    /* Enter new vithist info into LM state tree */

        /*      E_INFO("Start a new entry wid %d\n",ve->wid); */
        if (ve->rc != NULL)
            clean_up_rc_info(ve->rc, ve->n_rc);

        if (comp_rc != -1) {
            if (ve->rc == NULL) {
                ve->n_rc =
                    get_rc_nssid(kbc->dict2pid, ve->wid, kbc->dict);
                /* Always allocate n_ci for rc_info */
                ve->rc = ckd_calloc(vh->n_ci, sizeof(*ve->rc));
                clean_up_rc_info(ve->rc, ve->n_rc);
            }

            assert(comp_rc < ve->n_rc);
            if (ve->rc[comp_rc].score < tve->path.score) {
                ve->rc[comp_rc].score = tve->path.score;
                ve->rc[comp_rc].pred = tve->path.pred;
            }
        }


    }
    else {
        ve = vithist_id2entry(vh, vhid);

        /*      E_INFO("Replace the old entry\n"); */
        /*              E_INFO("Old entry wid %d, New entry wid %d\n",ve->wid, tve->wid); */

        if (comp_rc == -1) {
            if (ve->path.score < tve->path.score) {
                vithist_entry_dirty_cp(ve, tve, n_rc_info);
                assert(comp_rc < n_rc_info);
                if (ve->rc != NULL)
                    clean_up_rc_info(ve->rc, ve->n_rc);
            }

        }
        else {

            /* This is wrong, the score 
               Alright, how vhid was searched in the first place? 
             */
            if (ve->path.score < tve->path.score) {
                old_n_rc_info = ve->n_rc;
                vithist_entry_dirty_cp(ve, tve, n_rc_info);
                assert(comp_rc < n_rc_info);

                assert(ve->rc);
                clean_up_rc_info(ve->rc, ve->n_rc);
                ve->rc[comp_rc].score = tve->path.score;
                ve->rc[comp_rc].pred = tve->path.pred;
            }

        }

    }

    /* Update best exit score in this frame */
    if (vh->bestscore[vh->n_frm] < tve->path.score) {
        vh->bestscore[vh->n_frm] = tve->path.score;
        vh->bestvh[vh->n_frm] = vhid;
    }
}


void
vithist_rescore(vithist_t * vh, kbcore_t * kbc,
                s3wid_t wid, int32 ef, int32 score,
                int32 pred, int32 type, int32 rc)
{
    vithist_entry_t *pve, tve;
#ifdef OLD_LM_API
    s3lmwid32_t lwid;
#else
    int32 lwid;
#endif
    int32 se, fe;
    int32 i;

    assert(vh->n_frm == ef);
    if (pred == -1) {
        /* Always do E_FATAL assuming upper level function take care of error checking. */
        E_FATAL
            ("Hmm->out.history equals to -1 with score %d, some active phone was not computed?\n",
             score);
    }

    /* pve is the previous entry before word with wid or, se an fe is the
       first to the last entry before pve. So pve is w_{n-1} */

    pve = vithist_id2entry(vh, pred);

    /* Create a temporary entry with all the info currently available */
    tve.wid = wid;
    tve.sf = pve->ef + 1;
    tve.ef = ef;
    tve.type = type;
    tve.valid = 1;
    tve.ascr = score - pve->path.score;
    tve.lscr = 0;
    tve.rc = NULL;
    tve.n_rc = 0;


    if (dict_filler_word(kbcore_dict(kbc), wid)) {

        tve.path.score = score;
        tve.lscr = fillpen(kbcore_fillpen(kbc), wid);
        tve.path.score += tve.lscr;

        tve.path.pred = pred;
        tve.lmstate.lm3g = pve->lmstate.lm3g;
        vithist_enter(vh, kbc, &tve, rc);
    }
    else {
#ifndef OLD_LM_API
        int32 n_used;
#endif

        if (pred == 0) {            /* Special case for the initial <s> entry */
            se = 0;
            fe = 1;
        }
        else {
            se = vh->frame_start[pve->ef];
            fe = vh->frame_start[pve->ef + 1];
        }

        /* Now if it is a word, backtrack again to get all possible previous word
           So  pve becomes the w_{n-2}. 
         */

#ifdef OLD_LM_API
        lwid = kbc->lmset->cur_lm->dict2lmwid[wid];
#else
        lwid = ngram_wid(kbcore_lm(kbc), dict_wordstr(kbcore_dict(kbc), dict_basewid(kbcore_dict(kbc), wid)));
#endif

        tve.lmstate.lm3g.lwid[0] = lwid;

        for (i = se; i < fe; i++) {
            pve = vithist_id2entry(vh, i);

            if (pve->valid) {
                tve.path.score = pve->path.score + tve.ascr;
#ifdef OLD_LM_API
                tve.lscr = lm_tg_score(kbcore_lm(kbc),
                                       pve->lmstate.lm3g.lwid[1],
                                       pve->lmstate.lm3g.lwid[0],
                                       lwid, wid);
#else
                tve.lscr = ngram_tg_score(kbcore_lm(kbc), lwid, pve->lmstate.lm3g.lwid[0], pve->lmstate.lm3g.lwid[1], &n_used);
#endif
                tve.path.score += tve.lscr;
                if ((tve.path.score - vh->wbeam) >= vh->bestscore[vh->n_frm]) {
                    tve.path.pred = i;
                    tve.lmstate.lm3g.lwid[1] = pve->lmstate.lm3g.lwid[0];

                    vithist_enter(vh, kbc, &tve, rc);
                }
            }
        }
    }
}


/*
 * Garbage collect invalid entries in current frame, right after marking invalid entries.
 */
static void
vithist_frame_gc(vithist_t * vh, int32 frm)
{
    vithist_entry_t *ve, *tve;
    int32 se, fe, te, bs, bv;
    int32 i, j;
    int32 l;
    int32 n_rc_info;

    n_rc_info = 0;
    se = vh->frame_start[frm];
    fe = vh->n_entry - 1;
    te = se;

    bs = MAX_NEG_INT32;
    bv = -1;
    for (i = se; i <= fe; i++) {
        ve = vithist_id2entry(vh, i);
        if (ve->valid) {
            if (i != te) {      /* Move i to te */
                tve = vithist_id2entry(vh, te);
                /**tve = *ve;*/
                vithist_entry_cp(tve, ve);
            }

            if (ve->path.score > bs) {
                bs = ve->path.score;
                bv = te;
            }

            te++;
        }
    }


    /*    E_INFO("frm %d, bs %d vh->bestscore[frm] %d\n",frm, bs, vh->bestscore[frm]); */

    /* Can't assert this any more because history pruning could actually make the 
       best token go away 

     */
    assert(bs == vh->bestscore[frm]);


    vh->bestvh[frm] = bv;

    /* Free up entries [te..n_entry-1] */
    i = VITHIST_ID2BLK(vh->n_entry - 1);
    j = VITHIST_ID2BLK(te - 1);
    for (; i > j; --i) {

        for (l = 0; l < VITHIST_BLKSIZE; l++) {
            ve = vh->entry[i] + l;
            if (ve->rc != NULL) {
                ckd_free(ve->rc);
                ve->rc = NULL;
            }
        }

        ckd_free((void *) vh->entry[i]);
        vh->entry[i] = NULL;
    }
    vh->n_entry = te;
}



void
vithist_prune(vithist_t * vh, dict_t * dict, int32 frm,
              int32 maxwpf, int32 maxhist, int32 beam)
{
    int32 se, fe, filler_done, th;
    vithist_entry_t *ve;
    heap_t h;
    s3wid_t *wid;
    int32 i;

    assert(frm >= 0);

    se = vh->frame_start[frm];
    fe = vh->n_entry - 1;

    th = vh->bestscore[frm] + beam;

    h = heap_new();
    wid = (s3wid_t *) ckd_calloc(maxwpf + 1, sizeof(s3wid_t));
    wid[0] = BAD_S3WID;

    for (i = se; i <= fe; i++) {
        ve = vithist_id2entry(vh, i);
        heap_insert(h, (void *) ve, -(ve->path.score));
        ve->valid = 0;
    }

    /* Mark invalid entries: beyond maxwpf words and below threshold */
    filler_done = 0;
    while ((heap_pop(h, (void **) (&ve), &i) > 0)
           && ve->path.score >= th /* the score (or the cw scores) is above threshold */
           && maxhist > 0)        /* Number of history is larger than 0 */
    {
        if (dict_filler_word(dict, ve->wid)) {
            /* Major HACK!!  Keep only one best filler word entry per frame */
            if (filler_done)
                continue;
            filler_done = 1;
        }

        /*      E_INFO("ve->wid survived, maxwpf %d, maxhistpf %d\n",maxwpf,maxhist); */
        /* Check if this word already valid (e.g., under a different history) */
        for (i = 0; IS_S3WID(wid[i]) && (wid[i] != ve->wid); i++);
        if (NOT_S3WID(wid[i])) {
            /* New word; keep only if <maxwpf words already entered, even if >= thresh */
            if (maxwpf > 0) {
                wid[i] = ve->wid;
                wid[i + 1] = BAD_S3WID;

                --maxwpf;
                --maxhist;
                ve->valid = 1;
            }
        }
        else if (!vh->bghist) {
            --maxhist;
            ve->valid = 1;
        }
    }

    ckd_free((void *) wid);
    heap_destroy(h);

    /* Garbage collect invalid entries */
    vithist_frame_gc(vh, frm);
}


static void
vithist_lmstate_reset(vithist_t * vh)
{
    gnode_t *lgn, *gn;
    int32 i;
    vh_lms2vh_t *lms2vh, *child;

    for (lgn = vh->lwidlist; lgn; lgn = gnode_next(lgn)) {
        i = (int32) gnode_int32(lgn);
        lms2vh = vh->lms2vh_root[i];

        for (gn = lms2vh->children; gn; gn = gnode_next(gn)) {
            child = (vh_lms2vh_t *) gnode_ptr(gn);
            ckd_free((void *) child);
        }
        glist_free(lms2vh->children);

        ckd_free((void *) lms2vh);

        vh->lms2vh_root[i] = NULL;
    }

    glist_free(vh->lwidlist);
    vh->lwidlist = NULL;
}


void
vithist_frame_windup(vithist_t * vh, int32 frm, FILE * fp, kbcore_t * kbc)
{
    assert(vh->n_frm == frm);

    vh->n_frm++;
    vh->frame_start[vh->n_frm] = vh->n_entry;

    if (fp)
        vithist_dump(vh, frm, kbc, fp);

    vithist_lmstate_reset(vh);

    vh->bestscore[vh->n_frm] = MAX_NEG_INT32;
    vh->bestvh[vh->n_frm] = -1;
}


int32
vithist_utt_end(vithist_t * vh, kbcore_t * kbc)
{
    int32 f, i;
    int32 sv, nsv, scr, bestscore, bestvh, vhid;
    vithist_entry_t *ve, *bestve = 0;
#ifdef OLD_LM_API
    s3lmwid32_t endwid = BAD_S3LMWID32;
    lm_t *lm;
#else
    int32 endwid = NGRAM_INVALID_WID;
    ngram_model_t *lm;
#endif
    dict_t *dict;

    bestscore = MAX_NEG_INT32;
    bestvh = -1;

    /* Find last frame with entries in vithist table */
    /* by ARCHAN 20050525, it is possible that the last frame will not be reached in decoding */

    for (f = vh->n_frm - 1; f >= 0; --f) {
        sv = vh->frame_start[f];        /* First vithist entry in frame f */
        nsv = vh->frame_start[f + 1];   /* First vithist entry in next frame (f+1) */

        if (sv < nsv)
            break;
    }
    if (f < 0)
        return -1;

    if (f != vh->n_frm - 1)
        E_WARN("No word exit in frame %d, using exits from frame %d\n",
               vh->n_frm - 1, f);

    /* Terminate in a final </s> node (make this optional?) */
    lm = kbcore_lm(kbc);
    dict = kbcore_dict(kbc);

#ifdef OLD_LM_API
    endwid = lm_finishwid(lm);
#else
    endwid = ngram_wid(lm, S3_FINISH_WORD);
#endif

    for (i = sv; i < nsv; i++) {
#ifndef OLD_LM_API
        int32 n_used;
#endif
        ve = vithist_id2entry(vh, i);
        scr = ve->path.score;
#ifdef OLD_LM_API
        scr +=
            lm_tg_score(lm, ve->lmstate.lm3g.lwid[1],
                        ve->lmstate.lm3g.lwid[0], endwid,
                        dict_finishwid(dict));
#else
        scr += ngram_tg_score(lm, endwid, ve->lmstate.lm3g.lwid[0], ve->lmstate.lm3g.lwid[1], &n_used);
#endif

        if (bestscore < scr) {
            bestscore = scr;
            bestvh = i;
            bestve = ve;
        }
    }
    assert(bestvh >= 0);


    if (f != vh->n_frm - 1) {
        E_ERROR("No word exit in frame %d, using exits from frame %d\n",
                vh->n_frm - 1, f);

        /* Add a dummy silwid covering the remainder of the utterance */
        assert(vh->frame_start[vh->n_frm - 1] ==
               vh->frame_start[vh->n_frm]);
        vh->n_frm -= 1;
        vithist_rescore(vh, kbc, dict_silwid(dict), vh->n_frm,
                        bestve->path.score, bestvh, -1, -1);
        vh->n_frm += 1;
        vh->frame_start[vh->n_frm] = vh->n_entry;

        return vithist_utt_end(vh, kbc);
    }

    /*    vithist_dump(vh,-1,kbc,stdout); */
    /* Create an </s> entry */
    ve = vithist_entry_alloc(vh);

    ve->wid = dict_finishwid(dict);
    ve->sf = (bestve->ef == BAD_S3FRMID) ? 0 : bestve->ef + 1;
    ve->ef = vh->n_frm;
    ve->ascr = 0;
    ve->lscr = bestscore - bestve->path.score;
    ve->path.score = bestscore;
    ve->path.pred = bestvh;
    ve->type = 0;
    ve->valid = 1;
    ve->lmstate.lm3g.lwid[0] = endwid;
    ve->lmstate.lm3g.lwid[1] = ve->lmstate.lm3g.lwid[0];

    vhid = vh->n_entry - 1;


    /*    vithist_dump(vh,-1,kbc,stdout); */

    return vhid;

}


int32
vithist_partialutt_end(vithist_t * vh, kbcore_t * kbc)
{
    int32 f, i;
    int32 sv, nsv, scr, bestscore, bestvh;
    vithist_entry_t *ve, *bestve;
#ifdef OLD_LM_API
    s3lmwid32_t endwid;
    lm_t *lm;
#else
    ngram_model_t *lm;
    int32 endwid;
#endif
    dict_t *dict;

    /* Find last frame with entries in vithist table */
    for (f = vh->n_frm - 1; f >= 0; --f) {
        sv = vh->frame_start[f];        /* First vithist entry in frame f */
        nsv = vh->frame_start[f + 1];   /* First vithist entry in next frame (f+1) */

        if (sv < nsv)
            break;
    }
    if (f < 0)
        return -1;

    if (f != vh->n_frm - 1) {
        E_ERROR("No word exits from in block with last frame= %d\n",
                vh->n_frm - 1);
        return -1;
    }

    /* Terminate in a final </s> node (make this optional?) */
    lm = kbcore_lm(kbc);
    dict = kbcore_dict(kbc);

#ifdef OLD_LM_API
    endwid = lm_finishwid(lm);
#else
    endwid = ngram_wid(lm, S3_FINISH_WORD);
#endif

    bestscore = MAX_NEG_INT32;
    bestvh = -1;

    for (i = sv; i < nsv; i++) {
#ifndef OLD_LM_API
        int32 n_used;
#endif
        ve = vithist_id2entry(vh, i);

        scr = ve->path.score;
#ifdef OLD_LM_API
        scr +=
            lm_tg_score(lm, ve->lmstate.lm3g.lwid[1],
                        ve->lmstate.lm3g.lwid[0], endwid,
                        dict_finishwid(dict));
#else
        scr += ngram_tg_score(lm, endwid, ve->lmstate.lm3g.lwid[0], ve->lmstate.lm3g.lwid[1], &n_used);
#endif

        if (bestscore < scr) {
            bestscore = scr;
            bestvh = i;
            bestve = ve;
        }
    }

    return bestvh;
}


void
vithist_utt_reset(vithist_t * vh)
{
    int32 b;
    int32 ent;

    vithist_lmstate_reset(vh);

    for (b = VITHIST_ID2BLK(vh->n_entry - 1); b >= 0; --b) {

        /* If rc_info is used, then free them */
        if (b != 0)
            ent = VITHIST_BLKSIZE - 1;
        else
            ent = vh->n_entry - 1;
        ckd_free((void *) vh->entry[b]);
        vh->entry[b] = NULL;
    }
    vh->n_entry = 0;

    vh->bestscore[0] = MAX_NEG_INT32;
    vh->bestvh[0] = -1;
}


#if 0
static void
vithist_lmstate_subtree_dump(vithist_t * vh, kbcore_t * kbc,
                             vh_lmstate2vithist_t * lms2vh, int32 level,
                             FILE * fp)
{
    gnode_t *gn;
    vh_lmstate2vithist_t *child;
    int32 i;
#ifdef OLD_LM_API
    lm_t *lm;
#else
    ngram_model_t *lm;
#endif

    lm = kbcore_lm(kbc);

    for (gn = lms2vh->children; gn; gn = gnode_next(gn)) {
        child = (vh_lmstate2vithist_t *) gnode_ptr(gn);

        for (i = 0; i < level; i++)
            fprintf(fp, "\t");
#ifdef OLD_LM_API
        fprintf(fp, "\t%s -> %d\n", lm_wordstr(lm, child->state),
                child->vhid);
#else
        fprintf(fp, "\t%s -> %d\n", ngram_word(lm, child->state),
                child->vhid);
#endif

        vithist_lmstate_subtree_dump(vh, kbc, child, level + 1, fp);
    }
}


static void
vithist_lmstate_dump(vithist_t * vh, kbcore_t * kbc, FILE * fp)
{
    glist_t gl;
    gnode_t *lgn, *gn;
    int32 i;
    vh_lmstate2vithist_t *lms2vh;
    mdef_t *mdef;
#ifdef OLD_LM_API
    lm_t *lm;
#else
    ngram_model_t *lm;
#endif

    mdef = kbcore_mdef(kbc);
    lm = kbcore_lm(kbc);

    fprintf(fp, "LMSTATE\n");
    for (lgn = vh->lwidlist; lgn; lgn = gnode_next(lgn)) {
        i = (int32) gnode_int32(lgn);

        gl = vh->lmstate_root[i];
        assert(gl);

        for (gn = gl; gn; gn = gnode_next(gn)) {
            lms2vh = (vh_lmstate2vithist_t *) gnode_ptr(gn);

            fprintf(fp, "\t%s.%s -> %d\n",
#ifdef OLD_LM_API
                    lm_wordstr(lm, i), mdef_ciphone_str(mdef,
#else
                    ngram_word(lm, i), mdef_ciphone_str(mdef,
#endif
                                                        lms2vh->state),
                    lms2vh->vhid);
            vithist_lmstate_subtree_dump(vh, kbc, lms2vh, 1, fp);
        }
    }
    fprintf(fp, "END_LMSTATE\n");
    fflush(fp);
}
#endif


void
vithist_dump(vithist_t * vh, int32 frm, kbcore_t * kbc, FILE * fp)
{
    int32 i, j;
    dict_t *dict;
    vithist_entry_t *ve;
#ifdef OLD_LM_API
    lm_t *lm;
    s3lmwid32_t lwid;
#else
    ngram_model_t *lm;
    int32 lwid;
#endif
    int32 sf, ef;

    dict = kbcore_dict(kbc);
    lm = kbcore_lm(kbc);

    if (frm >= 0) {
        sf = frm;
        ef = frm;

        fprintf(fp, "VITHIST  frame %d  #entries %d\n",
                frm, vh->frame_start[sf + 1] - vh->frame_start[sf]);
    }
    else {
        sf = 0;
        ef = vh->n_frm - 1;

        fprintf(fp, "VITHIST  #frames %d  #entries %d\n", vh->n_frm,
                vh->n_entry);
    }
    fprintf(fp, "\t%7s %5s %5s %11s %9s %8s %7s %4s Word (LM-state)\n",
            "Seq/Val", "SFrm", "EFrm", "PathScr", "SegAScr", "SegLScr",
            "Pred", "Type");

    for (i = sf; i <= ef; i++) {
        fprintf(fp, "%5d BS: %11d BV: %8d\n", i, vh->bestscore[i],
                vh->bestvh[i]);

        for (j = vh->frame_start[i]; j < vh->frame_start[i + 1]; j++) {
            ve = vithist_id2entry(vh, j);

            fprintf(fp, "\t%c%6d %5d %5d %11d %9d %8d %7d %4d %s",
                    (ve->valid ? ' ' : '*'), j,
                    ve->sf, ve->ef, ve->path.score, ve->ascr, ve->lscr,
                    ve->path.pred, ve->type, dict_wordstr(dict, ve->wid));

#ifdef OLD_LM_API
            fprintf(fp, " (%s", lm_wordstr(lm, ve->lmstate.lm3g.lwid[0]));
            lwid = ve->lmstate.lm3g.lwid[1];
            if (IS_LMWID(lm, lwid))
                fprintf(fp, ", %s", lm_wordstr(lm, lwid));
#else
            fprintf(fp, " (%s", ngram_word(lm, ve->lmstate.lm3g.lwid[0]));
            lwid = ve->lmstate.lm3g.lwid[1];
            fprintf(fp, ", %s", ngram_word(lm, lwid));
#endif
            fprintf(fp, ")\n");
        }

        if (j == vh->frame_start[i])
            fprintf(fp, "\n");
    }

#if 0
    if (vh->lwidlist)
        vithist_lmstate_dump(vh, kbc, fp);
#endif

    fprintf(fp, "END_VITHIST\n");
    fflush(fp);
}

glist_t
vithist_backtrace(vithist_t * vh, int32 id, dict_t * dict)
{
    vithist_entry_t *ve;
    glist_t hyp;
    srch_hyp_t *h;
    s3cipid_t ci;

    hyp = NULL;
    ci = BAD_S3CIPID;
    while (id > 0) {
        /*      E_INFO("id %d\n",id); */
        ve = vithist_id2entry(vh, id);
        assert(ve);
        h = (srch_hyp_t *) ckd_calloc(1, sizeof(srch_hyp_t));
        h->id = ve->wid;
        h->sf = ve->sf;
        h->ef = ve->ef;
        h->ascr = ve->ascr;
        h->lscr = ve->lscr;

        /* FIX ME ! Senone-scale should be computed, Probably shouldn't be computed at here 
           h->senscale=ve->senscale;
         */
        h->type = ve->type;
        h->vhid = id;

        hyp = glist_add_ptr(hyp, h);

        id = ve->path.pred;
    }

    return hyp;
}

dag_t *
vithist_dag_build(vithist_t * vh, glist_t hyp, dict_t * dict, int32 endid, cmd_ln_t *config, logmath_t *logmath)
{
    glist_t *sfwid;             /* To maintain <start-frame, word-id> pair dagnodes */
    vithist_entry_t *ve, *ve2;
    gnode_t *gn, *gn2, *gn3;
    dagnode_t *dn, *dn2;
    int32 min_ef_range;
    int32 sf, ef, n_node;
    int32 f, i, k;
    dag_t *dag;

    dag = ckd_calloc(1, sizeof(*dag));
    dag_init(dag, config, logmath);
    sfwid = (glist_t *) ckd_calloc(vh->n_frm + 1, sizeof(glist_t));

    /* Min. endframes value that a node must persist for it to be not ignored */
    min_ef_range = cmd_ln_int32_r(config, "-min_endfr");

    n_node = 0;
    sf = 0;
    for (i = 0; i < vh->n_entry; i++) { /* This range includes the dummy <s> and </s> entries */
        ve = vithist_id2entry(vh, i);
        if (!ve->valid)
            continue;

        /*
         * The initial <s> entry (at 0) is a dummy, with start/end frame = -1.  But the old S3
         * code treats it like a real word, so we have to reintroduce it in the dag file with
         * a start time of 0.  And shift the start time of words starting at frame 0 up by 1.
         * MAJOR HACK!!
         */
        if (ve->sf == -1) {
            assert(ve->ef == -1);
            sf = ef = 0;
        }
        else if (ve->sf == 0) {
            assert(ve->ef > 0);
            sf = ve->sf + 1;
            ef = ve->ef;
        }
        else {
            sf = ve->sf;
            ef = ve->ef;
        }

        for (gn = sfwid[sf]; gn; gn = gnode_next(gn)) {
            dn = (dagnode_t *) gnode_ptr(gn);
            if (dn->wid == ve->wid)
                break;
        }
        if (!gn) {
            dn = listelem_malloc(dag->node_alloc);
            dn->wid = ve->wid;

            dn->node_ascr = ve->ascr;
            dn->node_lscr = ve->lscr;

            dn->sf = sf;
            dn->fef = ef;
            dn->lef = ef;
            dn->seqid = -1;     /* Initially all invalid, selected ones validated below */
            dn->hook = NULL;
            dn->predlist = NULL;
            dn->succlist = NULL;
            dn->reachable = 0;
            n_node++;

            sfwid[sf] = glist_add_ptr(sfwid[sf], (void *) dn);
        }
        else {
            dn->lef = ef;
        }

        if (i == endid)
            dag->end = dn;

        /*
         * Check if an entry already exists under dn->velist (generated by a different
         * LM context; retain only the best scoring one.
         */
        for (gn = (glist_t)dn->hook; gn; gn = gnode_next(gn)) {
            ve2 = (vithist_entry_t *) gnode_ptr(gn);
            if (ve2->ef == ve->ef)
                break;
        }
        if (gn) {
            if (ve->path.score > ve2->path.score)
                gnode_ptr(gn) = (void *) ve;
        }
        else
            dn->hook = glist_add_ptr((glist_t)dn->hook, (void *) ve);
    }

    /*
     * Validate segments with >=min_endfr end times.
     * But keep segments in the original hypothesis, regardless; mark them first.
     */
    for (gn = hyp; gn; gn = gnode_next(gn)) {
        /*
         * As long as the above comment about the MAJOR HACK is true
         * the start time has to be hacked here too
         */
        srch_hyp_t *h = (srch_hyp_t *) gnode_ptr(gn);
        int hacked_sf = h->sf == 0 ? 1 : h->sf;
        for (gn2 = sfwid[hacked_sf]; gn2; gn2 = gnode_next(gn2)) {
            dn = (dagnode_t *) gnode_ptr(gn2);
            if (h->id == dn->wid)
                dn->seqid = 0;  /* Do not discard (prune) this dagnode */
        }
    }

    /* Validate startwid and finishwid nodes */
    dn = (dagnode_t *) gnode_ptr(sfwid[0]);
    assert(dn->wid == dict_startwid(dict));
    dn->seqid = 0;
    dag->root = dn;
    dag->entry.node = dn;
    dag->entry.ascr = 0;
    dag->entry.next = NULL;
    dag->entry.pscr_valid = 0;
    dag->entry.bypass = NULL;

    dn = (dagnode_t *) gnode_ptr(sfwid[vh->n_frm]);
    assert(dn->wid == dict_finishwid(dict));
    dn->seqid = 0;
    /* If for some reason the final node is not the same as the end
     * node, make sure it exists and is also marked valid. */
    if (dag->end == NULL) {
        E_WARN("Final vithist entry %d not found, using </s> node\n", endid);
        dag->end = dn;
    }
    dag->end->seqid = 0;
    dag->final.node = dag->end;
    dag->final.ascr = 0;
    dag->final.next = NULL;
    dag->final.pscr_valid = 0;
    dag->final.bypass = NULL;
    /* Find the exit score for the end node. */
    for (gn = (glist_t)dag->end->hook; gn; gn = gnode_next(gn)) {
        ve2 = (vithist_entry_t *) gnode_ptr(gn);
        if (ve2->ef == vh->n_frm)
            dag->final.ascr = ve2->ascr;
    }

    /* Now prune dagnodes with <min_endfr end frames if not validated above */
    i = 0;
    for (f = 0; f <= vh->n_frm; ++f) {
        for (gn = sfwid[f]; gn; gn = gnode_next(gn)) {
            dn = (dagnode_t *) gnode_ptr(gn);
            if ((dn->lef - dn->fef > min_ef_range) || (dn->seqid >= 0)) {
                dn->seqid = i++;
                dn->alloc_next = dag->list;
                dag->list = dn;
            }
            else
                dn->seqid = -1; /* Flag: discard */
        }
    }

    for (f = 0; f < vh->n_frm; ++f) {
        for (gn = sfwid[f]; gn; gn = gnode_next(gn)) {
            dn = (dagnode_t *) gnode_ptr(gn);
            /* Look for transitions from this dagnode to later ones, if not discarded */
            if (dn->seqid < 0)
                continue;

            for (gn2 = (glist_t) dn->hook; gn2; gn2 = gnode_next(gn2)) {
                ve = (vithist_entry_t *) gnode_ptr(gn2);
                sf = (ve->ef < 0) ? 1 : (ve->ef + 1);
                for (gn3 = sfwid[sf]; gn3; gn3 = gnode_next(gn3)) {
                    dn2 = (dagnode_t *) gnode_ptr(gn3);
                    if (dn2->seqid >= 0)
                        dag_link(dag, dn, dn2, ve->ascr, ve->lscr, sf - 1, NULL);
                }
            }
        }
    }

    /* Free dagnodes structure */
    for (f = 0; f <= vh->n_frm; f++) {
        for (gn = sfwid[f]; gn; gn = gnode_next(gn)) {
            dn = (dagnode_t *) gnode_ptr(gn);
            glist_free((glist_t) dn->hook);
            dn->hook = NULL;
            if (dn->seqid == -1) /* If pruned, free the node too */
                listelem_free(dag->node_alloc, dn);
        }
        glist_free(sfwid[f]);
    }
    ckd_free((void *) sfwid);

    dag->filler_removed = 0;
    dag->fudged = 0;
    dag->nfrm = vh->n_frm;

    dag->maxedge = cmd_ln_int32_r(config, "-maxedge");
    /*
     * Set limit on max LM ops allowed after which utterance is aborted.
     * Limit is lesser of absolute max and per frame max.
     */
    dag->maxlmop = cmd_ln_int32_r(config, "-maxlmop");
    k = cmd_ln_int32_r(config, "-maxlpf");
    k *= dag->nfrm;
    if (k > 0 && dag->maxlmop > k)
        dag->maxlmop = k;
    dag->lmop = 0;

    return dag;
}


/* 
 * RAH, free memory allocated in vithist_free 
 */
void
vithist_free(vithist_t * v)
{

    if (v) {
        vithist_utt_reset(v);

        if (v->entry) {
            ckd_free((void *) v->entry);
        }

        if (v->frame_start)
            ckd_free((void *) v->frame_start);

        if (v->bestscore)
            ckd_free((void *) v->bestscore);

        if (v->bestvh)
            ckd_free((void *) v->bestvh);

        if (v->lms2vh_root)
            ckd_free((void *) v->lms2vh_root);

        ckd_free((void *) v);
    }

}

void
vithist_report(vithist_t * vh)
{
    E_INFO_NOFN("Initialization of vithist_t, report:\n");
    if (vh) {
        E_INFO_NOFN("Word beam = %d\n", vh->wbeam);
        E_INFO_NOFN("Bigram Mode =%d\n", vh->bghist);
        E_INFO_NOFN("\n");
    }
    else {
        E_INFO_NOFN("Viterbi history is (null)\n");
    }
}
