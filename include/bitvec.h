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
 * bitvec.h -- Bit vector type.
 *
 * **********************************************
 * CMU ARPA Speech Project
 *
 * Copyright (c) 1999 Carnegie Mellon University.
 * ALL RIGHTS RESERVED.
 * **********************************************
 * 
 * HISTORY
 * 
 * $Log: bitvec.h,v $
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
 * 13-Sep-1999	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon
 * 		Added bitvec_uint32size().
 * 
 * 05-Mar-1999	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon
 * 		Added bitvec_count_set().
 * 
 * 17-Jul-97	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon
 * 		Created.
 */


#ifndef __BITVEC_H__
#define __BITVEC_H__


#include "s2types.h"
#include "ckd_alloc.h"


typedef uint32 *bitvec_t;

/*
 * NOTE: The following definitions haven't been designed for arbitrary usage!!
 */

/* No. of uint32 words allocated to represent a bitvector of the given size n */
#define bitvec_uint32size(n)	(((n)+31)>>5)

#define bitvec_alloc(n)		((bitvec_t) ckd_calloc (((n)+31)>>5, sizeof(uint32)))

#define bitvec_free(v)		ckd_free((char *)(v))

#define bitvec_set(v,b)		(v[(b)>>5] |= (1 << ((b) & 0x001f)))

#define bitvec_clear(v,b)	(v[(b)>>5] &= ~(1 << ((b) & 0x001f)))

#define bitvec_clear_all(v,n)	memset(v, 0, (((n)+31)>>5)*sizeof(uint32))

#define bitvec_is_set(v,b)	(v[(b)>>5] & (1 << ((b) & 0x001f)))

#define bitvec_is_clear(v,b)	(! (bitvec_is_set(v,b)))


/*
 * Return the number of bits set in the given bit-vector.
 */
int32 bitvec_count_set (bitvec_t vec,	/* In: Bit vector to search */
			int32 len);	/* In: Lenght of above bit vector */

#endif
