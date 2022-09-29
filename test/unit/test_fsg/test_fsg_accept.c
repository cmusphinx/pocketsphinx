/**
 * Make sure FSG code accepts things it should and refuses things it shouldn't
 */
#include <pocketsphinx.h>
#include <stdio.h>
#include <string.h>

#include "lm/fsg_model.h"
#include "test_macros.h"

int
main(int argc, char *argv[])
{
    logmath_t *lmath;
    fsg_model_t *fsg;

    (void)argc;
    (void)argv;

    err_set_loglevel(ERR_INFO);
    lmath = logmath_init(1.0001, 0, 0);

    /* Try an easy one. */
    fsg = fsg_model_readfile(LMDIR "/goforward.fsg", lmath, 7.5);
    TEST_ASSERT(fsg);

    TEST_EQUAL(TRUE, fsg_model_accept(fsg, "GO FORWARD TEN METERS"));
    TEST_EQUAL(TRUE, fsg_model_accept(fsg, "GO FORWARD TEN METER"));
    TEST_EQUAL(TRUE, fsg_model_accept(fsg, "GO FORWARD SIX METER"));
    TEST_EQUAL(TRUE, fsg_model_accept(fsg, "GO FORWARD TEN METER"));
    TEST_EQUAL(FALSE, fsg_model_accept(fsg, "GO FORWARD TEN"));
    TEST_EQUAL(FALSE, fsg_model_accept(fsg, "GO FORWARD THREE FOUR METERS"));
    TEST_EQUAL(FALSE, fsg_model_accept(fsg, "GO FORWARD YOURSELF"));
    TEST_EQUAL(FALSE, fsg_model_accept(fsg, ""));
    fsg_model_free(fsg);
    
    /* Try some harder ones. */
    fsg = fsg_model_readfile(
        LMDIR "/../../regression/test.rightRecursion.fsg", lmath, 7.5);
    TEST_EQUAL(TRUE, fsg_model_accept(fsg, "stop"));
    TEST_EQUAL(TRUE, fsg_model_accept(fsg, "start"));
    TEST_EQUAL(TRUE, fsg_model_accept(fsg, "stop and start"));
    TEST_EQUAL(TRUE, fsg_model_accept(fsg, "stop and stop"));
    TEST_EQUAL(TRUE, fsg_model_accept(fsg, "start and start"));
    TEST_EQUAL(TRUE, fsg_model_accept(fsg,
                                      "start and start and start"));
    TEST_EQUAL(FALSE, fsg_model_accept(fsg, "stop stop"));
    TEST_EQUAL(FALSE, fsg_model_accept(fsg, ""));
    TEST_EQUAL(FALSE, fsg_model_accept(fsg, "and stop"));
    TEST_EQUAL(FALSE, fsg_model_accept(fsg, "stop and"));
    fsg_model_free(fsg);

    /* Try some harder ones. */
    fsg = fsg_model_readfile(
        LMDIR "/../../regression/test.nestedRightRecursion.fsg", lmath, 7.5);
    TEST_EQUAL(TRUE, fsg_model_accept(fsg, "something"));
    TEST_EQUAL(TRUE, fsg_model_accept(fsg, "another"));
    /* nothing else should be accepted!! */
    TEST_EQUAL(FALSE, fsg_model_accept(fsg, "another another"));
    TEST_EQUAL(FALSE, fsg_model_accept(fsg, "something another"));
    TEST_EQUAL(FALSE, fsg_model_accept(fsg, "something something"));
    fsg_model_free(fsg);

    fsg = fsg_model_readfile(
        LMDIR "/../../regression/test.kleene.fsg", lmath, 7.5);
    TEST_EQUAL(TRUE, fsg_model_accept(fsg, "please oh mighty computer kindly don't crash"));
    TEST_EQUAL(TRUE, fsg_model_accept(fsg, "please please please don't crash"));
    TEST_EQUAL(TRUE, fsg_model_accept(fsg, "please don't crash"));
    TEST_EQUAL(TRUE, fsg_model_accept(fsg, "kindly don't crash"));
    TEST_EQUAL(TRUE, fsg_model_accept(fsg, "oh mighty computer don't crash"));
    TEST_EQUAL(TRUE, fsg_model_accept(fsg, "don't crash"));
    TEST_EQUAL(FALSE, fsg_model_accept(fsg, "kindly oh mighty computer"));
    TEST_EQUAL(FALSE, fsg_model_accept(fsg, ""));
    fsg_model_free(fsg);

    fsg = fsg_model_readfile(
        LMDIR "/../../regression/test.command.fsg", lmath, 7.5);
    TEST_EQUAL(TRUE, fsg_model_accept(fsg, "oh mighty computer thanks"));
    TEST_EQUAL(TRUE, fsg_model_accept(fsg, ""));
    TEST_EQUAL(TRUE, fsg_model_accept(fsg, "go go go go go"));
    TEST_EQUAL(TRUE, fsg_model_accept(fsg, "stop"));
    TEST_EQUAL(TRUE, fsg_model_accept(fsg, "stop stop stop"));
    TEST_EQUAL(TRUE, fsg_model_accept(fsg, "could you stop thanks"));
    TEST_EQUAL(TRUE, fsg_model_accept(fsg, "could you stop"));
    TEST_EQUAL(TRUE, fsg_model_accept(fsg, "kindly go go go thank you"));
    TEST_EQUAL(FALSE, fsg_model_accept(fsg, "go stop"));
    TEST_EQUAL(FALSE, fsg_model_accept(fsg, "please kindly go please"));
    TEST_EQUAL(FALSE, fsg_model_accept(fsg, "please go please thanks"));
    fsg_model_free(fsg);

    fsg = fsg_model_readfile(
        LMDIR "/../../regression/right_recursion_53.fsg", lmath, 7.5);
    TEST_EQUAL(TRUE, fsg_model_accept(fsg, "ONE THOUSAND FIVE METER EQUAL TO CENTIMETER"));
    TEST_EQUAL(TRUE, fsg_model_accept(fsg, "ONE ONE THREE THREE THOUSAND TEN NINE CENTIMETER EQUAL TO METER"));
    TEST_EQUAL(TRUE, fsg_model_accept(fsg, "WHAT IS YOUR NAME"));
    TEST_EQUAL(TRUE, fsg_model_accept(fsg, "THREE METER EQUAL TO HOW MANY CENTIMETER"));
    TEST_EQUAL(FALSE, fsg_model_accept(fsg, ""));
    TEST_EQUAL(FALSE, fsg_model_accept(fsg, "METER EQUAL TO HOW MANY MILE"));
    TEST_EQUAL(FALSE, fsg_model_accept(fsg, "ONE THREE HOW MANY MILE"));
    TEST_EQUAL(FALSE, fsg_model_accept(fsg, "WHAT IS YOUR NAME HOW MANY THOUSAND THOUSAND"));
    TEST_EQUAL(FALSE, fsg_model_accept(fsg, "ONE TWO WHAT IS YOUR NAME"));
    TEST_EQUAL(FALSE, fsg_model_accept(fsg, "THOUSAND WHAT IS YOUR NAME"));
    fsg_model_free(fsg);
    logmath_free(lmath);
    
    return 0;
}
