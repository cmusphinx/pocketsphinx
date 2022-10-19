/* Example of simple PocketSphinx speech segmentation.
 *
 * MIT license (c) 2022, see LICENSE for more information.
 *
 * Author: David Huggins-Daines <dhdaines@gmail.com>
 */
/**
 * @example live_pulseaudio.c
 * @brief Speech recognition with live audio input and endpointing.
 *
 * This file shows how to use PocketSphinx with microphone input using
 * PulseAudio.
 *
 * To compile it, assuming you have built the library as in
 * \ref unix_install "these directions", you can run:
 *
 *     cmake --build build --target live_pulseaudio
 *
 * Alternately, if PocketSphinx is installed system-wide, you can run:
 *
 *     gcc -o live_pulseaudio live_pulseaudio.c \
 *         $(pkg-config --libs --cflags pocketsphinx libpulse-simple)
 *
 *
 */
#include <pulse/simple.h>
#include <pulse/error.h>
#include <pocketsphinx.h>
#include <signal.h>

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

    pa_simple *s;
    pa_sample_spec ss;
    int err;
    ps_decoder_t *decoder;
    ps_config_t *config;
    ps_endpointer_t *ep;
    short *frame;
    size_t frame_size;

    (void)argc; (void)argv;

    config = ps_config_init(NULL);
    ps_default_search_args(config);
    if ((decoder = ps_init(config)) == NULL)
        E_FATAL("PocketSphinx decoder init failed\n");
    if ((ep = ps_endpointer_init(0, 0.0, 0, 0, 0)) == NULL)
        E_FATAL("PocketSphinx endpointer init failed\n");
    frame_size = ps_endpointer_frame_size(ep);
    if ((frame = malloc(frame_size * sizeof(frame[0]))) == NULL)
        E_FATAL_SYSTEM("Failed to allocate frame");

    ss.format = PA_SAMPLE_S16NE;
    ss.channels = 1;
    ss.rate = ps_config_int(config, "samprate");
    if ((s = pa_simple_new(NULL, "live_pulseaudio", PA_STREAM_RECORD, NULL,
                           "live", &ss, NULL, NULL, &err)) == NULL)
        E_FATAL("Failed to connect to PulseAudio: %s\n",
                pa_strerror(err));
    if (signal(SIGINT, catch_sig) == SIG_ERR)
        E_FATAL_SYSTEM("Failed to set SIGINT handler");
    while (!global_done) {
        const int16 *speech;
        int prev_in_speech = ps_endpointer_in_speech(ep);
        if (pa_simple_read(s, frame,
                           frame_size * sizeof(frame[0]), &err) < 0) {
            E_ERROR("Error in pa_simple_read: %s\n",
                    pa_strerror(err));
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
    pa_simple_free(s);
    free(frame);
    ps_endpointer_free(ep);
    ps_free(decoder);
    ps_config_free(config);
        
    return 0;
}
