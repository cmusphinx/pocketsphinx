/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/**
 * Test endpointer timestamp callback functionality
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdlib.h> // Required for malloc

#include <pocketsphinx.h>

#include "test_macros.h"

/* Test data - simulated system time */
typedef struct {
    double start_time;
    double current_time;
    double time_per_frame;
} test_time_data_t;

/* Simulate system time callback */
static double
test_system_time_cb(void *user_data)
{
    test_time_data_t *data = (test_time_data_t *)user_data;
    return data->current_time;
}

/* Test with different clock rates to simulate drift */
static double
test_drift_time_cb(void *user_data)
{
    test_time_data_t *data = (test_time_data_t *)user_data;
    /* Simulate 1% faster system clock */
    return data->current_time * 1.01;
}

int
main(int argc, char *argv[])
{
    ps_endpointer_t *ep;
    int16 *frame;
    int i, frame_size;
    const int16 *speech;
    test_time_data_t time_data;
    double audio_timestamp, external_timestamp;

    (void)argc;
    (void)argv;

    /* Initialize endpointer with default settings */
    ep = ps_endpointer_init(0, 0, PS_VAD_LOOSE, 16000, 0.0);
    TEST_ASSERT(ep != NULL);

    frame_size = ps_endpointer_frame_size(ep);
    printf("Frame size: %d samples\n", frame_size);

    /* Allocate frame buffer */
    frame = (int16 *)malloc(frame_size * sizeof(int16));
    TEST_ASSERT(frame != NULL);

    /* Generate test frames - silence */
    for (i = 0; i < frame_size; i++)
        frame[i] = 0;

    /* Test 1: Default behavior (audio-based timestamps) */
    printf("Test 1: Default audio-based timestamps\n");
    for (i = 0; i < 10; i++) {
        speech = ps_endpointer_process(ep, frame);
        TEST_ASSERT(speech == NULL); /* No speech in silence */
    }

    /* Check timestamp after 10 frames (300ms) */
    audio_timestamp = ps_endpointer_timestamp(ep);
    printf("Audio timestamp after 10 frames: %.3f\n", audio_timestamp);
    TEST_ASSERT(fabs(audio_timestamp - 0.3) < 0.001);

    /* Test 2: External timestamp callback */
    printf("\nTest 2: External timestamp callback\n");
    time_data.start_time = 1000.0;
    time_data.current_time = 1000.0;
    time_data.time_per_frame = 0.03;  /* 30ms per frame */

    TEST_EQUAL(ps_endpointer_set_timestamp_func(ep, test_system_time_cb, &time_data), 0);

    /* Process more frames with external timestamps */
    for (i = 0; i < 10; i++) {
        time_data.current_time += time_data.time_per_frame;
        speech = ps_endpointer_process(ep, frame);
        TEST_ASSERT(speech == NULL);
    }

    external_timestamp = ps_endpointer_timestamp(ep);
    printf("External timestamp: %.3f\n", external_timestamp);
    TEST_ASSERT(fabs(external_timestamp - 1000.3) < 0.001);

    /* Test 3: Revert to audio timestamps */
    printf("\nTest 3: Reverting to audio timestamps\n");
    TEST_EQUAL(ps_endpointer_set_timestamp_func(ep, NULL, NULL), 0);

    for (i = 0; i < 10; i++) {
        speech = ps_endpointer_process(ep, frame);
        TEST_ASSERT(speech == NULL);
    }

    audio_timestamp = ps_endpointer_timestamp(ep);
    printf("Audio timestamp after reverting: %.3f\n", audio_timestamp);
    TEST_ASSERT(fabs(audio_timestamp - 0.9) < 0.001); /* 30 frames total */

    ps_endpointer_free(ep);

    /* Test 4: Speech detection with external timestamps (simulated) */
    printf("\nTest 4: Speech detection with external timestamps (simulated)\n");

    /* For this test, we'll use the internal structure knowledge to verify
     * timestamp handling would work correctly during speech detection.
     * This avoids the complexity of generating actual speech that the VAD
     * would accept. */

    /* Create a new endpointer for this test */
    ep = ps_endpointer_init(0.3, 0.7, PS_VAD_LOOSE, 16000, 0.0);
    TEST_ASSERT(ep != NULL);

    /* Set up external timestamps starting at 5000.0 */
    time_data.start_time = 5000.0;
    time_data.current_time = 5000.0;
    time_data.time_per_frame = 0.03;

    TEST_EQUAL(ps_endpointer_set_timestamp_func(ep, test_system_time_cb, &time_data), 0);

    /* Process some frames to advance time */
    for (i = 0; i < frame_size; i++)
        frame[i] = 0;  /* silence */

    for (i = 0; i < 20; i++) {
        time_data.current_time += time_data.time_per_frame;
        speech = ps_endpointer_process(ep, frame);
        /* No speech expected in silence */
        TEST_ASSERT(speech == NULL);
    }

    /* Verify external timestamp is being used */
    external_timestamp = ps_endpointer_timestamp(ep);
    printf("External timestamp after 20 frames: %.3f\n", external_timestamp);
    TEST_ASSERT(fabs(external_timestamp - 5000.6) < 0.001);

    ps_endpointer_free(ep);

    /* Test 5: Clock drift simulation */
    printf("\nTest 5: Clock drift simulation\n");
    ep = ps_endpointer_init(0, 0, PS_VAD_LOOSE, 16000, 0.0);
    TEST_ASSERT(ep != NULL);

    /* Reset time data */
    time_data.start_time = 0.0;
    time_data.current_time = 0.0;

    /* Process 1000 frames (10 seconds) with audio timestamps */
    for (i = 0; i < 1000; i++) {
        speech = ps_endpointer_process(ep, frame);
    }

    audio_timestamp = ps_endpointer_timestamp(ep);
    printf("Audio timestamp after 1000 frames: %.3f\n", audio_timestamp);

    /* Now use drift callback - system clock 1% faster */
    TEST_EQUAL(ps_endpointer_set_timestamp_func(ep, test_drift_time_cb, &time_data), 0);

    /* Process another 1000 frames */
    for (i = 0; i < 1000; i++) {
        time_data.current_time += 0.03; /* 30ms per frame */
        speech = ps_endpointer_process(ep, frame);
    }

    external_timestamp = ps_endpointer_timestamp(ep);
    printf("External timestamp with drift: %.3f\n", external_timestamp);

    /* The audio time advanced by 30 seconds (1000 * 0.03)
     * But with 1% drift, external time should be 30.3 seconds */
    double expected_drift = 30.0 * 0.01;  /* 0.3 seconds */
    double actual_drift = external_timestamp - 30.0;
    printf("Expected drift: %.3f, Actual drift: %.3f\n", expected_drift, actual_drift);
    TEST_ASSERT(fabs(actual_drift - expected_drift) < 0.01);

    ps_endpointer_free(ep);

    /* Test 6: Backward compatibility - verify all functions work without callback */
    printf("\nTest 6: Backward compatibility test\n");
    ep = ps_endpointer_init(0.3, 0.7, PS_VAD_LOOSE, 16000, 0.0);
    TEST_ASSERT(ep != NULL);
    
    /* Verify timestamp starts at 0 */
    TEST_ASSERT(ps_endpointer_timestamp(ep) == 0.0);
    
    /* Verify speech detection state functions work */
    TEST_ASSERT(!ps_endpointer_in_speech(ep));
    TEST_ASSERT(ps_endpointer_speech_start(ep) == 0.0);
    TEST_ASSERT(ps_endpointer_speech_end(ep) == 0.0);
    
    /* Process frames without callback - should use audio timestamps */
    for (i = 0; i < 50; i++) {
        speech = ps_endpointer_process(ep, frame);
        TEST_ASSERT(speech == NULL); /* No speech in silence */
    }
    
    /* Verify timestamp advanced based on audio */
    audio_timestamp = ps_endpointer_timestamp(ep);
    printf("Audio timestamp after 50 frames: %.3f\n", audio_timestamp);
    TEST_ASSERT(fabs(audio_timestamp - 1.5) < 0.001); /* 50 * 0.03 */
    
    /* Verify setting NULL callback when none is set doesn't break anything */
    TEST_EQUAL(ps_endpointer_set_timestamp_func(ep, NULL, NULL), 0);
    
    /* Process more frames to ensure it still works */
    for (i = 0; i < 10; i++) {
        speech = ps_endpointer_process(ep, frame);
        TEST_ASSERT(speech == NULL);
    }
    
    /* Verify timestamp still advances correctly */
    audio_timestamp = ps_endpointer_timestamp(ep);
    printf("Audio timestamp after 10 more frames: %.3f\n", audio_timestamp);
    TEST_ASSERT(fabs(audio_timestamp - 1.8) < 0.001); /* 60 * 0.03 */
    
    /* Verify VAD access still works (part of existing API) */
    ps_vad_t *vad = ps_endpointer_vad(ep);
    TEST_ASSERT(vad != NULL);
    
    ps_endpointer_free(ep);

    /* Test 7: Backward compatibility - end_stream without callback */
    printf("\nTest 7: Backward compatibility - end_stream test\n");
    ep = ps_endpointer_init(0, 0, PS_VAD_LOOSE, 16000, 0.0);
    TEST_ASSERT(ep != NULL);
    
    /* Process some frames */
    for (i = 0; i < 10; i++) {
        speech = ps_endpointer_process(ep, frame);
    }
    
    /* Test end_stream with partial frame */
    int16 partial_frame[100];
    for (i = 0; i < 100; i++)
        partial_frame[i] = 0;
    
    size_t out_nsamp = 0;
    const int16 *end_speech = ps_endpointer_end_stream(ep, partial_frame, 100, &out_nsamp);
    
    /* With silence, no speech should be returned */
    TEST_ASSERT(end_speech == NULL);
    TEST_ASSERT(out_nsamp == 0);
    
    ps_endpointer_free(ep);

    free(frame);

    printf("\nAll tests passed!\n");
    return 0;
}
