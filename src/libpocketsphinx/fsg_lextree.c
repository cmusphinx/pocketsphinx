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
 * fsg_lextree.c -- The collection of all the lextrees for the entire FSM.
 * 
 * **********************************************
 * CMU ARPA Speech Project
 *
 * Copyright (c) 2004 Carnegie Mellon University.
 * ALL RIGHTS RESERVED.
 * **********************************************
 * 
 * HISTORY
 * 
 * $Log: fsg_lextree.c,v $
 * Revision 1.1.1.1  2006/05/23 18:44:59  dhuggins
 * re-importation
 *
 * Revision 1.1  2004/07/16 00:57:11  egouvea
 * Added Ravi's implementation of FSG support.
 *
 * Revision 1.3  2004/06/23 20:32:16  rkm
 * *** empty log message ***
 *
 * Revision 1.2  2004/05/27 14:22:57  rkm
 * FSG cross-word triphones completed (but for single-phone words)
 *
 * Revision 1.1.1.1  2004/03/01 14:30:30  rkm
 *
 *
 * Revision 1.2  2004/02/27 15:05:21  rkm
 * *** empty log message ***
 *
 * Revision 1.1  2004/02/23 15:53:45  rkm
 * Renamed from fst to fsg
 *
 * Revision 1.2  2004/02/19 21:16:54  rkm
 * Added fsg_search.{c,h}
 *
 * Revision 1.1  2004/02/18 15:02:34  rkm
 * Added fsg_lextree.{c,h}
 *
 * 
 * 18-Feb-2004	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon
 * 		Started.
 */

/* System headers. */
#include <stdio.h>
#include <string.h>
#include <assert.h>

/* SphinxBase headers. */
#include <ckd_alloc.h>
#include <err.h>

/* Local headers. */
#include "fsg_lextree.h"

#define __FSG_DBG__		0

/**
 * Build the phone lextree for all transitions out of state from_state.
 * Return the root node of this tree.
 * Also, return a linear linked list of all allocated fsg_pnode_t nodes in
 * *alloc_head (for memory management purposes).
 */
static fsg_pnode_t *fsg_psubtree_init(fsg_lextree_t *tree,
                                      fsg_model_t *fsg,
                                      int32 from_state,
                                      fsg_pnode_t **alloc_head);

/**
 * Free the given lextree.  alloc_head: head of linear list of allocated
 * nodes updated by fsg_psubtree_init().
 */
static void fsg_psubtree_free(fsg_pnode_t *alloc_head);

/**
 * Dump the list of nodes in the given lextree to the given file.  alloc_head:
 * head of linear list of allocated nodes updated by fsg_psubtree_init().
 */
static void fsg_psubtree_dump(fsg_lextree_t *tree, fsg_pnode_t *alloc_head, FILE *fp);

/**
 * Compute the left and right context CIphone sets for each state.
 */
static void
fsg_lextree_lc_rc(fsg_lextree_t *lextree)
{
    int32 s, d, i, j;
    int32 n_ci;
    gnode_t *gn;
    fsg_model_t *fsg;
    fsg_link_t *l;
    int32 silcipid;
    int32 len;

    silcipid = bin_mdef_ciphone_id(lextree->mdef, "SIL");
    assert(silcipid >= 0);
    n_ci = bin_mdef_n_ciphone(lextree->mdef);

    fsg = lextree->fsg;
    /*
     * lextree->lc[s] = set of left context CIphones for state s.  Similarly, rc[s]
     * for right context CIphones.
     */
    lextree->lc = ckd_calloc_2d(fsg->n_state, n_ci + 1, sizeof(**lextree->lc));
    lextree->rc = ckd_calloc_2d(fsg->n_state, n_ci + 1, sizeof(**lextree->rc));

    for (s = 0; s < fsg->n_state; s++) {
        for (d = 0; d < fsg->n_state; d++) {
            for (gn = fsg->trans[s][d]; gn; gn = gnode_next(gn)) {
                int32 dictwid; /**< Dictionary (not FSG) word ID!! */

                l = (fsg_link_t *) gnode_ptr(gn);
                assert(l->wid >= 0);
                dictwid = dict_to_id(lextree->dict,
                                     fsg_model_word_str(lextree->fsg, l->wid));

                /*
                 * Add the first CIphone of l->wid to the rclist of state s, and
                 * the last CIphone to lclist of state d.
                 * (Filler phones are a pain to deal with.  There is no direct
                 * marking of a filler phone; but only filler words are supposed to
                 * use such phones, so we use that fact.  HACK!!  FRAGILE!!)
                 */
                if (fsg_model_is_filler(fsg, l->wid)) {
                    /* Filler phone; use silence phone as context */
                    lextree->rc[s][silcipid] = 1;
                    lextree->lc[d][silcipid] = 1;
                }
                else {
                    len = dict_pronlen(lextree->dict, dictwid);
                    lextree->rc[s][dict_ciphone(lextree->dict, dictwid, 0)] = 1;
                    lextree->lc[d][dict_ciphone(lextree->dict, dictwid, len - 1)] = 1;
                }
            }
        }

        /*
         * Add SIL phone to the lclist and rclist of each state.  Strictly
         * speaking, only needed at start and final states, respectively, but
         * all states considered since the user may change the start and final
         * states.  In any case, most applications would have a silence self
         * loop at each state, hence these would be needed anyway.
         */
        lextree->lc[s][silcipid] = 1;
        lextree->rc[s][silcipid] = 1;
    }

    /*
     * Propagate lc and rc lists past null transitions.  (Since FSG contains
     * null transitions closure, no need to worry about a chain of successive
     * null transitions.  Right??)
     */
    for (s = 0; s < fsg->n_state; s++) {
        for (d = 0; d < fsg->n_state; d++) {
            l = fsg->null_trans[s][d];
            if (l) {
                /*
                 * lclist(d) |= lclist(s), because all the words ending up at s, can
                 * now also end at d, becoming the left context for words leaving d.
                 */
                for (i = 0; i < n_ci; i++)
                    lextree->lc[d][i] |= lextree->lc[s][i];
                /*
                 * Similarly, rclist(s) |= rclist(d), because all the words leaving d
                 * can equivalently leave s, becoming the right context for words
                 * ending up at s.
                 */
                for (i = 0; i < n_ci; i++)
                    lextree->rc[s][i] |= lextree->rc[d][i];
            }
        }
    }

    /* Convert the bit-vector representation into a list */
    for (s = 0; s < fsg->n_state; s++) {
        j = 0;
        for (i = 0; i < n_ci; i++) {
            if (lextree->lc[s][i]) {
                lextree->lc[s][j] = i;
                j++;
            }
        }
        lextree->lc[s][j] = -1;     /* Terminate the list */

        j = 0;
        for (i = 0; i < n_ci; i++) {
            if (lextree->rc[s][i]) {
                lextree->rc[s][j] = i;
                j++;
            }
        }
        lextree->rc[s][j] = -1;     /* Terminate the list */
    }
}

/*
 * For now, allocate the entire lextree statically.
 */
fsg_lextree_t *
fsg_lextree_init(fsg_model_t * fsg, dict_t *dict,
                 bin_mdef_t *mdef, hmm_context_t *ctx,
                 int32 wip, int32 pip)
{
    int32 s;
    fsg_lextree_t *lextree;
    fsg_pnode_t *pn;

    lextree = ckd_calloc(1, sizeof(fsg_lextree_t));
    lextree->fsg = fsg;
    lextree->root = ckd_calloc(fsg_model_n_state(fsg),
                               sizeof(fsg_pnode_t *));
    lextree->alloc_head = ckd_calloc(fsg_model_n_state(fsg),
                                     sizeof(fsg_pnode_t *));
    lextree->ctx = ctx;
    lextree->dict = dict;
    lextree->mdef = mdef;
    lextree->wip = wip;
    lextree->pip = pip;

    /* Compute lc and rc for fsg. */
    fsg_lextree_lc_rc(lextree);

    /* Create lextree for each state */
    lextree->n_pnode = 0;
    for (s = 0; s < fsg_model_n_state(fsg); s++) {
        lextree->root[s] =
            fsg_psubtree_init(lextree, fsg, s, &(lextree->alloc_head[s]));

        for (pn = lextree->alloc_head[s]; pn; pn = pn->alloc_next)
            lextree->n_pnode++;
    }
    E_INFO("%d HMM nodes in lextree\n", lextree->n_pnode);

#if __FSG_DBG__
    fsg_lextree_dump(lextree, stdout);
#endif

    return lextree;
}


void
fsg_lextree_dump(fsg_lextree_t * lextree, FILE * fp)
{
    int32 s;

    for (s = 0; s < fsg_model_n_state(lextree->fsg); s++) {
        fprintf(fp, "State %5d root %p\n", s, lextree->root[s]);
        fsg_psubtree_dump(lextree, lextree->alloc_head[s], fp);
    }
    fflush(fp);
}


void
fsg_lextree_free(fsg_lextree_t * lextree)
{
    int32 s;

    if (lextree == NULL)
        return;

    if (lextree->fsg)
        for (s = 0; s < fsg_model_n_state(lextree->fsg); s++)
            fsg_psubtree_free(lextree->alloc_head[s]);

    ckd_free_2d(lextree->lc);
    ckd_free_2d(lextree->rc);
    ckd_free(lextree->root);
    ckd_free(lextree->alloc_head);
    ckd_free(lextree);
}

void
fsg_pnode_add_all_ctxt(fsg_pnode_ctxt_t * ctxt)
{
    int32 i;

    for (i = 0; i < FSG_PNODE_CTXT_BVSZ; i++)
        ctxt->bv[i] = 0xffffffff;
}


uint32
fsg_pnode_ctxt_sub(fsg_pnode_ctxt_t * src, fsg_pnode_ctxt_t * sub)
{
    int32 i;
    uint32 non_zero;

    non_zero = 0;

    for (i = 0; i < FSG_PNODE_CTXT_BVSZ; i++) {
        src->bv[i] = ~(sub->bv[i]) & src->bv[i];
        non_zero |= src->bv[i];
    }

    return non_zero;
}


/*
 * Add the word emitted by the given transition (fsglink) to the given lextree
 * (rooted at root), and return the new lextree root.  (There may actually be
 * several root nodes, maintained in a linked list via fsg_pnode_t.sibling.
 * "root" is the head of this list.)
 * lclist, rclist: sets of left and right context phones for this link.
 * alloc_head: head of a linear list of all allocated pnodes for the parent
 * FSG state, kept elsewhere and updated by this routine.
 * 
 * NOTE: No lextree structure for now; using a flat representation.
 */
static fsg_pnode_t *
psubtree_add_trans(fsg_lextree_t *lextree, 
                   fsg_pnode_t * root,
                   fsg_link_t * fsglink,
                   int16 *lclist, int16 *rclist,
                   fsg_pnode_t ** alloc_head)
{
    int16 **lcfwd;              /* Uncompressed left cross-word context map;
                                   lcfwd[left-diphone][p] = SSID for p.left-diphone */
    int16 **lcbwd;              /* Compressed left cross-word context map;
                                   lcbwd[left-diphone] = array of unique SSIDs for all
                                   possible left contexts */
    int16 **lcbwdperm;          /* For CIphone p, lcbwdperm[d][p] = index in lcbwd[d]
                                   containing the SSID for triphone p.d */
    int16 **rcbwd;              /* Uncompressed right cross-word context map;
                                   rcbwd[right-diphone][p] = SSID for right-diphone.p */
    int16 **rcfwd;              /* Compressed right cross-word context map; similar to
                                   lcbwd */
    int16 **rcfwdperm;

    int32 silcipid;             /* Silence CI phone ID */
    int32 pronlen;              /* Pronunciation length */
    int32 wid;                  /* FSG (not dictionary!!) word ID */
    int32 dictwid;              /* Dictionary (not FSG!!) word ID */
    int32 did;                  /* Diphone ID */
    int32 ssid;                 /* Senone Sequence ID */
    gnode_t *gn;
    fsg_pnode_t *pnode, *pred, *head;
    int32 n_ci, p, lc, rc;
    glist_t lc_pnodelist;       /* Temp pnodes list for different left contexts */
    glist_t rc_pnodelist;       /* Temp pnodes list for different right contexts */
    fsg_pnode_t **ssid_pnode_map;       /* Temp array of ssid->pnode mapping */
    int32 i, j;

    silcipid = bin_mdef_ciphone_id(lextree->mdef, "SIL");
    n_ci = bin_mdef_n_ciphone(lextree->mdef);
    lcfwd = lextree->dict->lcFwdTable;
    lcbwd = lextree->dict->lcBwdTable;
    lcbwdperm = lextree->dict->lcBwdPermTable;
    rcbwd = lextree->dict->rcBwdTable;
    rcfwd = lextree->dict->rcFwdTable;
    rcfwdperm = lextree->dict->rcFwdPermTable;

    wid = fsg_link_wid(fsglink);
    assert(wid >= 0);           /* Cannot be a null transition */
    dictwid = dict_to_id(lextree->dict,
                         fsg_model_word_str(lextree->fsg, wid));
    pronlen = dict_pronlen(lextree->dict, dictwid);
    assert(pronlen >= 1);

    assert(lclist[0] >= 0);     /* At least one phonetic context provided */
    assert(rclist[0] >= 0);

    head = *alloc_head;
    pred = NULL;

    if (pronlen == 1) {         /* Single-phone word */
        did = dict_phone(lextree->dict, dictwid, 0); /* Diphone ID or SSID */
        if (dict_mpx(lextree->dict, dictwid)) {      /* Only non-filler words are mpx */
            /*
             * Left diphone ID for single-phone words already assumes SIL is right
             * context; only left contexts need to be handled.
             */
            lc_pnodelist = NULL;

            for (i = 0; lclist[i] >= 0; i++) {
                lc = lclist[i];
                ssid = lcfwd[did][lc];  /* Use lcfwd for single-phone word, not lcbwd,
                                           as lcbwd would use only SIL as context */
                /* Check if this ssid already allocated for some other context */
                for (gn = lc_pnodelist; gn; gn = gnode_next(gn)) {
                    pnode = (fsg_pnode_t *) gnode_ptr(gn);

                    if (hmm_nonmpx_ssid(&pnode->hmm) == ssid) {
                        /* already allocated; share it for this context phone */
                        fsg_pnode_add_ctxt(pnode, lc);
                        break;
                    }
                }

                if (!gn) {      /* ssid not already allocated */
                    pnode =
                        (fsg_pnode_t *) ckd_calloc(1, sizeof(fsg_pnode_t));
                    pnode->ctx = lextree->ctx;
                    pnode->next.fsglink = fsglink;
                    pnode->logs2prob =
                        fsg_link_logs2prob(fsglink) + lextree->wip + lextree->pip;
                    pnode->ci_ext = dict_ciphone(lextree->dict, dictwid, 0);
                    pnode->ppos = 0;
                    pnode->leaf = TRUE;
                    pnode->sibling = root;      /* All root nodes linked together */
                    fsg_pnode_add_ctxt(pnode, lc);      /* Initially zeroed by calloc above */
                    pnode->alloc_next = head;
                    head = pnode;
                    root = pnode;

                    hmm_init(lextree->ctx, &pnode->hmm, FALSE, ssid, pnode->ci_ext);

                    lc_pnodelist =
                        glist_add_ptr(lc_pnodelist, (void *) pnode);
                }
            }

            glist_free(lc_pnodelist);
        }
        else {                  /* Filler word; no context modelled */
            ssid = did;         /* dict_phone() already has the right CIphone ssid */

            pnode = (fsg_pnode_t *) ckd_calloc(1, sizeof(fsg_pnode_t));
            pnode->ctx = lextree->ctx;
            pnode->next.fsglink = fsglink;
            pnode->logs2prob = fsg_link_logs2prob(fsglink) + lextree->wip + lextree->pip;
            pnode->ci_ext = silcipid;   /* Presents SIL as context to neighbors */
            pnode->ppos = 0;
            pnode->leaf = TRUE;
            pnode->sibling = root;
            fsg_pnode_add_all_ctxt(&(pnode->ctxt));
            pnode->alloc_next = head;
            head = pnode;
            root = pnode;

            hmm_init(lextree->ctx, &pnode->hmm, FALSE, ssid, pnode->ci_ext);
        }
    }
    else {                      /* Multi-phone word */
        assert(dict_mpx(lextree->dict, dictwid));    /* S2 HACK: pronlen>1 => mpx?? */

        ssid_pnode_map =
            (fsg_pnode_t **) ckd_calloc(n_ci, sizeof(fsg_pnode_t *));
        lc_pnodelist = NULL;
        rc_pnodelist = NULL;

        for (p = 0; p < pronlen; p++) {
            did = ssid = dict_phone(lextree->dict, dictwid, p);

            if (p == 0) {       /* Root phone, handle required left contexts */
                for (i = 0; lclist[i] >= 0; i++) {
                    lc = lclist[i];

                    j = lcbwdperm[did][lc];
                    ssid = lcbwd[did][j];
                    pnode = ssid_pnode_map[j];

                    if (!pnode) {       /* Allocate pnode for this new ssid */
                        pnode =
                            (fsg_pnode_t *) ckd_calloc(1,
                                                       sizeof
                                                       (fsg_pnode_t));
                        pnode->ctx = lextree->ctx;
                        pnode->logs2prob =
                            fsg_link_logs2prob(fsglink) + lextree->wip + lextree->pip;
                        pnode->ci_ext = dict_ciphone(lextree->dict, dictwid, 0);
                        pnode->ppos = 0;
                        pnode->leaf = FALSE;
                        pnode->sibling = root;  /* All root nodes linked together */
                        pnode->alloc_next = head;
                        head = pnode;
                        root = pnode;

                        hmm_init(lextree->ctx, &pnode->hmm, FALSE, ssid, pnode->ci_ext);

                        lc_pnodelist =
                            glist_add_ptr(lc_pnodelist, (void *) pnode);
                        ssid_pnode_map[j] = pnode;
                    }
                    else {
                        assert(hmm_nonmpx_ssid(&pnode->hmm) == ssid);
                    }
                    fsg_pnode_add_ctxt(pnode, lc);
                }
            }
            else if (p != pronlen - 1) {        /* Word internal phone */
                pnode = (fsg_pnode_t *) ckd_calloc(1, sizeof(fsg_pnode_t));
                pnode->ctx = lextree->ctx;
                pnode->logs2prob = lextree->pip;
                pnode->ci_ext = dict_ciphone(lextree->dict, dictwid, p);
                pnode->ppos = p;
                pnode->leaf = FALSE;
                pnode->sibling = NULL;
                if (p == 1) {   /* Predecessor = set of root nodes for left ctxts */
                    for (gn = lc_pnodelist; gn; gn = gnode_next(gn)) {
                        pred = (fsg_pnode_t *) gnode_ptr(gn);
                        pred->next.succ = pnode;
                    }
                }
                else {          /* Predecessor = word internal node */
                    pred->next.succ = pnode;
                }
                pnode->alloc_next = head;
                head = pnode;

                hmm_init(lextree->ctx, &pnode->hmm, FALSE, ssid, pnode->ci_ext);

                pred = pnode;
            }
            else {              /* Leaf phone, handle required right contexts */
                memset((void *) ssid_pnode_map, 0,
                       n_ci * sizeof(fsg_pnode_t *));

                for (i = 0; rclist[i] >= 0; i++) {
                    rc = rclist[i];

                    j = rcfwdperm[did][rc];
                    ssid = rcfwd[did][j];
                    pnode = ssid_pnode_map[j];

                    if (!pnode) {       /* Allocate pnode for this new ssid */
                        pnode =
                            (fsg_pnode_t *) ckd_calloc(1,
                                                       sizeof
                                                       (fsg_pnode_t));
                        pnode->ctx = lextree->ctx;
                        pnode->logs2prob = lextree->pip;
                        pnode->ci_ext = dict_ciphone(lextree->dict, dictwid, p);
                        pnode->ppos = p;
                        pnode->leaf = TRUE;
                        pnode->sibling = rc_pnodelist ?
                            (fsg_pnode_t *) gnode_ptr(rc_pnodelist) : NULL;
                        pnode->next.fsglink = fsglink;
                        pnode->alloc_next = head;
                        head = pnode;

                        hmm_init(lextree->ctx, &pnode->hmm, FALSE, ssid, pnode->ci_ext);

                        rc_pnodelist =
                            glist_add_ptr(rc_pnodelist, (void *) pnode);
                        ssid_pnode_map[j] = pnode;
                    }
                    else {
                        assert(hmm_nonmpx_ssid(&pnode->hmm) == ssid);
                    }
                    fsg_pnode_add_ctxt(pnode, rc);
                }

                if (p == 1) {   /* Predecessor = set of root nodes for left ctxts */
                    for (gn = lc_pnodelist; gn; gn = gnode_next(gn)) {
                        pred = (fsg_pnode_t *) gnode_ptr(gn);
                        pred->next.succ =
                            (fsg_pnode_t *) gnode_ptr(rc_pnodelist);
                    }
                }
                else {          /* Predecessor = word internal node */
                    pred->next.succ =
                        (fsg_pnode_t *) gnode_ptr(rc_pnodelist);
                }
            }
        }

        ckd_free((void *) ssid_pnode_map);
        glist_free(lc_pnodelist);
        glist_free(rc_pnodelist);
    }

    *alloc_head = head;

    return root;
}


/*
 * For now, this "tree" will be "flat"
 */
static fsg_pnode_t *
fsg_psubtree_init(fsg_lextree_t *lextree,
                  fsg_model_t * fsg, int32 from_state,
                  fsg_pnode_t ** alloc_head)
{
    int32 dst;
    gnode_t *gn;
    fsg_link_t *fsglink;
    fsg_pnode_t *root;
    int32 n_ci;

    root = NULL;
    assert(*alloc_head == NULL);

    n_ci = bin_mdef_n_ciphone(lextree->mdef);
    if (n_ci > (FSG_PNODE_CTXT_BVSZ * 32)) {
        E_FATAL
            ("#phones > %d; increase FSG_PNODE_CTXT_BVSZ and recompile\n",
             FSG_PNODE_CTXT_BVSZ * 32);
    }
    for (dst = 0; dst < fsg_model_n_state(fsg); dst++) {
        /* Add all links from from_state to dst */
        for (gn = fsg_model_trans(fsg, from_state, dst); gn;
             gn = gnode_next(gn)) {
            /* Add word emitted by this transition (fsglink) to lextree */
            fsglink = (fsg_link_t *) gnode_ptr(gn);

            assert(fsg_link_wid(fsglink) >= 0);     /* Cannot be a null trans */

            root = psubtree_add_trans(lextree, root, fsglink,
                                      lextree->lc[from_state],
                                      lextree->rc[dst],
                                      alloc_head);
        }
    }

    return root;
}


static void
fsg_psubtree_free(fsg_pnode_t * head)
{
    fsg_pnode_t *next;

    while (head) {
        next = head->alloc_next;
        hmm_deinit(&head->hmm);
        ckd_free(head);
        head = next;
    }
}

static void
fsg_psubtree_dump(fsg_lextree_t *tree, fsg_pnode_t * head, FILE * fp)
{
    int32 i;
    fsg_link_t *tl;

    for (; head; head = head->alloc_next) {
        /* Indentation */
        for (i = 0; i <= head->ppos; i++)
            fprintf(fp, "  ");

        fprintf(fp, "%p.@", head);    /* Pointer used as node ID */
        fprintf(fp, " %5d.SS", hmm_nonmpx_ssid(&head->hmm));
        fprintf(fp, " %10d.LP", head->logs2prob);
        fprintf(fp, " %p.SIB", head->sibling);
        fprintf(fp, " %s.%d", bin_mdef_ciphone_str(tree->mdef, head->ci_ext), head->ppos);
        if ((head->ppos == 0) || head->leaf) {
            fprintf(fp, " [");
            for (i = 0; i < FSG_PNODE_CTXT_BVSZ; i++)
                fprintf(fp, "%08x", head->ctxt.bv[i]);
            fprintf(fp, "]");
        }
        if (head->leaf) {
            tl = head->next.fsglink;
            fprintf(fp, " {%s[%d->%d](%d)}",
                    fsg_model_word_str(tree->fsg, tl->wid),
                    tl->from_state, tl->to_state, tl->logs2prob);
        }
        else {
            fprintf(fp, " %p.NXT", head->next.succ);
        }
        fprintf(fp, "\n");
    }

    fflush(fp);
}


void
fsg_psubtree_pnode_deactivate(fsg_pnode_t * pnode)
{
    hmm_clear(&pnode->hmm);
}
