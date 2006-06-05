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
 * logmsg.c - verbosity-controlled internal log messages.
 * 
 * HISTORY
 * 
 * $Log: logmsg.c,v $
 * Revision 1.1.1.1  2006/05/23 18:45:01  dhuggins
 * re-importation
 *
 * Revision 1.7  2004/12/10 16:48:56  rkm
 * Added continuous density acoustic model handling
 *
 */


#include <stdio.h>
#include <stdarg.h>

#include "s2types.h"
extern int32 verbosity_level;

#define INST_LOGGER(name, fh, level)		\
void name (char const *fmt, ...)		\
{						\
    va_list args;				\
						\
    va_start(args, fmt);			\
    if (verbosity_level >= level) {		\
	vfprintf(fh, fmt, args);		\
	fflush(fh);				\
    }						\
    va_end(args);				\
}

/* XXX: yeah, maybe these should be macros in the header file that
   call a common function, however it might be useful to have them do
   different things in the future. */
INST_LOGGER(log_error, stderr, 0)
INST_LOGGER(E_WARN, stderr, 1)
INST_LOGGER(E_INFO, stdout, 2)
INST_LOGGER(log_debug, stdout, 3)

/*
 * Local variables:
 *  c-basic-offset: 4
 *  c-indentation-style: "BSD"
 * End:
 */
