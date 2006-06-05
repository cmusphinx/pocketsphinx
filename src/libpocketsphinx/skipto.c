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
/************************************************************************
 *  skipover and skipto -- skip over characters in string
 *
 *  Usage:	p = skipto (string,charset);
 *		p = skipover (string,charset);
 *
 *  char *p,*charset,*string;
 *
 *  Skipto returns a pointer to the first character in string which
 *  is in the string charset; it "skips until" a character in charset.
 *  Skipover returns a pointer to the first character in string which
 *  is not in the string charset; it "skips over" characters in charset.
 ************************************************************************
 *
 * HISTORY
 * 26-Jun-81  David Smith (drs) at Carnegie-Mellon University
 *	Skipover, skipto rewritten to avoid inner loop at expense of space.
 *
 * 20-Nov-79  Steven Shafer (sas) at Carnegie-Mellon University
 *	Skipover, skipto adapted for VAX from skip() and skipx() on the PDP-11
 *	(from Ken Greer).  The names are more mnemonic.
 *
 *	Sindex adapted for VAX from indexs() on the PDP-11 (thanx to Ralph
 *	Guggenheim).  The name has changed to be more like the index()
 *	and rindex() functions from Bell Labs; the return value (pointer
 *	rather than integer) has changed partly for the same reason,
 *	and partly due to popular usage of this function.
 */

#include "strfuncs.h"

static unsigned char tab[256] = {
	0};

char *
skipto (unsigned char *string, unsigned char const *charset)
{
	register unsigned char const *setp;
	register unsigned char *strp;

	tab[0] = 1;		/* Stop on a null, too. */
	for (setp=charset;  *setp;  setp++) tab[*setp]=1;
	for (strp=string;  tab[*strp]==0;  strp++)  ;
	for (setp=charset;  *setp;  setp++) tab[*setp]=0;
	return strp;
}

char *
skipover (unsigned char *string, unsigned char const *charset)
{
    register unsigned char const *setp;
    register unsigned char *strp;

	tab[0] = 0;		/* Do not skip over nulls. */
	for (setp=charset;  *setp;  setp++) tab[*setp]=1;
	for (strp=string;  tab[*strp];  strp++)  ;
	for (setp=charset;  *setp;  setp++) tab[*setp]=0;
	return strp;
}
