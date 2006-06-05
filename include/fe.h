/* ====================================================================
 * Copyright (c) 1996-2004 Carnegie Mellon University.  All rights 
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
 * fe.h
 * 
 * $Log: fe.h,v $
 * Revision 1.1.1.1  2006/05/23 18:45:02  dhuggins
 * re-importation
 *
 * Revision 1.11  2005/02/05 02:15:02  egouvea
 * Removed fe_process(), never used
 *
 * Revision 1.10  2004/12/10 16:48:55  rkm
 * Added continuous density acoustic model handling
 *
 *
 */


#ifndef _NEW_FE_H_
#define _NEW_FE_H_

#include "fixpoint.h"

typedef struct{
    float32 SAMPLING_RATE;
    int32 FRAME_RATE;
    float32 WINDOW_LENGTH;
    int32 FB_TYPE;
    int32 NUM_CEPSTRA;
    int32 NUM_FILTERS;
    int32 FFT_SIZE;
    float32 LOWER_FILT_FREQ;
    float32 UPPER_FILT_FREQ;
    float32 PRE_EMPHASIS_ALPHA;

    char *wavfile;
    char *cepfile;
    char *ctlfile;
    char *wavdir;
    char *cepdir;
    char *wavext;
    char *cepext;
    int32 input_format;
    int32 is_batch;
    int32 is_single;
    int32 blocksize;
    int32 verbose;
    int32 machine_endian;
    int32 input_endian;
    int32 output_endian;
    int32 dither;
    int32 logspec;
    int32 doublebw;
    int32 nchans;
    int32 whichchan;
} param_t;

typedef struct fe_s fe_t;

#define MEL_SCALE 1
#define LOG_LINEAR 2

#define ON 1
#define OFF 0

/* Default values */
#define DEFAULT_SAMPLING_RATE 16000.0
#define DEFAULT_FRAME_RATE 100
#define DEFAULT_FRAME_SHIFT 160
/* The default below is set so that we have an integral number of
 * samples in a frame.
 */
#define DEFAULT_WINDOW_LENGTH 0.025625
/* Since the default sampling rate is 16000, let's make the default
 * fft size consistent with it.
 */
#define DEFAULT_FFT_SIZE 512
#define DEFAULT_FB_TYPE MEL_SCALE
#define DEFAULT_NUM_CEPSTRA 13
#define DEFAULT_NUM_FILTERS 40
#define DEFAULT_LOWER_FILT_FREQ 133.33334
#define DEFAULT_UPPER_FILT_FREQ 6855.4976
#define DEFAULT_PRE_EMPHASIS_ALPHA 0.97
#define DEFAULT_START_FLAG 0

#define BB_SAMPLING_RATE 16000
#define DEFAULT_BB_FFT_SIZE 512
#define DEFAULT_BB_FRAME_SHIFT 160
#define DEFAULT_BB_NUM_FILTERS 40
#define DEFAULT_BB_LOWER_FILT_FREQ 133.33334
#define DEFAULT_BB_UPPER_FILT_FREQ 6855.4976

#define NB_SAMPLING_RATE 8000
#define DEFAULT_NB_FFT_SIZE 256 /* NOTE!  In actual fact we will
				   default to 512, because this is
				   what SphinxTrain will do.  See
				   fbs_main.c:query_fe_params() */
#define DEFAULT_NB_FRAME_SHIFT 80
#define DEFAULT_NB_NUM_FILTERS 31
#define DEFAULT_NB_LOWER_FILT_FREQ 200
#define DEFAULT_NB_UPPER_FILT_FREQ 3500


#define DEFAULT_BLOCKSIZE 200000
#define DITHER  OFF

/* Interface */
fe_t *fe_init(param_t const *P);

int32 fe_start_utt(fe_t *FE);

int32 fe_end_utt(fe_t *FE, mfcc_t *cepvector);

int32 fe_close(fe_t *FE);

int32 fe_process_utt(fe_t *FE, int16 const *spch, int32 nsamps, mfcc_t **cep);

#endif
