/* ====================================================================
 * Copyright (c) 2005 Carnegie Mellon University.  All rights 
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
 * File: fixlog.c
 *
 * Description: Fast approximate fixed-point logarithms
 *
 * Author: David Huggins-Daines <dhuggins@cs.cmu.edu>
 *
 */
#include "s2types.h"
#include "fixpoint.h"
#include "fe.h"
#include "fe_internal.h"

/* Table of log2(x/64)*(1<<DEFAULT_RADIX) */
/* perl -e 'for (0..63) {my $x = 1 + $_/64; print "\t(uint32)(", log($x)/log(2), "*(1<<DEFAULT_RADIX)),\n"}' */
static uint32 logtable[] = {
	(uint32)(0*(1<<DEFAULT_RADIX)),
	(uint32)(0.0223678130284545*(1<<DEFAULT_RADIX)),
	(uint32)(0.0443941193584534*(1<<DEFAULT_RADIX)),
	(uint32)(0.0660891904577724*(1<<DEFAULT_RADIX)),
	(uint32)(0.0874628412503394*(1<<DEFAULT_RADIX)),
	(uint32)(0.108524456778169*(1<<DEFAULT_RADIX)),
	(uint32)(0.129283016944966*(1<<DEFAULT_RADIX)),
	(uint32)(0.149747119504682*(1<<DEFAULT_RADIX)),
	(uint32)(0.169925001442312*(1<<DEFAULT_RADIX)),
	(uint32)(0.189824558880017*(1<<DEFAULT_RADIX)),
	(uint32)(0.20945336562895*(1<<DEFAULT_RADIX)),
	(uint32)(0.228818690495881*(1<<DEFAULT_RADIX)),
	(uint32)(0.247927513443586*(1<<DEFAULT_RADIX)),
	(uint32)(0.266786540694901*(1<<DEFAULT_RADIX)),
	(uint32)(0.285402218862248*(1<<DEFAULT_RADIX)),
	(uint32)(0.303780748177103*(1<<DEFAULT_RADIX)),
	(uint32)(0.321928094887362*(1<<DEFAULT_RADIX)),
	(uint32)(0.339850002884625*(1<<DEFAULT_RADIX)),
	(uint32)(0.357552004618084*(1<<DEFAULT_RADIX)),
	(uint32)(0.375039431346925*(1<<DEFAULT_RADIX)),
	(uint32)(0.39231742277876*(1<<DEFAULT_RADIX)),
	(uint32)(0.409390936137702*(1<<DEFAULT_RADIX)),
	(uint32)(0.426264754702098*(1<<DEFAULT_RADIX)),
	(uint32)(0.442943495848728*(1<<DEFAULT_RADIX)),
	(uint32)(0.459431618637297*(1<<DEFAULT_RADIX)),
	(uint32)(0.475733430966398*(1<<DEFAULT_RADIX)),
	(uint32)(0.491853096329675*(1<<DEFAULT_RADIX)),
	(uint32)(0.507794640198696*(1<<DEFAULT_RADIX)),
	(uint32)(0.523561956057013*(1<<DEFAULT_RADIX)),
	(uint32)(0.539158811108031*(1<<DEFAULT_RADIX)),
	(uint32)(0.554588851677637*(1<<DEFAULT_RADIX)),
	(uint32)(0.569855608330948*(1<<DEFAULT_RADIX)),
	(uint32)(0.584962500721156*(1<<DEFAULT_RADIX)),
	(uint32)(0.599912842187128*(1<<DEFAULT_RADIX)),
	(uint32)(0.614709844115208*(1<<DEFAULT_RADIX)),
	(uint32)(0.62935662007961*(1<<DEFAULT_RADIX)),
	(uint32)(0.643856189774725*(1<<DEFAULT_RADIX)),
	(uint32)(0.658211482751795*(1<<DEFAULT_RADIX)),
	(uint32)(0.672425341971496*(1<<DEFAULT_RADIX)),
	(uint32)(0.686500527183218*(1<<DEFAULT_RADIX)),
	(uint32)(0.700439718141092*(1<<DEFAULT_RADIX)),
	(uint32)(0.714245517666123*(1<<DEFAULT_RADIX)),
	(uint32)(0.727920454563199*(1<<DEFAULT_RADIX)),
	(uint32)(0.741466986401147*(1<<DEFAULT_RADIX)),
	(uint32)(0.754887502163469*(1<<DEFAULT_RADIX)),
	(uint32)(0.768184324776926*(1<<DEFAULT_RADIX)),
	(uint32)(0.78135971352466*(1<<DEFAULT_RADIX)),
	(uint32)(0.794415866350106*(1<<DEFAULT_RADIX)),
	(uint32)(0.807354922057604*(1<<DEFAULT_RADIX)),
	(uint32)(0.820178962415188*(1<<DEFAULT_RADIX)),
	(uint32)(0.832890014164742*(1<<DEFAULT_RADIX)),
	(uint32)(0.845490050944375*(1<<DEFAULT_RADIX)),
	(uint32)(0.857980995127572*(1<<DEFAULT_RADIX)),
	(uint32)(0.870364719583405*(1<<DEFAULT_RADIX)),
	(uint32)(0.882643049361841*(1<<DEFAULT_RADIX)),
	(uint32)(0.894817763307944*(1<<DEFAULT_RADIX)),
	(uint32)(0.906890595608518*(1<<DEFAULT_RADIX)),
	(uint32)(0.918863237274595*(1<<DEFAULT_RADIX)),
	(uint32)(0.930737337562886*(1<<DEFAULT_RADIX)),
	(uint32)(0.94251450533924*(1<<DEFAULT_RADIX)),
	(uint32)(0.954196310386875*(1<<DEFAULT_RADIX)),
	(uint32)(0.965784284662087*(1<<DEFAULT_RADIX)),
	(uint32)(0.977279923499917*(1<<DEFAULT_RADIX)),
	(uint32)(0.988684686772166*(1<<DEFAULT_RADIX)),
};

int32 fixlog2(uint32 x)
{
	uint32 y;

	if (x == 0)
		return INT_MIN;

	/* Get the exponent. */
#ifdef __i386__
	asm ("bsrl %1, %0\n"
	     : "=r" (y) : "g" (x) : "cc");
	x <<= (31-y);
#elif defined(__ppc__)
	asm ("cntlzw %0, %1\n"
	     : "=r" (y) : "r" (x));
	x <<= y;
	y = 31-y;
#elif defined(XSCALE)
	asm ("clz %0, %1\n"
	     : "=r" (y) : "r" (x));
	x <<= y;
	y = 31-y;
#else
	for (y = 31; y >= 0; --y) {
		if (x & 0x80000000)
			break;
		x <<= 1;
	}
#endif
	y <<= DEFAULT_RADIX;
	/* Do a table lookup for the MSB of the mantissa. */
	x = (x >> 25) & 0x3f;
	return y + logtable[x];
}

int fixlog(uint32 x)
{
	int32 y;

	y = fixlog2(x);
	return FIXMUL(y, FIXLN_2);
}
