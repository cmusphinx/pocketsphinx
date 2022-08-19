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
 * ====================================================================
 */
/**
 * @file ps_vad.h Voice activity detection for PocketSphinx.
 */

#ifndef __PS_VAD_H__
#define __PS_VAD_H__


#include <sphinxbase/prim_type.h>
#include <pocketsphinx/export.h>

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

/**
 * Voice activity detection object.
 */
typedef struct ps_vad_s ps_vad_t;

/**
 * Voice activity detection "aggressiveness" levels.
 */
typedef enum ps_vad_mode_e {
    PS_VAD_LOOSE = 0,
    PS_VAD_MEDIUM_LOOSE = 1,
    PS_VAD_MEDIUM_STRICT = 2,
    PS_VAD_STRICT = 3
} ps_vad_mode_t;

/**
 * Classification of input frames returned by ps_vad_classify().
 */
typedef enum ps_vad_class_e {
    PS_VAD_ERROR = -1,
    PS_VAD_NOT_SPEECH = 0,
    PS_VAD_SPEECH = 1
} ps_vad_class_t;

#define PS_VAD_DEFAULT_SAMPLE_RATE 16000
#define PS_VAD_DEFAULT_FRAME_LENGTH 0.03

/**
 * Initialize voice activity detection.
 *
 *
 * @param mode "Aggressiveness" of voice activity detection.  Stricter
 *             values (see ps_vad_mode_t) are less likely to
 *             misclassify non-speech as speech.
 * @param sample_rate Sampling rate of input, or 0 for default (which can
 *                    be obtained with ps_vad_sample_rate()).  Only 8000,
 *                    16000, 32000, 48000 currently supported.
 * @param frame_length Frame length in seconds, or 0.0 for the default.  Only
 *                     0.01, 0.02, 0.03 currently supported.
 * @return VAD object or NULL on failure (invalid parameter for instance).
 */
POCKETSPHINX_EXPORT
ps_vad_t *ps_vad_init(ps_vad_mode_t mode, int sample_rate, float frame_length);

/**
 * Retain a pointer to voice activity detector.
 *
 * @param vad Voice activity detector.
 * @return Voice activity detector with incremented reference count.
 */
POCKETSPHINX_EXPORT
ps_vad_t *ps_vad_retain(ps_vad_t *vad);

/**
 * Release a pointer to voice activity detector.
 *
 * @param vad Voice activity detector.
 * @return New reference count (0 if freed).
 */
POCKETSPHINX_EXPORT
int ps_vad_free(ps_vad_t *vad);

/**
 * Set the input parameters for voice activity detection.
 *
 * @param sample_rate Sampling rate of input, or 0 for default (which can
 *                    be obtained with ps_vad_sample_rate()).  Only 8000,
 *                    16000, 32000, 48000 currently supported.
 * @param frame_length Frame length in seconds, or 0.0 for the default.  Only
 *                     0.01, 0.02, 0.03 currently supported.
 * @return 0 for success or -1 on error.
 */
POCKETSPHINX_EXPORT
int ps_vad_set_input_params(ps_vad_t *vad, int sample_rate, float frame_length);

/**
 * Get the sampling rate expected by voice activity detection.
 *
 * @param vad Voice activity detector.
 * @return Expected sampling rate.
 */
POCKETSPHINX_EXPORT
int ps_vad_sample_rate(ps_vad_t *vad);

/**
 * Get the number of samples expected by voice activity detection.
 *
 * @param vad Voice activity detector.
 * @return Size, in samples, of the frames passed to ps_vad_classify().
 */
POCKETSPHINX_EXPORT
size_t ps_vad_frame_size(ps_vad_t *vad);

/**
 * Classify a frame as speech or not speech.
 *
 * @param vad Voice activity detector.
 * @param frame Frame of input, must contain the number of samples
 *              returned by ps_vad_frame_size().
 * @return PS_VAD_SPEECH, PS_VAD_NOT_SPEECH, or PS_VAD_ERROR (see
 *         ps_vad_class_t).
 */
POCKETSPHINX_EXPORT
ps_vad_class_t ps_vad_classify(ps_vad_t *vad, const short *frame);

#ifdef __cplusplus
}
#endif

#endif /* __PS_VAD_H__ */
