#include <pocketsphinx.h>

#include "alignment.h"
#include "pocketsphinx_internal.h"

#include "test_macros.h"

int
main(int argc, char *argv[])
{
	bin_mdef_t *mdef;
	dict_t *dict;
	dict2pid_t *d2p;
	alignment_t *al;
	alignment_iter_t *itor;
	cmd_ln_t *config;

	config = cmd_ln_init(NULL, NULL, FALSE,
			     "-dict", MODELDIR "/lm/en_US/cmu07a.dic",
			     "-fdict", MODELDIR "/hmm/en_US/hub4wsj_sc_8k/noisedict",
			     "-dictcase", "no", NULL);
	mdef = bin_mdef_read(NULL, MODELDIR "/hmm/en_US/hub4wsj_sc_8k/mdef");
	dict = dict_init(config, mdef);
	d2p = dict2pid_build(mdef, dict);

	al = alignment_init(d2p);
	TEST_EQUAL(1, alignment_add_word(al, dict_wordid(dict, "<s>"), 0));
	TEST_EQUAL(2, alignment_add_word(al, dict_wordid(dict, "hello"), 0));
	TEST_EQUAL(3, alignment_add_word(al, dict_wordid(dict, "world"), 0));
	TEST_EQUAL(4, alignment_add_word(al, dict_wordid(dict, "</s>"), 0));
	TEST_EQUAL(0, alignment_populate(al));

	itor = alignment_words(al);
	TEST_EQUAL(alignment_iter_get(itor)->id.wid, dict_wordid(dict, "<s>"));
	itor = alignment_iter_next(itor);
	TEST_EQUAL(alignment_iter_get(itor)->id.wid, dict_wordid(dict, "hello"));
	itor = alignment_iter_next(itor);
	TEST_EQUAL(alignment_iter_get(itor)->id.wid, dict_wordid(dict, "world"));
	itor = alignment_iter_next(itor);
	TEST_EQUAL(alignment_iter_get(itor)->id.wid, dict_wordid(dict, "</s>"));
	itor = alignment_iter_next(itor);
	TEST_EQUAL(itor, NULL);

	printf("%d words %d phones %d states\n",
	       al->word.n_ent, al->sseq.n_ent, al->state.n_ent);

	alignment_free(al);
	dict_free(dict);
	dict2pid_free(d2p);
	bin_mdef_free(mdef);
	cmd_ln_free_r(config);

	return 0;
}
