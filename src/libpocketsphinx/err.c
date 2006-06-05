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
 * Package for checking and catching common errors, printing out
 * errors nicely, etc.
 * 
 * $Log: err.c,v $
 * Revision 1.1.1.1  2006/05/23 18:44:59  dhuggins
 * re-importation
 *
 * Revision 1.10  2004/12/10 16:48:56  rkm
 * Added continuous density acoustic model handling
 *
 *
 * 6/01/95  Paul Placeway  CMU speech group
 *
 * 6/02/95  Eric Thayer
 *	- Removed non-ANSI expresssions.  I don't know of any non-ANSI
 *		holdouts left anymore. (DEC using -std1, HP using -Aa,
 *		Sun using gcc or acc.)
 *      - Removed the automatic newline at the end of the error message
 *	  as that all S3 error messages have one in them now.
 *	- Added an error message option that does a perror() call.
 */

#include <stdio.h>
#include <stdlib.h>
#if ((! WIN32) && (! _SGI_SOURCE))
#include <sys/errno.h>
#else
#include <errno.h>
#endif

#include "s2types.h"
#include "err.h"

extern int32 verbosity_level;

void
_E__pr_header(char const *f, long ln, char const *msg)
{
    if (verbosity_level < 1)
	    return;

    (void) fflush(stdout);
    (void) fprintf(stderr, "%s: \"%s\", line %ld: ", msg, f, ln);
}

void
_E__pr_info_header(char const *f, long ln, char const *msg)
{
    if (verbosity_level < 2)
	    return;

    (void) fflush(stdout);
    /* make different format so as not to be parsed by emacs compile */
    (void) fprintf(stderr, "%s: %s(%ld): ", msg, f, ln);
}

void
_E__pr_warn( char const *fmt, ... ) 
{
    va_list pvar;

    if (verbosity_level < 1)
	    return;

    va_start(pvar, fmt);
    (void) vfprintf(stderr, fmt, pvar);
    va_end(pvar);

    (void) fflush(stderr);
}

void
_E__pr_info( char const *fmt, ... ) 
{
    va_list pvar;

    if (verbosity_level < 2)
	    return;

    va_start(pvar, fmt);
    (void) vfprintf(stderr, fmt, pvar);
    va_end(pvar);

    (void) fflush(stderr);
}

void _E__die_error( char const *fmt, ... ) 
{
    va_list pvar;

    va_start(pvar, fmt);

    (void) vfprintf(stderr, fmt, pvar);
    (void) fflush(stderr);

    va_end(pvar);

    (void) fflush(stderr);
    
    exit (-1);
}

void _E__fatal_sys_error( char const *fmt, ... ) 
{
    va_list pvar;

    va_start(pvar, fmt);
    (void) vfprintf(stderr, fmt, pvar);
    va_end(pvar);

    putc(';', stderr);
    putc(' ', stderr);

    perror("");
    
    (void) fflush(stderr);

    exit(errno);
}

void _E__sys_error( char const *fmt, ... ) 
{
    va_list pvar;

    va_start(pvar, fmt);
    (void) vfprintf(stderr, fmt, pvar);
    va_end(pvar);

    putc(';', stderr);
    putc(' ', stderr);

    perror("");

    (void) fflush(stderr);
}

void _E__abort_error( char const *fmt, ... ) 
{
    va_list pvar;

    va_start(pvar, fmt);
    (void) vfprintf(stderr, fmt, pvar);
    va_end(pvar);

    (void) fflush(stderr);

    abort ();
}

#ifdef TEST
main()
{
    char const *two = "two";
    char const *three = "three";
    FILE *fp;

    E_WARN("this is a simple test\n");

    E_WARN("this is a test with \"%s\" \"%s\".\n", "two", "arguments");

    E_WARN("foo %d is bar\n", 5);

    E_WARN("bar is foo\n");

    E_WARN("one\n", two, three);

    E_INFO("Just some information you might find interesting\n");

    fp = fopen("gondwanaland", "r");
    if (fp == NULL) {
	E_SYSTEM("Can't open gondwanaland for reading");
    }
}
#endif /* TEST */
