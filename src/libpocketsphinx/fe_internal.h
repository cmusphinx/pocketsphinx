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

#include "fe.h"
#include "fixpoint.h"

#ifdef FIXED_POINT
#ifdef FIXED16
typedef int16 frame_t;
typedef struct { int16 r, i; } complex;
#else
typedef fixed32 frame_t;
typedef struct { fixed32 r, i; } complex;
#endif
typedef fixed32 window_t;
typedef int32 powspec_t;
#else /* FIXED_POINT */
typedef float64 frame_t;
typedef float64 powspec_t;
typedef float64 window_t;
typedef struct { float64 r, i; } complex;
#endif /* FIXED_POINT */

typedef struct melfb_s {
    float32 sampling_rate;
    int32 num_cepstra;
    int32 num_filters;
    int32 fft_size;
    float32 lower_filt_freq;
    float32 upper_filt_freq;
    mfcc_t **filter_coeffs;
    mfcc_t **mel_cosine;
    mfcc_t *left_apex;
    int32 *width;
    int32 doublewide;
} melfb_t;

struct fe_s {
    float32 SAMPLING_RATE;
    int32 FRAME_RATE;
    int32 FRAME_SHIFT;
    float32 WINDOW_LENGTH;
    int32 FRAME_SIZE;
    int32 FFT_SIZE;
    int32 FB_TYPE;
    int32 NUM_CEPSTRA;
    float32 PRE_EMPHASIS_ALPHA;
    int16 *OVERFLOW_SAMPS;
    int32 NUM_OVERFLOW_SAMPS;    
    melfb_t *MEL_FB;
    int32 START_FLAG;
    int16 PRIOR;
    window_t *HAMMING_WINDOW;
    int32 FRAME_COUNTER;
};

#ifndef	M_PI
#define M_PI	(3.14159265358979323846)
#endif	/* M_PI */

#define FORWARD_FFT 1
#define INVERSE_FFT -1

/* functions */
int32 fe_build_melfilters(melfb_t *MEL_FB);
int32 fe_compute_melcosine(melfb_t *MEL_FB);
float32 fe_mel(float32 x);
float32 fe_melinv(float32 x);
void fe_pre_emphasis(int16 const *in, frame_t *out, int32 len, float32 factor, int16 prior);
void fe_create_hamming(window_t *in, int32 in_len);
void fe_hamming_window(frame_t *in, window_t *window, int32 in_len);
void fe_spec_magnitude(frame_t const *data, int32 data_len, powspec_t *spec, int32 fftsize);
void fe_frame_to_fea(fe_t *FE, frame_t *in, mfcc_t *fea);
void fe_mel_spec(fe_t *FE, powspec_t const *spec, powspec_t *mfspec);
void fe_mel_cep(fe_t *FE, powspec_t *mfspec, mfcc_t *mfcep);
int32 fe_fft(complex const *in, complex *out, int32 N, int32 invert);
int32 fe_fft_real(frame_t *x, int n, int m);
void fe_short_to_frame(int16 const *in, frame_t *out, int32 len);
void *fe_create_2d(int32 d1, int32 d2, int32 elem_size);
void fe_free_2d(void *arr);
void fe_print_current(fe_t *FE);
void fe_parse_general_params(param_t const *P, fe_t *FE);
void fe_parse_melfb_params(param_t const *P, melfb_t *MEL);

