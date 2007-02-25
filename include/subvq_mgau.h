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
 * subvq.h
 * 
 * **********************************************
 * CMU ARPA Speech Project
 *
 * Copyright (c) 1999 Carnegie Mellon University.
 * ALL RIGHTS RESERVED.
 * **********************************************
 * 
 * HISTORY
 * $Log$
 * Revision 1.1  2006/04/05  20:27:30  dhdfu
 * A Great Reorganzation of header files and executables
 * 
 * Revision 1.14  2006/02/22 17:43:32  arthchan2003
 * Merged from SPHINX3_5_2_RCI_IRII_BRANCH:
 * 1, vector_gautbl_free is not appropiate to be used in this case because it will free a certain piece of memory twice.
 * 2, Fixed dox-doc.
 *
 * Revision 1.13.4.1  2005/07/05 05:47:59  arthchan2003
 * Fixed dox-doc. struct level of documentation are included.
 *
 * Revision 1.13  2005/06/21 19:01:33  arthchan2003
 * Added $ keyword.
 *
 * Revision 1.3  2005/03/30 01:22:47  archan
 * Fixed mistakes in last updates. Add
 *
 * 
 * 20.Apr.2001  RAH (rhoughton@mediasite.com, ricky.houghton@cs.cmu.edu)
 *              Updated subvq_free () to free more allocated memory
 * 
 * 15-Dec-1999	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Moved subvq_t.{frm_sen_eval,frm_gau_eval} to cont_mgau.h.
 * 
 * 14-Dec-1999	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Added subvq_t.{frm_sen_eval,frm_gau_eval}.  Changed subvq_frame_eval to
 * 		return the normalization factor.
 * 
 * 06-Dec-1999	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Added subvq_subvec_eval_logs3().
 * 
 * 14-Oct-1999	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Changed ci_active flags input to sen_active in subvq_frame_eval().
 * 
 * 20-Jul-1999	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Added subvq_gautbl_eval_logs3().
 * 
 * 12-Mar-1999	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Started.
 */


#ifndef _SUBVQ_MGAU_H_
#define _SUBVQ_MGAU_H_

#include "sphinx_types.h"
#include "vector.h"
#include "log.h"

/** \file subvq.h
    \brief Implementation of Sub-vector quantization.
*/
#ifdef __cplusplus
extern "C" {
#endif
#if 0
} /* Fool Emacs into not indenting things. */
#endif

/**
 * Generic table of individual Gaussian vectors, with diagonal co-variance matrices (i.e.,
 * only the diagonal is maintained, as a vector).
 */
typedef struct vector_gautbl_s {
    int32 n_gau;	/**< #Gaussians in table */
    int32 veclen;	/**< Vector length */
    mean_t **mean;	/**< n_cw x veclen mean values */
    var_t **var;	/**< n_cw x veclen corresponding (diagonal) variance values */
    int32 *lrd;		/**< Log(Reciprocal(Determinant(Co-var. matrix))) */
} vector_gautbl_t;

/** \struct subvq_mgau_t
 * \brief SubVQ-based acoustic model object
 */
typedef struct subvq_mgau_s {
    int32 n_mgau;		/**< #codebooks (= senones) in original model */
    int32 n_density;		/**< #codewords (= densities) in original model */
    int32 n_sv;			/**< #Subvectors */
    int32 vqsize;		/**< #Codewords in each subvector quantized mean/var table */
    int32 **featdim;		/**< featdim[s] = Original feature dimensions in subvector s */
    vector_gautbl_t *gautbl;	/**< Vector-quantized Gaussians table for each sub-vector */
    int32 ***map;		/**< map[i][j] = map from original codebook(i)/codeword(j) to
				   sequence of nearest vector quantized subvector codewords;
				   so, each map[i][j] is of length n_sv.  Finally, map is
				   LINEARIZED, so that it indexes into a 1-D array of scores
				   rather than a 2-D array (for faster access). */ 
    int32 **mixw;		/**< Mixture weight array. */
    /* Working space used during evaluation. */
    mfcc_t *subvec;		/**< Subvector extracted from feature vector */
    int32 **vqdist;		/**< vqdist[i][j] = score (distance) for i-th subvector compared
				   to j-th subvector-codeword */
} subvq_mgau_t;


/**
 * SubVQ file format:
 *   VQParam #Original-Codebooks #Original-Codewords/codebook(max) -> #Subvectors #VQ-codewords
 *   Subvector 0 length <length> <feature-dim> <feature-dim> <feature-dim> ...
 *   Subvector 1 length <length> <feature-dim> <feature-dim> <feature-dim> ...
 *   ...
 *   Codebook 0
 *   Row 0 of mean/var values (interleaved) for subvector 0 codebook (in 1 line)
 *   Row 1 of above
 *   Row 2 of above
 *   ...
 *   Map 0
 *   Mappings for state 0 codewords (in original model) to codewords of this subvector codebook
 *   Mappings for state 1 codewords (in original model) to codewords of this subvector codebook
 *   Mappings for state 2 codewords (in original model) to codewords of this subvector codebook
 *   ...
 *   Repeated for each subvector codebook 1
 *   Repeated for each subvector codebook 2
 *   ...
 *   End
 *   @return initialized sub-vq
 */
subvq_mgau_t *subvq_mgau_init(const char *subvq_file,  /**< In: SubVQ Gaussian file */
                              float64 varfloor,  /**< In: Floor to be applied to variance values */
                              const char *mixw_file,   /**< In: Mixture weight file */
                              float64 mixwfloor, /**< In: Floor to be applied to variance values */
                              int32 topn         /**< UNUSED */
    );



/** Deallocate sub-vector quantization */
void subvq_mgau_free(subvq_mgau_t *vq /**< In: A sub-vector model */
    );


/**
 * Evaluate senone scores for one frame using a subvq model, scale
 * senone scores by subtracting the best.
 * @return The normalization factor (best senone absolute score).
 */
int32 subvq_mgau_frame_eval(subvq_mgau_t *vq,
                            mfcc_t **featbuf,
                            int32 frame,
                            int32 compallsen
    );	

#if 0
{ /* Stop indent from complaining */
#endif
#ifdef __cplusplus
}
#endif

#endif
