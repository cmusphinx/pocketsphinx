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
 * agc_emax.c - Estimated Max AGC
 * 
 * HISTORY
 * 
 * $Log: agc_emax.c,v $
 * Revision 1.1.1.1  2006/05/23 18:44:59  dhuggins
 * re-importation
 *
 * Revision 1.8  2004/12/10 16:48:56  rkm
 * Added continuous density acoustic model handling
 *
 * 
 * 25-Aug-98	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Modified the update procedure for estimated max to simply average over
 * 		the last history, and decay the history.
 * 
 * 20-Aug-98	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Bugfix: The extension of CMN to C0 interacted with AGC in a very
 * 		bad way, especially with their grossly inappropriate initial values...
 * 		Completely changed the implementation of agc_emax_proc(), to
 * 		monitor the max in the current utterance, and use it in estimating
 * 		the max for the next.  Also added agc_emax_update(), to be called at
 * 		the end of each utterance.
 * 
 * 01-May-96	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Added functions agcemax_set and agcemax_get.
 */


#include <stdio.h>
#include <string.h>
#include <err.h>

#include "s2types.h"
#include "fixpoint.h"

static mfcc_t max = FLOAT2MFCC(10.0);	/* Estimated C0 max used for AGC in current utterance */
/* Observed C0 max in current utterance (before update) */
static mfcc_t obs_max = FLOAT2MFCC(-1000.0);
static int obs_frame = 0;	/* Whether any data was observed after prev update */
static int obs_utt = 0;		/* Whether any utterances have been observed */
static mfcc_t obs_max_sum = 0;

int32 agcemax_set (double m)
{
    max = FLOAT2MFCC(m);
    E_INFO("AGCEMax: %.2f\n", m);
    return 0;
}

double agcemax_get ( void )
{
    return ((double) MFCC2FLOAT(max));
}

/* AGC_EMAX_PROC - do the agc
 *------------------------------------------------------------*
 */
int agc_emax_proc (mfcc_t *ocep, mfcc_t const *icep, int veclen)
{
    if (icep[0] > obs_max) {
	obs_max = icep[0];
	obs_frame = 1;
    }
    
    /* It seems NOT changing the estimated max during an utterance is best */
#if 0
    /* Adapt max rapidly to the current utt max and decay it very slowly. */
    if (icep[0] > max) {
	max = icep[0];		/* Estimated max was too low */
    } else if (obs_max < max) {
	max -= 0.01;		/* Estimated max may be too high; reduce a bit */
    }
#endif

    memcpy((char *)ocep, (char *)icep, sizeof(mfcc_t)*veclen);
    ocep[0] -= max;
    
    return 1;
}

/* Update estimated max for next utterance */
void agc_emax_update ( void )
{
    if (obs_frame) {	/* Update only if some data observed */
	obs_max_sum += obs_max;
	obs_utt++;
	
	/* Re-estimate max over past history; decay the history */
	max = obs_max_sum / obs_utt;
	if (obs_utt == 8) {
	    obs_max_sum /= 2;
	    obs_utt = 4;
	}
    }
    E_INFO("C0Max: obs= %.2f, new= %.2f\n", obs_max, max);
    
    obs_max = FLOAT2MFCC(-1000.0); /* Less than any real C0 value (hopefully!!) */
    obs_frame = 0;
}
