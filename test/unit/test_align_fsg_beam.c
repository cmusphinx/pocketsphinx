/* -*- c-basic-offset: 4 -*- */
#include <pocketsphinx.h>

#include "pocketsphinx_internal.h"
#include "fsg_search_internal.h"
#include "util/hash_table.h"
#include "test_macros.h"

int
main(int argc, char *argv[])
{
    ps_decoder_t *ps;
    ps_config_t *config;
    void *search_p;
    fsg_search_t *fsgs;

    (void)argc;
    (void)argv;
    err_set_loglevel(ERR_INFO);
    /* Stock asymmetry: wbeam differs from beam; forced-align FSG must use align_* */
    TEST_ASSERT(config =
                ps_config_parse_json(
                    NULL,
                    "loglevel: INFO, bestpath: false,"
                    "hmm: \"" MODELDIR "/en-us/en-us\","
                    "dict: \"" MODELDIR "/en-us/cmudict-en-us.dict\","
                    "samprate: 16000,"
                    "wbeam: 7e-29"));
    TEST_ASSERT(ps = ps_init(config));
    TEST_EQUAL(0, ps_set_align_text(ps, "go forward ten meters"));
    TEST_EQUAL(0, hash_table_lookup(ps->searches, PS_DEFAULT_ALIGN_SEARCH,
                                    &search_p));
    fsgs = (fsg_search_t *)search_p;
    TEST_EQUAL(fsgs->wbeam_orig, fsgs->beam_orig);
    TEST_EQUAL(fsgs->pbeam_orig, fsgs->beam_orig);

    ps_free(ps);
    ps_config_free(config);

    /* With align_use_main_beams, FSG uses global wbeam (asymmetric from beam). */
    TEST_ASSERT(config =
                ps_config_parse_json(
                    NULL,
                    "loglevel: ERROR, bestpath: false,"
                    "hmm: \"" MODELDIR "/en-us/en-us\","
                    "dict: \"" MODELDIR "/en-us/cmudict-en-us.dict\","
                    "samprate: 16000,"
                    "wbeam: 7e-29,"
                    "align_use_main_beams: yes"));
    TEST_ASSERT(ps = ps_init(config));
    TEST_EQUAL(0, ps_set_align_text(ps, "go forward ten meters"));
    TEST_EQUAL(0, hash_table_lookup(ps->searches, PS_DEFAULT_ALIGN_SEARCH,
                                    &search_p));
    fsgs = (fsg_search_t *)search_p;
    TEST_ASSERT(fsgs->wbeam_orig != fsgs->beam_orig);

    ps_free(ps);
    ps_config_free(config);
    return 0;
}
