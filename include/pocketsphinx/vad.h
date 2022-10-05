/* -*- c-basic-offset:4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 2022 David Huggins-Daines.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 */
/**
 * @file vad.h
 * @brief Simple voice activity detection
 *
 * Because doxygen is Bad Software, the actual documentation can only
 * exist in \ref ps_vad_t.  Sorry about that.
 */

#ifndef __PS_VAD_H__
#define __PS_VAD_H__


#include <pocketsphinx/prim_type.h>
#include <pocketsphinx/export.h>

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

/**
 * @struct ps_vad_t pocketsphinx/vad.h
 * @brief Voice activity detector.
 */
typedef struct ps_vad_s ps_vad_t;

/**
 * @enum ps_vad_mode_e pocketsphinx/vad.h
 * @brief Voice activity detection "aggressiveness" levels.
 */
typedef enum ps_vad_mode_e {
    PS_VAD_LOOSE = 0,
    PS_VAD_MEDIUM_LOOSE = 1,
    PS_VAD_MEDIUM_STRICT = 2,
    PS_VAD_STRICT = 3
} ps_vad_mode_t;

/**
 * @enum ps_vad_class_e pocketsphinx/vad.h
 * @brief Classification of input frames returned by ps_vad_classify().
 */
typedef enum ps_vad_class_e {
    PS_VAD_ERROR = -1,
    PS_VAD_NOT_SPEECH = 0,
    PS_VAD_SPEECH = 1
} ps_vad_class_t;

/**
 * Default sampling rate for voice activity detector
 */
#define PS_VAD_DEFAULT_SAMPLE_RATE 16000
/**
 * Default frame length for voice activity detector
 */
#define PS_VAD_DEFAULT_FRAME_LENGTH 0.03

/**
 * Initialize voice activity detection.
 *
 * @memberof ps_vad_t
 * @param mode "Aggressiveness" of voice activity detection.  Stricter
 *             values (see ps_vad_mode_t) are less likely to
 *             misclassify non-speech as speech.
 * @param sample_rate Sampling rate of input, or 0 for default (which can
 *                    be obtained with ps_vad_sample_rate()).  Only 8000,
 *                    16000, 32000, 48000 are directly supported.  See
 *                    ps_vad_set_input_params() for more information.
 * @param frame_length Frame length in seconds, or 0.0 for the default.  Only
 *                     0.01, 0.02, 0.03 currently supported.  **Actual** value
 *                     may differ, you must use ps_vad_frame_length() to
 *                     obtain it.
 * @return VAD object or NULL on failure (invalid parameter for instance).
 */
POCKETSPHINX_EXPORT
ps_vad_t *ps_vad_init(ps_vad_mode_t mode, int sample_rate, double frame_length);

/**
 * Retain a pointer to voice activity detector.
 *
 * @memberof ps_vad_t
 * @param vad Voice activity detector.
 * @return Voice activity detector with incremented reference count.
 */
POCKETSPHINX_EXPORT
ps_vad_t *ps_vad_retain(ps_vad_t *vad);

/**
 * Release a pointer to voice activity detector.
 *
 * @memberof ps_vad_t
 * @param vad Voice activity detector.
 * @return New reference count (0 if freed).
 */
POCKETSPHINX_EXPORT
int ps_vad_free(ps_vad_t *vad);

/**
 * Set the input parameters for voice activity detection.
 *
 * @memberof ps_vad_t
 * @param sample_rate Sampling rate of input, or 0 for default (which can
 *                    be obtained with ps_vad_sample_rate()).  Only 8000,
 *                    16000, 32000, 48000 are directly supported, others
 *                    will use the closest supported rate (within reason).
 *                    Note that this means that the actual frame length
 *                    may not be exactly the one requested, so you must
 *                    always use the one returned by ps_vad_frame_size()
 *                    (in samples) or ps_vad_frame_length() (in seconds).
 * @param frame_length Requested frame length in seconds, or 0.0 for the
 *                     default.  Only 0.01, 0.02, 0.03 currently supported.
 *                     **Actual frame length may be different, you must
 *                     always use ps_vad_frame_length() to obtain it.**
 * @return 0 for success or -1 on error.
 */
POCKETSPHINX_EXPORT
int ps_vad_set_input_params(ps_vad_t *vad, int sample_rate, double frame_length);

/**
 * Get the sampling rate expected by voice activity detection.
 *
 * @memberof ps_vad_t
 * @param vad Voice activity detector.
 * @return Expected sampling rate.
 */
POCKETSPHINX_EXPORT
int ps_vad_sample_rate(ps_vad_t *vad);

/**
 * Get the number of samples expected by voice activity detection.
 *
 * You **must** always ensure that the buffers passed to
 * ps_vad_classify() contain this number of samples (zero-pad them if
 * necessary).
 *
 * @memberof ps_vad_t
 * @param vad Voice activity detector.
 * @return Size, in samples, of the frames passed to ps_vad_classify().
 */
POCKETSPHINX_EXPORT
size_t ps_vad_frame_size(ps_vad_t *vad);

/**
 * Get the *actual* length of a frame in seconds.
 *
 * This may differ from the value requested in ps_vad_set_input_params().
 */
#define ps_vad_frame_length(vad) ((double)ps_vad_frame_size(vad) / ps_vad_sample_rate(vad))

/**
 * Classify a frame as speech or not speech.
 *
 * @memberof ps_vad_t
 * @param vad Voice activity detector.
 * @param frame Frame of input, **must** contain the number of
 *              samples returned by ps_vad_frame_size().
 * @return PS_VAD_SPEECH, PS_VAD_NOT_SPEECH, or PS_VAD_ERROR (see
 *         ps_vad_class_t).
 */
POCKETSPHINX_EXPORT
ps_vad_class_t ps_vad_classify(ps_vad_t *vad, const int16 *frame);

#ifdef __cplusplus
}
#endif

#endif /* __PS_VAD_H__ */
