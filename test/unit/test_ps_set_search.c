#include <pocketsphinx.h>

#include "test_macros.h"

static cmd_ln_t *
default_config()
{
    return cmd_ln_init(NULL, ps_args(), TRUE,
                       "-hmm", MODELDIR "/hmm/en_US/hub4wsj_sc_8k",
                       "-dict", MODELDIR "/lm/en_US/cmu07a.dic", NULL);
}

static void
test_no_search()
{
    cmd_ln_t *config = default_config();
    ps_decoder_t *ps = ps_init(config);
    TEST_ASSERT(ps_start_utt(ps, NULL));
    ps_free(ps);
    cmd_ln_free_r(config);
}

static void
test_default_fsg()
{
    cmd_ln_t *config = default_config();
    cmd_ln_set_str_r(config, "-fsg", MODELDIR "/lm/en/tidigits.fsg");
    ps_decoder_t *ps = ps_init(config);
    TEST_ASSERT(!ps_get_lm(ps, PS_DEFAULT_SEARCH));
    // TEST_ASSERT(ps_get_fsg(ps, PS_DEFAULT_SEARCH));
    ps_free(ps);
    cmd_ln_free_r(config);
}

static void
test_default_jsgf()
{
    cmd_ln_t *config = default_config();
    cmd_ln_set_str_r(config, "-jsgf", DATADIR "/goforward.gram");
    ps_decoder_t *ps = ps_init(config);
    TEST_ASSERT(!ps_get_lm(ps, PS_DEFAULT_SEARCH));
    // TEST_ASSERT(ps_get_fsg(ps, PS_DEFAULT_SEARCH));
    ps_free(ps);
    cmd_ln_free_r(config);
}

static void
test_default_lm()
{
    cmd_ln_t *config = default_config();
    cmd_ln_set_str_r(config, "-lm", MODELDIR "/lm/en_US/wsj0vp.5000.DMP");
    ps_decoder_t *ps = ps_init(config);
    //TEST_ASSERT(!ps_get_fsg(ps, PS_DEFAULT_SEARCH));
    TEST_ASSERT(ps_get_lm(ps, PS_DEFAULT_SEARCH));
    ps_free(ps);
    cmd_ln_free_r(config);
}

static void
test_default_lmctl()
{
    cmd_ln_t *config = default_config();
    cmd_ln_set_str_r(config, "-lmctl", DATADIR "/test.lmctl");
    ps_decoder_t *ps = ps_init(config);
    TEST_ASSERT(ps_get_lm(ps, "tidigits"));
    TEST_ASSERT(ps_get_lm(ps, "turtle"));
    TEST_ASSERT(!ps_set_search(ps, "turtle"));
    TEST_ASSERT(!ps_set_search(ps, "tidigits"));
    ps_free(ps);
    cmd_ln_free_r(config);
}

static void
test_set_search()
{
    cmd_ln_t *config = default_config();
    ps_decoder_t *ps = ps_init(config);
    //ps_set_fsg(ps, "tidigits", ...);
    //ps_set_lm(ps, "turtle", ...);
    //TEST_ASSERT(!ps_set_search(ps, "tidigits"));
    //TEST_ASSERT(!ps_set_search(ps, "turtle"));
    ps_free(ps);
    cmd_ln_free_r(config);
}

int
main(int argc, char* argv[])
{
    test_no_search();
    //test_default_fsg();
    //test_default_jsgf();
    test_default_lm();
    test_default_lmctl();
    test_set_search();

    return 0;
}

/* vim: set ts=4 sw=4: */

