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

#include <stdio.h>
#include "s2types.h"
#include "byteorder.h"
#include "err.h"

void swapLong(intp)
/*------------------------------------------------------------*
 * Swap the int32 integer at intp
 */
int32 *intp;
{
  *intp = ((*intp << 24) & 0xFF000000) |
	  ((*intp <<  8) & 0x00FF0000) |
	  ((*intp >>  8) & 0x0000FF00) |
	  ((*intp >> 24) & 0x000000FF);
}

void swapShortBuf (p, cnt)
int16 *p;
int32 cnt;
{
    while (cnt-- > 0) {
	*p = ((*p << 8) & 0x0FF00) |
	     ((*p >>  8) & 0x00FF);
	++p;	/* apollo compiler seems to break with *p++ = f(*p) */
    }
}

void swapLongBuf (p, cnt)
int32 *p;
int32 cnt;
{
    while (cnt-- > 0) {
	*p = ((*p << 24) & 0xFF000000) |
	     ((*p <<  8) & 0x00FF0000) |
	     ((*p >>  8) & 0x0000FF00) |
	     ((*p >> 24) & 0x000000FF);
	++p;	/* apollo compiler seems to break with *p++ = f(*p) */
    }
}


int fread_int32(FILE *fp, int min, int max, char const *name, int do_swap, int *inout_swap)
{
    int k;
    
    if (fread (&k, sizeof (int), 1, fp) != 1)
	E_FATAL("fread(%s) failed\n", name);
    /* Swap unconditionally, according to the value in inout_swap if it exists */
    if (do_swap) {
	if (inout_swap == NULL || *inout_swap)
	    SWAP_INT32(&k);
    }
    /* Otherwise use min and max to determine whether to swap, and set *inout_swap */
    else {
	if ((min > k) || (max < k)) {
	    E_INFO("%s = 0x%08x outside range[%d,%d], assuming byteswapped\n",
		   name, k, min, max);
	    SWAP_INT32(&k);
	    if (inout_swap)
		*inout_swap = 1;
	}
	else {
	    if (inout_swap)
		*inout_swap = 0;
	}
    }
    /* Now check the range of the (byteswapped) value */
    if ((min > k) || (max < k))
	E_FATAL("%s outside range [%d,%d]\n", name, min, max);
    return (k);
}

void
fwrite_int32 (FILE *fp, int val)
{
    fwrite (&val, sizeof(int), 1, fp);
}
