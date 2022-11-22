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
 * senone.h -- Mixture density weights associated with each tied state.
 */

#ifndef _MS_SENONE_H_
#define _MS_SENONE_H_

#include <pocketsphinx.h>

#include "ms_gauden.h"
#include "bin_mdef.h"

/** \file ms_senone.h
 *  \brief (Sphinx 3.0 specific) multiple streams senones. used with ms_gauden.h
 *  In Sphinx 3.0 family of tools, ms_senone is used to combine the Gaussian scores.
 *  Its existence is crucial in Sphinx 3.0 because 3.0 supports both SCHMM and CDHMM. 
 *  There are optimization scheme for SCHMM (e.g. compute the top-N Gaussian) that is 
 *  applicable to SCHMM than CDHMM.  This is wrapped in senone_eval_all. 
 */

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

typedef uint8 senprob_t;	/**< Senone logs3-probs, truncated to 8 bits */

/**
 * \struct senone_t
 * \brief 8-bit senone PDF structure. 
 * 
 * 8-bit senone PDF structure.  Senone pdf values are normalized, floored, converted to
 * logs3 domain, and finally truncated to 8 bits precision to conserve memory space.
 */
typedef struct senone_s {
    senprob_t ***pdf;		/**< gaussian density mixture weights, organized two possible
                                   ways depending on n_gauden:
                                   if (n_gauden > 1): pdf[sen][feat][codeword].  Not an
                                   efficient representation--memory access-wise--but
                                   evaluating the many codebooks will be more costly.
                                   if (n_gauden == 1): pdf[feat][codeword][sen].  Optimized
                                   for the shared-distribution semi-continuous case. */
    logmath_t *lmath;           /**< log math computation */
    uint32 n_sen;		/**< Number senones in this set */
    uint32 n_feat;		/**< Number feature streams */ 
    uint32 n_cw;		/**< Number codewords per codebook,stream */
    uint32 n_gauden;		/**< Number gaussian density codebooks referred to by senones */
    float32 mixwfloor;		/**< floor applied to each PDF entry */
    uint32 *mgau;		/**< senone-id -> mgau-id mapping for senones in this set */
    int32 *featscr;              /**< The feature score for every senone, will be initialized inside senone_eval_all */
    int32 aw;			/**< Inverse acoustic weight */
} senone_t;


/**
 * Load a set of senones (mixing weights and mixture gaussian codebook mappings) from
 * the given files.  Normalize weights for each codebook, apply the given floor, convert
 * PDF values to logs3 domain and quantize to 8-bits.
 * @return pointer to senone structure created.  Caller MUST NOT change its contents.
 */
senone_t *senone_init (gauden_t *g,             /**< In: codebooks */
                       char const *mixwfile,	/**< In: mixing weights file */
                       char const *mgau_mapfile,/**< In: file or magic string specifying
                                                   mapping from each senone to mixture
                                                   gaussian codebook.
                                                   If NULL divine it from gauden_t */
		       float32 mixwfloor,	/**< In: Floor value for senone weights */
                       logmath_t *lmath,        /**< In: log math computation */
                       bin_mdef_t *mdef         /**< In: model definition */
    );

/** Release memory allocated by senone_init. */
void senone_free(senone_t *s); /**< In: The senone_t to free */

/**
 * Evaluate the score for the given senone wrt to the given top N gaussian codewords.
 * @return senone score (in logs3 domain).
 */
int32 senone_eval (senone_t *s, int id,		/**< In: senone for which score desired */
		   gauden_dist_t **dist,	/**< In: top N codewords and densities for
						   all features, to be combined into
						   senone score.  IE, dist[f][i] = i-th
						   best <codeword,density> for feaure f */
		   int n_top		/**< In: Length of dist[f], for each f */
    );

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
