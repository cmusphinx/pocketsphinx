/* Test endpointing.
 *
 * MIT license (c) 2022, see LICENSE for more information.
 *
 * Author: David Huggins-Daines <dhdaines@gmail.com>
 */
#include <pocketsphinx.h>
#include "util/ckd_alloc.h"
#include "test_macros.h"

static int sample_rates[] = {
    8000,
    16000,
    32000,
    48000,
    11025,
    22050,
    //44100
};
static const int n_sample_rates = sizeof(sample_rates)/sizeof(sample_rates[0]);

static float labels[] = {
    0.24,
    1.80,
    2.88,
    12.12,
    12.51,
    15.75,
    16.44,
    20.87
};
static const int n_labels = sizeof(labels)/sizeof(labels[0]);


static FILE *
open_data(int sample_rate)
{
    char *soxcmd;
    int len;
    FILE *sox;
#define SOXCMD "sox -q -D -G " TESTDATADIR "/chan3.wav -r %d -t raw -"
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

static int
test_sample_rate(int sample_rate)
{
    ps_endpointer_t *ep;
    size_t frame_size, nsamp, end_nsamp;
    const short *speech;
    short *frame;
    FILE *fh;
    int i;

    E_INFO("Sample rate %d\n", sample_rate);
    ep = ps_endpointer_init(0, 0, 0, sample_rate, 0);
    frame_size = ps_endpointer_frame_size(ep);
    frame = ckd_calloc(sizeof(*frame), frame_size);
    fh = open_data(sample_rate);
    TEST_ASSERT(fh);
    i = 0;
    while ((nsamp = fread(frame, sizeof(*frame), frame_size, fh)) == frame_size) {
        int prev_in_speech = ps_endpointer_in_speech(ep);
        TEST_ASSERT(i < n_labels);
        speech = ps_endpointer_process(ep, frame);
        if (speech != NULL) {
            if (!prev_in_speech) {
                E_INFO("Speech start at %.2f (label %.2f)\n",
                       ps_endpointer_speech_start(ep), labels[i]);
                TEST_ASSERT(fabs(ps_endpointer_speech_start(ep)
                                 - labels[i++]) < 0.3); 
            }
            if (!ps_endpointer_in_speech(ep)) {
                E_INFO("Speech end at %.2f (label %.2f)\n",
                       ps_endpointer_speech_end(ep), labels[i]);
                TEST_ASSERT(fabs(ps_endpointer_speech_end(ep)
                                 /* FIXME: This difference should be smaller */
                                 - labels[i++]) < 0.8);
            }
        }
    }
    speech = ps_endpointer_end_stream(ep, frame, nsamp, &end_nsamp);
    if (speech != NULL) {
        TEST_ASSERT(i < n_labels);
        E_INFO("Speech end at %.2f (label %.2f)\n",
               ps_endpointer_speech_end(ep), labels[i]);
        TEST_ASSERT(fabs(ps_endpointer_speech_end(ep)
                         - labels[i]) < 0.3);
    }
    pclose(fh);
    ckd_free(frame);
    ps_endpointer_free(ep);

    return 0;
}

int
main(int argc, char *argv[])
{
    ps_endpointer_t *ep;
    int i;

    (void)argc; (void)argv;
    err_set_loglevel(ERR_INFO);
    /* Test initialization with default parameters. */
    ep = ps_endpointer_init(0, 0, 0, 0, 0);
    TEST_ASSERT(ep);
    /* Retain and release, should still be there. */
    TEST_ASSERT((ep = ps_endpointer_retain(ep)));
    TEST_ASSERT(ps_endpointer_free(ep));

    /* Test default parameters. */
    TEST_EQUAL(ps_endpointer_sample_rate(ep),
               PS_VAD_DEFAULT_SAMPLE_RATE);
    TEST_EQUAL(ps_endpointer_frame_size(ep),
               (int)(PS_VAD_DEFAULT_SAMPLE_RATE * PS_VAD_DEFAULT_FRAME_LENGTH));
    ps_endpointer_free(ep);


    /* Test rejection of unreasonable sample rates. */
    ep = ps_endpointer_init(0, 0, 0, 42, 0);
    TEST_ASSERT(ep == NULL);
    ep = ps_endpointer_init(0, 0, 0, 96000, 0);
    TEST_ASSERT(ep == NULL);

    /* Test rejection of unreasonable windows and ratios. */
    ep = ps_endpointer_init(0.3, 0.99, 0, 0, 0);
    TEST_ASSERT(ep == NULL);
    ep = ps_endpointer_init(0.03, 0.1, 0, 0, 0);
    TEST_ASSERT(ep == NULL);

    /* Test a variety of sample rates. */
    for (i = 0; i < n_sample_rates; ++i)
        test_sample_rate(sample_rates[i]);
        
    return 0;
}
