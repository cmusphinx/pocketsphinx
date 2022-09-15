#include <pocketsphinx.h>
#include <stdio.h>
#include <string.h>
#include "util/ckd_alloc.h"

#include "test_macros.h"

int
main(int argc, char *argv[])
{
	ps_decoder_t *ps;
	ps_config_t *config;
        char *pron;

	(void)argc;
	(void)argv;
	TEST_ASSERT(config =
                    ps_config_parse_json(
                        NULL, "hmm: \"" DATADIR "/tidigits/hmm\","
                        "dict: \"" DATADIR "/tidigits/lm/tidigits.dic\""));
	TEST_ASSERT(ps = ps_init(config));
	ps_config_free(config);

	TEST_ASSERT(config =
                    ps_config_parse_json(
                        NULL, "hmm: \"" MODELDIR "/en-us/en-us\","
                        "dict: \"" MODELDIR "/en-us/cmudict-en-us.dict\""));
	TEST_EQUAL(0, ps_reinit(ps, config));

	/* Reinit when adding words */
	ps_add_word(ps, "foobie", "F UW B IY", FALSE);
	ps_add_word(ps, "hellosomething", "HH EH L OW S", TRUE);

        /* Reinit features only, words should remain */
        ps_config_set_str(config, "cmninit", "41,-4,1");
	TEST_EQUAL(0, ps_reinit_feat(ps, config));
        TEST_EQUAL(0, strcmp(ps_config_str(ps_get_config(ps), "cmninit"),
                             "41,-4,1"));
        pron = ps_lookup_word(ps, "foobie");
        TEST_ASSERT(pron != NULL);
        TEST_EQUAL(0, strcmp(pron, "F UW B IY"));
        ckd_free(pron);

	/* Reinit with existing config */
	ps_reinit(ps, NULL);
        /* Words added above are gone, we expect that. */
        pron = ps_lookup_word(ps, "foobie");
        TEST_ASSERT(pron == NULL);
        /* Hooray! -cmninit isn't in feat.params anymore, so yes! */
        TEST_EQUAL(0, strcmp(ps_config_str(ps_get_config(ps), "cmninit"),
                                "41,-4,1"));

	ps_free(ps);
	ps_config_free(config);

	return 0;
}
