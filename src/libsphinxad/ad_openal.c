/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 1999-2014 Carnegie Mellon University.  All rights
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
#include <config.h>
#include <stdlib.h>
#include <stdio.h>

#include "ad.h"

#include <OpenAL/al.h>
#include <OpenAL/alc.h>

struct ad_rec_s {
    ALCdevice * device;
};

ad_rec_t *
ad_open_dev(const char * dev, int32 samples_per_sec)
{
    ad_rec_t * handle = malloc(sizeof(ad_rec_t));

    if (handle == NULL) {
        fprintf(stderr, "%s\n", "failed to allocate memory");
        abort();
    }

    handle -> device = alcCaptureOpenDevice(dev, samples_per_sec, AL_FORMAT_MONO16, samples_per_sec * 10);

    if (handle -> device == NULL) {
        free(handle);
        fprintf(stderr, "%s\n", "failed to open capture device");
        abort();
    }

    return handle;
}


ad_rec_t *
ad_open_sps(int32 samples_per_sec)
{
    return ad_open_dev(NULL, samples_per_sec);
}

ad_rec_t *
ad_open(void)
{
    return ad_open_sps(DEFAULT_SAMPLES_PER_SEC);
}


int32
ad_start_rec(ad_rec_t * r)
{
    alcCaptureStart(r -> device);
    return 0;
}


int32
ad_stop_rec(ad_rec_t * r)
{
    alcCaptureStop(r -> device);
    return 0;
}


int32
ad_read(ad_rec_t * r, int16 * buf, int32 max)
{
    ALCint number;

    alcGetIntegerv(r -> device, ALC_CAPTURE_SAMPLES, sizeof(number), &number);
    if (number >= 0) {
        number = (number < max ? number : max);
        alcCaptureSamples(r -> device, buf, number);
    }

    return number;
}


int32
ad_close(ad_rec_t * r)
{
    ALCboolean isClosed;

    isClosed = alcCaptureCloseDevice(r -> device);

    if (isClosed) {
        return 0;
    } else {
        return -1;
    }
}
