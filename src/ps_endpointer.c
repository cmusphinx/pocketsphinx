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

#include <sphinxbase/ckd_alloc.h>
#include <sphinxbase/err.h>
#include <pocketsphinx/ps_endpointer.h>

#include <string.h>

struct ps_endpointer_s {
    ps_vad_t *vad;
    int refcount;
    float ratio;
    float frame_length;
    int in_speech;
    int frame_size;
    int maxlen;
    int16 *buf;
    int8 *is_speech;
    int pos, n;
    float qstart_time, timestamp;
    float segment_start, segment_end;
};

ps_endpointer_t *
ps_endpointer_init(int nframes,
                   float ratio,
                   ps_vad_mode_t mode,
                   int sample_rate, float frame_length)
{
    ps_endpointer_t *ep = ckd_calloc(1, sizeof(*ep));
    if ((ep->vad = ps_vad_init(mode, sample_rate, frame_length)) == NULL)
        goto error_out;
    if (nframes == 0)
        nframes = PS_ENDPOINTER_DEFAULT_NFRAMES;
    if (ratio == 0.0)
        ratio = PS_ENDPOINTER_DEFAULT_RATIO;
    ep->maxlen = nframes;
    ep->ratio = ratio;
    ep->frame_length = ps_vad_frame_length(ep->vad);
    ep->frame_size = ps_endpointer_frame_size(ep);
    ep->buf = ckd_calloc(sizeof(*ep->buf),
                         ep->maxlen * ep->frame_size);
    ep->is_speech = ckd_calloc(1, ep->maxlen);
    return ep;
error_out:
    ps_endpointer_free(ep);
    return NULL;
}

ps_endpointer_t *
ps_endpointer_retain(ps_endpointer_t *ep)
{
    ++ep->refcount;
    return ep;
}

int
ps_endpointer_free(ps_endpointer_t *ep)
{
    if (ep == NULL)
        return 0;
    if (--ep->refcount > 0)
        return ep->refcount;
    ps_vad_free(ep->vad);
    if (ep->buf)
        ckd_free(ep->buf);
    if (ep->is_speech)
        ckd_free(ep->is_speech);
    ckd_free(ep);
    return 0;
}

ps_vad_t *
ps_endpointer_vad(ps_endpointer_t *ep)
{
    if (ep == NULL)
        return NULL;
    return ep->vad;
}

static int
ep_empty(ps_endpointer_t *ep)
{
    return ep->n == 0;
}

static int
ep_full(ps_endpointer_t *ep)
{
    return ep->n == ep->maxlen;
}

static int
ep_speech_count(ps_endpointer_t *ep)
{
    int count = 0;
    if (ep_empty(ep))
        ;
    else if (ep_full(ep)) {
        int i;
        for (i = 0; i < ep->maxlen; ++i)
            count += ep->is_speech[i];
    }
    else {
        int i = ep->pos, end = (ep->pos + ep->n) % ep->maxlen;
        count = ep->is_speech[i++];
        while (i != end) {
            count += ep->is_speech[i++];
            i = i % ep->maxlen;
        }
    }
    return count;
}

static int
ep_push(ps_endpointer_t *ep, int is_speech, const int16 *frame)
{
    int i = (ep->pos + ep->n) % ep->maxlen;
    int16 *dest = ep->buf + (i * sizeof(*ep->buf) * ep->frame_size);
    memcpy(dest, frame, sizeof(*ep->buf) * ep->frame_size);
    ep->is_speech[i] = is_speech;
    if (ep_full(ep)) {
        ep->qstart_time += ep->frame_length;
        ep->pos = (ep->pos + 1) % ep->maxlen;
    }
    else
        ep->n++;
    return ep->n;
}

static int16 *
ep_pop(ps_endpointer_t *ep, int *out_is_speech)
{
    int16 *pcm;
    if (ep_empty(ep))
        return NULL;
    ep->qstart_time += ep->frame_length;
    if (out_is_speech)
        *out_is_speech = ep->is_speech[ep->pos];
    pcm = ep->buf + (ep->pos * sizeof(*ep->buf) * ep->frame_size);
    ep->pos = (ep->pos + 1) % ep->maxlen;
    ep->n--;
    return pcm;
}

const int16 *
ps_endpointer_process(ps_endpointer_t *ep,
                      const int16 *frame)
{
    int is_speech, speech_count;
    if (ep == NULL || ep->vad == NULL)
        return NULL;
    if (ep->in_speech && ep_full(ep))
        E_FATAL("VAD queue overflow (should not happen)");
    is_speech = ps_vad_classify(ep->vad, frame);
    ep_push(ep, is_speech, frame);
    ep->timestamp += ep->frame_length;
    speech_count = ep_speech_count(ep);
    if (ep->in_speech) {
        if (speech_count < (1.0 - ep->ratio) * ep->maxlen) {
            /* Return only the first frame.  Either way it's sort of
               arbitrary, but this avoids having to drain the queue to
               prevent overlapping segments.  It's also closer to what
               human annotators will do. */
            int16 *pcm = ep_pop(ep, NULL);
            ep->segment_end = ep->qstart_time;
            ep->in_speech = FALSE;
            return pcm;
        }
    }
    else {
        if (speech_count > ep->ratio * ep->maxlen) {
            ep->segment_start = ep->qstart_time;
            ep->segment_end = 0;
            ep->in_speech = TRUE;
        }
    }
    if (ep->in_speech)
        return ep_pop(ep, NULL);
    else
        return NULL;
}

int
ps_endpointer_in_speech(ps_endpointer_t *ep)
{
    return ep->in_speech;
}

float
ps_endpointer_segment_start(ps_endpointer_t *ep)
{
    return ep->segment_start;
}

float
ps_endpointer_segment_end(ps_endpointer_t *ep)
{
    return ep->segment_end;
}
