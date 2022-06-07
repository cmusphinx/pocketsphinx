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
 * cmn.c -- Various forms of cepstral mean normalization
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef _MSC_VER
#pragma warning (disable: 4244)
#endif

#include "sphinxbase/ckd_alloc.h"
#include "sphinxbase/err.h"
#include "sphinxbase/cmn.h"

/* NOTE!  These must match the enum in cmn.h */
const char *cmn_type_str[] = {
    "none",
    "batch",
    "live"
};
const char *cmn_alt_type_str[] = {
    "none",
    "current",
    "prior"
};
static const int n_cmn_type_str = sizeof(cmn_type_str)/sizeof(cmn_type_str[0]);

cmn_type_t
cmn_type_from_str(const char *str)
{
    int i;

    for (i = 0; i < n_cmn_type_str; ++i) {
        if (0 == strcmp(str, cmn_type_str[i]) || 0 == strcmp(str, cmn_alt_type_str[i]))
            return (cmn_type_t)i;
    }
    E_FATAL("Unknown CMN type '%s'\n", str);
    return CMN_NONE;
}

cmn_t *
cmn_init(int32 veclen)
{
    cmn_t *cmn;
    cmn = (cmn_t *) ckd_calloc(1, sizeof(cmn_t));
    cmn->veclen = veclen;
    cmn->cmn_mean = (mfcc_t *) ckd_calloc(veclen, sizeof(mfcc_t));
    cmn->cmn_var = (mfcc_t *) ckd_calloc(veclen, sizeof(mfcc_t));
    cmn->sum = (mfcc_t *) ckd_calloc(veclen, sizeof(mfcc_t));
    cmn->nframe = 0;

    return cmn;
}


void
cmn(cmn_t *cmn, mfcc_t ** mfc, int32 varnorm, int32 n_frame)
{
    mfcc_t *mfcp;
    mfcc_t t;
    int32 i, f;
    int32 n_pos_frame;

    assert(mfc != NULL);

    if (n_frame <= 0)
        return;

    /* If cmn->cmn_mean wasn't NULL, we need to zero the contents */
    memset(cmn->cmn_mean, 0, cmn->veclen * sizeof(mfcc_t));

    /* Find mean cep vector for this utterance */
    for (f = 0, n_pos_frame = 0; f < n_frame; f++) {
        mfcp = mfc[f];

        /* Skip zero energy frames */
        if (mfcp[0] < 0)
    	    continue;

        for (i = 0; i < cmn->veclen; i++) {
            cmn->cmn_mean[i] += mfcp[i];
        }

        n_pos_frame++;
    }

    for (i = 0; i < cmn->veclen; i++)
        cmn->cmn_mean[i] /= n_pos_frame;

    E_INFO("CMN: ");
    for (i = 0; i < cmn->veclen; i++)
        E_INFOCONT("%5.2f ", MFCC2FLOAT(cmn->cmn_mean[i]));
    E_INFOCONT("\n");
    if (!varnorm) {
        /* Subtract mean from each cep vector */
        for (f = 0; f < n_frame; f++) {
            mfcp = mfc[f];
            for (i = 0; i < cmn->veclen; i++)
                mfcp[i] -= cmn->cmn_mean[i];
        }
    }
    else {
        /* Scale cep vectors to have unit variance along each dimension, and subtract means */
        /* If cmn->cmn_var wasn't NULL, we need to zero the contents */
        memset(cmn->cmn_var, 0, cmn->veclen * sizeof(mfcc_t));

        for (f = 0; f < n_frame; f++) {
            mfcp = mfc[f];

            for (i = 0; i < cmn->veclen; i++) {
                t = mfcp[i] - cmn->cmn_mean[i];
                cmn->cmn_var[i] += MFCCMUL(t, t);
            }
        }
        for (i = 0; i < cmn->veclen; i++)
            /* Inverse Std. Dev, RAH added type case from sqrt */
            cmn->cmn_var[i] = FLOAT2MFCC(sqrt((float64)n_frame / MFCC2FLOAT(cmn->cmn_var[i])));

        for (f = 0; f < n_frame; f++) {
            mfcp = mfc[f];
            for (i = 0; i < cmn->veclen; i++)
                mfcp[i] = MFCCMUL((mfcp[i] - cmn->cmn_mean[i]), cmn->cmn_var[i]);
        }
    }
}

/* 
 * RAH, free previously allocated memory
 */
void
cmn_free(cmn_t * cmn)
{
    if (cmn != NULL) {
        if (cmn->cmn_var)
            ckd_free((void *) cmn->cmn_var);

        if (cmn->cmn_mean)
            ckd_free((void *) cmn->cmn_mean);

        if (cmn->sum)
            ckd_free((void *) cmn->sum);

        ckd_free((void *) cmn);
    }
}
