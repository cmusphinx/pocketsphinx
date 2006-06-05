/* ====================================================================
 * Copyright (c) 1999-2001 Carnegie Mellon University.  All rights
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
 * cdcn.h -- From the robust speech group at CMU-ECE/SCS
 * 
 * **********************************************
 * CMU ARPA Speech Project
 *
 * Copyright (c) 1996 Carnegie Mellon University.
 * ALL RIGHTS RESERVED.
 * **********************************************
 * 
 * HISTORY
 * 
 * $Log: cdcn.h,v $
 * Revision 1.1.1.1  2006/05/23 18:45:02  dhuggins
 * re-importation
 *
 * Revision 1.6  2005/08/19 18:47:13  egouvea
 * Synchronized the CDCN with the robust version. Mainly, made the
 * feature dimension an argument, and not a hardwired constant.
 *
 * Revision 1.5  2004/12/10 16:48:55  rkm
 * Added continuous density acoustic model handling
 *
 * 
 * 19-Jun-96	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Copied from Uday Jain.  Added ifndef _CDCN_H_.
 */


#ifndef _CDCN_H_
#define _CDCN_H_


/*************************************************************************
 *
 * Defines all the numbers used in CDCN.
 * Also defines the suitcase structure that CDCN uses.
 *
 *************************************************************************/

#define NUM_COEFF		13	/* Number of cepstral coefficients */
#define SPEECH_THRESHOLD	1.0	/* Values in Np (1 Np = 4.34 dB) */
#define N			32	/* Parameters for FFT */
#define N2			(2 * N)
#define M			5       /* 2 ^ M = N, so M and N are tied */
#define OFFSET_LIKELIHOOD	4.0

/*
 * Since the following definitions are common, define them only if they
 * havent been defined already
 */
#ifndef TRUE
#define TRUE			1
#endif

#ifndef FALSE
#define FALSE			0
#endif

typedef struct suitcase
{
  float **means;		/* The means of the training speech distributions */
  float **variance;	/* Variance of speech codebook */
  float *probs;		/* Ratio of a-priori prob. of modes to mod variance */
  int    num_codes; 	/* Number of codewords in codebook */
  int    n_dim; 	/* Dimension of codewords in codebook */
  float  *noise;
  float  *tilt;
  float **corrbook;	
  int    firstcall;
  int    run_cdcn;
} CDCN_type;

int 	cdcn_init (char const *filename, CDCN_type *cdcn_variables);
float   cdcn_update (float **z, int num_frames, CDCN_type *cdcn_variables);
void    cdcn_norm (float *z, CDCN_type *cdcn_variables);
void    block_cdcn_norm (float **z,
			 int num_frames,
			 CDCN_type *cdcn_variabls);
void
cdcn_converged_update (float **z, int num_frames, CDCN_type *cdcn_variables,
		       int max_iteration);

CDCN_type *uttproc_get_cdcn_ptr ( void );

#endif /* _CDCN_H_ */
