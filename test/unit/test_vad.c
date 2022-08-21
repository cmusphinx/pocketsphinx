/* Test voice activity detection.
 *
 * MIT license (c) 2022, see LICENSE for more information.
 *
 * Author: David Huggins-Daines <dhdaines@gmail.com>
 */

#include <pocketsphinx.h>
#include "test_macros.h"

const char *expecteds[] = {
    "011110111111111111111111111100",
    "011110111111111111111111111100",
    "000000111111111111111111110000",
    "000000111111111111111100000000"
};

int
main(int argc, char *argv[])
{
    ps_vad_t *vader;
    int mode;
    short *frame;

    (void)argc; (void)argv;
    /* Test initialization with default parameters. */
    vader = ps_vad_init(0, 0, 0);
    TEST_ASSERT(vader);
    /* Retain and release, should still be there. */
    TEST_ASSERT((vader = ps_vad_retain(vader)));
    TEST_ASSERT(ps_vad_free(vader));

    /* Test default frame size. */
    TEST_EQUAL(ps_vad_frame_size(vader),
               (int)(PS_VAD_DEFAULT_SAMPLE_RATE * PS_VAD_DEFAULT_FRAME_LENGTH));
    TEST_EQUAL(ps_vad_frame_length(vader), PS_VAD_DEFAULT_FRAME_LENGTH);
    TEST_ASSERT(ps_vad_free(vader) == 0);

    /* Test VAD modes with py-webrtcvad test data. */
    for (mode = 0; mode < 4; ++mode) {
        FILE *fh;
        size_t frame_size;
        char *classification, *c;

        c = classification = ckd_calloc(1, strlen(expecteds[mode]) + 1);
        vader = ps_vad_init(mode, 8000, 0.03);
        TEST_ASSERT(vader);
        frame_size = ps_vad_frame_size(vader);
        frame = ckd_calloc(sizeof(*frame), frame_size);
        TEST_ASSERT(frame);
        fh = fopen(DATADIR "/vad/test-audio.raw", "rb");
        TEST_ASSERT(fh);
        while (fread(frame, sizeof(*frame), frame_size, fh) == frame_size) {
            int is_speech = ps_vad_classify(vader, frame);
            TEST_ASSERT(is_speech != PS_VAD_ERROR);
            *c++ = (is_speech == PS_VAD_SPEECH) ? '1' : '0';
        }
        TEST_EQUAL(0, strcmp(expecteds[mode], classification));
        ckd_free(classification);
        ps_vad_free(vader);
        ckd_free(frame);
        fclose(fh);
    }

    /* Test a variety of sampling rates. */

    return 0;
}
