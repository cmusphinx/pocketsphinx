#include <pocketsphinx.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "lm/jsgf.h"
#include "lm/fsg_model.h"

#include "pocketsphinx_internal.h"
#include "fsg_search_internal.h"
#include "test_macros.h"

int
main(int argc, char *argv[])
{
    ps_decoder_t *ps;
    cmd_ln_t *config;
    jsgf_t *jsgf;
    jsgf_rule_t *rule;
    fsg_model_t *fsg;
    FILE *rawfh;
    char const *hyp;
    int32 score, prob;

    (void)argc;
    (void)argv;
    TEST_ASSERT(config =
                ps_config_parse_json(
                    NULL,
                    "hmm: \"" MODELDIR "/en-us/en-us\","
                    "dict: \"" DATADIR "/turtle.dic\","
                    "samprate: 16000"));
    TEST_ASSERT(ps = ps_init(config));

    jsgf = jsgf_parse_file(DATADIR "/goforward.gram", NULL);
    TEST_ASSERT(jsgf);
    rule = jsgf_get_rule(jsgf, "goforward.move2");
    TEST_ASSERT(rule);
    fsg = jsgf_build_fsg(jsgf, rule, ps->lmath, 7.5);
    TEST_ASSERT(fsg);
    fsg_model_write(fsg, stdout);
    ps_add_fsg(ps, "goforward.move2", fsg);
    ps_activate_search(ps, "goforward.move2"); 
    TEST_ASSERT(rawfh = fopen(DATADIR "/goforward.raw", "rb"));
    ps_decode_raw(ps, rawfh, -1);
    hyp = ps_get_hyp(ps, &score);
    prob = ps_get_prob(ps);
    printf("%s (%d, %d)\n", hyp, score, prob);
    TEST_EQUAL(0, strcmp("go forward ten meters", hyp));
    ps_free(ps);
    fclose(rawfh);
    ps_config_free(config);


    TEST_ASSERT(config =
                ps_config_parse_json(
                    NULL,
                    "hmm: \"" MODELDIR "/en-us/en-us\","
                    "dict: \"" DATADIR "/turtle.dic\","
                    "jsgf: \"" DATADIR "/goforward.gram\","
                    "samprate: 16000"));
    TEST_ASSERT(ps = ps_init(config));
    TEST_ASSERT(rawfh = fopen(DATADIR "/goforward.raw", "rb"));
    ps_decode_raw(ps, rawfh, -1);
    hyp = ps_get_hyp(ps, &score);
    prob = ps_get_prob(ps);
    printf("%s (%d, %d)\n", hyp, score, prob);
    TEST_EQUAL(0, strcmp("go forward ten meters", hyp));
    ps_free(ps);
    fclose(rawfh);
    ps_config_free(config);

    TEST_ASSERT(config =
                ps_config_parse_json(
                    NULL,
                    "hmm: \"" MODELDIR "/en-us/en-us\","
                    "dict: \"" DATADIR "/turtle.dic\","
                    "jsgf: \"" DATADIR "/goforward.gram\","
                    "toprule: goforward.move2, samprate: 16000"));
    TEST_ASSERT(ps = ps_init(config));
    TEST_ASSERT(rawfh = fopen(DATADIR "/goforward.raw", "rb"));
    ps_decode_raw(ps, rawfh, -1);
    hyp = ps_get_hyp(ps, &score);
    prob = ps_get_prob(ps);
    printf("%s (%d, %d)\n", hyp, score, prob);
    TEST_EQUAL(0, strcmp("go forward ten meters", hyp));
    ps_free(ps);
    ps_config_free(config);
    fclose(rawfh);

    TEST_ASSERT(config =
                ps_config_parse_json(
                    NULL,
                    "hmm: \"" MODELDIR "/en-us/en-us\","
                    "dict: \"" DATADIR "/turtle.dic\","
                    "jsgf: \"" DATADIR "/defective.gram\","));
    TEST_ASSERT(NULL == ps_init(config));
    ps_config_free(config);

    return 0;
}
