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
/* -*- mode:c; indent-tabs-mode:t; c-basic-offset:4; comment-column:40 -*-
 *
 * Sphinx II libad (Linux)
 * ^^^^^^^^^^^^^^^^^^^^^^^
 * $Id: ad_alsa.c,v 1.1.1.1 2006/05/23 18:45:02 dhuggins Exp $
 *
 * John G. Dorsey (jd5q+@andrew.cmu.edu)
 * Engineering Design Research Center
 * Carnegie Mellon University
 * ***************************************************************************
 * 
 * REVISION HISTORY
 *
 * 12-Dec-2000  David Huggins-Daines <dhd@cepstral.com> at Cepstral LLC
 *		Make this at least compile with the new ALSA API.
 *
 * 05-Nov-1999	Sean Levy (snl@stalphonsos.com) at St. Alphonsos, LLC.
 *		Ported to ALSA so I can actually get working full-duplex.
 *
 * 09-Aug-1999  Kevin Lenzo (lenzo@cs.cmu.edu) at Cernegie Mellon University.
 *              Incorporated nickr@cs.cmu.edu's changes (marked below) and
 *              SPS_EPSILON to allow for sample rates that are "close enough".
 * 
 * 15-Jun-1999	M. K. Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon Univ.
 *		Consolidated all ad functions into
 *		this one file.  Added ad_open_sps().
 * 		Other cosmetic changes for consistency (e.g., use of err.h).
 * 
 * 18-May-1999	Kevin Lenzo (lenzo@cs.cmu.edu) added <errno.h>.
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/asoundlib.h>
#include <errno.h>

#include "s2types.h"
#include "ad.h"

#define AUDIO_FORMAT SND_PCM_SFMT_S16_LE /* 16-bit signed, little endian */
#define INPUT_GAIN   (80)
#define SPS_EPSILON   200

static int
setparams(int32 sps, snd_pcm_t *handle)
{
    int err;
    struct snd_pcm_channel_params params;

    memset(&params, 0, sizeof(params));
    params.mode = SND_PCM_MODE_BLOCK;
    params.channel = SND_PCM_CHANNEL_PLAYBACK;
    params.format.format = SND_PCM_SFMT_MU_LAW;
    params.format.voices = 1;
    params.format.rate = 8000;
    if ((err = snd_pcm_plugin_params(handle, &params)) < 0) {
	fprintf(stderr, "Playback parameters error: %s\n", snd_strerror(err));
	return -1;
    }
    memset(&params, 0, sizeof(params));
    params.mode = SND_PCM_MODE_BLOCK;
    params.channel = SND_PCM_CHANNEL_CAPTURE;
    params.format.format = SND_PCM_SFMT_S16_LE;
    params.format.voices = 1;
    params.format.rate = sps;
    if ((err = snd_pcm_plugin_params(handle, &params)) < 0) {
	fprintf(stderr, "Record parameters error: %s\n", snd_strerror(err));
	return -1;
    }
    return 0;
}

static int
setlevels(int card, int device)
{
    snd_mixer_t *handle;
    snd_mixer_element_t element;
    int idx;
    int err;

    if ((err = snd_mixer_open(&handle, card, device)) < 0) { 
	fprintf(stderr, "open failed: %s\n", snd_strerror(err)); 
	return -1;
    } 
    memset((void *)&element, '\0', sizeof(element));
    strcpy(element.eid.name, "MIC Input Switch");
    element.eid.index = 0;
    element.eid.type = SND_MIXER_ETYPE_SWITCH1;
    err = snd_mixer_element_build(handle, &element);
    if (err < 0) {
	fprintf(stderr, "error building input switch element: %s\n",snd_strerror(err));
	snd_mixer_close(handle); 
	return -1;
    }
    /* This is a bit ugly and hardcoded, should be fixed when the ad layer
       API is a bit richer. SNL */
    for (idx = 0; idx < element.data.switch1.sw; idx++)
	snd_mixer_set_bit(element.data.switch1.psw, idx, 1);
    err = snd_mixer_element_write(handle, &element);
    if (err < 0) {
	fprintf(stderr, "error writing switch element: %s\n",snd_strerror(err));
	snd_mixer_close(handle);
	return -1;
    }
    snd_mixer_element_free(&element);
    snd_mixer_close(handle); 
    return 0;
}

ad_rec_t *ad_open_sps (int32 sps)
{
    ad_rec_t *handle;
    struct snd_pcm_channel_info info;
    snd_pcm_t *dspH;

    int err;
    int card;
    int dev;

    if (sps != DEFAULT_SAMPLES_PER_SEC) {
      if(abs(sps - DEFAULT_SAMPLES_PER_SEC) <= SPS_EPSILON) {
	  fprintf(stderr, "Audio sampling rate %d is within %d of %d samples/sec\n",
		  sps, SPS_EPSILON, DEFAULT_SAMPLES_PER_SEC);
      } else {
	fprintf(stderr, "Audio sampling rate %d not supported; must be %d samples/sec\n",
		sps, DEFAULT_SAMPLES_PER_SEC);
	return NULL;
      }
    }
    card = 0;
    dev = 0;
    
    err = snd_pcm_open(&dspH, card, dev, SND_PCM_OPEN_DUPLEX); /* XXX */
    if (err < 0) {
	fprintf(stderr, "Error opening audio device %d,%din full-duplex: %s\n",
		card, dev, snd_strerror(err));
	return NULL;
    }
    err = snd_pcm_plugin_info(dspH, &info);
    if (err < 0) {
	fprintf(stderr, "Error getting PCM plugin info: %s\n", snd_strerror(err));
	return NULL;
    }

    if (setparams(sps, dspH) < 0) {
	return NULL;
    }
    if (setlevels(0, 0) < 0) {
	return NULL;
    }

    if ((handle = (ad_rec_t *) calloc (1, sizeof(ad_rec_t))) == NULL) {
	fprintf(stderr, "calloc(%d) failed\n", sizeof(ad_rec_t));
	abort();
    }
    
    handle->dspH = dspH;
    handle->recording = 0;
    handle->sps = sps;
    handle->bps = sizeof(int16);

    return(handle);
}

ad_rec_t *ad_open ( void )
{
    return ad_open_sps (DEFAULT_SAMPLES_PER_SEC);
}

int32 ad_close (ad_rec_t *handle)
{
    if (handle->dspH == NULL)
	return AD_ERR_NOT_OPEN;
    
    if (handle->recording) {
	if (ad_stop_rec (handle) < 0)
	    return AD_ERR_GEN;
    }
    snd_pcm_close(handle->dspH);
    free(handle);
    
    return(0);
}

int32 ad_start_rec (ad_rec_t *handle)
{
    if (handle->dspH == NULL)
	return AD_ERR_NOT_OPEN;
    
    if (handle->recording)
	return AD_ERR_GEN;
    
    /* Sample rate, format, input mix settings, &c. are configured
     * with ioctl(2) calls under Linux. It makes more sense to handle
     * these at device open time and consider the state of the device
     * to be fixed until closed.
     */
    
    handle->recording = 1;

    /* rkm@cs: This doesn't actually do anything.  How do we turn recording on/off? */

    return(0);
}

int32 ad_stop_rec (ad_rec_t *handle)
{
    int err;

    if (handle->dspH == NULL)
	return AD_ERR_NOT_OPEN;
    
    if (! handle->recording)
	return AD_ERR_GEN;

    err = snd_pcm_plugin_flush(handle->dspH, SND_PCM_CHANNEL_CAPTURE);
    if (err < 0) {
	fprintf(stderr, "Audio capture sync failed: %s\n", snd_strerror(err));
	return AD_ERR_GEN;
    }
    
    handle->recording = 0;
    
    return (0);
}

int32 ad_read (ad_rec_t *handle, int16 *buf, int32 max)
{
    int32 length;
    static int infoz = 0;

    length = max * handle->bps;		/* #samples -> #bytes */
    length = snd_pcm_read(handle->dspH, buf, length);
    if (length > 0) {
	if (length % 2) {
	    fprintf(stderr, "snd_pcm_read returned an odd number: %d\n", length);
	    abort();
	}
	length /= handle->bps;
    } else if (length < 0) {
	fprintf(stderr, "Audio read error\n");
	return AD_ERR_GEN;
    } else if (!handle->recording)
	return AD_EOF;
#if 0					/* only do this if you're stumped */
    if ((length != 0) && (++infoz < 200))
	fprintf(stderr, "ad_read(0x%08lx,0x%08lx,%d) => %d\n", handle, buf, max,length);
#endif
    return length;
}
