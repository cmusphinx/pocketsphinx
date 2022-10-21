/* Example of simple PocketSphinx speech segmentation.
 *
 * MIT license (c) 2022, see LICENSE for more information.
 *
 * Author: David Huggins-Daines <dhdaines@gmail.com>
 */
/**
 * @example live_portaudio.c
 * @brief Speech recognition with live audio input and endpointing.
 *
 * This file shows how to use PocketSphinx with microphone input using
 * PortAudio (v19 and above).
 *
 * To compile it, assuming you have built the library as in
 * \ref unix_install "these directions", you can run:
 *
 *     cmake --build build --target live_portaudio
 *
 * Alternately, if PocketSphinx is installed system-wide, you can run:
 *
 *     gcc -o live_portaudio live_portaudio.c \
 *         $(pkg-config --libs --cflags pocketsphinx portaudio-2.0)
 *
 *
 */
#include <portaudio.h>
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

    PaStream *stream;
    PaError err;
    ps_decoder_t *decoder;
    ps_config_t *config;
    ps_endpointer_t *ep;
    short *frame;
    size_t frame_size;

    (void)argc; (void)argv;

    config = ps_config_init(NULL);
    ps_default_search_args(config);

    if ((err = Pa_Initialize()) != paNoError)
        E_FATAL("Failed to initialize PortAudio: %s\n",
                Pa_GetErrorText(err));
    if ((decoder = ps_init(config)) == NULL)
        E_FATAL("PocketSphinx decoder init failed\n");
    if ((ep = ps_endpointer_init(0, 0.0, 0, 0, 0)) == NULL)
        E_FATAL("PocketSphinx endpointer init failed\n");
    frame_size = ps_endpointer_frame_size(ep);
    if ((frame = malloc(frame_size * sizeof(frame[0]))) == NULL)
        E_FATAL_SYSTEM("Failed to allocate frame");
    if ((err = Pa_OpenDefaultStream(&stream, 1, 0, paInt16,
                                    ps_config_int(config, "samprate"),
                                    frame_size, NULL, NULL)) != paNoError)
        E_FATAL("Failed to open PortAudio stream: %s\n",
                Pa_GetErrorText(err));
    if ((err = Pa_StartStream(stream)) != paNoError)
        E_FATAL("Failed to start PortAudio stream: %s\n",
                Pa_GetErrorText(err));
    if (signal(SIGINT, catch_sig) == SIG_ERR)
        E_FATAL_SYSTEM("Failed to set SIGINT handler");
    while (!global_done) {
        const int16 *speech;
        int prev_in_speech = ps_endpointer_in_speech(ep);
        if ((err = Pa_ReadStream(stream, frame, frame_size)) != paNoError) {
            E_ERROR("Error in PortAudio read: %s\n",
                Pa_GetErrorText(err));
            break;
        }
        speech = ps_endpointer_process(ep, frame);
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
    if ((err = Pa_StopStream(stream)) != paNoError)
        E_FATAL("Failed to stop PortAudio stream: %s\n",
                Pa_GetErrorText(err));
    if ((err = Pa_Terminate()) != paNoError)
        E_FATAL("Failed to terminate PortAudio: %s\n",
                Pa_GetErrorText(err));
    free(frame);
    ps_endpointer_free(ep);
    ps_free(decoder);
    ps_config_free(config);
        
    return 0;
}
