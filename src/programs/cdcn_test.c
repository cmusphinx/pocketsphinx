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
 * cdcn_test.c -- Main program to test CDCN functionality.
 * 
 * HISTORY
 * 
 * 05-Jan-1999	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Started.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "s2types.h"
#include "fbs.h"
#include "fe.h"
#include "cdcn.h"

#include "ckd_alloc.h"
#include "err.h"

extern char *build_uttid (const char *utt);  
extern int32 uttproc_parse_ctlfile_entry (char *line,
					    char *filename,
					    int32 *sf, int32 *ef,
					    char *idspec);
extern int32 query_sampling_rate ( void );
extern int32 query_adc_input ( void );
extern void uttfile_close ( void );

int main (int32 argc, char *argv[])
{
    char line[4096], filename[4096], idspec[4096], *uttid, *result;
    int32 sf, ef, sps, adcin, nf;
    int16 adbuf[4096];
    int32 i, k;
    float32 **mfcbuf;
    CDCN_type *cdcn;
    param_t param;
    fe_t *fe = NULL;


    fbs_init (argc, argv);     
    /* Assume that cdcn_init is part of the above fbs_init() */
    cdcn = uttproc_get_cdcn_ptr();
    
    adcin = query_adc_input();
    assert (adcin);	/* Limited to processing audio input files (not cep) */
    sps = query_sampling_rate();

    memset(&param, 0, sizeof(param));
    param.SAMPLING_RATE = (float)sps;

    if ((fe = fe_init (&param)) == NULL)
    {
        E_ERROR("fe_init() failed to initialize\n");
        exit (-1);
    }
    mfcbuf = (float32 **) ckd_calloc_2d (8192, 13, sizeof(float32));
    
    /* Process "control file" input through stdin */
    while (fgets (line, sizeof(line), stdin) != NULL) {
	if (uttproc_parse_ctlfile_entry (line, filename, &sf, &ef, idspec) < 0)
	    continue;
	assert ((sf < 0) && (ef < 0));	/* Processing entire input file */

	uttid = build_uttid (idspec);

	uttproc_begin_utt (uttid);
	
	/* Convert raw data file to cepstra */
	if (uttfile_open (filename) < 0) {
	    E_ERROR("uttfile_open(%s) failed\n", filename);
	    continue;
	}
	fe_start_utt(fe);
	nf = 0;
	while ((k = adc_file_read (adbuf, 4096)) >= 0) {
	    k = fe_process_utt (fe, adbuf, k, mfcbuf+nf);
	    nf += k;
	    /* WARNING!! No check for mfcbuf overflow */
	}
	fe_end_utt(fe, mfcbuf[nf]);
	fe_close(fe);
	uttfile_close ();
	
	if (nf <= 0) {
	    E_ERROR("Empty utterance\n");
	    continue;
	} else
	    E_INFO("%d frames\n", nf);
	
	/* Update CDCN module */
	cdcn_converged_update (mfcbuf, /* cepstra buffer */
			       nf, /* Number of frames */
			       cdcn, /* The CDCN wrapper */
			       1 /* One iteration */
			       );

	/* CDCN */
	for (i = 0; i < nf; i++)
	    cdcn_norm (mfcbuf[i], cdcn);

	/* Process normalized cepstra */
	uttproc_cepdata (mfcbuf, nf, 1);
	uttproc_end_utt ();
	uttproc_result (&k, &result, 1);
	printf ("\n");
	fflush (stdout);

    }

    ckd_free_2d((void **)mfcbuf);
    
    fbs_end ();
    return 0;
}
