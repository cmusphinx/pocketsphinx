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
    short *frame;
    size_t frame_size;
    int err;
    HWAVEIN wavein;
    WAVEFORMATEX wavefmt;
    HANDLE event;
    WAVEHDR hdrs[32];
    int i;
    size_t bufsize;

    /* Initialize decoder and endpointer */
    config = ps_config_init(NULL);
    ps_default_search_args(config);
    ps_config_set_str(config, "loglevel", "INFO");
    if ((decoder = ps_init(config)) == NULL)
        E_FATAL("PocketSphinx decoder init failed\n");
    if ((ep = ps_endpointer_init(0, 0.0, 0,
                                 ps_config_int(config, "samprate"),
                                 0)) == NULL)
        E_FATAL("PocketSphinx endpointer init failed\n");
    /* Frame size in samples (not bytes) */
    frame_size = ps_endpointer_frame_size(ep);
    if ((frame = malloc(frame_size * 2)) == NULL)
        E_FATAL_SYSTEM("Failed to allocate frame");
    /* Tell Windows what format we want (NOTE: may not be available...) */
    wavefmt.wFormatTag = WAVE_FORMAT_PCM;
    wavefmt.nChannels = 1;
    wavefmt.nSamplesPerSec = ps_endpointer_sample_rate(ep);
    wavefmt.wBitsPerSample = 16;
    wavefmt.nBlockAlign = 2;
    wavefmt.nAvgBytesPerSec = wavefmt.nSamplesPerSec * wavefmt.nBlockAlign;
    wavefmt.cbSize = 0;
    /* Create an event to tell us when a new buffer is ready. */
    event = CreateEventA(NULL, TRUE, FALSE, "buffer_ready");
    /* Open the recording device. */
    CHECK(waveInOpen(&wavein, WAVE_MAPPER, &wavefmt,
                     (DWORD_PTR)event, 0, CALLBACK_EVENT));
    /* Create a good but arbitrary number of buffers. */
    memset(hdrs, 0, sizeof(hdrs));
    bufsize = frame_size * 2;
    for (i = 0; i < 32; ++i) {
        hdrs[i].lpData = malloc(bufsize);
        hdrs[i].dwBufferLength = (DWORD)bufsize;
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
        int prev_in_speech = ps_endpointer_in_speech(ep);
        WaitForSingleObject(event, INFINITE);
        /* Get as many buffers as we can. */
        while (hdrs[i].dwFlags & WHDR_DONE) {
            CHECK(waveInUnprepareHeader(wavein, &hdrs[i], sizeof(hdrs[i])));
            CHECK(waveInPrepareHeader(wavein, &hdrs[i], sizeof(hdrs[i])));
            CHECK(waveInAddBuffer(wavein, &hdrs[i], sizeof(hdrs[i])));
            if (++i == 32)
                i = 0;
            /* Process them one by one. */
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
        /* Wait for another buffer. */
        ResetEvent(event);
    }
    /* Stop recording, cancel all buffers, and free them. */
    CHECK(waveInStop(wavein));
    CHECK(waveInReset(wavein));
    for (i = 0; i < 32; ++i) {
        if (hdrs[i].dwFlags & WHDR_PREPARED)
            CHECK(waveInUnprepareHeader(wavein, &hdrs[i],
                                        sizeof(hdrs[i])));
        free(hdrs[i].lpData);
    }

    return 0;
}
