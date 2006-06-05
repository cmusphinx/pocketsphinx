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
 * cont_mgau.h -- Mixture Gaussians for continuous HMM models.
 *
 * **********************************************
 * CMU ARPA Speech Project
 *
 * Copyright (c) 1997 Carnegie Mellon University.
 * ALL RIGHTS RESERVED.
 * **********************************************
 *
 * HISTORY
 * 
 * $Log: cont_mgau.h,v $
 * Revision 1.1.1.1  2006/05/23 18:45:02  dhuggins
 * re-importation
 *
 * Revision 1.1  2004/12/10 16:48:58  rkm
 * Added continuous density acoustic model handling
 *
 * 
 * 22-Nov-2004	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Imported from s3.2, for supporting s3 format continuous
 * 		acoustic models.
 * 
 * 15-Dec-1999	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Added mgau_model_t.{frm_sen_eval,frm_gau_eval}.
 * 		Added mgau_var_nzvec_floor().
 * 
 * 28-Mar-1999	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University.
 * 		Started.
 */


#ifndef __CONT_MGAU_H__
#define __CONT_MGAU_H__


#include "s2types.h"


/*
 * Mixture Gaussians: Weighted set of Gaussian densities, each with its own mean vector and
 * diagonal covariance matrix.  Specialized for continuous HMMs to improve speed performance.
 * So, a separate mixture Gaussian, with its own mixture weights, for each HMM state.  Also,
 * a single feature stream assumed.  (In other words, the mgau_t structure below represents
 * a senone in a fully continuous HMM model.)
 * 
 * Given a Gaussian density with mean vector m and diagonal variance vector v, and some
 * input vector x, all of length n, the Mahalanobis distance of x from the Gaussian mean m
 * is given by:
 *     {1/sqrt((2pi)^n * det(v))} * exp{-Sum((x[i] - m[i])^2 / (2v[i]))}
 * To speed up this evaluation, the first sub-expression ({1/sqrt...}) can be precomputed at
 * initialization, and so can 1/2v[i] in the second sub-expression.  Secondly, recognition
 * systems work with log-likelihood values, so these distances or likelihood values are
 * computed in log-domain.  Finally, float32 operations are costlier than int32 ones, so
 * the log-values are converted to logs3 domain (see libmisc/logs3.h) (but before the mixing
 * weights are applied).  Thus, to reiterate, the final scores are (int32) logs3 values.
 */


/*
 * A single mixture-Gaussian model for one senone (see above comment).
 */
typedef struct {
    int32 n_comp;	/* #Component Gaussians in this mixture.  NOTE: May be 0 (for the
			   untrained states). */
    float32 **mean;	/* The n_comp means of the Gaussians. */
    float32 **var;	/* The n_comp (diagonal) variances of the Gaussians.  Could be
			   converted to 1/(2*var) for faster computation (see above comment). */
    int32 *mixw;	/* Mixture weights for the n_comp components (int32 instead of float32
			   because these values are in logs3 domain) */
    float32 *lrd;	/* Log(Reciprocal(Determinant (variance))).  (Then there is also a
			   (2pi)^(veclen) involved...) */
} mgau_t;


/*
 * The set of mixture-Gaussians in an acoustic model.
 */
typedef struct {
    int32 n_mgau;	/* #Mixture Gaussians in this model (i.e., #senones) */
    int32 max_comp;	/* Max components in any mixture */
    int32 veclen;	/* Vector length of the Gaussian density means (and diagonal vars) */
    mgau_t *mgau;	/* The n_mgau mixture Gaussians */
    float64 distfloor;	/* Mahalanobis distances can underflow when finally converted to
			   logs3 values.  To prevent this, floor the log values first. */
    /* Statistics */
    int32 frm_sen_eval;		/* #Senones evaluated in the most recent frame */
    int32 frm_gau_eval;		/* #Gaussian densities evaluated in the most recent frame */
} mgau_model_t;


/* Access macros */
#define mgau_n_mgau(g)		((g)->n_mgau)
#define mgau_max_comp(g)	((g)->max_comp)
#define mgau_veclen(g)		((g)->veclen)
#define mgau_n_comp(g,m)	((g)->mgau[m].n_comp)
#define mgau_mean(g,m,c)	((g)->mgau[m].mean[c])
#define mgau_var(g,m,c)		((g)->mgau[m].var[c])
#define mgau_mixw(g,m,c)	((g)->mgau[m].mixw[c])
#define mgau_lrd(g,m,c)		((g)->mgau[m].lrd[c])
#define mgau_frm_sen_eval(g)	((g)->frm_sen_eval)
#define mgau_frm_gau_eval(g)	((g)->frm_gau_eval)


/*
 * Create a new mixture Gaussian model from the given files (Sphinx3 format).  Optionally,
 * apply the precomputations mentioned in the main comment above.
 * Return value: pointer to the model created if successful; NULL if error.
 */
mgau_model_t *
mgau_init (char *meanfile,	/* In: File containing means of mixture gaussians */
	   char *varfile,	/* In: File containing variances of mixture gaussians */
	   float64 varfloor,	/* In: Floor value applied to variances; e.g., 0.0001 */
	   char *mixwfile,	/* In: File containing mixture weights */
	   float64 mixwfloor,	/* In: Floor value for mixture weights; e.g., 0.0000001 */
	   int32 precomp);	/* In: If TRUE, create and precompute mgau_t.lrd and also
				   transform each var value to 1/(2*var).  (If FALSE, one
				   cannot use the evaluation routines provided here.) */

/*
 * Floor any variance vector that is non-zero (vector).
 * Return value: No. of variance VALUES floored.
 */
int32 mgau_var_nzvec_floor (mgau_model_t *g, float64 floor);


/*
 * Evaluate a single mixture Gaussian at the given vector x; i.e., compute the Mahalanobis
 * distance of x from each mean in the mixture, and combine them using the mixture weights.
 * Return value: The final score from this evaluation (a logs3 domain value).  NOTE: if the
 * specified mixture is empty, S3_LOGPROB_ZERO is returned (see libmisc/libmisc.h).
 */
int32
mgau_eval (mgau_model_t *g,	/* In: The entire mixture Gaussian model */
	   int32 m,		/* In: The chosen mixture in the model (i.e., g->mgau[m]) */
	   int32 *active_comp,	/* In: An optional, -1 terminated list of active component
				   indices; if non-NULL, only the specified components are
				   used in the evaluation. */
	   float32 *x);		/* In: Input observation vector (of length g->veclen). */

/*
 * Like mgau_eval, but return the scores of the individual components, instead of combining
 * them into a senone score.  Return value: Best component score.
 */
int32 mgau_comp_eval (mgau_model_t *g,	/* In: Set of mixture Gaussians */
		      int32 m,		/* In: Mixture being considered */
		      float32 *x,	/* In: Input vector being compared to the components */
		      int32 *score);	/* Out: Array of scores for each component */

#endif
