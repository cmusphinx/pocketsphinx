#include "lm/jsgf.h"

#include <pocketsphinx.h>
#include "pocketsphinx_internal.h"
#include "lm/ngram_model.h"

#include "test_macros.h"

static ps_config_t *
default_config()
{
    ps_config_t *config = ps_config_init(NULL);
    ps_config_set_str(config, "hmm", MODELDIR "/en-us/en-us");
    ps_config_set_str(config, "dict", MODELDIR "/en-us/cmudict-en-us.dict");
    return config;
}

static void
test_no_search()
{
    cmd_ln_t *config = default_config();
    ps_decoder_t *ps = ps_init(config);
    /* Expect failure and error message here */
    TEST_ASSERT(ps_start_utt(ps) < 0);
    ps_free(ps);
    ps_config_free(config);
}

static void
test_default_fsg()
{
    cmd_ln_t *config = ps_config_init(NULL);
    ps_config_set_str(config, "hmm", DATADIR "/tidigits/hmm");
    ps_config_set_str(config, "dict", DATADIR "/tidigits/lm/tidigits.dic");
    ps_config_set_str(config, "fsg", DATADIR "/tidigits/lm/tidigits.fsg");
    ps_decoder_t *ps = ps_init(config);
    TEST_ASSERT(!ps_get_lm(ps, PS_DEFAULT_SEARCH));
    TEST_ASSERT(ps_get_fsg(ps, PS_DEFAULT_SEARCH));
    TEST_EQUAL(ps_get_fsg(ps, PS_DEFAULT_SEARCH),
               ps_get_fsg(ps, NULL));
    ps_free(ps);
    ps_config_free(config);
}

static void
test_default_jsgf()
{
    cmd_ln_t *config = default_config();
    ps_config_set_str(config, "jsgf", DATADIR "/goforward.gram");
    ps_decoder_t *ps = ps_init(config);
    TEST_ASSERT(!ps_get_lm(ps, PS_DEFAULT_SEARCH));
    TEST_ASSERT(ps_get_fsg(ps, PS_DEFAULT_SEARCH));
    TEST_EQUAL(ps_get_fsg(ps, PS_DEFAULT_SEARCH),
               ps_get_fsg(ps, NULL));
    ps_free(ps);
    ps_config_free(config);
}

static void
test_default_lm()
{
    cmd_ln_t *config = default_config();
    ps_config_set_str(config, "lm", MODELDIR "/en-us/en-us.lm.bin");
    ps_decoder_t *ps = ps_init(config);
    TEST_ASSERT(!ps_get_fsg(ps, PS_DEFAULT_SEARCH));
    TEST_ASSERT(ps_get_lm(ps, PS_DEFAULT_SEARCH));
    TEST_EQUAL(ps_get_lm(ps, PS_DEFAULT_SEARCH),
               ps_get_lm(ps, NULL));
    ps_free(ps);
    ps_config_free(config);
}

static void
test_default_lmctl()
{
    cmd_ln_t *config = default_config();
    ps_config_set_str(config, "lmctl", DATADIR "/test.lmctl");
    ps_config_set_str(config, "lmname", "tidigits");
    ps_decoder_t *ps = ps_init(config);
    TEST_ASSERT(ps_get_lm(ps, "tidigits"));
    TEST_ASSERT(ps_get_lm(ps, "turtle"));
    TEST_ASSERT(!ps_activate_search(ps, "turtle"));
    TEST_EQUAL(ps_get_lm(ps, "turtle"),
               ps_get_lm(ps, NULL));
    TEST_ASSERT(!ps_activate_search(ps, "tidigits"));
    TEST_EQUAL(ps_get_lm(ps, "tidigits"),
               ps_get_lm(ps, NULL));
    ps_free(ps);
    ps_config_free(config);
}

static void
test_activate_search()
{
    cmd_ln_t *config = default_config();
    ps_decoder_t *ps = ps_init(config);
    ps_search_iter_t *itor;

    jsgf_t *jsgf = jsgf_parse_file(DATADIR "/goforward.gram", NULL);
    fsg_model_t *fsg = jsgf_build_fsg(jsgf,
                                      jsgf_get_rule(jsgf, "goforward.move"),
                                      ps->lmath, ps_config_int(config, "lw"));
    TEST_ASSERT(!ps_add_fsg(ps, "goforward", fsg));
    fsg_model_free(fsg);
    jsgf_grammar_free(jsgf);

    TEST_ASSERT(!ps_add_jsgf_file(ps, "goforward_other", DATADIR "/goforward.gram"));
    // Second time
    TEST_ASSERT(!ps_add_jsgf_file(ps, "goforward_other", DATADIR "/goforward.gram"));

    ngram_model_t *lm = ngram_model_read(config, DATADIR "/tidigits/lm/tidigits.lm.bin",
                                         NGRAM_AUTO, ps->lmath);
    TEST_ASSERT(!ps_add_lm(ps, "tidigits", lm));
    ngram_model_free(lm);

    TEST_ASSERT(!ps_activate_search(ps, "tidigits"));
    TEST_EQUAL(ps_get_lm(ps, "tidigits"),
               ps_get_lm(ps, NULL));
    TEST_EQUAL(0, strcmp("tidigits", ps_current_search(ps)));

    TEST_ASSERT(!ps_activate_search(ps, "goforward"));
    TEST_EQUAL(ps_get_fsg(ps, "goforward"),
               ps_get_fsg(ps, NULL));
    TEST_EQUAL(0, strcmp("goforward", ps_current_search(ps)));
    
    itor = ps_search_iter(ps);
    TEST_EQUAL(0, strcmp("goforward_other", ps_search_iter_val(itor)));
    itor = ps_search_iter_next(itor);
    TEST_EQUAL(0, strcmp("tidigits", ps_search_iter_val(itor)));
    itor = ps_search_iter_next(itor);
    TEST_EQUAL(0, strcmp("goforward", ps_search_iter_val(itor)));
    itor = ps_search_iter_next(itor);
    TEST_EQUAL(0, strcmp("_default_pl", ps_search_iter_val(itor)));
    itor = ps_search_iter_next(itor);
    TEST_EQUAL(NULL, itor);

    TEST_EQUAL(0, ps_remove_search(ps, "goforward"));
    TEST_EQUAL(ps_get_fsg(ps, "goforward"), NULL);
    TEST_EQUAL(ps_current_search(ps), NULL);
    TEST_ASSERT(!ps_activate_search(ps, "tidigits"));
    TEST_EQUAL(0, strcmp("tidigits", ps_current_search(ps)));
    TEST_EQUAL(0, ps_remove_search(ps, "goforward_other"));
    TEST_EQUAL(0, strcmp("tidigits", ps_current_search(ps)));
    
    TEST_ASSERT(!ps_start_utt(ps));
    TEST_ASSERT(!ps_end_utt(ps));
    ps_free(ps);
    ps_config_free(config);
}

static void
test_check_mode()
{
    cmd_ln_t *config = default_config();
    ps_decoder_t *ps = ps_init(config);

    TEST_ASSERT(!ps_add_jsgf_file(ps, "goforward", DATADIR "/goforward.gram"));

    ngram_model_t *lm = ngram_model_read(config, DATADIR "/tidigits/lm/tidigits.lm.bin",
                                         NGRAM_AUTO, ps->lmath);
    TEST_ASSERT(!ps_add_lm(ps, "tidigits", lm));
    ngram_model_free(lm);

    TEST_ASSERT(!ps_activate_search(ps, "tidigits"));
    TEST_EQUAL(ps_get_lm(ps, "tidigits"),
               ps_get_lm(ps, NULL));
    TEST_EQUAL(0, strcmp("tidigits", ps_current_search(ps)));

    ps_start_utt(ps);
    TEST_EQUAL(-1, ps_activate_search(ps, "tidigits"));
    TEST_EQUAL(-1, ps_activate_search(ps, "goforward"));    
    ps_end_utt(ps);
    
    ps_free(ps);
    ps_config_free(config);
}

int
main(int argc, char* argv[])
{
  (void)argc;
  (void)argv;
    test_no_search();
    test_default_fsg();
    test_default_jsgf();
    test_default_lm();
    test_default_lmctl();
    test_activate_search();
    test_check_mode();

    return 0;
}
