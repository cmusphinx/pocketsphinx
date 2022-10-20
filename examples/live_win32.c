/* Example of simple PocketSphinx speech segmentation.
 *
 * MIT license (c) 2022, see LICENSE for more information.
 *
 * Author: David Huggins-Daines <dhdaines@gmail.com>
 */
/**
 * @example live_win32.c
 * @brief Speech recognition with live audio input and endpointing.
 *
 * This file shows how to use PocketSphinx with microphone input using
 * the Win32 wave API (the only one of many terrible audio APIs on
 * Windows that isn't made even more terrible by requiring you to use
 * C++ in an unmanaged environment).
 *
 * To build it, you should be able to find a "live_win32" target in
 * your favorite IDE after running CMake - in Visual Studio Code, look
 * in the "CMake" tab.
 */

#include <windows.h>
#include <mmsystem.h>
#include <pocketsphinx.h>
#include <signal.h>

/**
 * Group together various UPPERCASE JUNK in a single struct.
 */
typedef struct {
    HGLOBAL h_whdr;
    LPWAVEHDR p_whdr;
    HGLOBAL h_buf;
    LPSTR p_buf;
} wbuf_t;

/**
 * Audio recording structure. 
 */
typedef struct rec_s {
    HWAVEIN h_wavein;	/*<< "HANDLE" to the audio input device */
    /** 
     * Recording buffers provided to the audio driver.
     *
     * These work as a ring-buffer.  Each buffer will be the
     * appropriate size for the endpointer.
     */
    wbuf_t *wi_buf;
    int32 n_buf;	/* #Recording buffers provided to system */
    int32 recording;
    int32 curbuf;	/* Current buffer with data for application */
    int32 lastbuf;	/* Last buffer containing data after recording stopped */
    int32 sps;		/* Samples/sec */
    int32 bps;		/* Bytes/sample */
} rec_t;

#define WAVEIN_ERROR(msg, ret)                                  \
    do {                                                        \
        char errbuf[1024];                                      \
        waveInGetErrorText(ret, errbuf, sizeof(errbuf));        \
        E_ERROR("%s: error %08x: %s\n", msg, ret, errbuf);      \
    } while (0)

static void
wavein_free_buf(wbuf_t * b)
{
    GlobalUnlock(b->h_whdr);
    GlobalFree(b->h_whdr);
    GlobalUnlock(b->h_buf);
    GlobalFree(b->h_buf);
}

static int32
wavein_alloc_buf(wbuf_t * b, int32 samples_per_buf)
{
    HGLOBAL h_buf;              /* handle to data buffer */
    LPSTR p_buf;                /* pointer to data buffer */
    HGLOBAL h_whdr;             /* handle to header */
    LPWAVEHDR p_whdr;           /* pointer to header */

    h_buf =
        GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE,
                    samples_per_buf * sizeof(int16));
    if (!h_buf) {
        fprintf(stderr, "GlobalAlloc failed\n");
        return -1;
    }
    if ((p_buf = GlobalLock(h_buf)) == NULL) {
        GlobalFree(h_buf);
        fprintf(stderr, "GlobalLock failed\n");
        return -1;
    }
    h_whdr = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, sizeof(WAVEHDR));
    if (h_whdr == NULL) {
        GlobalUnlock(h_buf);
        GlobalFree(h_buf);

        fprintf(stderr, "GlobalAlloc failed\n");
        return -1;
    }
    if ((p_whdr = GlobalLock(h_whdr)) == NULL) {
        GlobalUnlock(h_buf);
        GlobalFree(h_buf);
        GlobalFree(h_whdr);

        fprintf(stderr, "GlobalLock failed\n");
        return -1;
    }
    b->h_buf = h_buf;
    b->p_buf = p_buf;
    b->h_whdr = h_whdr;
    b->p_whdr = p_whdr;
    p_whdr->lpData = p_buf;
    p_whdr->dwBufferLength = samples_per_buf * sizeof(int16);
    p_whdr->dwUser = 0L;
    p_whdr->dwFlags = 0L;
    p_whdr->dwLoops = 0L;
    return 0;
}

static int32
wavein_enqueue_buf(HWAVEIN h, LPWAVEHDR whdr)
{
    int32 st;

    if ((st = waveInPrepareHeader(h, whdr, sizeof(WAVEHDR))) != 0) {
        WAVEIN_ERROR("waveInPrepareHeader", st);
        return -1;
    }
    if ((st = waveInAddBuffer(h, whdr, sizeof(WAVEHDR))) != 0) {
        WAVEIN_ERROR("waveInAddBuffer", st);
        return -1;
    }
    return 0;
}

static HWAVEIN
wavein_open(int32 samples_per_sec)
{
    WAVEFORMATEX wfmt;
    int32 st;
    HWAVEIN h;

    wfmt.wFormatTag = WAVE_FORMAT_PCM;
    wfmt.nChannels = 1;
    wfmt.nSamplesPerSec = samples_per_sec;
    wfmt.nAvgBytesPerSec = samples_per_sec * 2;
    wfmt.nBlockAlign = 2;
    wfmt.wBitsPerSample = 16;
    st = waveInOpen(&h, WAVE_MAPPER, &wfmt, (DWORD) 0L, 0L,
                    (DWORD) CALLBACK_NULL);
    if (st != 0) {
        WAVEIN_ERROR("waveInOpen", st);
        return NULL;
    }
    return h;
}

static int32
wavein_close(rec_t * r)
{
    int32 i, st;

    /* Unprepare all buffers */
    for (i = 0; i < r->n_buf; i++) {
        if (!(r->wi_buf[i].p_whdr->dwFlags & WHDR_PREPARED))
            continue;
        st = waveInUnprepareHeader(r->h_wavein,
                                   r->wi_buf[i].p_whdr, sizeof(WAVEHDR));
        if (st != 0) {
            WAVEIN_ERROR("waveInUnprepareHeader", st);
            return -1;
        }
    }
    /* Free buffers */
    for (i = 0; i < r->n_buf; i++)
        wavein_free_buf(&(r->wi_buf[i]));
    free(r->wi_buf);
    if ((st = waveInClose(r->h_wavein)) != 0) {
        WAVEIN_ERROR("waveInClose", st);
        return -1;
    }
    free(r);
    return 0;
}

rec_t *
wave_open(int32 sps, int32 nbuf, int32 bufsize)
{
    rec_t *r;
    int32 i, j;
    HWAVEIN h;

    if ((h = wavein_open(sps)) == NULL)
        return NULL;

    if ((r = (rec_t *) calloc(1, sizeof(rec_t))) == NULL) {
        fprintf(stderr, "calloc(1, %d) failed\n", sizeof(rec_t));
        waveInClose(h);
        return NULL;
    }
    r->n_buf = nbuf;
    if (r->n_buf < 32)
        r->n_buf = 32;
    if ((r->wi_buf =
         (wbuf_t *) calloc(r->n_buf, sizeof(wbuf_t))) == NULL) {
        fprintf(stderr, "calloc(%d, %d) failed\n", r->n_buf,
                sizeof(wbuf_t));
        free(r);
        waveInClose(h);
        return NULL;
    }
    for (i = 0; i < r->n_buf; i++) {
        if (wavein_alloc_buf(&(r->wi_buf[i]), bufsize) < 0) {
            for (j = 0; j < i; j++)
                wavein_free_buf(&(r->wi_buf[j]));
            free(r->wi_buf);
            free(r);
            waveInClose(h);
            return NULL;
        }
    }
    r->h_wavein = h;
    r->recording = 0;
    r->curbuf = r->n_buf - 1;   /* current buffer with data for application */
    r->lastbuf = r->curbuf;
    r->sps = sps;
    r->bps = 2;
    return r;
}

int32
wave_close(rec_t * r)
{
    if (r->recording)
        if (wave_stop_recording(r) < 0)
            return -1;

    if (wavein_close(r) < 0)
        return -1;

    return 0;
}

int32
wave_start_recording(rec_t * r)
{
    int32 i;

    if (r->recording)
        return -1;
    for (i = 0; i < r->n_buf; i++)
        if (wavein_enqueue_buf(r->h_wavein, r->wi_buf[i].p_whdr) < 0)
            return -1;
    r->curbuf = r->n_buf - 1;   /* current buffer with data for application */
    if (waveInStart(r->h_wavein) != 0)
        return -1;
    r->recording = 1;
    return 0;
}

int32
wave_stop_recording(rec_t * r)
{
    int32 i, st;

    if (!r->recording)
        return -1;
    if (waveInStop(r->h_wavein) != 0)
        return -1;
    if ((st = waveInReset(r->h_wavein)) != 0) {
        WAVEIN_ERROR("waveInReset", st);
        return -1;
    }
    /* BUSY-Wait until all buffers marked done (YARK!) */
    for (i = 0; i < r->n_buf; i++)
        while (!(r->wi_buf[i].p_whdr->dwFlags & WHDR_DONE));
    if ((r->lastbuf = r->curbuf - 1) < 0)
        r->lastbuf = r->n_buf - 1;
    r->recording = 0;
    return 0;
}

int32
wave_read_buf(rec_t *r, int16 *buf)
{
    LPWAVEHDR whdr;

    /* Check if all recorded data exhausted */
    if ((!r->recording) && (r->curbuf == r->lastbuf))
        return 0;
    /* Look for next buffer with recording data */
    if (++r->curbuf >= r->n_buf)
        r->curbuf = 0;
    if (!(r->wi_buf[r->curbuf].p_whdr->dwFlags & WHDR_DONE))
        return 0;
    /* Copy data from curbuf to buf */
    memcpy(buf, whdr->lpData, r->wi_buf[t].p_whdr->dwBytesRecorded);
    if (r->recording) {
        /* Return empty buffer to system */
        int st = waveInUnprepareHeader(r->h_wavein,
                                       whdr, sizeof(WAVEHDR));
        if (st != 0) {
            WAVEIN_ERROR("waveInUnprepareHeader", st);
            return -1;
        }
        if (wavein_enqueue_buf(r->h_wavein, whdr) < 0)
            return -1;
    }
    return len;
}

static int global_done = 0;
static void
catch_sig(int signum)
{
    (void)signum;
    global_done = 1;
}

int
main(int argc, char *argv[])
{

    int err;
    ps_decoder_t *decoder;
    ps_config_t *config;
    ps_endpointer_t *ep;
    short *frame;
    size_t frame_size;
    rec_t *rec;

    (void)argc; (void)argv;

    config = ps_config_init(NULL);
    ps_default_search_args(config);
    if ((decoder = ps_init(config)) == NULL)
        E_FATAL("PocketSphinx decoder init failed\n");
    if ((ep = ps_endpointer_init(0, 0.0, 0,
                                 ps_config_int(config, "samprate"), 0)) == NULL)
        E_FATAL("PocketSphinx endpointer init failed\n");
    frame_size = ps_endpointer_frame_size(ep);
    if ((rec = wave_open(ps_endpointer_sample_rate(ep),
                         32, frame_size * sizeof(frame[0]))) == NULL)
        E_FATAL("Failed to open audio");
    if ((frame = malloc(frame_size * sizeof(frame[0]))) == NULL)
        E_FATAL_SYSTEM("Failed to allocate frame");
    if (signal(SIGINT, catch_sig) == SIG_ERR)
        E_FATAL_SYSTEM("Failed to set SIGINT handler");
    if ((err = wave_start_recording(rec)) < 0)
        E_FATAL("Failed to start recording\n");
    while (!global_done) {
        const int16 *speech;
        int prev_in_speech = ps_endpointer_in_speech(ep);
        if (wave_read(s, frame) < 0) {
            E_ERROR("Failed to read frame");
            break;
        }
        speech = ps_endpointer_process(ep, frame);
        if (speech != NULL) {
            const char *hyp;
            if (!prev_in_speech) {
                fprintf(stderr, "Speech start at %.2f\n",
                        ps_endpointer_speech_start(ep));
                ps_start_utt(decoder);
            }
            if (ps_process_raw(decoder, speech, frame_size, FALSE, FALSE) < 0)
                E_FATAL("ps_process_raw() failed\n");
            if ((hyp = ps_get_hyp(decoder, NULL)) != NULL)
                fprintf(stderr, "PARTIAL RESULT: %s\n", hyp);
            if (!ps_endpointer_in_speech(ep)) {
                fprintf(stderr, "Speech end at %.2f\n",
                        ps_endpointer_speech_end(ep));
                ps_end_utt(decoder);
                if ((hyp = ps_get_hyp(decoder, NULL)) != NULL)
                    printf("%s\n", hyp);
            }
        }
    }
    if ((err = wave_stop_recording(rec)) < 0)
        E_FATAL("Failed to start recording\n");
    if ((err = wave_close(rec)) < 0)
        E_FATAL("Failed to close audio\n");
    free(frame);
    ps_endpointer_free(ep);
    ps_free(decoder);
    ps_config_free(config);
        
    return 0;
}
