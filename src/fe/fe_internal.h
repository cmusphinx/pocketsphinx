/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
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

#ifndef __FE_INTERNAL_H__
#define __FE_INTERNAL_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "sphinxbase/fe.h"
#include "sphinxbase/fixpoint.h"

#include "fe_noise.h"
#include "fe_prespch_buf.h"
#include "fe_type.h"

#ifdef __cplusplus
extern "C" {
#endif
#if 0
/* Fool Emacs. */
}
#endif

/* Values for the 'logspec' field. */
enum {
	RAW_LOG_SPEC = 1,
	SMOOTH_LOG_SPEC = 2
};

/* Values for the 'transform' field. */
enum {
	LEGACY_DCT = 0,
	DCT_II = 1,
        DCT_HTK = 2
};

typedef struct melfb_s melfb_t;
/** Base Struct to hold all structure for MFCC computation. */
struct melfb_s {
    float32 sampling_rate;
    int32 num_cepstra;
    int32 num_filters;
    int32 fft_size;
    float32 lower_filt_freq;
    float32 upper_filt_freq;
    /* DCT coefficients. */
    mfcc_t **mel_cosine;
    /* Filter coefficients. */
    mfcc_t *filt_coeffs;
    int16 *spec_start;
    int16 *filt_start;
    int16 *filt_width;
    /* Luxury mobile home. */
    int32 doublewide;
    char const *warp_type;
    char const *warp_params;
    uint32 warp_id;
    /* Precomputed normalization constants for unitary DCT-II/DCT-III */
    mfcc_t sqrt_inv_n, sqrt_inv_2n;
    /* Value and coefficients for HTK-style liftering */
    int32 lifter_val;
    mfcc_t *lifter;
    /* Normalize filters to unit area */
    int32 unit_area;
    /* Round filter frequencies to DFT points (hurts accuracy, but is
       useful for legacy purposes) */
    int32 round_filters;
};

/* sqrt(1/2), also used for unitary DCT-II/DCT-III */
#define SQRT_HALF FLOAT2MFCC(0.707106781186548)

typedef struct vad_data_s {
    uint8 in_speech;
    int16 pre_speech_frames;
    int16 post_speech_frames;
    prespch_buf_t* prespch_buf;
} vad_data_t;

/** Structure for the front-end computation. */
struct fe_s {
    cmd_ln_t *config;
    int refcount;

    float32 sampling_rate;
    int16 frame_rate;
    int16 frame_shift;

    float32 window_length;
    int16 frame_size;
    int16 fft_size;

    uint8 fft_order;
    uint8 feature_dimension;
    uint8 num_cepstra;
    uint8 remove_dc;
    uint8 log_spec;
    uint8 swap;
    uint8 dither;
    uint8 transform;
    uint8 remove_noise;
    uint8 remove_silence;

    float32 pre_emphasis_alpha;
    int16 pre_emphasis_prior;
    int32 dither_seed;

    int16 num_overflow_samps;    
    size_t num_processed_samps;

    /* Twiddle factors for FFT. */
    frame_t *ccc, *sss;
    /* Mel filter parameters. */
    melfb_t *mel_fb;
    /* Half of a Hamming Window. */
    window_t *hamming_window;

    /* Noise removal  */
    noise_stats_t *noise_stats;

    /* VAD variables */
    int16 pre_speech;
    int16 post_speech;
    int16 start_speech;
    float32 vad_threshold;
    vad_data_t *vad_data;

    /* Temporary buffers for processing. */
    /* FIXME: too many of these. */
    int16 *spch;
    frame_t *frame;
    powspec_t *spec, *mfspec;
    int16 *overflow_samps;
};

void fe_init_dither(int32 seed);

/* Apply 1/2 bit noise to a buffer of audio. */
int32 fe_dither(int16 *buffer, int32 nsamps);

/* Load a frame of data into the fe. */
int fe_read_frame(fe_t *fe, int16 const *in, int32 len);

/* Shift the input buffer back and read more data. */
int fe_shift_frame(fe_t *fe, int16 const *in, int32 len);

/* Process a frame of data into features. */
void fe_write_frame(fe_t *fe, mfcc_t *feat, int32 store_pcm);

/* Initialization functions. */
int32 fe_build_melfilters(melfb_t *MEL_FB);
int32 fe_compute_melcosine(melfb_t *MEL_FB);
void fe_create_hamming(window_t *in, int32 in_len);
void fe_create_twiddle(fe_t *fe);

fixed32 fe_log_add(fixed32 x, fixed32 y);
fixed32 fe_log_sub(fixed32 x, fixed32 y);

/* Miscellaneous processing functions. */
void fe_spec2cep(fe_t * fe, const powspec_t * mflogspec, mfcc_t * mfcep);
void fe_dct2(fe_t *fe, const powspec_t *mflogspec, mfcc_t *mfcep, int htk);
void fe_dct3(fe_t *fe, const mfcc_t *mfcep, powspec_t *mflogspec);

#ifdef __cplusplus
}
#endif

#endif /* __FE_INTERNAL_H__ */
