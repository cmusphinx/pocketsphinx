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

/* norm.c - feature normalization
 * 
 * HISTORY
 * 
 * $Log: norm.c,v $
 * Revision 1.1.1.1  2006/05/23 18:45:01  dhuggins
 * re-importation
 *
 * Revision 1.7  2004/12/10 16:48:56  rkm
 * Added continuous density acoustic model handling
 *
 * 
 * 20-Aug-98	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Extended normalization to C0.
 * 
 |------------------------------------------------------------*
 | $Header: /usr0/cvsroot/pocketsphinx/src/libpocketsphinx/norm.c,v 1.1.1.1 2006/05/23 18:45:01 dhuggins Exp $
 |------------------------------------------------------------*
 | Description
 |	norm_mean()	- compute the mean of the input vectors
 |			  and then subtract the mean from the
 |			  input vectors. Leave coefficient 0
 |			  untouched.
 */

#include <stdlib.h>

#include "s2types.h"
#include "fixpoint.h"

#ifdef DEBUG
#define dprintf(x)	printf x
#else
#define dprintf(x)
#endif

void
norm_mean(mfcc_t *vec,		/* the data */
	  int32	nvec,		/* number of vectors (frames) */
	  int32	veclen)		/* number of elements (coefficients) per vector */
{
    static mfcc_t     *mean = 0;
    mfcc_t              *data;
    int32               i, f;

    if (mean == 0)
	mean = (mfcc_t *) calloc (veclen, sizeof (mfcc_t));

    for (i = 0; i < veclen; i++)
	mean[i] = 0;

    /*
     * Compute the sum
     */
    for (data = vec, f = 0; f < nvec; f++, data += veclen) {
	for (i = 0; i < veclen; i++)
	    mean[i] += data[i];
    }

    /*
     * Compute the mean
     */
    dprintf(("Mean Vector\n"));
    for (i = 0; i < veclen; i++) {
	mean[i] /= nvec;
	dprintf(("[%d]%.3f, ", i, MFCC2FLOAT(mean[i])));
    }
    dprintf(("\n"));
    
    /*
     * Normalize the data
     */
    for (data = vec, f = 0; f < nvec; f++, data += veclen) {
	for (i = 0; i < veclen; i++)
	    data[i] -= mean[i];
    }
}
