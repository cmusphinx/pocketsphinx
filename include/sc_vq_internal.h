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


#ifndef __S2_SCVQ_INTERNAL_H__
#define __S2_SCVQ_INTERNAL_H__		1


#define NUM_ALPHABET	256
/* centered 5 frame difference of c[0]...c[12] with centered 9 frame
 * of c[1]...c[12].  Don't ask me why it was designed this way! */
#define DCEP_VECLEN	25
#define DCEP_LONGWEIGHT	0.5
#define POW_VECLEN	3	/* pow, diff pow, diff diff pow */
#define DEF_VAR_FLOOR	0.00001

#define MAX_DIFF_WINDOW	9	/* # frames include cur */

#define INPUT_MASK	0xf
#define DIFF_MASK	0x7

#define INDEX(x,m) ((x)&(m))	/* compute new circular buff index */

#define INPUT_INDEX(x) INDEX(x,INPUT_MASK)
#define DIFF_INDEX(x) INDEX(x,DIFF_MASK)

typedef struct {
  union {
    int32	score;
    int32	dist;	/* distance to next closest vector */
  } val;
  int32 codeword;		/* codeword (vector index) */
} vqFeature_t;
typedef vqFeature_t *vqFrame_t;

typedef enum {SCVQ_HEAD, SCVQ_TAIL, SCVQ_DEQUEUE} ht_t;

#define FBUFMASK	0x3fff	/* (# frames)-1 of topN features to keep in circ buff */

#define WORST_DIST	(int32)(0x80000000)
#define BTR		>
#define NEARER		>
#define WORSE		<
#define FARTHER		<

#ifdef FIXED_POINT
/** Subtract GMM component b (assumed to be positive) and saturate */
#define GMMSUB(a,b) \
	(((a)-(b) > a) ? (INT_MIN) : ((a)-(b)))
/** Add GMM component b (assumed to be positive) and saturate */
#define GMMADD(a,b) \
	(((a)+(b) < a) ? (INT_MAX) : ((a)+(b)))
#else
#define GMMSUB(a,b) ((a)-(b))
#define GMMADD(a,b) ((a)+(b))
#endif

#endif
