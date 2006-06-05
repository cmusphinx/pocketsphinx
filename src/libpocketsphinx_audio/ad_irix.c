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
/*
 * ad.c -- Wraps a "sphinx-II standard" audio interface around the basic audio
 * 		utilities.
 * 
 * HISTORY
 * 
 * 11-Jun-96	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University.
 * 		Modified to conform to new A/D API.
 * 
 * 12-May-96	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University.
 * 		Dummy template created.
 */

#include <stdio.h>
#include <string.h>
#include <dmedia/audio.h>

#include "s2types.h"
#include "ad.h"

#define QUIT(x)		{fprintf x; exit(-1);}

ad_rec_t *ad_open_sps (int32 samples_per_sec)
{
  // fprintf(stderr, "A/D library not implemented\n");
    ad_rec_t *handle;
    ALpv          pv; 
    int device  = AL_DEFAULT_INPUT;
    ALconfig portconfig = alNewConfig(); 
    ALport port; 
    int32 sampleRate;
    long long gainValue = alDoubleToFixed(8.5); 
    
    pv.param = AL_GAIN; 
    pv.sizeIn = 1; 
    pv.value.ptr = &gainValue; 
    
    if (alSetParams(device, &pv, 1)<0) {
      fprintf(stderr, "setparams failed: %s\n",alGetErrorString(oserror()));
      return NULL; 
    }
    

    pv.param = AL_RATE;
    pv.value.ll = alDoubleToFixed(samples_per_sec);
    
    if (alSetParams(device, &pv, 1)<0) {
      fprintf(stderr, "setparams failed: %s\n",alGetErrorString(oserror()));
      return NULL; 
    }
    
    if (pv.sizeOut < 0) {
      /*
       * Not all devices will allow setting of AL_RATE (for example, digital 
       * inputs run only at the frequency of the external device).  Check
       * to see if the rate was accepted.
       */
      fprintf(stderr, "AL_RATE was not accepted on the given resource\n");
      return NULL; 
    }
    
    if (alGetParams(device, &pv, 1)<0) {
        fprintf(stderr, "getparams failed: %s\n",alGetErrorString(oserror()));
     }
    
    sampleRate = (int32)alFixedToDouble(pv.value.ll);
#if 0
    printf("sample rate is set to %d\n", sampleRate);
#endif

    if (alSetChannels(portconfig, 1) < 0) {
      fprintf(stderr, "alSetChannels failed: %s\n",alGetErrorString(oserror()));
      return NULL; 
    }

    port = alOpenPort(" Sphinx-II input port", "r", portconfig); 

    if (!port) {
      fprintf(stderr, "alOpenPort failed: %s\n", alGetErrorString(oserror()));
      return NULL; 
    }
    if ((handle = (ad_rec_t *) calloc (1, sizeof(ad_rec_t))) == NULL) {
      fprintf(stderr, "calloc(%d) failed\n", sizeof(ad_rec_t));
      abort();
    }

    handle->audio = port; 
    handle->recording = 0;
    handle->sps = sampleRate;
    handle->bps = sizeof(int16);

    alFreeConfig(portconfig); 

    return handle;
}

ad_rec_t *ad_open ( void )
{
    return ad_open_sps(DEFAULT_SAMPLES_PER_SEC);
}

int32 ad_start_rec (ad_rec_t *r)
{
    return 0;
}

int32 ad_stop_rec (ad_rec_t *r)
{
  /* how do we start/stop recording on an sgi? */
    return 0;
}

int32 ad_read (ad_rec_t *r, int16 *buf, int32 max)
{
  int32 length = alGetFilled(r->audio);
  
  if (length > max) {
    alReadFrames(r->audio, buf, max);
    return max; 
  } else {
    alReadFrames(r->audio, buf, length); 
    return length;
  }
}

int32 ad_close (ad_rec_t *r)
{
  if (r->audio) 
    alClosePort(r->audio); 
  free(r); 
  return 0;
}
