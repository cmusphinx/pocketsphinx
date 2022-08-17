/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 2022 Carnegie Mellon University.  All rights
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

#include <stdlib.h>

#include <sphinxbase/ckd_alloc.h>
#include <pocketsphinx/ps_vad.h>

#include "common_audio/vad/include/webrtc_vad.h"
#include "common_audio/vad/vad_core.h"

struct ps_vad_s {
    VadInstT v;
    int refcount;
    int sample_rate;
    int frame_size;
};

ps_vad_t *
ps_vad_init(ps_vad_mode_t mode, int sample_rate, float frame_length)
{
    ps_vad_t *vad = ckd_calloc(1, sizeof(*vad));
    vad->refcount = 1;
    WebRtcVad_Init((VadInst *)vad);
    if (WebRtcVad_set_mode((VadInst *)vad, mode) < 0)
        goto error_out;
    if (ps_vad_set_input_params(vad, sample_rate, frame_length) < 0)
        goto error_out;
    return vad;
error_out:
    ckd_free(vad);
    return NULL;
}

ps_vad_t *
ps_vad_retain(ps_vad_t *vad)
{
    ++vad->refcount;
    return vad;
}

int
ps_vad_free(ps_vad_t *vad)
{
    if (vad == NULL)
        return 0;
    if (--vad->refcount > 0)
        return vad->refcount;
    ckd_free(vad);
    return 0;
}

int
ps_vad_set_input_params(ps_vad_t *vad, int sample_rate, float frame_length)
{
    size_t frame_size;
    int rv;

    if (sample_rate == 0)
        sample_rate = PS_VAD_DEFAULT_SAMPLE_RATE;
    if (frame_length == 0)
        frame_length = PS_VAD_DEFAULT_FRAME_LENGTH;
    frame_size = (size_t)(sample_rate * frame_length);
    if ((rv = WebRtcVad_ValidRateAndFrameLength(sample_rate, frame_size)) < 0)
        return rv;
    vad->sample_rate = sample_rate;
    vad->frame_size = frame_size;
    return rv;
}

int
ps_vad_sample_rate(ps_vad_t *vad)
{
    if (vad == NULL)
        return -1;
    return vad->sample_rate;
}

size_t
ps_vad_frame_size(ps_vad_t *vad)
{
    if (vad == NULL)
        return -1;
    return vad->frame_size;
}

ps_vad_class_t
ps_vad_classify(ps_vad_t *vad, const short *frame)
{
    return WebRtcVad_Process((VadInst *)vad, vad->sample_rate,
                             frame, vad->frame_size);
}
