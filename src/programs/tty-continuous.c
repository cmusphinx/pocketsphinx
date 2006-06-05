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
 * demo.c -- An example SphinxII program using continuous listening/silence filtering
 * 		to segment speech into utterances that are then decoded.
 * 
 * HISTORY
 *
 * 15-Jun-99    Kevin A. Lenzo (lenzo@cs.cmu.edu) at Carnegie Mellon University
 *              Added i386_linux and used ad_open_sps instead of ad_open
 * 
 * 14-Jun-96	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University.
 * 		Created.
 */

/*
 * This is a simple, tty-based example of a SphinxII client that uses continuous listening
 * with silence filtering to automatically segment a continuous stream of audio input
 * into utterances that are then decoded.
 * 
 * Remarks:
 *   - Each utterance is ended when a silence segment of at least 1 sec is recognized.
 *   - Single-threaded implementation for portability.
 *   - Uses fbs8 audio library; can be replaced with an equivalent custom library.
 */

#include <stdio.h>
#include <signal.h>
#include <setjmp.h>
#include <string.h>

#include "s2types.h"
#include "err.h"
#include "ad.h"
#include "cont_ad.h"
#include "fbs.h"

#ifdef WIN32
#include <time.h>
#else
#include <sys/types.h>
#include <sys/time.h>
#endif

#define SAMPLE_RATE   16000

static ad_rec_t *ad;

/* Sleep for specified msec */
static void sleep_msec (int32 ms)
{
#ifdef WIN32
    Sleep(ms);
#else
    /* ------------------- Unix ------------------ */
    struct timeval tmo;
    
    tmo.tv_sec = 0;
    tmo.tv_usec = ms*1000;
    
    select(0, NULL, NULL, NULL, &tmo);
#endif
}

/*
 * Main utterance processing loop:
 *     for (;;) {
 * 	   wait for start of next utterance;
 * 	   decode utterance until silence of at least 1 sec observed;
 * 	   print utterance result;
 *     }
 */
static void utterance_loop()
{
    int16 adbuf[4096];
    int32 k, fr, ts, rem;
    char *hyp;
    cont_ad_t *cont;
    char word[256];
    
    /* Initialize continuous listening module */
    if ((cont = cont_ad_init (ad, ad_read)) == NULL)
	E_FATAL("cont_ad_init failed\n");
    if (ad_start_rec (ad) < 0)
	E_FATAL("ad_start_rec failed\n");
    if (cont_ad_calib (cont) < 0)
	E_FATAL("cont_ad_calib failed\n");

    for (;;) {
	/* Indicate listening for next utterance */
        printf ("READY....\n"); fflush (stdout); fflush (stderr);
	
	/* Await data for next utterance */
	while ((k = cont_ad_read (cont, adbuf, 4096)) == 0)
	    sleep_msec(200);
	
	if (k < 0)
	    E_FATAL("cont_ad_read failed\n");
	
	/*
	 * Non-zero amount of data received; start recognition of new utterance.
	 * NULL argument to uttproc_begin_utt => automatic generation of utterance-id.
	 */
	if (uttproc_begin_utt (NULL) < 0)
	    E_FATAL("uttproc_begin_utt() failed\n");
	uttproc_rawdata (adbuf, k, 0);
	printf ("Listening...\n"); fflush (stdout);

	/* Note timestamp for this first block of data */
	ts = cont->read_ts;

	/* Decode utterance until end (marked by a "long" silence, >1sec) */
	for (;;) {
	    /* Read non-silence audio data, if any, from continuous listening module */
	    if ((k = cont_ad_read (cont, adbuf, 4096)) < 0)
		E_FATAL("cont_ad_read failed\n");
	    if (k == 0) {
		/*
		 * No speech data available; check current timestamp with most recent
		 * speech to see if more than 1 sec elapsed.  If so, end of utterance.
		 */
		if ((cont->read_ts - ts) > DEFAULT_SAMPLES_PER_SEC)
		    break;
	    } else {
		/* New speech data received; note current timestamp */
		ts = cont->read_ts;
	    }
	    
	    /*
	     * Decode whatever data was read above.  NOTE: Non-blocking mode!!
	     * rem = #frames remaining to be decoded upon return from the function.
	     */
	    rem = uttproc_rawdata (adbuf, k, 0);

	    /* If no work to be done, sleep a bit */
	    if ((rem == 0) && (k == 0))
		sleep_msec (20);
	}
	
	/*
	 * Utterance ended; flush any accumulated, unprocessed A/D data and stop
	 * listening until current utterance completely decoded
	 */
	ad_stop_rec (ad);
	while (ad_read (ad, adbuf, 4096) >= 0);
	cont_ad_reset (cont);

	printf ("Stopped listening, please wait...\n"); fflush (stdout);
#if 0
	/* Power histogram dump (FYI) */
	cont_ad_powhist_dump (stdout, cont);
#endif
	/* Finish decoding, obtain and print result */
	uttproc_end_utt ();
	if (uttproc_result (&fr, &hyp, 1) < 0)
	    E_FATAL("uttproc_result failed\n");
	printf ("%d: %s\n", fr, hyp); fflush (stdout);
	
	/* Exit if the first word spoken was GOODBYE */
	sscanf (hyp, "%s", word);
	if (strcmp (word, "goodbye") == 0)
	    break;

	/* Resume A/D recording for next utterance */
	if (ad_start_rec (ad) < 0)
	    E_FATAL("ad_start_rec failed\n");
    }

    cont_ad_close (cont);
}

static jmp_buf jbuf;
static void sighandler(int signo)
{
    longjmp(jbuf, 1);
}

int main (int argc, char *argv[])
{
    /* Make sure we exit cleanly (needed for profiling among other things) */
    signal(SIGINT, &sighandler);

    fbs_init (argc, argv);
    
    if ((ad = ad_open_sps (SAMPLE_RATE)) == NULL)
	E_FATAL("ad_open_sps failed\n");

    E_INFO("%s COMPILED ON: %s, AT: %s\n\n", argv[0], __DATE__, __TIME__);

    if (setjmp(jbuf) == 0) {
	utterance_loop ();
    }

    fbs_end ();
    ad_close (ad);

    return 0;
}
