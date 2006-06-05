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

/*
 * HISTORY
 *
 * $Log: strcasecmp.c,v $
 * Revision 1.1.1.1  2006/05/23 18:45:01  dhuggins
 * re-importation
 *
 * Revision 1.7  2004/12/10 16:48:57  rkm
 * Added continuous density acoustic model handling
 *
 * 
 * 14-Oct-92 Eric Thayer (eht+@cmu.edu) Carnegie Mellon University
 *	added formal declarations for a and
 * 
 * 14-Oct-92 Eric Thayer (eht+@cmu.edu) Carnegie Mellon University
 *	installed ulstrcmp() for strcasecmp() because DEC alpha for some reason
 *	seg faults on call to strcasecmp(). (OSF/1 BL8)
 */

#include "strfuncs.h"

/*
 * case INSENSITIVE.
 * Compare strings:  s1>s2: >0  s1==s2: 0  s1<s2: <0
 */

static int
ulstrcmp(char const *str1, char const *str2)
{
    register char c1,c2;

    for(;;) {
	c1 = *str1++;
	if (c1 <= 'Z')
	    if (c1 >= 'A')
		c1 += 040;
	c2 = *str2++;
	if (c2 <= 'Z')
	    if (c2 >= 'A')
		c2 += 040;
	if (c1 != c2)
	    break;
	if (c1 == '\0')
	    return(0);
    }
    return(c1 - c2);
}

int
mystrcasecmp (char const *a, char const *b)
{
    if (a && b)
	return ulstrcmp(a,b);
    else
    	return 1;
}
