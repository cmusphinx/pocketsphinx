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
 * Interface for "semi-continuous vector quantization", a.k.a. Sphinx2
 * fast GMM computation.
 */

#ifndef __S2_SEMI_MGAU_H__
#define __S2_SEMI_MGAU_H__

#include "sphinx_types.h"
#include "fe.h"
#include "kdtree.h"

typedef struct {
    union {
        int32	score;
        int32	dist;	/* distance to next closest vector */
    } val;
    int32 codeword;		/* codeword (vector index) */
} vqFeature_t;
typedef vqFeature_t *vqFrame_t;

typedef struct s2_semi_mgau_s s2_semi_mgau_t;
struct s2_semi_mgau_s {
    int32   **dets;	/* det values foreach feature */
    mean_t  **means;	/* mean vectors foreach feature */
    var_t   **vars;	/* var vectors foreach feature */

    unsigned char ***OPDF_8B; /* mixture weights */

    int32 n_feat;	/* Number of feature streams */
    int32 *veclen;	/* Length of feature streams */
    int32 n_density;	/* Number of mixtures per codebook */

    int32 topN;		/* Number of top densities to compute (<S2_MAX_TOPN) */
    int32 CdWdPDFMod;	/* Legacy thing, actually means number of mixw */

    kd_tree_t **kdtrees;
    uint32 n_kdtrees;
    uint32 kd_maxdepth;
    int32 kd_maxbbi;

    int32 num_frames;
    int32 ds_ratio;

    int32 use_mmap; /**< Are mixture weights mmap()ed? */

    /* Top-N scores and codewords from current, last frame. */
    vqFeature_t **f, **lastf;
    int32 *score_tmp;
};

s2_semi_mgau_t *s2_semi_mgau_init(const char *mean_path, const char *var_path,
				  float64 varfloor, const char *mixw_path,
				  float64 mixwfloor, int32 topn);

void s2_semi_mgau_free(s2_semi_mgau_t *s);

int32 s2_semi_mgau_frame_eval(s2_semi_mgau_t *s,
			      mfcc_t **featbuf,
			      int32 frame,
                              int32 compallsen);

int32 s2_semi_mgau_load_kdtree(s2_semi_mgau_t *s, const char *kdtree_path,
			       uint32 maxdepth, int32 maxbbi);


#endif /*  __S2_SEMI_MGAU_H__ */
