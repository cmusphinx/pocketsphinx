/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/**
 * Example of using external timestamp sources with ps_endpointer_t
 * to avoid timestamp drift between audio and system clocks.
 */

#include <stdio.h>
#include <time.h>
#ifdef __APPLE__
#include <sys/time.h>
#endif
#include <pocketsphinx.h>

/* Get system time in seconds (monotonic clock) */
static double
get_system_time(void *user_data)
{
    struct timespec ts;
    (void)user_data; /* unused */

#ifdef __APPLE__
    /* macOS doesn't have clock_gettime before 10.12 */
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
#else
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1000000000.0;
#endif
}

int
main(int argc, char *argv[])
{
    ps_endpointer_t *ep;
    int16 frame[480]; /* Frame buffer - size determined by endpointer */
    int frame_size;
    double start_time;
    int i;

    (void)argc;
    (void)argv;

    /* Initialize endpointer with default settings */
    ep = ps_endpointer_init(0, 0, PS_VAD_LOOSE, 16000, 0.0);
    if (ep == NULL) {
        fprintf(stderr, "Failed to initialize endpointer\n");
        return 1;
    }

    frame_size = ps_endpointer_frame_size(ep);
    printf("Endpointer frame size: %d samples\n", frame_size);

    /* Set external timestamp callback to use system time */
    if (ps_endpointer_set_timestamp_func(ep, get_system_time, NULL) < 0) {
        fprintf(stderr, "Failed to set timestamp callback\n");
        ps_endpointer_free(ep);
        return 1;
    }

    start_time = get_system_time(NULL);
    printf("Starting at system time: %.3f\n", start_time);

    /* Simulate processing audio frames */
    printf("\nProcessing frames with system timestamps:\n");
    for (i = 0; i < 100; i++) {
        const int16 *speech;

        /* Generate dummy audio data (silence in this example) */
        int j;
        for (j = 0; j < frame_size; j++)
            frame[j] = 0;

        /* Process frame */
        speech = ps_endpointer_process(ep, frame);

        /* Every 10 frames, print timestamp */
        if (i % 10 == 9) {
            double current_time = ps_endpointer_timestamp(ep);
            double elapsed = current_time - start_time;
            printf("Frame %3d: System time: %.3f, Elapsed: %.3f sec\n",
                   i + 1, current_time, elapsed);
        }

        /* If speech is detected (won't happen with silence) */
        if (speech != NULL) {
            printf("Speech detected at: %.3f\n",
                   ps_endpointer_speech_start(ep));
        }
    }

    /* Demonstrate reverting to audio timestamps */
    printf("\nReverting to audio-based timestamps:\n");
    ps_endpointer_set_timestamp_func(ep, NULL, NULL);

    for (i = 0; i < 20; i++) {
        const int16 *speech;
        int j;

        for (j = 0; j < frame_size; j++)
            frame[j] = 0;

        speech = ps_endpointer_process(ep, frame);
        (void)speech; /* unused */
    }

    /* Audio timestamp continues from where it was */
    printf("Audio timestamp after 20 more frames: %.3f\n",
           ps_endpointer_timestamp(ep));

    ps_endpointer_free(ep);
    printf("\nExample completed successfully.\n");

    return 0;
}
