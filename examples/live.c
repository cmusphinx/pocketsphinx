/* Example of simple PocketSphinx speech segmentation.
 *
 * MIT license (c) 2022, see LICENSE for more information.
 *
 * Author: David Huggins-Daines <dhdaines@gmail.com>
 */
/**
 * @example live.c
 * @brief Speech recognition with live audio input and endpointing.
 *
 * This file shows how to use PocketSphinx in conjunction with `sox`
 * to detect and recognize speech from the default audio input device.
 *
 * This file shows how to use PocketSphinx to recognize a single input
 * file.  To compile it, assuming you have built the library as in
 * \ref unix_install "these directions", you can run:
 *
 *     cmake --build build --target live
 *
 * Alternately, if PocketSphinx is installed system-wide, you can run:
 *
 *     gcc -o live live.c $(pkg-config --libs --cflags pocketsphinx)
 *
 * Sadly, this example does *not* seem to work on Windows, even if you
 * manage to get `sox` in your `PATH` (which is not easy), because it
 * seems that it can't actually read from the microphone.  Try
 * live_win32.c or live_portaudio.c instead.
 */
#include <pocketsphinx.h>
#include <signal.h>

static int global_done = 0;
static void
catch_sig(int signum)
{
    (void)signum;
    global_done = 1;
}

#ifdef WIN32
#define popen _popen
#define pclose _pclose
#endif

static FILE *
popen_sox(int sample_rate)
{
    char *soxcmd;
    int len;
    FILE *sox;
    #define SOXCMD "sox -q -r %d -c 1 -b 16 -e signed-integer -d -t raw -"
    len = snprintf(NULL, 0, SOXCMD, sample_rate);
    if ((soxcmd = malloc(len + 1)) == NULL)
        E_FATAL_SYSTEM("Failed to allocate string");
    if (snprintf(soxcmd, len + 1, SOXCMD, sample_rate) != len)
        E_FATAL_SYSTEM("snprintf() failed");
    if ((sox = popen(soxcmd, "r")) == NULL)
        E_FATAL_SYSTEM("Failed to popen(%s)", soxcmd);
    free(soxcmd);

    return sox;
}

int
main(int argc, char *argv[])
{
    ps_decoder_t *decoder;
    ps_config_t *config;
    ps_endpointer_t *ep;
    FILE *sox;
    short *frame;
    size_t frame_size;

    (void)argc; (void)argv;
    config = ps_config_init(NULL);
    ps_default_search_args(config);
    if ((decoder = ps_init(config)) == NULL)
        E_FATAL("PocketSphinx decoder init failed\n");

    if ((ep = ps_endpointer_init(0, 0.0, 0, 0, 0)) == NULL)
        E_FATAL("PocketSphinx endpointer init failed\n");
    sox = popen_sox(ps_endpointer_sample_rate(ep));
    frame_size = ps_endpointer_frame_size(ep);
    if ((frame = malloc(frame_size * sizeof(frame[0]))) == NULL)
        E_FATAL_SYSTEM("Failed to allocate frame");
    if (signal(SIGINT, catch_sig) == SIG_ERR)
        E_FATAL_SYSTEM("Failed to set SIGINT handler");
    while (!global_done) {
        const int16 *speech;
        int prev_in_speech = ps_endpointer_in_speech(ep);
        size_t len, end_samples;
        if ((len = fread(frame, sizeof(frame[0]),
                         frame_size, sox)) != frame_size) {
            if (len > 0) {
                speech = ps_endpointer_end_stream(ep, frame,
                                                  frame_size,
                                                  &end_samples);
            }
            else
                break;
        } else {
            speech = ps_endpointer_process(ep, frame);
        }
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
    free(frame);
    if (pclose(sox) < 0)
        E_ERROR_SYSTEM("Failed to pclose(sox)");
    ps_endpointer_free(ep);
    ps_free(decoder);
    ps_config_free(config);
        
    return 0;
}
