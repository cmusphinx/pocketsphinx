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

#include <string.h>

#include "dict2pid.h"
#include "hmm.h"


/**
 * @file dict2pid.c - dictionary word to senone sequence mappings
 */

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
compress_right_context_tree(bin_mdef_t * mdef, dict2pid_t * d2p,
                            s3ssid_t ***rdiph_rc)
{
    int32 n_ci;
    int32 b, l, r;
    s3ssid_t *rmap;
    s3ssid_t *tmpssid;
    s3cipid_t *tmpcimap;
    size_t alloc;

    n_ci = mdef->n_ciphone;

    tmpssid = ckd_calloc(n_ci, sizeof(s3ssid_t));
    tmpcimap = ckd_calloc(n_ci, sizeof(s3cipid_t));

    d2p->rssid =
        (xwdssid_t **) ckd_calloc(mdef->n_ciphone, sizeof(xwdssid_t *));
    alloc = mdef->n_ciphone * sizeof(xwdssid_t *);

    for (b = 0; b < n_ci; b++) {
        d2p->rssid[b] =
            (xwdssid_t *) ckd_calloc(mdef->n_ciphone, sizeof(xwdssid_t));
        alloc += mdef->n_ciphone * sizeof(xwdssid_t);

        for (l = 0; l < n_ci; l++) {
            rmap = rdiph_rc[b][l];
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

    E_INFO("Allocated %d bytes (%d KiB) for word-final triphones\n",
           (int)alloc, (int)alloc / 1024);
    ckd_free(tmpssid);
    ckd_free(tmpcimap);
}

static void
compress_left_right_context_tree(bin_mdef_t * mdef, dict2pid_t * d2p)
{
    int32 n_ci;
    int32 b, l, r;
    s3ssid_t *rmap;
    s3ssid_t *tmpssid;
    s3cipid_t *tmpcimap;
    size_t alloc;

    n_ci = mdef->n_ciphone;

    tmpssid = ckd_calloc(n_ci, sizeof(s3ssid_t));
    tmpcimap = ckd_calloc(n_ci, sizeof(s3cipid_t));

    assert(d2p->lrdiph_rc);

    d2p->lrssid =
        (xwdssid_t **) ckd_calloc(mdef->n_ciphone, sizeof(xwdssid_t *));
    alloc = mdef->n_ciphone * sizeof(xwdssid_t *);

    for (b = 0; b < n_ci; b++) {

        d2p->lrssid[b] =
            (xwdssid_t *) ckd_calloc(mdef->n_ciphone, sizeof(xwdssid_t));
        alloc += mdef->n_ciphone * sizeof(xwdssid_t);

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

    E_INFO("Allocated %d bytes (%d KiB) for single-phone word triphones\n",
           (int)alloc, (int)alloc / 1024);
}

/**
   ARCHAN, A duplicate of get_rc_npid in ctxt_table.h.  I doubt whether it is correct
   because the compressed map has not been checked. 
*/
int32
get_rc_nssid(dict2pid_t * d2p, s3wid_t w, dict_t * dict)
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
dict2pid_get_rcmap(dict2pid_t * d2p, s3wid_t w, dict_t * dict)
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

static void
populate_lrdiph(dict2pid_t *d2p, bin_mdef_t *mdef, s3ssid_t ***rdiph_rc, s3cipid_t b)
{
    s3cipid_t l, r;

    for (l = 0; l < bin_mdef_n_ciphone(mdef); l++) {
        for (r = 0; r < bin_mdef_n_ciphone(mdef); r++) {
            s3pid_t p;
            p = bin_mdef_phone_id_nearest(mdef, (s3cipid_t) b,
                                          (s3cipid_t) l,
                                          (s3cipid_t) r,
                                          WORD_POSN_SINGLE);
            d2p->lrdiph_rc[b][l][r]
                = bin_mdef_pid2ssid(mdef, p);
            if (r == bin_mdef_silphone(mdef))
                d2p->ldiph_lc[b][r][l]
                    = bin_mdef_pid2ssid(mdef, p);
            if (rdiph_rc && l == bin_mdef_silphone(mdef))
                rdiph_rc[b][l][r]
                    = bin_mdef_pid2ssid(mdef, p);
            assert(IS_S3SSID(bin_mdef_pid2ssid(mdef, p)));
            E_DEBUG(2,("%s(%s,%s) => %d / %d\n",
                       bin_mdef_ciphone_str(mdef, b),
                       bin_mdef_ciphone_str(mdef, l),
                       bin_mdef_ciphone_str(mdef, r),
                       p, bin_mdef_pid2ssid(mdef, p)));
        }
    }
}

int
dict2pid_add_word(dict2pid_t *d2p,
                  bin_mdef_t *mdef,
                  dict_t *d,
                  int32 wid)
{
    if (dict_pronlen(d,wid) > 2) {
        /* Calculate and store internal ssids (FIXME: actually the
         * internal array should go away). */
        if (wid >= d2p->n_dictsize) {
            s3ssid_t *tmp = d2p->internal[0];
            int i;

            /* Expand internal phone array (FIXME... need to something about it) */
            d2p->n_dictsize = wid+1;
            d2p->internal = ckd_realloc(d2p->internal,
                                        d2p->n_dictsize
                                        * sizeof(*d2p->internal));
            /* Calculate offset from old to new data array. */
            tmp = d2p->internal[0];
            /* Reallocate and adjust pointers in internal array. */
            d2p->internal[0] = ckd_realloc(d2p->internal[0],
                                           (d2p->n_internal + dict_pronlen(d, wid) - 2)
                                           * sizeof(*d2p->internal[0]));
            for (i = 1; i < d2p->n_dictsize; ++i) {
                d2p->internal[i] += (d2p->internal[0] - tmp);
            }
            d2p->internal[wid] = d2p->internal[0] + d2p->n_internal;
            d2p->n_internal += (dict_pronlen(d, wid) - 2);
            for (i = 1; i < dict_pronlen(d, wid) - 1; ++i) {
                int32 p = bin_mdef_phone_id_nearest(mdef,
                                                    dict_pron(d, wid, i),
                                                    dict_pron(d, wid, i-1),
                                                    dict_pron(d, wid, i+1),
                                                    WORD_POSN_INTERNAL);
                d2p->internal[wid][i-1] = bin_mdef_pid2ssid(mdef, p);
            }
        }
    }
    if (dict_pronlen(d, wid) > 1) {
        s3cipid_t l;
        /* Make sure we have left and right context diphones for this
         * word. */
        if (d2p->ldiph_lc[dict_first_phone(d, wid)][dict_second_phone(d, wid)][0]
            == BAD_S3SSID) {
            E_INFO("Filling in left-context diphones for %s(?,%s)\n",
                   bin_mdef_ciphone_str(mdef, dict_first_phone(d, wid)),
                   bin_mdef_ciphone_str(mdef, dict_second_phone(d, wid)));
            for (l = 0; l < bin_mdef_n_ciphone(mdef); l++) {
                s3ssid_t p
                    = bin_mdef_phone_id_nearest(mdef,
                                                dict_first_phone(d, wid), l,
                                                dict_second_phone(d, wid),
                                                WORD_POSN_BEGIN);
                d2p->ldiph_lc[dict_first_phone(d, wid)][dict_second_phone(d, wid)][l]
                    = bin_mdef_pid2ssid(mdef, p);
            }
        }
        if (d2p->rssid[dict_last_phone(d, wid)][dict_second_last_phone(d, wid)].n_ssid
            == 0) {
            s3ssid_t *rmap;
            s3ssid_t *tmpssid;
            s3cipid_t *tmpcimap;
            s3cipid_t r;

            E_INFO("Filling in right-context diphones for %s(%s,?)\n",
                   bin_mdef_ciphone_str(mdef, dict_last_phone(d, wid)),
                   bin_mdef_ciphone_str(mdef, dict_second_last_phone(d, wid)));
            rmap = ckd_calloc(bin_mdef_n_ciphone(mdef), sizeof(*rmap));
            for (r = 0; r < bin_mdef_n_ciphone(mdef); r++) {
                s3ssid_t p
                    = bin_mdef_phone_id_nearest(mdef,
                                                dict_last_phone(d, wid),
                                                dict_second_last_phone(d, wid), r,
                                                WORD_POSN_END);
                rmap[r] = bin_mdef_pid2ssid(mdef, p);
            }
            tmpssid = ckd_calloc(bin_mdef_n_ciphone(mdef), sizeof(*tmpssid));
            tmpcimap = ckd_calloc(bin_mdef_n_ciphone(mdef), sizeof(*tmpcimap));
            compress_table(rmap, tmpssid, tmpcimap, bin_mdef_n_ciphone(mdef));
            for (r = 0; r < mdef->n_ciphone && tmpssid[r] != BAD_S3SSID; r++)
                ;
            d2p->rssid[dict_last_phone(d, wid)][dict_second_last_phone(d, wid)].ssid = tmpssid;
            d2p->rssid[dict_last_phone(d, wid)][dict_second_last_phone(d, wid)].cimap = tmpcimap;
            d2p->rssid[dict_last_phone(d, wid)][dict_second_last_phone(d, wid)].n_ssid = r;
            ckd_free(rmap);
        }
    }
    else {
        /* Make sure we have a left-right context triphone entry for
         * this word. */
        E_INFO("Filling in context triphones for %s(?,?)\n",
               bin_mdef_ciphone_str(mdef, dict_first_phone(d, wid)));
        if (d2p->lrdiph_rc[dict_first_phone(d, wid)][0][0] == BAD_S3SSID) {
            populate_lrdiph(d2p, mdef, NULL, dict_first_phone(d, wid));
        }
    }

    return 0;
}

dict2pid_t *
dict2pid_build(bin_mdef_t * mdef, dict_t * dict)
{
    dict2pid_t *dict2pid;
    s3ssid_t *internal, ***rdiph_rc;
    bitvec_t *ldiph, *rdiph, *single;
    int32 pronlen;
    int32 i, b, l, r, w, p;

    E_INFO("Building PID tables for dictionary\n");
    assert(mdef);
    assert(dict);

    dict2pid = (dict2pid_t *) ckd_calloc(1, sizeof(dict2pid_t));

    dict2pid->n_dictsize = dict_size(dict);
    E_INFO("Allocating %d * %d bytes (%d KiB) for word-internal arrays\n",
           dict2pid->n_dictsize, sizeof(s3ssid_t *),
           dict2pid->n_dictsize * sizeof(s3ssid_t *) / 1024);
    dict2pid->internal =
        (s3ssid_t **) ckd_calloc(dict2pid->n_dictsize, sizeof(s3ssid_t *));
    E_INFO("Allocating %d^3 * %d bytes (%d KiB) for word-initial triphones\n",
           mdef->n_ciphone, sizeof(s3ssid_t),
           mdef->n_ciphone * mdef->n_ciphone * mdef->n_ciphone * sizeof(s3ssid_t) / 1024);
    dict2pid->ldiph_lc =
        (s3ssid_t ***) ckd_calloc_3d(mdef->n_ciphone, mdef->n_ciphone,
                                     mdef->n_ciphone, sizeof(s3ssid_t));
    /* Only used internally to generate rssid */
    rdiph_rc =
        (s3ssid_t ***) ckd_calloc_3d(mdef->n_ciphone, mdef->n_ciphone,
                                     mdef->n_ciphone, sizeof(s3ssid_t));

    dict2pid->n_ci = mdef->n_ciphone;
    dict2pid->lrdiph_rc = (s3ssid_t ***) ckd_calloc_3d(mdef->n_ciphone,
                                                       mdef->n_ciphone,
                                                       mdef->n_ciphone,
                                                       sizeof
                                                       (s3ssid_t));
    /* Actually could use memset for this, if BAD_S3SSID is guaranteed
     * to be 65535... */
    for (b = 0; b < mdef->n_ciphone; ++b) {
        for (r = 0; r < mdef->n_ciphone; ++r) {
            for (l = 0; l < mdef->n_ciphone; ++l) {
                dict2pid->ldiph_lc[b][r][l] = BAD_S3SSID;
                dict2pid->lrdiph_rc[b][l][r] = BAD_S3SSID;
                rdiph_rc[b][l][r] = BAD_S3SSID;
            }
        }
    }

    /* Count number of word-internal phones. */
    dict2pid->n_internal = 0;
    for (w = 0; w < dict2pid->n_dictsize; w++) {
        pronlen = dict_pronlen(dict, w);
        if (pronlen < 0)
            E_FATAL("Pronunciation-length(%s)= %d\n",
                    dict_wordstr(dict, w), pronlen);
        dict2pid->n_internal += (pronlen == 1 ? 0 : pronlen - 2);
    }

    E_INFO("Allocating %d entries of %d bytes (%d KiB) for internal ssids\n",
           dict2pid->n_internal, sizeof(s3ssid_t),
           dict2pid->n_internal * sizeof(s3ssid_t) / 1024);
    internal = (s3ssid_t *) ckd_calloc(dict2pid->n_internal, sizeof(s3ssid_t));

    /* Track which diphones / ciphones have been seen. */
    ldiph = bitvec_alloc(mdef->n_ciphone * mdef->n_ciphone);
    rdiph = bitvec_alloc(mdef->n_ciphone * mdef->n_ciphone);
    single = bitvec_alloc(mdef->n_ciphone);

    for (w = 0; w < dict2pid->n_dictsize; w++) {
        dict2pid->internal[w] = internal;
        pronlen = dict_pronlen(dict, w);

        if (pronlen >= 2) {
            b = dict_pron(dict, w, 0);
            r = dict_pron(dict, w, 1);
            /* Populate ldiph_lc */
            if (bitvec_is_clear(ldiph, b * mdef->n_ciphone + r)) {
                /* Mark this diphone as done */
                bitvec_set(ldiph, b * mdef->n_ciphone + r);

                /* Record all possible ssids for b(?,r) */
                for (l = 0; l < bin_mdef_n_ciphone(mdef); l++) {
                    p = bin_mdef_phone_id_nearest(mdef, (s3cipid_t) b,
                                              (s3cipid_t) l, (s3cipid_t) r,
                                              WORD_POSN_BEGIN);
                    dict2pid->ldiph_lc[b][r][l] = bin_mdef_pid2ssid(mdef, p);
                }
            }

            /* Now find ssids for all the word internal triphones and
             * place them in internal[i].  FIXME: This is so totally
             * not necessary and a big waste of memory. */
            for (i = 1; i < pronlen - 1; i++) {
                l = b;
                b = r;
                r = dict_pron(dict, w, i + 1);

                p = bin_mdef_phone_id_nearest(mdef, (s3cipid_t) b,
                                          (s3cipid_t) l, (s3cipid_t) r,
                                          WORD_POSN_INTERNAL);
                internal[i-1] = bin_mdef_pid2ssid(mdef, p);
            }

            /* Populate rdiph_rc */
            l = b;
            b = r;
            if (bitvec_is_clear(rdiph, b * mdef->n_ciphone + l)) {
                /* Mark this diphone as done */
                bitvec_set(rdiph, b * mdef->n_ciphone + l);

                for (r = 0; r < bin_mdef_n_ciphone(mdef); r++) {
                    p = bin_mdef_phone_id_nearest(mdef, (s3cipid_t) b,
                                              (s3cipid_t) l, (s3cipid_t) r,
                                              WORD_POSN_END);
                    rdiph_rc[b][l][r] = bin_mdef_pid2ssid(mdef, p);
                }
            }
        }
        else if (pronlen == 1) {
            b = dict_pron(dict, w, 0);
            E_DEBUG(1,("Building tables for single phone word %s phone %d = %s\n",
                       dict_wordstr(dict, w), b, bin_mdef_ciphone_str(mdef, b)));
            /* Populate lrdiph_rc (and also ldiph_lc, rdiph_rc if needed) */
            if (bitvec_is_clear(single, b)) {
                populate_lrdiph(dict2pid, mdef, rdiph_rc, b);
                bitvec_set(single, b);
            }
        }
        internal += (pronlen == 1 ? 0 : pronlen - 2);
    }

    bitvec_free(ldiph);
    bitvec_free(rdiph);
    bitvec_free(single);

    /* Try to compress rdiph_rc into rdiph_rc_compressed */
    compress_right_context_tree(mdef, dict2pid, rdiph_rc);
    compress_left_right_context_tree(mdef, dict2pid);

    ckd_free_3d(rdiph_rc);

    dict2pid_report(dict2pid);
    return dict2pid;
}

dict2pid_t *
dict2pid_retain(dict2pid_t *d2p)
{
    ++d2p->refcount;
    return d2p;
}

int
dict2pid_free(dict2pid_t * d2p)
{
    if (d2p == NULL)
        return 0;
    if (--d2p->refcount > 0)
        return d2p->refcount;

    if (d2p->ldiph_lc)
        ckd_free_3d((void ***) d2p->ldiph_lc);

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
    return 0;
}

void
dict2pid_report(dict2pid_t * d2p)
{
}

void
dict2pid_dump(FILE * fp, dict2pid_t * d2p, bin_mdef_t * mdef, dict_t * dict)
{
    int32 w, p, pronlen;
    int32 i, j, b, l, r;

    fprintf(fp, "# INTERNAL (wd comssid ssid ssid ... ssid comssid)\n");
    for (w = 0; w < d2p->n_dictsize; w++) {
        fprintf(fp, "%30s ", dict_wordstr(dict, w));

        pronlen = dict_pronlen(dict, w);
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

    fprintf(fp, "# SSEQ %d (senid senid ...)\n", mdef->n_sseq);
    for (i = 0; i < mdef->n_sseq; i++) {
        fprintf(fp, "%5d ", i);
        for (j = 0; j < bin_mdef_n_emit_state(mdef); j++)
            fprintf(fp, " %5d", mdef->sseq[i][j]);
        fprintf(fp, "\n");
    }
    fprintf(fp, "#\n");
    fprintf(fp, "# END\n");

    fflush(fp);
}
