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
 * the Win32 Waveform Audio API (the only one of many terrible audio
 * APIs on Windows that isn't made even more terrible by requiring you
 * to use C++ in an unmanaged environment).
 *
 * To build it, you should be able to find a "live_win32" target in
 * your favorite IDE after running CMake - in Visual Studio Code, look
 * in the "CMake" tab.
 *
 * Microphones on Windows tend to be miscalibrated with the recording
 * level set much too high by default, so the endpointer may give a
 * lot of false positives at first.  Programs like Audacity seem to
 * work around this somehow, but I don't really know how they do it.
 */

#include <windows.h>
#include <mmsystem.h>
#include <pocketsphinx.h>
#include <signal.h>

static int global_done = 0;
static void
catch_sig(int signum)
{
    (void)signum;
    global_done = 1;
}

#define CHECK(expr)                                             \
    do {                                                        \
        int err;                                                \
        if ((err = expr) != 0)                                  \
        {                                                       \
            char errbuf[MAXERRORLENGTH];                        \
            waveInGetErrorText(err, errbuf, sizeof(errbuf));    \
            E_FATAL("error %08x: %s\n", err, errbuf);           \
        }                                                       \
    } while (0)

int main(int argc, char *argv[])
{
    ps_decoder_t *decoder;
    ps_config_t *config;
    ps_endpointer_t *ep;
    size_t frame_size;
    HWAVEIN wavein;
    WAVEFORMATEX wavefmt;
    HANDLE event;
    /* A large but somewhat arbitrary number of buffers. */
#define NBUF 100 /* 100 * 0.03 = 3 seconds */
    WAVEHDR hdrs[NBUF];
    int i;

    (void)argc; (void)argv;
    /* Initialize decoder and endpointer */
    config = ps_config_init(NULL);
    ps_default_search_args(config);
    if ((decoder = ps_init(config)) == NULL)
        E_FATAL("PocketSphinx decoder init failed\n");
    if ((ep = ps_endpointer_init(0, 0.0, 0,
                                 ps_config_int(config, "samprate"),
                                 0)) == NULL)
        E_FATAL("PocketSphinx endpointer init failed\n");
    /* Frame size in samples (not bytes) */
    frame_size = ps_endpointer_frame_size(ep);
    /* Tell Windows what format we want (NOTE: may not be available...) */
    wavefmt.wFormatTag = WAVE_FORMAT_PCM;
    wavefmt.nChannels = 1;
    wavefmt.nSamplesPerSec = ps_endpointer_sample_rate(ep);
    wavefmt.wBitsPerSample = 16;
    wavefmt.nBlockAlign = 2;
    wavefmt.nAvgBytesPerSec = wavefmt.nSamplesPerSec * wavefmt.nBlockAlign;
    wavefmt.cbSize = 0;
    /* Create an event to tell us when a new buffer is ready. */
    event = CreateEvent(NULL, TRUE, FALSE, "buffer_ready");
    /* Open the recording device. */
    CHECK(waveInOpen(&wavein, WAVE_MAPPER, &wavefmt,
                     (DWORD_PTR)event, 0, CALLBACK_EVENT));
    /* Create buffers. */
    memset(hdrs, 0, sizeof(hdrs));
    for (i = 0; i < NBUF; ++i) {
        hdrs[i].lpData = malloc(frame_size * 2);
        hdrs[i].dwBufferLength = (DWORD)frame_size * 2;
        CHECK(waveInPrepareHeader(wavein, &hdrs[i], sizeof(hdrs[i])));
        CHECK(waveInAddBuffer(wavein, &hdrs[i], sizeof(hdrs[i])));
    }
    /* Start recording. */
    CHECK(waveInStart(wavein));
    i = 0;
    if (signal(SIGINT, catch_sig) == SIG_ERR)
        E_FATAL_SYSTEM("Failed to set SIGINT handler");
    while (!global_done) {
        const int16 *speech;
        WaitForSingleObject(event, INFINITE);
        /* Get as many buffers as we can. */
        while (hdrs[i].dwFlags & WHDR_DONE) {
            int prev_in_speech = ps_endpointer_in_speech(ep);
            int16 *frame = (int16 *)hdrs[i].lpData;
            /* Process them one by one. */
            speech = ps_endpointer_process(ep, frame);
            CHECK(waveInUnprepareHeader(wavein, &hdrs[i], sizeof(hdrs[i])));
            CHECK(waveInPrepareHeader(wavein, &hdrs[i], sizeof(hdrs[i])));
            CHECK(waveInAddBuffer(wavein, &hdrs[i], sizeof(hdrs[i])));
            if (++i == NBUF)
                i = 0;
            if (speech != NULL) {
                const char *hyp;
                if (!prev_in_speech) {
                    fprintf(stderr, "Speech start at %.2f\n",
                            ps_endpointer_speech_start(ep));
		    fflush(stderr); /* For broken MSYS2 terminal */
                    ps_start_utt(decoder);
                }
                if (ps_process_raw(decoder, speech, frame_size, FALSE, FALSE) < 0)
                    E_FATAL("ps_process_raw() failed\n");
                if ((hyp = ps_get_hyp(decoder, NULL)) != NULL) {
                    fprintf(stderr, "PARTIAL RESULT: %s\n", hyp);
		    fflush(stderr);
		}
                if (!ps_endpointer_in_speech(ep)) {
                    fprintf(stderr, "Speech end at %.2f\n",
                            ps_endpointer_speech_end(ep));
		    fflush(stderr);
                    ps_end_utt(decoder);
                    if ((hyp = ps_get_hyp(decoder, NULL)) != NULL) {
                        printf("%s\n", hyp);
			fflush(stdout);
		    }
                }
            }
        }
        /* Wait for another buffer. */
        ResetEvent(event);
    }
    /* Stop recording, cancel all buffers, and free them. */
    CHECK(waveInStop(wavein));
    CHECK(waveInReset(wavein));
    for (i = 0; i < NBUF; ++i) {
        if (hdrs[i].dwFlags & WHDR_PREPARED)
            CHECK(waveInUnprepareHeader(wavein, &hdrs[i],
                                        sizeof(hdrs[i])));
        free(hdrs[i].lpData);
    }
    CloseHandle(event);
    ps_endpointer_free(ep);
    ps_free(decoder);
    ps_config_free(config);

    return 0;
}
