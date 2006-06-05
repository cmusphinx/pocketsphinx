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
 * tty-ptt.c -- An example SphinxII client using the standard client libraries.
 * 
 * HISTORY
 *
 * 15-Jun-99    Kevin A. Lenzo (lenzo@cs.cmu.edu) at Carnegie Mellon University
 *              Added i386_linux and used ad_open_sps instead of ad_open
 * 
 * 01-Jun-96	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University.
 * 		Created.
 */

/*
 * This is a simple, tty-based example of a SphinxII client.  The user marks 
 * the beginning and end of each utterance by hitting the return (<CR>) key.
 * (On Windows NT systems, the user only marks the beginning; the system 
 * automatically stops listening after LISTENTIME seconds.)
 * 
 * Remarks:
 *   - Single-threaded implementation for portability.
 *   - Uses audio library; can be replaced with an equivalent custom library.
 */

#include <stdio.h>
#include <string.h>

#ifdef WIN32
#include <time.h>
#else
#include <sys/types.h>
#include <sys/time.h>
#endif

#include "s2types.h"
#include "err.h"
#include "ad.h"
#include "fbs.h"

#define LISTENTIME		5.0
#define SAMPLE_RATE             16000

static int32 last_fr;		/* Last frame for which partial result was reported */
static ad_rec_t *ad;

/* Function for reporting partial recognition result */
static void update_result ( void )
{
    int32 fr;
    char *hyp;
    
    if (uttproc_partial_result (&fr, &hyp) < 0)
	E_FATAL("uttproc_partial_result() failed\n");

    if ((fr >= 0) && (fr != last_fr)) {
	printf ("@Frm %d: %s\n", fr, hyp);
	fflush (stdout);
	last_fr = fr;
    }
}

/* Determine if the user has indicated end of utterance (keyboard hit at end of utt) */
static int32 speaking (int32 ns)
{
#ifdef WIN32
    return (ns > (LISTENTIME*DEFAULT_SAMPLES_PER_SEC)) ? 0 : 1;
#else
    /* ------------------- Unix ------------------ */
    /* Check for a keyboard hit, BUT NON-BLOCKING */
    fd_set readfds;
    struct timeval zero_tmo;
    int32 status;
    char line[1024];
    
    FD_ZERO(&readfds);
    FD_SET(0 /* stdin */, &readfds);
    zero_tmo.tv_sec = 0;
    zero_tmo.tv_usec = 0;
    
    status = (select(1, &readfds, NULL, NULL, &zero_tmo) <= 0);
    if (! status) {	/* Assume user typed something at the terminal to stop speaking */
	fgets (line, sizeof(line), stdin);

	if (line[0] == 'r') {
	    E_INFO("Stopping utt...\n");
	    uttproc_stop_utt ();
	    
	    E_INFO("Restarting utt...\n");
	    uttproc_restart_utt ();
	    
	    return 1;
	}
    }
    
    return (status);
#endif
}

static void ui_ready ( void )
{
#ifdef WIN32
    printf ("\nSystem will listen for ~ %.1f sec of speech\n", LISTENTIME);
    printf ("Hit <cr> before speaking: ");
#else
    printf ("\nHit <cr> BEFORE and AFTER speaking: ");
#endif
    fflush (stdout);
}

/* Main utterance processing loop: decode each utt */
static void utterance_loop()
{
    int32 fr;
    char *hyp;
    char line[1024];
    int16 adbuf[4096];
    int32 k;
    int32 ns;		/* #Samples read from audio in this utterance */
    int32 hwm;		/* High Water Mark: to know when to report partial result */
	int32 recording;
    
    for (;;) {		/* Loop for each new utterance */
	ui_ready ();

	fgets (line, sizeof(line), stdin);
	if ((line[0] == 'q') || (line[0] == 'Q'))
	    return;
	
	ad_start_rec(ad);	/* Start A/D recording for this utterance */
	recording = 1;

	ns = 0;
	hwm = 4000;	/* Next partial result reported after 4000 samples */
	last_fr = -1;	/* Frame count at last partial result reported */

	/* Begin utterance */
	if (uttproc_begin_utt (NULL) < 0)
	    E_FATAL("uttproc_begin_utt() failed\n");

	/* Send audio data to decoder until end of utterance */
	for (;;) {
	    /*
	     * Read audio data (NON-BLOCKING).  Use your favourite substitute here.
	     * NOTE: In our implementation, ad_read returns -1 upon end of utterance.
	     */
	    if ((k = ad_read (ad, adbuf, 4096)) < 0)
		break;

	    /* Send whatever data was read above to decoder */
	    uttproc_rawdata (adbuf, k, 0);
	    ns += k;

	    /* Time to report partial result? (every 4000 samples or 1/4 sec) */
	    if (ns > hwm) {
		update_result ();
		hwm = ns+4000;
	    }

	    /*
	     * Check for end of utterance indication from user.
	     * NOTE: Our way of finishing an utterance is to stop the A/D, but
	     * continue to read A/D data in order to empty system audio buffers.
	     * The ad_read function returns -1 when audio recording has been stopped
	     * and no more data is available, exiting this for-loop (see above).
	     * Other implementations can adopt a different approach; eg, exit the
	     * loop right here if (! speaking()).
	     */
	    if (recording && (! speaking(ns))) {
		ad_stop_rec(ad);
		E_INFO("A/D Stopped\n");
		recording = 0;
	    }
	}
	
	uttproc_end_utt ();
	
	printf ("PLEASE WAIT...\n");
	fflush (stdout);

	for (;;) {
	    if ((k = uttproc_result (&fr, &hyp, 0)) == 0) {
		printf ("\nFINAL RESULT @frm %d: %s\n", fr, hyp);
		break;
	    }
	    if (k < 0) {
		E_INFO("uttproc_result_noblock() failed\n");
		break;
	    }
	    if (! (k & 0x1f))
		update_result ();
	}
    }
}

int
main (int32 argc, char *argv[])
{
    fbs_init (argc, argv);
    
    if ((ad = ad_open_sps(SAMPLE_RATE)) == NULL)
	E_FATAL("ad_open_sps failed\n");

    E_INFO("%s COMPILED ON: %s, AT: %s\n\n", argv[0], __DATE__, __TIME__);

    utterance_loop ();
    
    ad_close (ad);
    fbs_end ();
    return 0;
}
