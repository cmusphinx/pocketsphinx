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
 *  nxtarg -- strip off arguments from a string
 *
 *  Usage:  p = nxtarg (&q,brk);
 *	char *p,*q,*brk;
 *	extern char _argbreak;
 *
 *	q is pointer to next argument in string
 *	after call, p points to string containing argument,
 *	q points to remainder of string
 *
 *  Leading blanks and tabs are skipped; the argument ends at the
 *  first occurence of one of the characters in the string "brk".
 *  When such a character is found, it is put into the external
 *  variable "_argbreak", and replaced by a null character; if the
 *  arg string ends before that, then the null character is
 *  placed into _argbreak;
 *  If "brk" is 0, then " " is substituted.
 *
 *  HISTORY
 * 01-Jul-83  Steven Shafer (sas) at Carnegie-Mellon University
 *	Bug fix: added check for "back >= front" in loop to chop trailing
 *	white space.
 *
 * 20-Nov-79  Steven Shafer (sas) at Carnegie-Mellon University
 *	Rewritten for VAX.  By popular demand, a table of break characters
 *	has been added (implemented as a string passed into nxtarg).
 *
 *  Originally	from klg (Ken Greer); IUS/SUS UNIX.
 */

#include <stdlib.h>
#include "strfuncs.h"

#ifndef NULL
#define NULL (void *)0
#endif

static char _argbreak;

char *
nxtarg (char **q, char const *brk)
{
	register char *front;
	register char *back;
	front = *q;			/* start of string */
	/* leading blanks and tabs */
	while (*front && (*front == ' ' || *front == '\t')) front++;
	/* find break character at end */
	if (brk == NULL)  brk = " ";
	back = skipto (front,brk);
	_argbreak = *back;
	*q = (*back ? back+1 : back);	/* next arg start loc */
	/* elim trailing blanks and tabs */
	back -= 1;
	while ((back >= front) && (*back == ' ' || *back == '\t')) back--;
	back++;
	if (*back)  *back = '\0';
	return (front);
}
