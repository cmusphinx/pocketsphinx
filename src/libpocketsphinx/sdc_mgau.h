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
 * Interface for subspace distribution clustered GMM evaluation.
 */

#ifndef __SDC_MGAU_H__
#define __SDC_MGAU_H__

/* SphinxBase headesr. */
#include <fe.h>
#include <logmath.h>
#include <mmio.h>

/* Local headers. */
#include "acmod.h"
#include "hmm.h"
#include "bin_mdef.h"
#include "s2_semi_mgau.h"

typedef struct sdc_mgau_s sdc_mgau_t;
struct sdc_mgau_s {
    ps_mgau_t base;     /**< base structure. */
    cmd_ln_t *config;   /* configuration parameters */

    mean_t  **means;	/* mean vectors foreach feature */
    var_t   **vars;	/* inverse var vectors foreach feature */
    var_t   **dets;	/* det values foreach feature */

    int32 n_mixw_den;  /* Number of distributions per senone */
    uint8 **mixw;     /* Mixture weight distributions */
    uint8 ***sdmap;    /* Mapping from senones to codewords */

    int32 n_sv;		/* Number of subspaces */
    int32 *veclen;	/* Length of subspaces */
    int32 n_density;	/* Number of distributions per codebook */
    int32 n_sen;	/* Number of senones */

    int32 num_frames;
    int32 ds_ratio;

    int32 **cb_scores;
    int32 *score_tmp;

    /* Log-add table for compressed values. */
    logmath_t *lmath_8b;
};

ps_mgau_t *sdc_mgau_init(cmd_ln_t *config, logmath_t *lmath, bin_mdef_t *mdef);
void sdc_mgau_free(ps_mgau_t *s);
int32 sdc_mgau_frame_eval(ps_mgau_t *s,
                          int16 *senone_scores,
                          uint8 *senone_active,
                          int32 n_senone_active,
                          mfcc_t **featbuf,
                          int32 frame,
                          int32 compallsen);
int32 sdc_mgau_mllr_transform(ps_mgau_t *s,
                              float32 ***A,
                              float32 **b,
                              float32 **h,
                              int32 *cb2mllr);

#endif /*  __SDC_MGAU_H__ */
