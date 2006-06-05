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
 * audio_utils.c -- From Bob Brennan for Sun audio utilities.
 * 
 * HISTORY
 * 
 * 15-Jul-98	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Removed '#include sun/audioio.h'.
 * 
 * 02-Aug-95	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Created from Bob Brennan's original.
 */

#include	<stdio.h>
#include        <stdlib.h>
#include	<sys/types.h>
#include	<sys/ioctl.h>
#include	<fcntl.h>
#include	<stropts.h>
#include	<errno.h>
#include        <unistd.h>
#include	"audio_utils_sunos.h"

#define		AUDIO_DEF_GAIN	50

#define QUIT(x)		{fprintf x; exit(-1);}

static	int	audioFd = -1;
static	int	sampleRate;
static	int	samplePrecision;
static	int	sampleEncoding;

/*
 * Open audio, put it in paused state, and set IO to non-blocking mode.
 */
int	audioOpen(int rate)
{
    int		fd;
    int		non_blocking;

    if (rate == 16000) {
	sampleRate = 16000;
	samplePrecision = 16;
	sampleEncoding = AUDIO_ENCODING_LINEAR;
    }
    else if (rate == 8000) {
	sampleRate = 8000;
	samplePrecision = 8;
	sampleEncoding = AUDIO_ENCODING_ULAW;
    }
    else
	QUIT((stderr, "audioOpen: unsupported rate %d\n", rate));

    /*
     * Open the device for read-only access and do not block (wait) if
     * the device is currently busy.
     */
    fd = open("/dev/audio", O_RDONLY | O_NDELAY);
    if (fd < 0) {
	if (errno == EBUSY)
	    fprintf(stderr, "audioOpen: audio device is busy\n");
	else
	    perror("audioOpen error");
	exit(1);
    }
    audioFd = fd;

    audioPause ();
    audioFlush ();
    non_blocking = 1;
    ioctl (audioFd, FIONBIO, &non_blocking);

    return fd;
}

void	audioPause(void)
{
    audio_info_t	info;
    int			err;

    /*
     * Make sure that the audio device is open
     */
    if (audioFd < 0)
        QUIT((stderr, "audioPauseAndFlush: audio device not open\n"));

    /*
     * Pause and flush the input stream
     */
    AUDIO_INITINFO(&info);
    info.record.pause = 1;
    err = ioctl(audioFd, AUDIO_SETINFO, &info);
    if (err)
	QUIT((stderr, "pause ioctl err %d\n", err));
}

void	audioFlush(void)
{
    audio_info_t	info;
    int			err;

    /*
     * Make sure that the audio device is open
     */
    if (audioFd < 0)
	QUIT((stderr, "audioPauseAndFlush: audio device not open\n"));

    /*
     * Flush the current input queue
     */
    err = ioctl(audioFd, I_FLUSH, FLUSHR);
    if (err)
	QUIT((stderr, "flush ioctl err %d\n", err));
}

void	audioStartRecord(void)
{
    audio_info_t	info;
    int			err;

    /*
     * Make sure that the audio device is open
     */
    if (audioFd < 0)
	QUIT((stderr, "audioStartRecord: audio device not open\n"));

    /*
     * Setup the input stream the way we want it and start recording.
     */
    AUDIO_INITINFO(&info);
    info.record.sample_rate = sampleRate;
    info.record.channels = 1;
    info.record.precision = samplePrecision;
    info.record.encoding = sampleEncoding;
#if 0
    info.record.gain = AUDIO_DEF_GAIN;
    info.record.port = AUDIO_MICROPHONE;
#else
    info.record.gain = 80;
    info.record.port = AUDIO_LINE_IN;
#endif
    info.record.samples = 0;
    info.record.pause = 0;
    info.record.error = 0;

    err = ioctl(audioFd, AUDIO_SETINFO, &info);
    if (err)
        QUIT((stderr, "startRecord ioctl err %d\n", err));
}

void	audioStopRecord(void)
{
    audioPause();
}

void	audioClose(void)
{
    /*
     * Make sure that the audio device is open
     */
    if (audioFd < 0)
	QUIT((stderr, "audioRecord: audio device not open\n"));
    close(audioFd);
    audioFd = -1;
}

int	audioSetRecordGain(int gain)
{
    audio_info_t	info;
    int			err;
    int			currentGain;

    /*
     * Make sure that the audio device is open
     */
    if (audioFd < 0)
	QUIT((stderr, "audioSetRecordGain: audio device not open\n"));

    /*
     * Fetch the current gain
     */
    err = ioctl(audioFd, AUDIO_GETINFO, &info);
    if (err)
        QUIT((stderr, "audioSetRecordGain ioctl err %d\n", err));
    currentGain = info.record.gain;

    /*
     * Make sure that desired gain is within an acceptable range
     */
    if (gain < AUDIO_MIN_GAIN || gain > AUDIO_MAX_GAIN) {
	fprintf(stderr, "audioSetRecordGain: gain %d out of range %d-%d\n",
	    gain, AUDIO_MIN_GAIN, AUDIO_MAX_GAIN);
	return currentGain;
    }

    /*
     * Setup the input stream the way we want it and start recording.
     */
    AUDIO_INITINFO(&info);
    info.record.gain = gain;
    err = ioctl(audioFd, AUDIO_SETINFO, &info);
    if (err)
        QUIT((stderr, "startRecord ioctl err %d\n", err));
    return currentGain;
}

#ifdef MAIN
void main (int argc, char *argv[])
{
    unsigned short buf[16000];
    int audio_fd, i, j, k;

    audio_fd = audioOpen (16000);
    audioStartRecord ();

    for (i = 0; i < 5; i++) {
        /* simulate compute for a while */
        for (j = 0; j < 200000; j++);

	/* read whatever accumulated data is available, upto 16k max */
	k = read (audio_fd, buf, 16000*sizeof(short));

	/* print some of the new data */
	printf (" %5d", k);
	for (j = 0; j < 20; j++)
	    printf (" %04x", buf[j]);
	printf ("\n");
	fflush (stdout);
    }

    /* simulate compute for a while and stop recording */
    for (j = 0; j < 1000000; j++);
    audioStopRecord ();

    /* read whatever accumulated data is available, upto 16k max */
    k = read (audio_fd, buf, 16000*sizeof(short));
    printf (" %5d\n", k);

    audioClose ();
}
#endif
