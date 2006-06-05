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
 * vector.h -- vector routines.
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
 * $Log: vector.h,v $
 * Revision 1.1.1.1  2006/05/23 18:45:03  dhuggins
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
 * 10-Mar-1999	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University.
 * 		Added vector_accum(), vector_vqlabel(), and vector_vqgen().
 * 
 * 09-Mar-1999	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University.
 * 		Added vector_is_zero(), vector_cmp(), and vector_dist_eucl().
 * 		Changed the name vector_dist_eval to vector_dist_maha.
 * 
 * 30-Dec-95	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University.
 * 		Created from Eric Thayer's version.
 */


#ifndef __VECTOR_H__
#define __VECTOR_H__


#include "s2types.h"


typedef float32 *vector_t;


/* Why do these belong here?? */
typedef struct {
    int32 r;		/* #rows, */
    int32 c;		/* #cols in an array */
} arraysize_t;

typedef struct {
    int32 x;		/* x-coordinate, */
    int32 y;		/* y-coordinate of a point */
} point_t;

typedef struct {
    float32 x;		/* x-coordinate, */
    float32 y;		/* y-coordinate of a point */
} fpoint_t;		/* Like point_t, but with float32 values */


/*
 * The reason for some of the "trivial" routines below is that they could be OPTIMIZED for SPEED
 * at some point.
 */


/* Floor all elements of v[0..dim-1] to min value of f */
void vector_floor(vector_t v, int32 dim, float64 f);


/* Floor all non-0 elements of v[0..dim-1] to min value of f */
void vector_nz_floor(vector_t v, int32 dim, float64 f);


/*
 * Normalize the elements of the given vector so that they sum to 1.0.  If the sum is 0.0
 * to begin with, the vector is left untouched.  Return value: The normalization factor.
 */
float64 vector_sum_norm(vector_t v, int32 dim);


/* Print vector in one line, in %11.4e format, terminated by newline */
void vector_print(FILE *fp, vector_t v, int32 dim);


/* Return TRUE iff given vector is all 0.0 */
int32 vector_is_zero (float32 *vec,	/* In: Vector to be checked */
		      int32 len);	/* In: Length of above vector */

/*
 * Search for the max or min valued component in the given vector.  The vector length must be
 * strictly > 0 (val[] must have at least one valid value).  If more than one component
 * qualifies, the first one is chosen.
 * Return value: the INDEX of the selected val[] component (not the value itself).
 */
int32 vector_maxcomp_int32 (int32 *val, int32 len);
int32 vector_mincomp_int32 (int32 *val, int32 len);
int32 vector_maxcomp_float32 (float32 *val, int32 len);
int32 vector_mincomp_float32 (float32 *val, int32 len);


/* Accumulate src into dst */
void vector_accum (float32 *dst,	/* In/Out: dst[i] += src[i] */
		   float32 *src,	/* In: See above */
		   int32 len);		/* In: Length of above vectors */

/*
 * Compare the two given vectors.  Return -1, 0, or +1 depending on v1 <, =, or > v2 (as
 * determined by the earliest unequal component).
 */
int32 vector_cmp (float32 *v1, float32 *v2,	/* In: Vectors to be compared */
		  int32 len);		/* In: Length of each vector */

/*
 * Compute the mean vector from the given set of vector data.
 * Return value: 0 if successful, -1 if any error (there shouldn't be any error).
 */
int32 vector_mean (float32 *mean,	/* Out: Computed mean; caller should allocate
					   this memory */
		   float32 **data,	/* In: Set of data over which mean is computed */
		   int32 n_vec,		/* In: #Vectors in data */
		   int32 n_dim);	/* In: Dimensionality of each vector (and mean) */

/*
 * Return the Euclidean distance (actually, distance^2) between the two given vectors.
 */
float64 vector_dist_eucl (float32 *v1, float32 *v2,	/* In: The two input vectors */
			  int32 len);	/* In: Length of each vector */

/*
 * Given a Gaussian density with mean vector m and diagonal variance vector v, and some
 * input vector x, all of length n, the Mahalanobis distance of x from the Gaussian mean m
 * is given by:
 *     {1/sqrt((2pi)^n * det(v))} * exp{-Sum((x[i] - m[i])^2 / (2v[i]))}
 * To speed up this evaluation, the first sub-expression ({1/sqrt...}) can be precomputed at
 * initialization, and so can 1/2v[i] in the second sub-expression.  Secondly, recognition
 * systems work with log-likelihood values, so these distances or likelihood values are
 * computed in log-domain.
 * 
 * Carry out the above precomputations on the given variance vector; update var with 1/2var IN
 * PLACE, and return the value of the first sub-expression as a log value.  (The variance values
 * are assumed to be strictly > 0.0.
 */
float64
vector_maha_precomp (float32 *var,	/* In/Out: Variance vector, modified to 1/2var */
		     int32 len);	/* In: Length of the above vector */

/*
 * Mahalanobis distance, using some precomputed portions of variance and determinant.
 * (Actually not "distance" at all, but Gaussian density value.)
 * Computed in log domain.
 * Return value: logbase-e(likelihood value).
 */
float64
vector_dist_maha (float32 *vec,		/* In: Vector for which distance from mean
					   being computed */
		  float32 *mean,	/* In: Mean vector */
		  float32 *varinv,	/* In: Variance vector, modified according to
					   vector_maha_precomp() */
		  float64 norm,		/* In: Normalization factor computed by
					   vector_maha_precomp() */
		  int32 len);		/* In: Length of each vector involved */

/*
 * Generate a vector quantized table for the input data, using the K-means algorithm, and the
 * Euclidean distance measure.
 * Return value: The squared quantization error of the final result.
 */
float64 vector_vqgen (float32 **data,	/* In: Input Data to be quantized */
		      int32 rows,	/* In: #Rows in input data */
		      int32 cols,	/* In: #Cols in input data (and output VQ table) */
		      int32 vqrows,	/* In: #Rows in vector quantized result */
		      float64 epsilon,	/* In: Convergence limit; stop k-means iterations when
					   the relative reduction in total squared error in
					   successive iterations ((sqerr[t]-sqerr[t+1])/sqerr[t])
					   is below this value. */
		      int32 maxiter,	/* In: Max #iterations, regardless of convlimit */
		      float32 **mean,	/* Out: Vector quantized array of centroids.  Caller
					   must allocate this array */
		      int32 *map);	/* Out: data->mean mapping; mean[map[i]] is the closest
					   quantized vector corresponding to the input vector
					   data[i].  Caller must allocate this array */

/*
 * Find the entry (row) of mean that is closest (according to Euclidean distance) to vec.
 * Return value: index of the selected row.
 */
int32 vector_vqlabel (float32 *vec,	/* In: Vector to be compared against VQ table */
		      float32 **mean,	/* In: Table of centroids */
		      int32 rows,	/* In: #Rows in mean */
		      int32 cols,	/* In: #Cols in mean (and vec) */
		      float64 *sqerr);	/* Out: Squared error (distance) between vec and the
					   selected codeword.  May be NULL if not needed. */

/*
 * Treat the given vector as a PDF and compute its entropy (-sum(p * log2(p))).  (The PDF values
 * must be >= 0.0, of course.)  The zero elements of the PDF are skipped.
 */
float64 vector_pdf_entropy (float32 *p,	/* In: Vector (PDF) whose entropy is to be computed */
			    int32 len);	/* In: Length of p[] */

/*
 * Return cross-entropy of two PDFs; like vector_entropy, but (-sum(p1 * log2(p2))).  (The PDF
 * values must be >= 0.0, of course.)  The zero elements of p2 are skipped.
 */
float64 vector_pdf_cross_entropy (float32 *p1,
				  float32 *p2,	/* In: Zero elements of p2 are skipped */
				  int32 len);

/*
 * Generic table of individual Gaussian vectors, with diagonal co-variance matrices (i.e.,
 * only the diagonal is maintained, as a vector).
 */
typedef struct {
    int32 n_gau;	/* #Gaussians in table */
    int32 veclen;	/* Vector length */
    float32 **mean;	/* n_cw x veclen mean values */
    float32 **var;	/* n_cw x veclen corresponding (diagonal) variance values */
    float32 *lrd;	/* Log(Reciprocal(Determinant(Co-var. matrix))) */
    float64 distfloor;	/* When Mahalanobis distance values are converted to logs3 domain, the
			   int32 result can underflow.  To avoid this, floor before conversion */
} vector_gautbl_t;


/*
 * The obvious allocation and free routines for vector_gautbl_t.{mean,var,lrd}.  Also sets
 * gautbl->{n_mgau, veclen, distfloor}; the last to the equivalent of S3_LOGPROB_ZERO.
 */
void vector_gautbl_alloc (vector_gautbl_t *gautbl,
			  int32 n_gau, int32 veclen);
void vector_gautbl_free (vector_gautbl_t *gautbl);


/*
 * Floor all the variance values in the given Gaussians table.
 */
void vector_gautbl_var_floor (vector_gautbl_t *gautbl,	/* In/Out: Table to be updated */
			      float64 floor);		/* In: Floor value */

/*
 * Carry out the vector_maha_precomp, above, over all the Gaussians in the given table, updating
 * gau->lrd[] as a result.  The needed memory must have been already allocated by the caller.
 */
void vector_gautbl_maha_precomp (vector_gautbl_t *gau);


/*
 * Compare the input vector to the given range of subvector codewords in the given codebook,
 * Enter the computed Mahalanobis distances, converted into logs3 values, into the given score
 * array.  (Why bother with the {offset,count} range??  For "blocking" such computation so that
 * cache behaviour is improved.)
 */
void
vector_gautbl_eval_logs3 (vector_gautbl_t *gau,	/* In: Table of Gaussians */
			  int32 offset,	/* In: First VQ codeword in the range to be evaluated */
			  int32 count,	/* In: #Codewords to be evaluated (range size) */
			  float32 *x,	/* In: Input vector being compared to codewords */
			  int32 *scr);	/* Out: Mahalanobis distance scores (logs3 values).
					   Caller must allocate this array.  Note that only
					   score[offset..offset+count-1] are updated. */

#endif /* VECTOR_H */ 


/*
 * Log record.  Maintained by RCS.
 *
 * Revision 1.1.1.1  2004/03/29 20:29:40  rkm
 *
 *
 * Revision 1.1.1.1  2004/03/05 16:55:44  rkm
 *
 *
 * Revision 1.1.1.1  2000/02/28 18:05:54  rkm
 * Imported Sources
 *
 * Revision 1.1.1.1  1999/11/23 20:24:18  rkm
 * imported sources
 *
 * Revision 1.2  1995/10/09  20:55:35  eht
 * Changes for prim_type.h
 *
 * Revision 1.1  1995/08/15  13:44:14  eht
 * Initial revision
 *
 *
 */
