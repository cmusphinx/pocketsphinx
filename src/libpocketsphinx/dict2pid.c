/* -*- c-basic-offset:4; indent-tabs-mode: nil -*- */
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
 * dict2pid.c -- Triphones for dictionary
 * 
 * **********************************************
 * CMU ARPA Speech Project
 *
 * Copyright (c) 1999 Carnegie Mellon University.
 * ALL RIGHTS RESERVED.
 * **********************************************
 * 
 * HISTORY
 * $Log$
 * Revision 1.7  2006/02/22  21:05:16  arthchan2003
 * Merged from branch SPHINX3_5_2_RCI_IRII_BRANCH:
 * 
 * 1, Added logic to handle bothe composite and non composite left
 * triphone.  Composite left triphone's logic (the original one) is
 * tested thoroughly. The non-composite triphone (or full triphone) is
 * found to have bugs.  The latter is fended off from the users in the
 * library level.
 * 
 * 2, Fixed dox-doc.
 * 
 * Revision 1.6.4.5  2005/11/17 06:13:49  arthchan2003
 * Use compressed right context in expansion in triphones.
 *
 * Revision 1.6.4.4  2005/10/17 04:48:45  arthchan2003
 * Free resource correctly in dict2pid.
 *
 * Revision 1.6.4.3  2005/10/07 19:03:38  arthchan2003
 * Added xwdssid_t structure.  Also added compression routines.
 *
 * Revision 1.6.4.2  2005/09/25 19:13:31  arthchan2003
 * Added optional full triphone expansion support when building context phone mapping.
 *
 * Revision 1.6.4.1  2005/07/17 05:21:28  arthchan2003
 * Add panic signal to the code, also commentted ldiph_comsseq.
 *
 * Revision 1.6  2005/06/21 21:03:49  arthchan2003
 * 1, Introduced a reporting routine. 2, Fixed doyxgen documentation, 3, Added  keyword.
 *
 * Revision 1.4  2005/04/21 23:50:26  archan
 * Some more refactoring on the how reporting of structures inside kbcore_t is done, it is now 50% nice. Also added class-based LM test case into test-decode.sh.in.  At this moment, everything in search mode 5 is already done.  It is time to test the idea whether the search can really be used.
 *
 * Revision 1.3  2005/03/30 01:22:46  archan
 * Fixed mistakes in last updates. Add
 *
 * 
 * 14-Sep-1999	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Added dict2pid_comsseq2sen_active().
 * 
 * 04-May-1999	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Started.
 */


#include <string.h>

#include "dict2pid.h"


/** \file dict2pid.c
 * \brief Implementation of dict2pid
 * 
 * A general remark, notice "comsseq" sometimes means compressed
 * sequence.  It should be understood as differnet thing as
 * composite in the source code.
 */

/**
 * Build a glist of triphone senone-sequence IDs (ssids) derivable from [b][r] at the word
 * begin position.  If no triphone found in mdef, include the ssid for basephone b.
 * Return the generated glist.
 */
static glist_t
ldiph_comsseq(bin_mdef_t * mdef,                /**< a model definition*/
              int32 b,                  /**< base phone */
              int32 r                   /**< right context */
    )
{
    int32 l, p, ssid;
    glist_t g;

    g = NULL;
    for (l = 0; l < bin_mdef_n_ciphone(mdef); l++) {
        p = bin_mdef_phone_id(mdef, (s3cipid_t) b, (s3cipid_t) l,
                          (s3cipid_t) r, WORD_POSN_BEGIN);

        if (IS_S3PID(p)) {
            gnode_t *gn;
            ssid = bin_mdef_pid2ssid(mdef, p);
            for (gn = g; gn; gn = gnode_next(gn))
                if (gnode_int32(gn) == ssid)
                    break;
            if (gn == NULL)
                g = glist_add_int32(g, ssid);
        }
    }
    if (!g)
        g = glist_add_int32(g, bin_mdef_pid2ssid(mdef, b));

    return g;
}


/**
 * Build a glist of triphone senone-sequence IDs (ssids) derivable from [r][b] at the word
 * end position.  If no triphone found in mdef, include the ssid for basephone b.
 * Return the generated glist.
 */
static glist_t
rdiph_comsseq(bin_mdef_t * mdef, int32 b, int32 l)
{
    int32 r, p, ssid;
    glist_t g;

    g = NULL;
    for (r = 0; r < bin_mdef_n_ciphone(mdef); r++) {
        p = bin_mdef_phone_id(mdef, (s3cipid_t) b, (s3cipid_t) l,
                          (s3cipid_t) r, WORD_POSN_END);

        if (IS_S3PID(p)) {
            gnode_t *gn;
            ssid = bin_mdef_pid2ssid(mdef, p);
            for (gn = g; gn; gn = gnode_next(gn))
                if (gnode_int32(gn) == ssid)
                    break;
            if (gn == NULL)
                g = glist_add_int32(g, ssid);
        }
    }
    if (!g)
        g = glist_add_int32(g, bin_mdef_pid2ssid(mdef, b));

    return g;
}


/**
 * Build a glist of triphone senone-sequence IDs (ssids) derivable from [b] as a single
 * phone word.  If no triphone found in mdef, include the ssid for basephone b.
 * Return the generated glist.
 */
static glist_t
single_comsseq(bin_mdef_t * mdef, int32 b)
{
    int32 l, r, p, ssid;
    glist_t g;

    g = NULL;
    for (l = 0; l < bin_mdef_n_ciphone(mdef); l++) {
        for (r = 0; r < bin_mdef_n_ciphone(mdef); r++) {
            p = bin_mdef_phone_id(mdef, (s3cipid_t) b, (s3cipid_t) l,
                              (s3cipid_t) r, WORD_POSN_SINGLE);

            if (IS_S3PID(p)) {
                gnode_t *gn;
                ssid = bin_mdef_pid2ssid(mdef, p);
                for (gn = g; gn; gn = gnode_next(gn))
                    if (gnode_int32(gn) == ssid)
                        break;
                if (gn == NULL)
                    g = glist_add_int32(g, ssid);
            }
        }
    }
    if (!g)
        g = glist_add_int32(g, bin_mdef_pid2ssid(mdef, b));

    return g;
}


/**
 * Build a glist of triphone senone-sequence IDs (ssids) derivable from [b] as a single
 * phone word, with a given left context l.  If no triphone found in mdef, include the ssid
 * for basephone b.  Return the generated glist.
 */
static glist_t
single_lc_comsseq(bin_mdef_t * mdef, int32 b, int32 l)
{
    int32 r, p, ssid;
    glist_t g;

    g = NULL;
    for (r = 0; r < bin_mdef_n_ciphone(mdef); r++) {
        p = bin_mdef_phone_id(mdef, (s3cipid_t) b, (s3cipid_t) l,
                          (s3cipid_t) r, WORD_POSN_SINGLE);

        if (IS_S3PID(p)) {
            gnode_t *gn;
            ssid = bin_mdef_pid2ssid(mdef, p);
            for (gn = g; gn; gn = gnode_next(gn))
                if (gnode_int32(gn) == ssid)
                    break;
            if (gn == NULL)
                g = glist_add_int32(g, ssid);
        }
    }
    if (!g)
        g = glist_add_int32(g, bin_mdef_pid2ssid(mdef, b));

    return g;
}

#if 0
/*Comment to make compiler happy.  Though, please make sure it is in-sync with single_lc_comsseq*/
/**
 * Build a glist of triphone senone-sequence IDs (ssids) derivable
 * from [b] as a single phone word, with a given right context r.  If
 * no triphone found in mdef, include the ssid for basephone b.
 * Return the generated glist.
 */

static glist_t
single_rc_comsseq(bin_mdef_t * mdef, int32 b, int32 r)
{
    int32 l, p, ssid;
    glist_t g;

    g = NULL;
    for (l = 0; l < bin_mdef_n_ciphone(mdef); l++) {
        p = bin_mdef_phone_id(mdef, (s3cipid_t) b, (s3cipid_t) l,
                          (s3cipid_t) r, WORD_POSN_SINGLE);

        if (IS_S3PID(p)) {
            gnode_t *gn;
            ssid = bin_mdef_pid2ssid(mdef, p);
            for (gn = g; gn; gn = gnode_next(gn))
                if (gnode_int32(gn) == ssid)
                    break;
            if (gn == NULL)
                g = glist_add_int32(g, ssid);
        }
    }
    if (!g)
        g = glist_add_int32(g, bin_mdef_pid2ssid(mdef, b));

    return g;
}
#endif


/**
 * Convert the glist of ssids to a composite sseq id.  Return the composite ID.
 */
static s3ssid_t
ssidlist2comsseq(glist_t g, bin_mdef_t * mdef, dict2pid_t * dict2pid,
                 hash_table_t * hs, /* For composite states */
                 hash_table_t * hp) /* For composite senone seq */
{                
    int32 i, j, n, s, ssid;
    s3senid_t **sen;
    s3senid_t *comsenid;
    gnode_t *gn;

    n = glist_count(g);
    if (n <= 0)
        E_FATAL("Panic: length(ssidlist)= %d\n", n);

    /* Space for list of senones for each state, derived from the given glist */
    sen =
        (s3senid_t **) ckd_calloc(bin_mdef_n_emit_state(mdef),
                                  sizeof(s3senid_t *));
    for (i = 0; i < bin_mdef_n_emit_state(mdef); i++) {
        sen[i] = (s3senid_t *) ckd_calloc(n + 1, sizeof(s3senid_t));
        sen[i][0] = BAD_S3SENID;        /* Sentinel */
    }
    /* Space for composite senone ID for each state position */
    comsenid =
        (s3senid_t *) ckd_calloc(bin_mdef_n_emit_state(mdef),
                                 sizeof(s3senid_t));

    /* Expand g into an array of arrays of unique senone IDs, one for
     * each state in the model. */
    for (gn = g; gn; gn = gnode_next(gn)) {
        ssid = gnode_int32(gn);

        /* Expand ssid into individual states (senones); insert in sen[][] if not present */
        for (i = 0; i < bin_mdef_n_emit_state(mdef); i++) {
            s = mdef->sseq[ssid][i];

            for (j = 0; (IS_S3SENID(sen[i][j])) && (sen[i][j] != s); j++);
            if (NOT_S3SENID(sen[i][j])) {
                sen[i][j] = s;
                sen[i][j + 1] = BAD_S3SENID;
            }
        }
    }

    /* Convert senones list for each state position into composite state */
    for (i = 0; i < bin_mdef_n_emit_state(mdef); i++) {
        /* Count number of unique senones for this state. */
        for (j = 0; IS_S3SENID(sen[i][j]); j++);
        assert(j > 0);

        /* Map set of senones to composite senone ID. */
        j = (long)hash_table_enter_bkey(hs, (char *) (sen[i]), j * sizeof(s3senid_t),
					(void *)(long)dict2pid->n_comstate);
        /* Did this set of senones already exist? */
        if (j == dict2pid->n_comstate)
            dict2pid->n_comstate++;     /* if not, it's a new composite senone */
        else
            ckd_free((void *) sen[i]);

        /* Composite senone ID for this state. */
        comsenid[i] = j;
    }
    ckd_free(sen);

    /* Map sequence of composite senids (one per state) to composite sseq ID */
    j = (long) hash_table_enter_bkey(hp, (char *) comsenid,
				     mdef->n_emit_state * sizeof(s3senid_t),
				     (void *)(long)dict2pid->n_comsseq);
    /* Did it already exist? */
    if (j == dict2pid->n_comsseq) {
        /* if not, it's a new composite senone sequence. */
        dict2pid->n_comsseq++;
        if (dict2pid->n_comsseq >= MAX_S3SENID)
            E_FATAL
                ("#Composite sseq limit(%d) reached; increase MAX_S3SENID\n",
                 dict2pid->n_comsseq);
    }
    else
        ckd_free((void *) comsenid);

    return ((s3ssid_t) j);
}

void
compress_table(s3ssid_t * uncomp_tab, s3ssid_t * com_tab,
               s3cipid_t * ci_map, int32 n_ci)
{
    int32 found;
    int32 r;
    int32 tmp_r;

    for (r = 0; r < n_ci; r++) {
        com_tab[r] = BAD_S3SSID;
        ci_map[r] = BAD_S3CIPID;
    }
    /** Compress this map */
    for (r = 0; r < n_ci; r++) {

        found = 0;
        for (tmp_r = 0; tmp_r < r && com_tab[tmp_r] != BAD_S3SSID; tmp_r++) {   /* If it appears before, just filled in cimap; */
            if (uncomp_tab[r] == com_tab[tmp_r]) {
                found = 1;
                ci_map[r] = tmp_r;
                break;
            }
        }

        if (found == 0) {
            com_tab[tmp_r] = uncomp_tab[r];
            ci_map[r] = tmp_r;
        }
    }
}


static void
compress_right_context_tree(bin_mdef_t * mdef, dict2pid_t * d2p)
{
    int32 n_ci;
    int32 b, l, r;
    int32 *rmap;
    s3ssid_t *tmpssid;
    s3cipid_t *tmpcimap;

    n_ci = mdef->n_ciphone;

    tmpssid = ckd_calloc(n_ci, sizeof(s3ssid_t));
    tmpcimap = ckd_calloc(n_ci, sizeof(s3cipid_t));

    assert(d2p->rdiph_rc);
    d2p->rssid =
        (xwdssid_t **) ckd_calloc(mdef->n_ciphone, sizeof(xwdssid_t *));

    for (b = 0; b < n_ci; b++) {

        d2p->rssid[b] =
            (xwdssid_t *) ckd_calloc(mdef->n_ciphone, sizeof(xwdssid_t));

        for (l = 0; l < n_ci; l++) {

            rmap = d2p->rdiph_rc[b][l];

            compress_table(rmap, tmpssid, tmpcimap, mdef->n_ciphone);

            for (r = 0; r < mdef->n_ciphone && tmpssid[r] != BAD_S3SSID;
                 r++);

            if (tmpssid[0] != BAD_S3SSID) {
                d2p->rssid[b][l].ssid = ckd_calloc(r, sizeof(s3ssid_t));
                memcpy(d2p->rssid[b][l].ssid, tmpssid,
                       r * sizeof(s3ssid_t));
                d2p->rssid[b][l].cimap =
                    ckd_calloc(mdef->n_ciphone, sizeof(s3cipid_t));
                memcpy(d2p->rssid[b][l].cimap, tmpcimap,
                       (mdef->n_ciphone) * sizeof(s3cipid_t));
                d2p->rssid[b][l].n_ssid = r;
            }
            else {
                d2p->rssid[b][l].ssid = NULL;
                d2p->rssid[b][l].cimap = NULL;
                d2p->rssid[b][l].n_ssid = 0;
            }

        }
    }

    /* Try to compress lrdiph_rc into lrdiph_rc_compressed */
    ckd_free(tmpssid);
    ckd_free(tmpcimap);


}

static void
compress_left_right_context_tree(bin_mdef_t * mdef, dict2pid_t * d2p)
{
    int32 n_ci;
    int32 b, l, r;
    int32 *rmap;
    s3ssid_t *tmpssid;
    s3cipid_t *tmpcimap;

    n_ci = mdef->n_ciphone;

    tmpssid = ckd_calloc(n_ci, sizeof(s3ssid_t));
    tmpcimap = ckd_calloc(n_ci, sizeof(s3cipid_t));

    assert(d2p->lrdiph_rc);

    d2p->lrssid =
        (xwdssid_t **) ckd_calloc(mdef->n_ciphone, sizeof(xwdssid_t *));

    for (b = 0; b < n_ci; b++) {

        d2p->lrssid[b] =
            (xwdssid_t *) ckd_calloc(mdef->n_ciphone, sizeof(xwdssid_t));

        for (l = 0; l < n_ci; l++) {
            rmap = d2p->lrdiph_rc[b][l];

            compress_table(rmap, tmpssid, tmpcimap, mdef->n_ciphone);

            for (r = 0; r < mdef->n_ciphone && tmpssid[r] != BAD_S3SSID;
                 r++);

            if (tmpssid[0] != BAD_S3SSID) {
                d2p->lrssid[b][l].ssid = ckd_calloc(r, sizeof(s3ssid_t));
                memcpy(d2p->lrssid[b][l].ssid, tmpssid,
                       r * sizeof(s3ssid_t));
                d2p->lrssid[b][l].cimap =
                    ckd_calloc(mdef->n_ciphone, sizeof(s3cipid_t));
                memcpy(d2p->lrssid[b][l].cimap, tmpcimap,
                       (mdef->n_ciphone) * sizeof(s3cipid_t));
                d2p->lrssid[b][l].n_ssid = r;
            }
            else {
                d2p->lrssid[b][l].ssid = NULL;
                d2p->lrssid[b][l].cimap = NULL;
                d2p->lrssid[b][l].n_ssid = 0;
            }
        }
    }

    /* Try to compress lrdiph_rc into lrdiph_rc_compressed */
    ckd_free(tmpssid);
    ckd_free(tmpcimap);


}

/**
   ARCHAN, A duplicate of get_rc_npid in ctxt_table.h.  I doubt whether it is correct
   because the compressed map has not been checked. 
*/
int32
get_rc_nssid(dict2pid_t * d2p, s3wid_t w, s3dict_t * dict)
{
    int32 pronlen;
    s3cipid_t b, lc;

    pronlen = dict->word[w].pronlen;
    b = dict->word[w].ciphone[pronlen - 1];

    if (pronlen == 1) {
        /* Is this true ?
           No known left context.  But all cimaps (for any l) are identical; pick one 
        */
        /*E_INFO("Single phone word\n"); */
        return (d2p->lrssid[b][0].n_ssid);
    }
    else {
        /*    E_INFO("Multiple phone word\n"); */
        lc = dict->word[w].ciphone[pronlen - 2];
        return (d2p->rssid[b][lc].n_ssid);
    }

}

s3cipid_t *
dict2pid_get_rcmap(dict2pid_t * d2p, s3wid_t w, s3dict_t * dict)
{
    int32 pronlen;
    s3cipid_t b, lc;

    pronlen = dict->word[w].pronlen;
    b = dict->word[w].ciphone[pronlen - 1];

    if (pronlen == 1) {
        /* Is this true ?
           No known left context.  But all cimaps (for any l) are identical; pick one 
        */
        /*E_INFO("Single phone word\n"); */
        return (d2p->lrssid[b][0].cimap);
    }
    else {
        /*    E_INFO("Multiple phone word\n"); */
        lc = dict->word[w].ciphone[pronlen - 2];
        return (d2p->rssid[b][lc].cimap);
    }

}




static void
free_compress_map(xwdssid_t ** tree, int32 n_ci)
{
    int32 b, l;
    for (b = 0; b < n_ci; b++) {
        for (l = 0; l < n_ci; l++) {
            ckd_free(tree[b][l].ssid);
            ckd_free(tree[b][l].cimap);
        }
        ckd_free(tree[b]);
    }
    ckd_free(tree);
}


/* RAH 4.16.01 This code has several leaks that must be fixed */
dict2pid_t *
dict2pid_build(bin_mdef_t * mdef, s3dict_t * dict, int32 is_composite, logmath_t *logmath)
{
    dict2pid_t *dict2pid;
    s3ssid_t *internal, **ldiph, **rdiph, *single;
    int32 pronlen;
    hash_table_t *hs, *hp;
    glist_t g;
    gnode_t *gn;
    s3senid_t *sen;
    hash_entry_t *he;
    int32 *cslen;
    int32 i, j, b, l, r, w, n, p;

    E_INFO("Building PID tables for dictionary\n");
    assert(mdef);
    assert(dict);


    dict2pid = (dict2pid_t *) ckd_calloc(1, sizeof(dict2pid_t));

    dict2pid->n_dictsize = s3dict_size(dict);
    dict2pid->internal =
        (s3ssid_t **) ckd_calloc(s3dict_size(dict), sizeof(s3ssid_t *));
    dict2pid->ldiph_lc =
        (s3ssid_t ***) ckd_calloc_3d(mdef->n_ciphone, mdef->n_ciphone,
                                     mdef->n_ciphone, sizeof(s3ssid_t));
    dict2pid->rdiph_rc =
        (s3ssid_t ***) ckd_calloc_3d(mdef->n_ciphone, mdef->n_ciphone,
                                     mdef->n_ciphone, sizeof(s3ssid_t));
    dict2pid->is_composite = is_composite;

    dict2pid->n_ci = mdef->n_ciphone;
    if (dict2pid->is_composite) {
        dict2pid->single_lc = (s3ssid_t **) ckd_calloc_2d(mdef->n_ciphone,
                                                          mdef->n_ciphone,
                                                          sizeof
                                                          (s3ssid_t));
        dict2pid->lrdiph_rc = NULL;
        dict2pid->rssid = NULL;
        dict2pid->lrssid = NULL;

    }
    else {

        dict2pid->lrdiph_rc = (s3ssid_t ***) ckd_calloc_3d(mdef->n_ciphone,
                                                           mdef->n_ciphone,
                                                           mdef->n_ciphone,
                                                           sizeof
                                                           (s3ssid_t));
        dict2pid->single_lc = NULL;


    }

    dict2pid->comstate = NULL;
    dict2pid->comsseq = NULL;
    dict2pid->comwt = NULL;

    dict2pid->n_comstate = 0;
    dict2pid->n_comsseq = 0;
    dict2pid->is_composite = is_composite;

    hs = hash_table_new(mdef->n_ciphone * mdef->n_ciphone * mdef->n_emit_state,
			HASH_CASE_YES);
    hp = hash_table_new(mdef->n_ciphone * mdef->n_ciphone, HASH_CASE_YES);

    for (w = 0, n = 0; w < s3dict_size(dict); w++) {
        pronlen = s3dict_pronlen(dict, w);
        if (pronlen < 0)
            E_FATAL("Pronunciation-length(%s)= %d\n",
                    s3dict_wordstr(dict, w), pronlen);
        n += pronlen;
    }

    internal = (s3ssid_t *) ckd_calloc(n, sizeof(s3ssid_t));


    /* Temporary */
    ldiph =
        (s3ssid_t **) ckd_calloc_2d(mdef->n_ciphone, mdef->n_ciphone,
                                    sizeof(s3ssid_t));
    rdiph =
        (s3ssid_t **) ckd_calloc_2d(mdef->n_ciphone, mdef->n_ciphone,
                                    sizeof(s3ssid_t));
    single = (s3ssid_t *) ckd_calloc(mdef->n_ciphone, sizeof(s3ssid_t));
    for (b = 0; b < mdef->n_ciphone; b++) {
        for (l = 0; l < mdef->n_ciphone; l++) {
            for (r = 0; r < mdef->n_ciphone; r++) {
                dict2pid->ldiph_lc[b][r][l] = BAD_S3SSID;
                dict2pid->rdiph_rc[b][l][r] = BAD_S3SSID;
            }

            if (dict2pid->is_composite) {
                assert(dict2pid->single_lc);
                dict2pid->single_lc[b][l] = BAD_S3SSID;
            }

            ldiph[b][l] = BAD_S3SSID;
            rdiph[b][l] = BAD_S3SSID;
        }
        single[b] = BAD_S3SSID;
    }

    for (w = 0; w < s3dict_size(dict); w++) {
        dict2pid->internal[w] = internal;
        pronlen = s3dict_pronlen(dict, w);

        if (pronlen >= 2) {

            /** This segments of code take care of the initialization of 
                internal[0] and ldiph[b][r][l]
	    */

            /* Find or create a composite senone sequence for b(?,r) */
            b = s3dict_pron(dict, w, 0);
            r = s3dict_pron(dict, w, 1);
            if (NOT_S3SSID(ldiph[b][r])) {

                if (dict2pid->is_composite) {
                    /* Get all ssids for b(?,r) */
                    g = ldiph_comsseq(mdef, b, r);
                    /* Build a composite sseq from those ssids */
                    ldiph[b][r] =
                        ssidlist2comsseq(g, mdef, dict2pid, hs, hp);
                    glist_free(g);
                }

                /* Record all possible ssids for b(?,r) */
                for (l = 0; l < bin_mdef_n_ciphone(mdef); l++) {
                    p = bin_mdef_phone_id_nearest(mdef, (s3cipid_t) b,
                                              (s3cipid_t) l, (s3cipid_t) r,
                                              WORD_POSN_BEGIN);
                    dict2pid->ldiph_lc[b][r][l] = bin_mdef_pid2ssid(mdef, p);
                }
            }

            /* And ... only use it if we are not doing full triphones. (?!) */
            if (dict2pid->is_composite)
                internal[0] = ldiph[b][r];
            else
                internal[0] = BAD_S3SSID;

            /* Now find ssids for all the word internal triphones and
             * place them in internal[i].  */
            for (i = 1; i < pronlen - 1; i++) {
                l = b;
                b = r;
                r = s3dict_pron(dict, w, i + 1);

                p = bin_mdef_phone_id_nearest(mdef, (s3cipid_t) b,
                                          (s3cipid_t) l, (s3cipid_t) r,
                                          WORD_POSN_INTERNAL);
                internal[i] = bin_mdef_pid2ssid(mdef, p);
            }

            /** This part will take care of the initialization of 
		internal[pronlen-1] and rdiph[b][l][r]. Notice that this 
		is symmetric to the first part of the code. 
            */

            l = b;
            b = r;
            if (NOT_S3SSID(rdiph[b][l])) {
                if (dict2pid->is_composite) {
                    g = rdiph_comsseq(mdef, b, l);
                    rdiph[b][l] =
                        ssidlist2comsseq(g, mdef, dict2pid, hs, hp);
                    glist_free(g);
                }

                for (r = 0; r < bin_mdef_n_ciphone(mdef); r++) {
                    p = bin_mdef_phone_id_nearest(mdef, (s3cipid_t) b,
                                              (s3cipid_t) l, (s3cipid_t) r,
                                              WORD_POSN_BEGIN);
                    dict2pid->rdiph_rc[b][l][r] = bin_mdef_pid2ssid(mdef, p);
                }
            }

            if (dict2pid->is_composite)
                internal[pronlen - 1] = rdiph[b][l];
            else
                internal[pronlen - 1] = BAD_S3SSID;

        }
        else if (pronlen == 1) {

            b = s3dict_pron(dict, w, 0);

            if (dict2pid->is_composite) {
                assert(dict2pid->single_lc);

                /* Find or build composite senone sequence for b(?,?) */
                if (NOT_S3SSID(single[b])) {

                    g = single_comsseq(mdef, b);
                    single[b] =
                        ssidlist2comsseq(g, mdef, dict2pid, hs, hp);
                    glist_free(g);

                    /* Record all possible *composite* ssids for b(?,?) */
                    for (l = 0; l < bin_mdef_n_ciphone(mdef); l++) {
                        g = single_lc_comsseq(mdef, b, l);
                        dict2pid->single_lc[b][l] =
                            ssidlist2comsseq(g, mdef, dict2pid, hs, hp);
                        glist_free(g);
                    }
                }
                internal[0] = single[b];
            }
            else {
                /* Don't compress but build table directly */
                if (NOT_S3SSID(single[b])) {
                    for (l = 0; l < bin_mdef_n_ciphone(mdef); l++) {
                        for (r = 0; r < bin_mdef_n_ciphone(mdef); r++) {
                            p = bin_mdef_phone_id_nearest(mdef, (s3cipid_t) b,
                                                      (s3cipid_t) l,
                                                      (s3cipid_t) r,
                                                      WORD_POSN_SINGLE);
                            dict2pid->lrdiph_rc[b][l][r] =
                                bin_mdef_pid2ssid(mdef, p);
                        }
                    }
                }
                internal[pronlen - 1] = BAD_S3SSID;
            }

        }
        else {
            E_FATAL("panic: pronlen=0, what's going on?\n");
        }

        if (!dict2pid->is_composite) {
            /*      E_INFO("internal[0] %d, internal[pronlen-1] %d\n", internal[0],internal[pronlen-1]); */
            assert(internal[0] == BAD_S3SSID
                   && internal[pronlen - 1] == BAD_S3SSID);
        }

        internal += pronlen;
    }

    ckd_free_2d((void **) ldiph);
    ckd_free_2d((void **) rdiph);
    ckd_free((void *) single);

    if (dict2pid->is_composite) {
        /* Count the length of each composite state (i.e. how many
         * actual senones it maps to). */
        /* n_comstate will have been set through calls to ssidlist2comsseq(). */
        cslen = (int32 *) ckd_calloc(dict2pid->n_comstate, sizeof(int32));
        /* as will the entries of hs. */
        g = hash_table_tolist(hs, &n);
        assert(n == dict2pid->n_comstate);
        n = 0;
        /* Iterate over entries of hs to figure out how much to allocate. */
        for (gn = g; gn; gn = gnode_next(gn)) {
            he = (hash_entry_t *) gnode_ptr(gn);
            /* Key is a set of actual senone IDs. */
            sen = (s3senid_t *) hash_entry_key(he);
            for (i = 0; IS_S3SENID(sen[i]); i++);

            /* Value is the composite state ID. */
            cslen[(long)hash_entry_val(he)] = i + 1;  /* +1 for terminating sentinel */

            n += (i + 1);
        }
        /* Allocate the composite state to senone list table. */
        dict2pid->comstate =
            (s3senid_t **) ckd_calloc(dict2pid->n_comstate,
                                      sizeof(s3senid_t *));
        sen = (s3senid_t *) ckd_calloc(n, sizeof(s3senid_t));
        for (i = 0; i < dict2pid->n_comstate; i++) {
            dict2pid->comstate[i] = sen;
            sen += cslen[i];
        }

        /* Build the composite state to senone list table from hs. */
        for (gn = g; gn; gn = gnode_next(gn)) {
            he = (hash_entry_t *) gnode_ptr(gn);
            sen = (s3senid_t *) hash_entry_key(he);
            i = (long)hash_entry_val(he);

            for (j = 0; j < cslen[i]; j++)
                dict2pid->comstate[i][j] = sen[j];
            assert(sen[j - 1] == BAD_S3SENID);

            ckd_free((void *) sen);
            sen = NULL;
        }
        ckd_free(cslen);
        glist_free(g);

        /* Allocate space for composite sseq table */
        /* n_comsseq will have been set through calls to ssidlist2comsseq(). */
        dict2pid->comsseq =
            (s3senid_t **) ckd_calloc(dict2pid->n_comsseq,
                                      sizeof(s3senid_t *));

        for (i = 0; i < dict2pid->n_comsseq; i++) {
            dict2pid->comsseq[i] = NULL;
        }

        /* as will the entries of hp. */
        g = hash_table_tolist(hp, &n);
        assert(n == dict2pid->n_comsseq);

        /* Build composite sseq table by iterating over hp. */
        for (gn = g; gn; gn = gnode_next(gn)) {
            he = (hash_entry_t *) gnode_ptr(gn);
            /* Value: composite ssid */
            i = (long)hash_entry_val(he);
            /* Key: array of composite state IDs. */
            dict2pid->comsseq[i] = (s3senid_t *) hash_entry_key(he);
        }
        glist_free(g);

        /* Weight for each composite state. */
        /* These are weighted inversely to the number of normal
         * senones which make them up.  I'm guessing that the
         * reasoning behind this is that the more different senones
         * combined into a single composite score, the less relevant
         * that score will be. */
        dict2pid->comwt =
            (int32 *) ckd_calloc(dict2pid->n_comstate, sizeof(int32));
        for (i = 0; i < dict2pid->n_comstate; i++) {
            sen = dict2pid->comstate[i];

            for (j = 0; IS_S3SENID(sen[j]); j++);
            /* if comstate i has N states, its weight= 1/N */
            dict2pid->comwt[i] = -logmath_log(logmath, (float64) j);
        }
    }

    if (!(dict2pid->is_composite)) {
        assert(dict2pid->comstate == NULL);
        assert(dict2pid->comsseq == NULL);
        assert(dict2pid->comwt == NULL);
        assert(dict2pid->single_lc == NULL);
        assert(dict2pid->n_comstate == 0);
        assert(dict2pid->n_comsseq == 0);

        /* Try to compress rdiph_rc into rdiph_rc_compressed
           This should be moved to a function.
        */

        compress_right_context_tree(mdef, dict2pid);
        compress_left_right_context_tree(mdef, dict2pid);

    }
    else {
        assert(dict2pid->rssid == NULL);
        assert(dict2pid->lrssid == NULL);
    }

    hash_table_free(hs);
    hash_table_free(hp);

    return dict2pid;
}

void
dict2pid_free(dict2pid_t * d2p)
{
    int32 i;

    if (d2p) {
        if (d2p->comwt)
            ckd_free((void *) d2p->comwt);
        if (d2p->comsseq) {

            for (i = 0; i < d2p->n_comsseq; i++) {
                if (d2p->comsseq[i] != NULL) {
                    ckd_free((void *) d2p->comsseq[i]);
                }
            }
            ckd_free((void *) d2p->comsseq);
        }

        if (d2p->comstate) {
            ckd_free((void **) d2p->comstate[0]);
            ckd_free((void **) d2p->comstate);
        }

        if (d2p->single_lc)
            ckd_free_2d((void *) d2p->single_lc);

        if (d2p->ldiph_lc)
            ckd_free_3d((void ***) d2p->ldiph_lc);


        if (d2p->rdiph_rc)
            ckd_free_3d((void ***) d2p->rdiph_rc);

        if (d2p->lrdiph_rc)
            ckd_free_3d((void ***) d2p->lrdiph_rc);

        if (d2p->internal) {
            ckd_free((void *) d2p->internal[0]);
            ckd_free((void **) d2p->internal);
        }

        if (d2p->rssid)
            free_compress_map(d2p->rssid, d2p->n_ci);

        if (d2p->lrssid)
            free_compress_map(d2p->lrssid, d2p->n_ci);

        ckd_free(d2p);
    }

}



void
dict2pid_report(dict2pid_t * d2p)
{
    E_INFO_NOFN("Initialization of dict2pid_t, report:\n");
    if (d2p->is_composite) {
        E_INFO_NOFN("Dict2pid is in composite triphone mode\n");
        E_INFO_NOFN("%d composite states; %d composite sseq\n",
                    d2p->n_comstate, d2p->n_comsseq);
    }
    else {
        E_INFO_NOFN("Dict2pid is in normal triphone mode\n");
    }
    E_INFO_NOFN("\n");


}

/**
 * Populate composite senone score array.
 *
 * The composite senone score is the maximum of its component senones'
 * scores, scaled down by the number of component senones.
 */
void
dict2pid_comsenscr(dict2pid_t * d2p, int32 * senscr, int32 * comsenscr)
{
    int32 i, j;
    int32 best;
    s3senid_t *comstate, k;

    for (i = 0; i < d2p->n_comstate; i++) {
        comstate = d2p->comstate[i];

        best = senscr[comstate[0]];
        for (j = 1;; j++) {
            k = comstate[j];
            if (NOT_S3SENID(k))
                break;
            if (best < senscr[k])
                best = senscr[k];
        }

        comsenscr[i] = best + d2p->comwt[i];
    }
}

/**
 * Mark senones active based on a set of active composite senones.
 */
void
dict2pid_comsseq2sen_active(dict2pid_t * d2p, bin_mdef_t * mdef,
                            bitvec_t * comssid, bitvec_t * sen)
{
    int32 ss, cs, i, j;
    s3senid_t *csp, *sp;        /* Composite state pointer */

    for (ss = 0; ss < d2p->n_comsseq; ss++) {
        if (bitvec_is_set(comssid,ss)) {
            csp = d2p->comsseq[ss];

            for (i = 0; i < bin_mdef_n_emit_state(mdef); i++) {
                cs = csp[i];
                sp = d2p->comstate[cs];

                for (j = 0; IS_S3SENID(sp[j]); j++)
                    bitvec_set(sen, sp[j]);
            }
        }
    }
}


void
dict2pid_dump(FILE * fp, dict2pid_t * d2p, bin_mdef_t * mdef, s3dict_t * dict)
{
    int32 w, p, pronlen;
    int32 i, j, b, l, r;

    fprintf(fp, "# INTERNAL (wd comssid ssid ssid ... ssid comssid)\n");
    for (w = 0; w < s3dict_size(dict); w++) {
        fprintf(fp, "%30s ", s3dict_wordstr(dict, w));

        pronlen = s3dict_pronlen(dict, w);
        for (p = 0; p < pronlen; p++)
            fprintf(fp, " %5d", d2p->internal[w][p]);
        fprintf(fp, "\n");
    }
    fprintf(fp, "#\n");

    fprintf(fp, "# LDIPH_LC (b r l ssid)\n");
    for (b = 0; b < bin_mdef_n_ciphone(mdef); b++) {
        for (r = 0; r < bin_mdef_n_ciphone(mdef); r++) {
            for (l = 0; l < bin_mdef_n_ciphone(mdef); l++) {
                if (IS_S3SSID(d2p->ldiph_lc[b][r][l]))
                    fprintf(fp, "%6s %6s %6s %5d\n", bin_mdef_ciphone_str(mdef, (s3cipid_t) b), bin_mdef_ciphone_str(mdef, (s3cipid_t) r), bin_mdef_ciphone_str(mdef, (s3cipid_t) l), d2p->ldiph_lc[b][r][l]);      /* RAH, ldiph_lc is returning an int32, %d expects an int16 */
            }
        }
    }
    fprintf(fp, "#\n");

    fprintf(fp, "# SINGLE_LC (b l comssid)\n");
    for (b = 0; b < bin_mdef_n_ciphone(mdef); b++) {
        for (l = 0; l < bin_mdef_n_ciphone(mdef); l++) {
            if (IS_S3SSID(d2p->single_lc[b][l]))
                fprintf(fp, "%6s %6s %5d\n", bin_mdef_ciphone_str(mdef, (s3cipid_t) b), bin_mdef_ciphone_str(mdef, (s3cipid_t) l), d2p->single_lc[b][l]);       /* RAH, single_lc is returning an int32, %d expects an int16 */
        }
    }
    fprintf(fp, "#\n");

    fprintf(fp, "# SSEQ %d (senid senid ...)\n", mdef->n_sseq);
    for (i = 0; i < mdef->n_sseq; i++) {
        fprintf(fp, "%5d ", i);
        for (j = 0; j < bin_mdef_n_emit_state(mdef); j++)
            fprintf(fp, " %5d", mdef->sseq[i][j]);
        fprintf(fp, "\n");
    }
    fprintf(fp, "#\n");

    fprintf(fp, "# COMSSEQ %d (comstate comstate ...)\n", d2p->n_comsseq);
    for (i = 0; i < d2p->n_comsseq; i++) {
        fprintf(fp, "%5d ", i);
        for (j = 0; j < bin_mdef_n_emit_state(mdef); j++)
            fprintf(fp, " %5d", d2p->comsseq[i][j]);
        fprintf(fp, "\n");
    }
    fprintf(fp, "#\n");

    fprintf(fp, "# COMSTATE %d (senid senid ...)\n", d2p->n_comstate);
    for (i = 0; i < d2p->n_comstate; i++) {
        fprintf(fp, "%5d ", i);
        for (j = 0; IS_S3SENID(d2p->comstate[i][j]); j++)
            fprintf(fp, " %5d", d2p->comstate[i][j]);
        fprintf(fp, "\n");
    }
    fprintf(fp, "#\n");
    fprintf(fp, "# END\n");

    fflush(fp);
}


#if 0
for (r = 0; r < mdef->n_ciphone; r++) {
    printf("%d ", rmap[r]);
}

printf("\n");
fflush(stdout);

for (r = 0; r < mdef->n_ciphone; r++) {
    printf("%d ", tmpssid[r]);
}

printf("\n");
fflush(stdout);
for (r = 0; r < mdef->n_ciphone; r++) {
    printf("%d ", tmpcimap[r]);
}

printf("\n");
fflush(stdout);

for (r = 0; r < dict2pid->rssid[b][l].n_ssid; r++) {
    printf("%d ", dict2pid->rssid[b][l].ssid[r]);
}

printf("\n");
fflush(stdout);

if (dict2pid->rssid[b][l].n_ssid > 0) {
    for (r = 0; r < mdef->n_ciphone; r++) {
        printf("%d ", dict2pid->rssid[b][l].cimap[r]);
    }
}
printf("\n");

fflush(stdout);
#endif
