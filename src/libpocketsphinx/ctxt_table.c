/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 1995-2004 Carnegie Mellon University.  All rights
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
 * ctxt_table.c -- Building a context table , split from flat_fwd.c (or fwd.c)
 *
 * **********************************************
 * CMU ARPA Speech Project
 *
 * Copyright (c) 1995 Carnegie Mellon University.
 * ALL RIGHTS RESERVED.
 * **********************************************
 * 14-Jul-05    ARCHAN (archan@cs.cmu.edu) at Carnegie Mellon Unversity 
 *              First created it. 
 *
 * $Log$
 * Revision 1.2  2006/02/22  20:46:05  arthchan2003
 * Merged from branch SPHINX3_5_2_RCI_IRII_BRANCH: ctxt_table is a wrapper of the triphone context structure and its maniuplations which were used in flat_fwd.c .  The original flat_fwd.c was very long (3000) lines.  It was broken in 5 parts, ctxt_table is one of the 5.
 * 
 * Revision 1.1.2.3  2005/10/09 19:55:33  arthchan2003
 * Changed int8 to uint8. This follows Dave's change in the trunk.
 *
 * Revision 1.1.2.2  2005/09/27 07:39:17  arthchan2003
 * Added ctxt_table_free.
 *
 * Revision 1.1.2.1  2005/09/25 19:08:25  arthchan2003
 * Move context table from search to here.
 *
 * Revision 1.1.2.4  2005/09/07 23:32:03  arthchan2003
 * 1, Added get_lcpid in parrallel with get_rcpid. 2, Also fixed small mistakes in the macro.
 *
 * Revision 1.1.2.3  2005/07/24 01:32:54  arthchan2003
 * Flush the output of the cross word triphone in ctxt_table.c
 *
 * Revision 1.1.2.2  2005/07/17 05:42:27  arthchan2003
 * Added super-detailed comments ctxt_table.h. Also added dimension to the arrays that stores all context tables.
 *
 * Revision 1.1.2.1  2005/07/15 07:48:32  arthchan2003
 * split the hmm (whmm_t) and context building process (ctxt_table_t) from the the flat_fwd.c
 *
 *
 */

#include <string.h>
#include <ckd_alloc.h>

#include "ctxt_table.h"

void
dump_xwdssidmap(xwdssid_t ** x, bin_mdef_t * mdef)
{
    s3cipid_t b, c1, c2;
    s3ssid_t p;

    for (b = 0; b < mdef->n_ciphone; b++) {
        if (!x[b])
            continue;

        for (c1 = 0; c1 < mdef->n_ciphone; c1++) {
            if (!x[b][c1].cimap)
                continue;

            printf("n_ssid(%s, %s) = %d\n",
                   bin_mdef_ciphone_str(mdef, b), bin_mdef_ciphone_str(mdef, c1),
                   x[b][c1].n_ssid);

            for (c2 = 0; c2 < mdef->n_ciphone; c2++) {
                p = x[b][c1].ssid[x[b][c1].cimap[c2]];
                printf("  %10s %5d\n", bin_mdef_ciphone_str(mdef, c2), p);
            }
        }
    }
    fflush(stdout);
}

/**
 * Utility function for building cross-word ssid maps.  Looks up
 * senone sequence IDs.
 */
int32
xwdssid_compress(s3pid_t p, s3ssid_t * out_ssid, s3cipid_t * map, s3cipid_t ctx,
                 int32 n, bin_mdef_t * mdef)
{
    s3cipid_t i;
    int32 ssid;

    ssid = bin_mdef_pid2ssid(mdef, p);
    for (i = 0; i < n; i++) {
        if (out_ssid[i] == ssid) {
            /* This state sequence same as a previous ones; just map to it */
            map[ctx] = i;
            return n;
        }
    }

    /* This state sequence different from all previous ones; allocate new entry */
    map[ctx] = n;
    out_ssid[n] = ssid;

    return (n + 1);
}


/**
 * Given base b, and right context rc, build left context cross-word triphones map
 * for all left context ciphones.  Compress map to unique list.
 */
void
build_lcssid(ctxt_table_t * ct, s3cipid_t b, s3cipid_t rc, bin_mdef_t * mdef,
             uint8 *word_end_ci, s3pid_t *tmp_xwdssid)

{
    s3cipid_t lc, *map;
    s3ssid_t p;
    int32 n;

    /* Maps left CI phone to index in "compressed" phone ID table */
    map = (s3cipid_t *) ckd_calloc(mdef->n_ciphone, sizeof(s3cipid_t));

    n = 0;
    for (lc = 0; lc < mdef->n_ciphone; lc++) {
        p = bin_mdef_phone_id_nearest(mdef, b, lc, rc, WORD_POSN_BEGIN);
        if ((!mdef->phone[b].info.ci.filler) && word_end_ci[lc] &&
            bin_mdef_is_ciphone(mdef, p))
            ct->n_backoff_ci++;

        n = xwdssid_compress(p, tmp_xwdssid, map, lc, n, mdef);
    }

    /* Copy/Move to lcssid */
    ct->lcssid[b][rc].cimap = map;
    ct->lcssid[b][rc].n_ssid = n;
    /* Array of phone IDs with unique senone sequences, pointed to by cimap. */
    ct->lcssid[b][rc].ssid = (s3ssid_t *) ckd_calloc(n, sizeof(s3ssid_t));
    memcpy(ct->lcssid[b][rc].ssid, tmp_xwdssid, n * sizeof(s3ssid_t));
}


/**
 * Given base b, and left context lc, build right context cross-word triphones map
 * for all right context ciphones.  Compress map to unique list.
 *
 * \note This is identical to build_lcssid except for right contexts.
 */
void
build_rcssid(ctxt_table_t * ct, s3cipid_t b, s3cipid_t lc, bin_mdef_t * mdef,
             uint8 *word_start_ci, s3pid_t *tmp_xwdssid)
{
    s3cipid_t rc, *map;
    s3ssid_t p;
    int32 n;

    /* Maps right CI phone to index in "compressed" phone ID table */
    map = (s3cipid_t *) ckd_calloc(mdef->n_ciphone, sizeof(s3cipid_t));

    n = 0;
    for (rc = 0; rc < mdef->n_ciphone; rc++) {
        p = bin_mdef_phone_id_nearest(mdef, b, lc, rc, WORD_POSN_END);
        if ((!mdef->phone[b].info.ci.filler) && word_start_ci[rc] &&
            bin_mdef_is_ciphone(mdef, p))
            ct->n_backoff_ci++;

        n = xwdssid_compress(p, tmp_xwdssid, map, rc, n, mdef);
    }

    /* Copy/Move to rcssid */
    ct->rcssid[b][lc].cimap = map;
    ct->rcssid[b][lc].n_ssid = n;
    /* Array of phone IDs with unique senone sequences, pointed to by cimap. */
    ct->rcssid[b][lc].ssid = (s3ssid_t *) ckd_calloc(n, sizeof(s3ssid_t));
    memcpy(ct->rcssid[b][lc].ssid, tmp_xwdssid, n * sizeof(s3ssid_t));
}

/**
 * Given base b for a single-phone word, build context cross-word triphones map
 * for all left and right context ciphones.
 *
 * These are uncompressed for some reason.
 */
void
build_lrcssid(ctxt_table_t * ct, s3cipid_t b, bin_mdef_t * mdef,
              uint8 *word_start_ci, uint8 *word_end_ci)
{
    s3cipid_t rc, lc;

    for (lc = 0; lc < mdef->n_ciphone; lc++) {
        s3ssid_t p;

        ct->lrcssid[b][lc].ssid =
            (s3ssid_t *) ckd_calloc(mdef->n_ciphone, sizeof(s3ssid_t));
        ct->lrcssid[b][lc].cimap =
            (s3cipid_t *) ckd_calloc(mdef->n_ciphone, sizeof(s3cipid_t));

        for (rc = 0; rc < mdef->n_ciphone; rc++) {
            p = bin_mdef_phone_id_nearest(mdef, b, lc, rc, WORD_POSN_SINGLE);
            ct->lrcssid[b][lc].cimap[rc] = rc;
            ct->lrcssid[b][lc].ssid[rc] = bin_mdef_pid2ssid(mdef, p);
            if ((!mdef->phone[b].info.ci.filler) && word_start_ci[rc]
                && word_end_ci[lc] && bin_mdef_is_ciphone(mdef, p))
                ct->n_backoff_ci++;
        }
        ct->lrcssid[b][lc].n_ssid = mdef->n_ciphone;
    }
}




/**
 * Build within-word triphones sequence for each word.  The extreme ends are not needed
 * since cross-word modelling is used for those.  (See lcssid, rcssid, lrcssid.)
 */

void
build_wwssid(ctxt_table_t * ct, s3dict_t * dict, bin_mdef_t * mdef)
{
    s3wid_t w;
    s3ssid_t p;
    int32 pronlen, l;
    s3cipid_t b, lc, rc;

    E_INFO("Building within-word triphones\n");
    ct->n_backoff_ci = 0;

    ct->wwssid = (s3ssid_t **) ckd_calloc(dict->n_word, sizeof(s3ssid_t *));
    for (w = 0; w < dict->n_word; w++) {
        pronlen = dict->word[w].pronlen;
        if (pronlen >= 3)
            ct->wwssid[w] =
                (s3ssid_t *) ckd_calloc(pronlen - 1, sizeof(s3ssid_t));
        else
            continue;

        lc = dict->word[w].ciphone[0];
        b = dict->word[w].ciphone[1];
        for (l = 1; l < pronlen - 1; l++) {
            rc = dict->word[w].ciphone[l + 1];
            p = bin_mdef_phone_id_nearest(mdef, b, lc, rc, WORD_POSN_INTERNAL);
            ct->wwssid[w][l] = bin_mdef_pid2ssid(mdef, p);
            if ((!mdef->phone[b].info.ci.filler) && bin_mdef_is_ciphone(mdef, p))
                ct->n_backoff_ci++;

            lc = b;
            b = rc;
        }
#if 0
        printf("%-25s ", dict->word[w].word);
        for (l = 1; l < pronlen - 1; l++)
            printf(" %5d", ct->wwssid[w][l]);
        printf("\n");
#endif
    }

    E_INFO("%d within-word triphone instances mapped to CI-phones\n",
           ct->n_backoff_ci);

}

/**
 * Build cross-word triphones map for the entire dictionary.
 */
void
build_xwdssid_map(ctxt_table_t * ct, s3dict_t * dict, bin_mdef_t * mdef)
{
    s3wid_t w;
    int32 pronlen;
    s3cipid_t b, lc, rc;
    s3ssid_t *tmp_xwdssid;
    uint8 *word_start_ci;
    uint8 *word_end_ci;


    ct->n_backoff_ci = 0;

    /* Build cross-word triphone models */
    E_INFO("Building cross-word triphones\n");

    word_start_ci = (uint8 *) ckd_calloc(mdef->n_ciphone, sizeof(uint8));
    word_end_ci = (uint8 *) ckd_calloc(mdef->n_ciphone, sizeof(uint8));

    /* Mark word beginning and ending ciphones that occur in given dictionary */
    for (w = 0; w < dict->n_word; w++) {
        word_start_ci[dict->word[w].ciphone[0]] = 1;
        word_end_ci[dict->word[w].ciphone[dict->word[w].pronlen - 1]] = 1;
    }
    ct->lcssid =
        (xwdssid_t **) ckd_calloc(mdef->n_ciphone, sizeof(xwdssid_t *));
    ct->rcssid =
        (xwdssid_t **) ckd_calloc(mdef->n_ciphone, sizeof(xwdssid_t *));
    ct->lrcssid =
        (xwdssid_t **) ckd_calloc(mdef->n_ciphone, sizeof(xwdssid_t *));
    tmp_xwdssid = (s3ssid_t *) ckd_calloc(mdef->n_ciphone, sizeof(s3ssid_t));

    for (w = 0; w < dict->n_word; w++) {
        pronlen = dict->word[w].pronlen;
        if (pronlen > 1) {
            /* Multi-phone word; build rcmap and lcmap if not already present */
	    /* Right edge of word: rcmap */
            b = dict->word[w].ciphone[pronlen - 1];
            lc = dict->word[w].ciphone[pronlen - 2];
            if (!ct->rcssid[b])
                ct->rcssid[b] =
                    (xwdssid_t *) ckd_calloc(mdef->n_ciphone,
                                             sizeof(xwdssid_t));
            if (!ct->rcssid[b][lc].cimap)
                build_rcssid(ct, b, lc, mdef,
                             word_start_ci, tmp_xwdssid);

            /* Left edge of word: lcmap */
            b = dict->word[w].ciphone[0];
            rc = dict->word[w].ciphone[1];
            if (!ct->lcssid[b])
                ct->lcssid[b] =
                    (xwdssid_t *) ckd_calloc(mdef->n_ciphone,
                                             sizeof(xwdssid_t));
            if (!ct->lcssid[b][rc].cimap)
                build_lcssid(ct, b, rc, mdef,
                             word_end_ci, tmp_xwdssid);

        }
        else {
            /* Single-phone word; build lrcmap if not already present */
            b = dict->word[w].ciphone[0];
            if (!ct->lrcssid[b]) {
                ct->lrcssid[b] =
                    (xwdssid_t *) ckd_calloc(mdef->n_ciphone,
                                             sizeof(xwdssid_t));
                build_lrcssid(ct, b, mdef,
                              word_start_ci, word_end_ci);
            }
        }
    }

    ckd_free(word_start_ci);
    ckd_free(word_end_ci);
    ckd_free(tmp_xwdssid);
    E_INFO("%d cross-word triphones mapped to CI-phones\n",
           ct->n_backoff_ci);

#if 0
    E_INFO("LCXWDSSID\n");
    dump_xwdssidmap(ct->lcssid, mdef);

    E_INFO("RCXWDSSID\n");
    dump_xwdssidmap(ct->rcssid, mdef);

    E_INFO("LRCXWDSSID\n");
    dump_xwdssidmap(ct->lrcssid, mdef);
#endif
}

ctxt_table_t *
ctxt_table_init(s3dict_t * dict, bin_mdef_t * mdef)
{
    ctxt_table_t *ct;
    ct = (ctxt_table_t *) ckd_calloc(1, sizeof(ctxt_table_t));

    ct->n_ci = mdef->n_ciphone;
    ct->n_word = dict->n_word;

    build_wwssid(ct, dict, mdef);
    build_xwdssid_map(ct, dict, mdef);

    return ct;
}

void
xwdssid_free(xwdssid_t **xs, int32 n_ci)
{
    s3cipid_t b, c;

    if (xs == NULL)
        return;
    for (b = 0; b < n_ci; ++b) {
        if (xs[b] == NULL)
            continue;
        for (c = 0; c < n_ci; ++c) {
            ckd_free(xs[b][c].cimap);
            ckd_free(xs[b][c].ssid);
        }
        ckd_free(xs[b]);
    }
    ckd_free(xs);
}

void
ctxt_table_free(ctxt_table_t * ct)
{
    xwdssid_free(ct->lcssid, ct->n_ci);
    xwdssid_free(ct->rcssid, ct->n_ci);
    xwdssid_free(ct->lrcssid, ct->n_ci);

    if (ct->wwssid) {
        s3wid_t w;
        for (w = 0; w < ct->n_word; ++w)
            ckd_free(ct->wwssid[w]);
        ckd_free(ct->wwssid);
    }
    ckd_free(ct);
}

s3cipid_t *
get_rc_cimap(ctxt_table_t * ct, s3wid_t w, s3dict_t * dict)
{
    int32 pronlen;
    s3cipid_t b, lc;

    pronlen = dict->word[w].pronlen;
    b = dict->word[w].ciphone[pronlen - 1];
    if (pronlen == 1) {
        /* No known left context.  But all cimaps (for any l) are identical; pick one */
        return (ct->lrcssid[b][0].cimap);
    }
    else {
        lc = dict->word[w].ciphone[pronlen - 2];
        return (ct->rcssid[b][lc].cimap);
    }
}

s3cipid_t *
get_lc_cimap(ctxt_table_t * ct, s3wid_t w, s3dict_t * dict)
{
    int32 pronlen;
    s3cipid_t b, rc;

    pronlen = dict->word[w].pronlen;
    b = dict->word[w].ciphone[0];
    if (pronlen == 1) {
        /* No known right context.  But all cimaps (for any l) are identical; pick one */
        return (ct->lrcssid[b][0].cimap);
    }
    else {
        rc = dict->word[w].ciphone[1];
        return (ct->lcssid[b][rc].cimap);
    }
}


void
get_rcssid(ctxt_table_t * ct, s3wid_t w, s3ssid_t ** ssid, int32 * nssid,
           s3dict_t * dict)
{
    int32 pronlen;
    s3cipid_t b, lc;

    pronlen = dict->word[w].pronlen;
    assert(pronlen > 1);

    b = dict->word[w].ciphone[pronlen - 1];
    lc = dict->word[w].ciphone[pronlen - 2];

    *ssid = ct->rcssid[b][lc].ssid;
    *nssid = ct->rcssid[b][lc].n_ssid;
}

void
get_lcssid(ctxt_table_t * ct, s3wid_t w, s3ssid_t ** ssid, int32 * nssid,
           s3dict_t * dict)
{
    int32 pronlen;
    s3cipid_t b, rc;

    pronlen = dict->word[w].pronlen;
    assert(pronlen > 1);

    b = dict->word[w].ciphone[0];
    rc = dict->word[w].ciphone[1];

    *ssid = ct->lcssid[b][rc].ssid;
    *nssid = ct->lcssid[b][rc].n_ssid;
}

int32
ct_get_rc_nssid(ctxt_table_t * ct, s3wid_t w, s3dict_t * dict)
{
    int32 pronlen;
    s3cipid_t b, lc;

    pronlen = dict->word[w].pronlen;
    b = dict->word[w].ciphone[pronlen - 1];
    assert(ct);
    assert(ct->lrcssid);
    if (pronlen == 1) {
        /* No known left context.  But all cimaps (for any l) are identical; pick one */
        return (ct->lrcssid[b][0].n_ssid);
    }
    else {
        lc = dict->word[w].ciphone[pronlen - 2];
        return (ct->rcssid[b][lc].n_ssid);
    }
}
