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

#include <string.h>

#include <pocketsphinx.h>

#include "util/ckd_alloc.h"
#include "util/bio.h"
#include "util/vector.h"

#include "tmat.h"
#include "hmm.h"

#define TMAT_PARAM_VERSION		"1.0"


/**
 * Checks that no transition matrix in the given object contains backward arcs.
 * @returns 0 if successful, -1 if check failed.
 */
static int32 tmat_chk_uppertri(tmat_t *tmat, logmath_t *lmath);


/**
 * Checks that transition matrix arcs in the given object skip over
 * at most 1 state.  
 * @returns 0 if successful, -1 if check failed.  
 */

static int32 tmat_chk_1skip(tmat_t *tmat, logmath_t *lmath);


void
tmat_dump(tmat_t * tmat, FILE * fp)
{
    int32 i, src, dst;

    for (i = 0; i < tmat->n_tmat; i++) {
        fprintf(fp, "TMAT %d = %d x %d\n", i, tmat->n_state,
                tmat->n_state + 1);
        for (src = 0; src < tmat->n_state; src++) {
            for (dst = 0; dst <= tmat->n_state; dst++)
                fprintf(fp, " %12d", tmat->tp[i][src][dst]);
            fprintf(fp, "\n");
        }
        fprintf(fp, "\n");
    }
    fflush(fp);
}


/*
 * Check model tprob matrices that they conform to upper-triangular assumption;
 * i.e. no "backward" transitions allowed.
 */
int32
tmat_chk_uppertri(tmat_t * tmat, logmath_t *lmath)
{
    int32 i, src, dst;

    (void) lmath;
    /* Check that each tmat is upper-triangular */
    for (i = 0; i < tmat->n_tmat; i++) {
        for (dst = 0; dst < tmat->n_state; dst++)
            for (src = dst + 1; src < tmat->n_state; src++)
                if (tmat->tp[i][src][dst] < 255) {
                    E_ERROR("tmat[%d][%d][%d] = %d\n",
                            i, src, dst, tmat->tp[i][src][dst]);
                    return -1;
                }
    }

    return 0;
}


int32
tmat_chk_1skip(tmat_t * tmat, logmath_t *lmath)
{
    int32 i, src, dst;

    (void) lmath;
    for (i = 0; i < tmat->n_tmat; i++) {
        for (src = 0; src < tmat->n_state; src++)
            for (dst = src + 3; dst <= tmat->n_state; dst++)
                if (tmat->tp[i][src][dst] < 255) {
                    E_ERROR("tmat[%d][%d][%d] = %d\n",
                            i, src, dst, tmat->tp[i][src][dst]);
                    return -1;
                }
    }

    return 0;
}


tmat_t *
tmat_init(char const *file_name, logmath_t *lmath, float64 tpfloor, int32 breport)
{
    char tmp;
    int32 n_src, n_dst, n_tmat;
    FILE *fp;
    int32 byteswap, chksum_present;
    uint32 chksum;
    float32 **tp;
    int32 i, j, k, tp_per_tmat;
    char **argname, **argval;
    tmat_t *t;


    if (breport) {
        E_INFO("Reading HMM transition probability matrices: %s\n",
               file_name);
    }

    t = (tmat_t *) ckd_calloc(1, sizeof(tmat_t));

    if ((fp = fopen(file_name, "rb")) == NULL)
        E_FATAL_SYSTEM("Failed to open transition file '%s' for reading", file_name);

    /* Read header, including argument-value info and 32-bit byteorder magic */
    if (bio_readhdr(fp, &argname, &argval, &byteswap) < 0)
        E_FATAL("Failed to read header from file '%s'\n", file_name);

    /* Parse argument-value list */
    chksum_present = 0;
    for (i = 0; argname[i]; i++) {
        if (strcmp(argname[i], "version") == 0) {
            if (strcmp(argval[i], TMAT_PARAM_VERSION) != 0)
                E_WARN("Version mismatch(%s): %s, expecting %s\n",
                       file_name, argval[i], TMAT_PARAM_VERSION);
        }
        else if (strcmp(argname[i], "chksum0") == 0) {
            chksum_present = 1; /* Ignore the associated value */
        }
    }
    bio_hdrarg_free(argname, argval);
    argname = argval = NULL;

    chksum = 0;

    /* Read #tmat, #from-states, #to-states, arraysize */
    if ((bio_fread(&n_tmat, sizeof(int32), 1, fp, byteswap, &chksum)
         != 1)
        || (bio_fread(&n_src, sizeof(int32), 1, fp, byteswap, &chksum) !=
            1)
        || (bio_fread(&n_dst, sizeof(int32), 1, fp, byteswap, &chksum) !=
            1)
        || (bio_fread(&i, sizeof(int32), 1, fp, byteswap, &chksum) != 1)) {
        E_FATAL("Failed to read header from '%s'\n", file_name);
    }
    if (n_tmat >= MAX_INT16)
        E_FATAL("%s: Number of transition matrices (%d) exceeds limit (%d)\n", file_name,
                n_tmat, MAX_INT16);
    t->n_tmat = n_tmat;
    
    if (n_dst != n_src + 1)
        E_FATAL("%s: Unsupported transition matrix. Number of source states (%d) != number of target states (%d)-1\n", file_name,
                n_src, n_dst);
    t->n_state = n_src;

    if (i != t->n_tmat * n_src * n_dst) {
        E_FATAL
            ("%s: Invalid transitions. Number of coefficients (%d) doesn't match expected array dimension: %d x %d x %d\n",
             file_name, i, t->n_tmat, n_src, n_dst);
    }

    /* Allocate memory for tmat data */
    t->tp = ckd_calloc_3d(t->n_tmat, n_src, n_dst, sizeof(***t->tp));

    /* Temporary structure to read in the float data */
    tp = ckd_calloc_2d(n_src, n_dst, sizeof(**tp));

    /* Read transition matrices, normalize and floor them, and convert to log domain */
    tp_per_tmat = n_src * n_dst;
    for (i = 0; i < t->n_tmat; i++) {
        if (bio_fread(tp[0], sizeof(float32), tp_per_tmat, fp,
                      byteswap, &chksum) != tp_per_tmat) {
            E_FATAL("Failed to read transition matrix %d from '%s'\n", i, file_name);
        }

        /* Normalize and floor */
        for (j = 0; j < n_src; j++) {
            if (vector_sum_norm(tp[j], n_dst) == 0.0)
                E_WARN("Normalization failed for transition matrix %d from state %d\n",
                       i, j);
            vector_nz_floor(tp[j], n_dst, tpfloor);
            vector_sum_norm(tp[j], n_dst);

            /* Convert to logs3. */
            for (k = 0; k < n_dst; k++) {
                int ltp;
#if 0 /* No, don't do this!  It will subtly break 3-state HMMs. */
                /* For these ones, we floor them even if they are
                 * zero, otherwise HMM evaluation goes nuts. */
                if (k >= j && k-j < 3 && tp[j][k] == 0.0f)
                    tp[j][k] = tpfloor;
#endif
                /* Log and quantize them. */
                ltp = -logmath_log(lmath, tp[j][k]) >> SENSCR_SHIFT;
                if (ltp > 255) ltp = 255;
                t->tp[i][j][k] = (uint8)ltp;
            }
        }
    }

    ckd_free_2d(tp);

    if (chksum_present)
        bio_verify_chksum(fp, byteswap, chksum);

    if (fread(&tmp, 1, 1, fp) == 1)
        E_ERROR("Non-empty file beyond end of data\n");

    fclose(fp);

    if (tmat_chk_uppertri(t, lmath) < 0)
        E_FATAL("Tmat not upper triangular\n");
    if (tmat_chk_1skip(t, lmath) < 0)
        E_FATAL("Topology not Left-to-Right or Bakis\n");

    return t;
}

void
tmat_report(tmat_t * t)
{
    E_INFO_NOFN("Initialization of tmat_t, report:\n");
    E_INFO_NOFN("Read %d transition matrices of size %dx%d\n",
                t->n_tmat, t->n_state, t->n_state + 1);
    E_INFO_NOFN("\n");

}

/* 
 *  RAH, Free memory allocated in tmat_init ()
 */
void
tmat_free(tmat_t * t)
{
    if (t) {
        if (t->tp)
            ckd_free_3d(t->tp);
        ckd_free(t);
    }
}
