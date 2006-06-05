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
 * allphone-test.c -- Test of a simple driver for allphone decoding.o
 * 
 * HISTORY
 *
 * 08-Feb-2000  Kevin Lenzo <lenzo@cs.cmu.edu> at Carnegie Mellon University
 *              Changed wording and routine name.
 * 
 * 
 * 11-Sep-1998	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Started.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "s2types.h"
#include "fbs.h"

/*
 * This application reads in filenames, one at a time, and performs allphone
 * recognition on each.  The result is written to stdout.  (But note that the
 * recognition engine also writes a log to stdout.  So a real application might
 * prefer to write the results to a file instead.)
 */

int
main (int argc, char *argv[])
{
    char utt[4096];
    search_hyp_t *h;
    
    /* argc, argv: The usual argument for batchmode allphone decoding. */
    fbs_init (argc, argv);

    for (;;) {
	printf ("Audio filename (without extension): ");
	if (scanf ("%s", utt) != 1)
	    break;

	printf ("%s:\n", utt);

	h = uttproc_allphone_file (utt);
	
	for (; h; h = h->next)
	    printf ("%4d %4d %s\n", h->sf, h->ef, h->word);
	printf ("\n");
    }
    
    fbs_end();
    return 0;
}

