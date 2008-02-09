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
 * senscr.c -- 	Senone score computation module.
 * 		Hides details of s2 (semi-continuous) and s3 (continuous)
 * 		models, and computes generic "senone scores".
 *
 * HISTORY
 * 
 * 02-Dec-2004	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Added acoustic score weight (applied only to S3 continuous
 * 		acoustic models).
 * 
 * 01-Dec-2004	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University.
 * 		Added senone active flag related functions.
 * 
 * 20-Nov-2004	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University.
 * 		Started.
 */

/* System headers. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

/* SphinxBase headers. */
#include <sphinx_types.h>
#include <err.h>
#include <ckd_alloc.h>

/* Local headers. */
#include "kb.h"
#include "s2_semi_mgau.h"
#include "ms_mgau.h"
#include "phone.h"
#include "search.h"
#include "search_const.h"
#include "senscr.h"

/* Global variables shared by search and GMM computations. */
int32 *senone_scores;
int32 *senone_active;
int32 n_senone_active;
bitvec_t *senone_active_vec;

static int32
senscr_compute(mfcc_t **feat, int32 frame_idx, int32 all)
{
    int32 best, bestidx;

    if (all) {
        if (g_ms_mgau)
            best = ms_cont_mgau_frame_eval(g_ms_mgau, senone_scores,
                                           senone_active, n_senone_active,
                                           feat, frame_idx, TRUE, &bestidx);
        else
            best = s2_semi_mgau_frame_eval(g_semi_mgau, senone_scores,
                                           senone_active, n_senone_active,
                                           feat, frame_idx, TRUE, &bestidx);
    }
    else {
        if (g_ms_mgau)
            best = ms_cont_mgau_frame_eval(g_ms_mgau, senone_scores,
                                           senone_active, n_senone_active,
                                           feat, frame_idx, FALSE, &bestidx);
        else
            best = s2_semi_mgau_frame_eval(g_semi_mgau, senone_scores,
                                           senone_active, n_senone_active,
                                           feat, frame_idx, FALSE, &bestidx);
    }

    return best;
}


int32
senscr_all(mfcc_t **feat, int32 frame_idx)
{
    return senscr_compute(feat, frame_idx, TRUE);
}


int32
senscr_active(mfcc_t **feat, int32 frame_idx)
{
    return senscr_compute(feat, frame_idx, FALSE);
}

void
sen_active_clear(void)
{
    memset(senone_active_vec, 0, (bin_mdef_n_sen(g_mdef) + BITVEC_WIDTH - 1)
           / BITVEC_WIDTH * sizeof(bitvec_t));
    n_senone_active = 0;
}

#define MPX_BITVEC_SET(h,i)                                                     \
            if ((h)->s.mpx_ssid[i] != -1)                                       \
                BITVEC_SET(senone_active_vec,                                   \
                           bin_mdef_sseq2sen(g_mdef, (h)->s.mpx_ssid[i], (i)));
#define NONMPX_BITVEC_SET(h,i)                                          \
                BITVEC_SET(senone_active_vec,                           \
                           bin_mdef_sseq2sen(g_mdef, (h)->s.ssid, (i)));

void
hmm_sen_active(hmm_t * hmm)
{
    int i;

    if (hmm_is_mpx(hmm)) {
        switch (hmm_n_emit_state(hmm)) {
        case 5:
            MPX_BITVEC_SET(hmm, 4);
            MPX_BITVEC_SET(hmm, 3);
        case 3:
            MPX_BITVEC_SET(hmm, 2);
            MPX_BITVEC_SET(hmm, 1);
            MPX_BITVEC_SET(hmm, 0);
            break;
        default:
            for (i = 0; i < hmm_n_emit_state(hmm); ++i) {
                MPX_BITVEC_SET(hmm, i);
            }
        }
    }
    else {
        switch (hmm_n_emit_state(hmm)) {
        case 5:
            NONMPX_BITVEC_SET(hmm, 4);
            NONMPX_BITVEC_SET(hmm, 3);
        case 3:
            NONMPX_BITVEC_SET(hmm, 2);
            NONMPX_BITVEC_SET(hmm, 1);
            NONMPX_BITVEC_SET(hmm, 0);
            break;
        default:
            for (i = 0; i < hmm_n_emit_state(hmm); ++i) {
                NONMPX_BITVEC_SET(hmm, i);
            }
        }
    }
}

#ifdef BITVEC_SEN_ACTIVE
int32
sen_active_flags2list(void)
{
    int32 i, j, total_dists, total_bits;
    bitvec_t *flagptr, bits;

    total_dists = bin_mdef_n_sen(g_mdef);

    j = 0;
    total_bits = total_dists & -BITVEC_WIDTH;
    for (i = 0, flagptr = senone_active_vec; i < total_bits; flagptr++) {
        bits = *flagptr;

        if (bits == 0) {
            i += BITVEC_WIDTH;
            continue;
        }

        if (bits & (1 << 0))
            senone_active[j++] = i;
        ++i;
        if (bits & (1 << 1))
            senone_active[j++] = i;
        ++i;
        if (bits & (1 << 2))
            senone_active[j++] = i;
        ++i;
        if (bits & (1 << 3))
            senone_active[j++] = i;
        ++i;
        if (bits & (1 << 4))
            senone_active[j++] = i;
        ++i;
        if (bits & (1 << 5))
            senone_active[j++] = i;
        ++i;
        if (bits & (1 << 6))
            senone_active[j++] = i;
        ++i;
        if (bits & (1 << 7))
            senone_active[j++] = i;
        ++i;
#if BITVEC_WIDTH > 8
        if (bits & (1 << 8))
            senone_active[j++] = i;
        ++i;
        if (bits & (1 << 9))
            senone_active[j++] = i;
        ++i;
        if (bits & (1 << 10))
            senone_active[j++] = i;
        ++i;
        if (bits & (1 << 11))
            senone_active[j++] = i;
        ++i;
        if (bits & (1 << 12))
            senone_active[j++] = i;
        ++i;
        if (bits & (1 << 13))
            senone_active[j++] = i;
        ++i;
        if (bits & (1 << 14))
            senone_active[j++] = i;
        ++i;
        if (bits & (1 << 15))
            senone_active[j++] = i;
        ++i;
#if BITVEC_WIDTH == 32
        if (bits & (1 << 16))
            senone_active[j++] = i;
        ++i;
        if (bits & (1 << 17))
            senone_active[j++] = i;
        ++i;
        if (bits & (1 << 18))
            senone_active[j++] = i;
        ++i;
        if (bits & (1 << 19))
            senone_active[j++] = i;
        ++i;
        if (bits & (1 << 20))
            senone_active[j++] = i;
        ++i;
        if (bits & (1 << 21))
            senone_active[j++] = i;
        ++i;
        if (bits & (1 << 22))
            senone_active[j++] = i;
        ++i;
        if (bits & (1 << 23))
            senone_active[j++] = i;
        ++i;
        if (bits & (1 << 24))
            senone_active[j++] = i;
        ++i;
        if (bits & (1 << 25))
            senone_active[j++] = i;
        ++i;
        if (bits & (1 << 26))
            senone_active[j++] = i;
        ++i;
        if (bits & (1 << 27))
            senone_active[j++] = i;
        ++i;
        if (bits & (1 << 28))
            senone_active[j++] = i;
        ++i;
        if (bits & (1 << 29))
            senone_active[j++] = i;
        ++i;
        if (bits & (1 << 30))
            senone_active[j++] = i;
        ++i;
        if (bits & (1 << 31))
            senone_active[j++] = i;
        ++i;
#endif                          /* BITVEC_WIDTH == 32 */
#endif                          /* BITVEC_WIDTH > 8 */
    }

    for (; i < total_dists; ++i)
        if (*flagptr & (1 << (i % BITVEC_WIDTH)))
            senone_active[j++] = i;

    n_senone_active = j;

    return j;
}
#else
int32
sen_active_flags2list(void)
{
    int32 i, j, total_dists, total_words, bits;
    uint8 *flagptr;

    total_dists = bin_mdef_n_sen(g_mdef);

    j = 0;
    total_words = total_dists & ~3;
    for (i = 0, flagptr = senone_active_vec; i < total_words;) {
        bits = *(int32 *) flagptr;
        if (bits == 0) {
            flagptr += 4;
            i += 4;
            continue;
        }
        if (*flagptr++)
            senone_active[j++] = i;
        ++i;
        if (*flagptr++)
            senone_active[j++] = i;
        ++i;
        if (*flagptr++)
            senone_active[j++] = i;
        ++i;
        if (*flagptr++)
            senone_active[j++] = i;
        ++i;
    }

    for (; i < total_dists; ++i, ++flagptr)
        if (*flagptr)
            senone_active[j++] = i;

    n_senone_active = j;

    return j;
}
#endif
