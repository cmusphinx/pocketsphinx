/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/**
 * Test VAD allocation safety
 */

#include <stdio.h>
#include <pocketsphinx.h>
#include "common_audio/vad/include/webrtc_vad.h"

#include "test_macros.h"

int
main(int argc, char *argv[])
{
    VadInst *vad;
    int i;

    (void)argc;
    (void)argv;

    /* Test repeated create/destroy */
    for (i = 0; i < 100; i++) {
        vad = WebRtcVad_Create();
        TEST_ASSERT(vad != NULL);
        TEST_ASSERT(WebRtcVad_Init(vad) == 0);
        WebRtcVad_Free(vad);
    }

    /* Test NULL handling */
    WebRtcVad_Free(NULL);

    return 0;
}
