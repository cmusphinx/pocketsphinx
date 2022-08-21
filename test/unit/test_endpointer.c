/* Test endpointing.
 *
 * MIT license (c) 2022, see LICENSE for more information.
 *
 * Author: David Huggins-Daines <dhdaines@gmail.com>
 */
#include <pocketsphinx.h>
#include "test_macros.h"

int
main(int argc, char *argv[])
{
    ps_endpointer_t *ep;

    (void)argc; (void)argv;
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
        
    return 0;
}
