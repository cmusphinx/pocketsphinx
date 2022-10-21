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
 * @file endpointer.h
 * @brief VAD-based endpointer for PocketSphinx
 *
 * Because doxygen is Bad Software, the actual documentation can only
 * exist in \ref ps_endpointer_t.  Sorry about that.
 */

#ifndef __PS_ENDPOINTER_H__
#define __PS_ENDPOINTER_H__


#include <pocketsphinx/prim_type.h>
#include <pocketsphinx/export.h>

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

#include <pocketsphinx/prim_type.h>
#include <pocketsphinx/export.h>
#include <pocketsphinx/vad.h>

/**
 * @struct ps_endpointer_t pocketsphinx/endpointer.h
 * @brief Simple voice activity detection based endpointing
 */
typedef struct ps_endpointer_s ps_endpointer_t;

/**
 * Default window in seconds of audio to use for speech start/end decision.
 */
#define PS_ENDPOINTER_DEFAULT_WINDOW 0.3
/**
 * Default ratio of frames in window to trigger start/end decision.
 */
#define PS_ENDPOINTER_DEFAULT_RATIO 0.9

/**
 * Initialize endpointing.
 *
 * @memberof ps_endpointer_t
 * @param window Seconds of audio to use in speech start/end decision,
 *               or 0 to use the default (PS_ENDPOINTER_DEFAULT_WINDOW).
 * @param ratio Ratio of frames needed to trigger start/end decision,
 *              or 0 for the default (PS_ENDPOINTER_DEFAULT_RATIO).
 * @param mode "Aggressiveness" of voice activity detection.  Stricter
 *             values (see ps_vad_mode_t) are less likely to
 *             misclassify non-speech as speech.
 * @param sample_rate Sampling rate of input, or 0 for default (which can
 *                    be obtained with ps_vad_sample_rate()).  Only 8000,
 *                    16000, 32000, 48000 are directly supported, others
 *                    will use the closest supported rate (within reason).
 *                    Note that this means that the actual frame length
 *                    may not be exactly the one requested, so you must
 *                    always use the one returned by
 *                    ps_endpointer_frame_size()
 *                    (in samples) or ps_endpointer_frame_length() (in
 *                    seconds).
 * @param frame_length Requested frame length in seconds, or 0.0 for the
 *                     default.  Only 0.01, 0.02, 0.03 currently supported.
 *                     **Actual frame length may be different, you must
 *                     always use ps_endpointer_frame_length() to obtain it.**
 * @return Endpointer object or NULL on failure (invalid parameter for
 * instance).
 */
POCKETSPHINX_EXPORT
ps_endpointer_t *ps_endpointer_init(double window,
                                    double ratio,
                                    ps_vad_mode_t mode,
                                    int sample_rate, double frame_length);

/**
 * Retain a pointer to endpointer
 *
 * @memberof ps_endpointer_t
 * @param ep Endpointer.
 * @return Endpointer with incremented reference count.
 */
POCKETSPHINX_EXPORT
ps_endpointer_t *ps_endpointer_retain(ps_endpointer_t *ep);

/**
 * Release a pointer to endpointer.
 *
 * @memberof ps_endpointer_t
 * @param ep Endpointer
 * @return New reference count (0 if freed).
 */
POCKETSPHINX_EXPORT
int ps_endpointer_free(ps_endpointer_t *ep);

/**
 * Get the voice activity detector used by the endpointer.
 *
 * @memberof ps_endpointer_t
 * @return VAD object. The endpointer retains ownership of this
 * object, so you must use ps_vad_retain() if you wish to use it
 * outside of the lifetime of the endpointer.
 */
POCKETSPHINX_EXPORT
ps_vad_t *ps_endpointer_vad(ps_endpointer_t *ep);

/**
 * Get the frame size (in samples) consumed by the endpointer.
 *
 * Multiply this by 2 to get the size of the frame buffer required.
 */
#define ps_endpointer_frame_size(ep) ps_vad_frame_size(ps_endpointer_vad(ep))

/**
 * Get the frame length (in seconds) consumed by the endpointer.
 */
#define ps_endpointer_frame_length(ep) ps_vad_frame_length(ps_endpointer_vad(ep))

/**
 * Get the sample rate required by the endpointer.
 */
#define ps_endpointer_sample_rate(ep) ps_vad_sample_rate(ps_endpointer_vad(ep))

/**
 * Process a frame of audio, returning a frame if in a speech region.
 *
 * Note that the endpointer is *not* thread-safe.  You must call all
 * endpointer functions from the same thread.
 *
 * @memberof ps_endpointer_t
 * @param ep Endpointer.
 * @param frame Frame of data, must contain ps_endpointer_frame_size()
 *              samples.
 * @return NULL if no speech available, or pointer to a frame of
 *         ps_endpointer_frame_size() samples (no more and no less).
 */
POCKETSPHINX_EXPORT
const int16 *ps_endpointer_process(ps_endpointer_t *ep,
                                   const int16 *frame);

/**
 * Process remaining samples at end of stream.
 *
 * Note that the endpointer is *not* thread-safe.  You must call all
 * endpointer functions from the same thread.
 *
 * @memberof ps_endpointer_t
 * @param ep Endpointer.
 * @param frame Frame of data, must contain ps_endpointer_frame_size()
 *              samples or less.
 * @param nsamp: Number of samples in frame.
 * @param out_nsamp: Output, number of samples available.
 * @return Pointer to available samples, or NULL if none available.
 */
POCKETSPHINX_EXPORT
const int16 *ps_endpointer_end_stream(ps_endpointer_t *ep,
                                      const int16 *frame,
                                      size_t nsamp,
                                      size_t *out_nsamp);

/**
 * Get the current state (speech/not-speech) of the endpointer.
 *
 * This function can be used to detect speech/non-speech transitions.
 * If it returns 0, and a subsequent call to ps_endpointer_process()
 * returns non-NULL, this indicates a transition to speech.
 * Conversely, if ps_endpointer_process() returns non-NULL and a
 * subsequent call to this function returns 0, this indicates a
 * transition to non-speech.
 *
 * @memberof ps_endpointer_t
 * @param ep Endpointer.
 * @return non-zero if in a speech segment after processing the last
 *         frame of data.
 */
POCKETSPHINX_EXPORT
int ps_endpointer_in_speech(ps_endpointer_t *ep);

/**
 * Get the start time of the last speech segment.
 * @memberof ps_endpointer_t
 */
POCKETSPHINX_EXPORT
double ps_endpointer_speech_start(ps_endpointer_t *ep);

/**
 * Get the end time of the last speech segment
 * @memberof ps_endpointer_t
 */
POCKETSPHINX_EXPORT
double ps_endpointer_speech_end(ps_endpointer_t *ep);

#ifdef __cplusplus
}
#endif

#endif /* __PS_ENDPOINTER_H__ */
