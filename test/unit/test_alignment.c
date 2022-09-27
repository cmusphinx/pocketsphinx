#include <pocketsphinx.h>

#include "ps_alignment_internal.h"
#include "pocketsphinx_internal.h"

#include "test_macros.h"

int
main(int argc, char *argv[])
{
	bin_mdef_t *mdef;
	dict_t *dict;
	dict2pid_t *d2p;
	ps_alignment_t *al;
	ps_alignment_iter_t *itor, *itor2;
	cmd_ln_t *config;
        int score, start, duration;

	(void)argc;
	(void)argv;
	config = ps_config_parse_json(NULL,
                                      "dict: \"" MODELDIR "/en-us/cmudict-en-us.dict\", "
                                      "fdict: \"" MODELDIR "/en-us/en-us/noisedict\"");
	mdef = bin_mdef_read(NULL, MODELDIR "/en-us/en-us/mdef");
	dict = dict_init(config, mdef);
	d2p = dict2pid_build(mdef, dict);

	al = ps_alignment_init(d2p);
	TEST_EQUAL(1, ps_alignment_add_word(al, dict_wordid(dict, "<s>"), 0, 0));
	TEST_EQUAL(2, ps_alignment_add_word(al, dict_wordid(dict, "hello"), 0, 0));
	TEST_EQUAL(3, ps_alignment_add_word(al, dict_wordid(dict, "world"), 0, 0));
	TEST_EQUAL(4, ps_alignment_add_word(al, dict_wordid(dict, "</s>"), 0, 0));
	TEST_EQUAL(0, ps_alignment_populate(al));

	itor = ps_alignment_words(al);
	TEST_EQUAL(ps_alignment_iter_get(itor)->id.wid, dict_wordid(dict, "<s>"));
	itor = ps_alignment_iter_next(itor);
	TEST_EQUAL(ps_alignment_iter_get(itor)->id.wid, dict_wordid(dict, "hello"));
        score = ps_alignment_iter_seg(itor, &start, &duration);
        TEST_EQUAL(0, strcmp(ps_alignment_iter_name(itor), "hello"));
        TEST_EQUAL(score, 0);
        TEST_EQUAL(start, 0);
        TEST_EQUAL(duration, 0);
	itor = ps_alignment_iter_next(itor);
	TEST_EQUAL(ps_alignment_iter_get(itor)->id.wid, dict_wordid(dict, "world"));
	itor = ps_alignment_iter_next(itor);
	TEST_EQUAL(ps_alignment_iter_get(itor)->id.wid, dict_wordid(dict, "</s>"));
	itor = ps_alignment_iter_next(itor);
	TEST_EQUAL(itor, NULL);

	printf("%d words %d phones %d states\n",
	       ps_alignment_n_words(al),
	       ps_alignment_n_phones(al),
	       ps_alignment_n_states(al));

	itor = ps_alignment_words(al);
	itor = ps_alignment_iter_next(itor);
	TEST_EQUAL(ps_alignment_iter_get(itor)->id.wid, dict_wordid(dict, "hello"));
	itor2 = ps_alignment_iter_children(itor);
        ps_alignment_iter_free(itor);
	TEST_EQUAL(0, strcmp(ps_alignment_iter_name(itor2), "HH"));
	itor2 = ps_alignment_iter_next(itor2);
	TEST_EQUAL(0, strcmp(ps_alignment_iter_name(itor2), "AH"));
	itor2 = ps_alignment_iter_next(itor2);
	TEST_EQUAL(0, strcmp(ps_alignment_iter_name(itor2), "L"));
	itor2 = ps_alignment_iter_next(itor2);
	TEST_EQUAL(0, strcmp(ps_alignment_iter_name(itor2), "OW"));
	itor2 = ps_alignment_iter_next(itor2);
        TEST_EQUAL(NULL, itor2);

	ps_alignment_free(al);
	dict_free(dict);
	dict2pid_free(d2p);
	bin_mdef_free(mdef);
	ps_config_free(config);

	return 0;
}
