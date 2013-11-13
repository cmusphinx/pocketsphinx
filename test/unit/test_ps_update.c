#include <ngram_search.h>
#include <pocketsphinx.h>

#include "test_macros.h"

static void test_reinit_fsg()
{
       ps_decoder_t *ps;
       cmd_ln_t *config;
       config = cmd_ln_init(NULL, ps_args(), TRUE,
                            "-hmm", MODELDIR "/hmm/en_US/hub4wsj_sc_8k",
                            "-lm", MODELDIR "/lm/en_US/wsj0vp.5000.DMP",
                            "-dict", MODELDIR "/lm/en_US/cmu07a.dic",
                            NULL);
       ps = ps_init(config);
       TEST_ASSERT(!ps_set_search(ps, PS_SEARCH_FSG));
       ps_free(ps);
       cmd_ln_free_r(config);
}

static void test_reinit_fsg_missing()
{
       ps_decoder_t *ps;
       cmd_ln_t *config;
       config = cmd_ln_init(NULL, ps_args(), TRUE,
                "-hmm", MODELDIR "/hmm/en_US/hub4wsj_sc_8k",
                "-dict", MODELDIR "/lm/en_US/cmu07a.dic",
                            NULL);
       ps = ps_init(config);
       cmd_ln_set_str_r(config, "-fsg", "/some/fsg");
       /* Should fail */
       TEST_ASSERT(ps_set_search(ps, PS_SEARCH_FSG));
       ps_free(ps);
       cmd_ln_free_r(config);
}

static void test_reinit_lm()
{
        ps_decoder_t *ps;
        cmd_ln_t *config;
        config = cmd_ln_init(NULL, ps_args(), TRUE,
                     "-hmm", MODELDIR "/hmm/en_US/hub4wsj_sc_8k",
                 "-lm", MODELDIR "/lm/en_US/wsj0vp.5000.DMP",
                 "-dict", MODELDIR "/lm/en_US/cmu07a.dic",
                             NULL);
        ps = ps_init(config);
        TEST_ASSERT(!ps_set_search(ps, PS_SEARCH_NGRAM));
        ps_free(ps);
        cmd_ln_free_r(config);
}

static void test_switch_fsg_lm()
{
    ps_decoder_t *ps;
    cmd_ln_t *config;

    config = cmd_ln_init(NULL, ps_args(), TRUE,
                         "-hmm", MODELDIR "/hmm/en_US/hub4wsj_sc_8k",
                         "-lm", MODELDIR "/lm/en_US/wsj0vp.5000.DMP",
                         "-dict", MODELDIR "/lm/en_US/cmu07a.dic",
                         NULL);
    ps = ps_init(config);
    TEST_ASSERT(ps_get_lmset(ps) != NULL);
    TEST_ASSERT(!ps_set_search(ps, PS_SEARCH_FSG));
    TEST_ASSERT(ps_get_lmset(ps) != NULL);
    TEST_ASSERT(!ps_set_search(ps, PS_SEARCH_NGRAM));
    TEST_ASSERT(ps_get_lmset(ps) != NULL);
    ps_free(ps);
    cmd_ln_free_r(config);
}

static void test_set_lm()
{
    ps_decoder_t *ps;
    cmd_ln_t *config;
    ngram_model_t *lm;

    config = cmd_ln_init(NULL, ps_args(), TRUE,
                         "-hmm", MODELDIR "/hmm/en_US/hub4wsj_sc_8k",
                         "-dict", MODELDIR "/lm/en_US/cmu07a.dic",
                         NULL);
    ps = ps_init(config);
    lm = ngram_model_read(config, MODELDIR "/lm/en/turtle.DMP",
                          NGRAM_AUTO, ps_get_logmath(ps));

    TEST_ASSERT(NULL == ps_get_lmset(ps));
    ngram_search_set_lm(lm);
    ps_set_search(ps, PS_SEARCH_NGRAM);
    TEST_ASSERT(ps_get_lmset(ps));

    ps_free(ps);
    cmd_ln_free_r(config);
}

int main(int argc, char* argv[])
{
    test_reinit_fsg();
    test_reinit_fsg_missing();
    test_reinit_lm();
    test_switch_fsg_lm();
    test_set_lm();
    return 0;
}

/* vim: set ts=4 sw=4: */
