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
 * gauden.h -- gaussian density module.
 *
 * **********************************************
 * CMU ARPA Speech Project
 *
 * Copyright (c) 1996 Carnegie Mellon University.
 * ALL RIGHTS RESERVED.
 * **********************************************
 *
 * HISTORY
 * $Log$
 * Revision 1.1  2006/04/05  20:27:30  dhdfu
 * A Great Reorganzation of header files and executables
 * 
 * Revision 1.8  2006/02/22 17:09:55  arthchan2003
 * Merged from SPHINX3_5_2_RCI_IRII_BRANCH: 1, Followed Dave's change, keep active to be uint8 instead int8 in gauden_dist_norm.\n 2, Introdued gauden_dump and gauden_dump_ind.  This allows debugging of ms_gauden routine. \n 3, Introduced gauden_free, this fixed some minor memory leaks. \n 4, gauden_init accept an argument precompute to specify whether the distance is pre-computed or not.\n 5, Added license. \n 6, Fixed dox-doc.
 *
 *
 *
 * Revision 1.6.4.6  2006/01/16 19:45:59  arthchan2003
 * Change the gaussian density dumping routine to a function.
 *
 * Revision 1.6.4.5  2005/10/09 19:51:05  arthchan2003
 * Followed Dave's changed in the trunk.
 *
 * Revision 1.7  2005/10/05 00:31:14  dhdfu
 * Make int8 be explicitly signed (signedness of 'char' is
 * architecture-dependent).  Then make a bunch of things use uint8 where
 * signedness is unimportant, because on the architecture where 'char' is
 * unsigned, it is that way for a reason (signed chars are slower).
 *
 * Revision 1.6.4.4  2005/09/25 18:54:20  arthchan2003
 * Added a flag to turn on and off precomputation.
 *
 * Revision 1.6.4.3  2005/08/03 18:53:44  dhdfu
 * Add memory deallocation functions.  Also move all the initialization
 * of ms_mgau_model_t into ms_mgau_init (duh!), which entails removing it
 * from decode_anytopo and friends.
 *
 * Revision 1.6.4.2  2005/07/20 19:39:01  arthchan2003
 * Added licences in ms_* series of code.
 *
 * Revision 1.6.4.1  2005/07/05 05:47:59  arthchan2003
 * Fixed dox-doc. struct level of documentation are included.
 *
 * Revision 1.6  2005/06/21 18:55:09  arthchan2003
 * 1, Add comments to describe this modules, 2, Fixed doxygen documentation. 3, Added $ keyword.
 *
 * Revision 1.4  2005/06/13 04:02:55  archan
 * Fixed most doxygen-style documentation under libs3decoder.
 *
 * Revision 1.3  2005/03/30 01:22:47  archan
 * Fixed mistakes in last updates. Add
 *
 * 
 * 26-Sep-96	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University.
 * 		Added gauden_mean_reload() for application of MLLR.
 * 
 * 20-Jan-96	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University.
 * 		Added active argument to gauden_dist_norm and gauden_dist_norm_global,
 * 		and made the latter a static function.
 * 
 * 06-Nov-95	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University.
 * 		Initial version created.
 * 		Very liberally borrowed/adapted from Eric's S3 trainer implementation.
 */


#ifndef _LIBFBS_GAUDEN_H_
#define _LIBFBS_GAUDEN_H_

/** \file ms_gauden.h
 * \brief (Sphinx 3.0 specific) Gaussian density module.
 *
 * Gaussian density distribution implementation. There are two major
 * difference bettwen ms_gauden and cont_mgau. One is the fact that
 * ms_gauden only take cares of the Gaussian computation part where
 * cont_mgau actually take care of senone computation as well. The
 * other is the fact that ms_gauden is a multi-stream implementation
 * of GMM computation.
 *
 */

/* SphinxBase headers. */
#include <sphinx_types.h>
#include <feat.h>
#include <logmath.h>

/* Local headers. */
#include "vector.h"

#ifdef __cplusplus
extern "C" {
#endif
#if 0
} /* Fool Emacs into not indenting things. */
#endif

/**
 * \struct gauden_t
 * \brief Multivariate gaussian mixture density parameters
 */
typedef struct {
    mfcc_t ****mean;	/**< mean[codebook][feature][codeword] vector */
    mfcc_t ****var;	/**< like mean; diagonal covariance vector only */
    mfcc_t ***det;	/**< log(determinant) for each variance vector;
			   actually, log(sqrt(2*pi*det)) */
    logmath_t *lmath;   /**< log math computation */
    int32 n_mgau;	/**< #codebooks */
    int32 n_feat;	/**< #feature streams in each codebook */
    int32 n_density;	/**< #gaussian densities in each codebook-feature stream */
    int32 *featlen;	/**< feature length for each feature */
} gauden_t;

/**
 * \struct gauden_dist_t
 * \brief Structure to store distance (density) values for a given input observation wrt density values in some given codebook.
 */
typedef struct {
    int32 id;		/**< Index of codeword (gaussian density) */
    int32 dist;		/**< Density value for input observation wrt above codeword;
                           NOTE: result in logs3 domain; hence int32 */

} gauden_dist_t;


/**
 * Read mixture gaussian codebooks from the given files.  Allocate memory space needed
 * for them.  Apply the specified variance floor value.
 * Return value: ptr to the model created; NULL if error.
 * (See Sphinx3 model file-format documentation.)
 */
gauden_t *
gauden_init (char *meanfile,	/**< Input: File containing means of mixture gaussians */
	     char *varfile,	/**< Input: File containing variances of mixture gaussians */
	     mfcc_t varfloor,	/**< Input: Floor value to be applied to variances */
	     int32 precompute,  /**< Input: Whether we should precompute */
             logmath_t *lmath
    );

/** Release memory allocated by gauden_init. */
void gauden_free(gauden_t *g); /**< In: The gauden_t to free */

/**
 * Reload mixture Gaussian means from the given file.  The means must have already
 * been loaded at least once (using gauden_init).
 * @return 0 if successful, -1 otherwise.
 */
int32 gauden_mean_reload (gauden_t *g,		/**< In/Out: g->mean to be reloaded */
			  char *meanfile	/**< In: File to reload means from */
    );

/**
 * Compute gaussian density values for the given input observation vector wrt the
 * specified mixture gaussian codebook (which may consist of several feature streams).
 * Density values are left UNnormalized.
 * @return 0 if successful, -1 otherwise.
 */
int32
gauden_dist (gauden_t *g,	/**< In: handle to entire ensemble of codebooks */
	     s3mgauid_t mgau,	/**< In: codebook for which density values to be evaluated
				   (g->{mean,var}[mgau]) */
	     int32 n_top,	/**< In: #top densities to be evaluated */
	     mfcc_t **obs,	/**< In: Observation vector; obs[f] = for feature f */
	     gauden_dist_t **out_dist
	     /**< Out: n_top best codewords and density values,
		in worsening order, for each feature stream.
		out_dist[f][i] = i-th best density for feature f.
		Caller must allocate memory for this output */
    );


/**
 * Normalize density values (previously computed by gauden_dist).
 * Two cases:  If (g->n_mgau == 1), normalize such that the sum of the n_top codeword
 * scores for each feature in dist sums to 1 (in prob domain).
 * Otherwise, normalize by dividing the density value (subtracting, in logprob domain) for
 * each codeword by the best one.
 * @return scaling applied to every senone score as a result of the normalization.
 */
int32
gauden_dist_norm (gauden_t *g,		/**< In: handle to all collection of codebooks */
		  int32 n_top,		/**< In: #density values computed per feature */
		  gauden_dist_t ***dist,/**< In/Out: n_top density indices and values for
					   each feature.  On return, density values are
					   normalized. */
		  uint8 *active	/**< In: active[gid] is non-0 iff codebook gid is
				   active.  If NULL, all codebooks active */
    );


/**
   Dump the definitionn of Gaussian distribution. 
*/
void gauden_dump (const gauden_t *g  /**< In: Gaussian distribution g*/
    );

/**
   Dump the definition of Gaussian distribution of a particular index to the standard output stream
*/
void gauden_dump_ind (const gauden_t *g,  /**< In: Gaussian distribution g*/
		      int senidx          /**< In: The senone index of the Gaussian */
    );

#if 0
{ /* Stop indent from complaining */
#endif
#ifdef __cplusplus
}
#endif

#endif /* GAUDEN_H */ 
