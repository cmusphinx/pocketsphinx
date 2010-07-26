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

	mdef = bin_mdef_read(NULL, MODELDIR "/hmm/en_US/hub4wsj_sc_8k/mdef");
	dict = dict_init(cmd_ln_init(NULL, NULL, FALSE,
				     "-dict", MODELDIR "/lm/en_US/cmu07a.dic",
				     "-fdict", MODELDIR "/hmm/en_US/hub4wsj_sc_8k/noisedict",
				     "-dictcase", "no", NULL),
			 mdef);
	d2p = dict2pid_build(mdef, dict);

	al = alignment_init(d2p);
	alignment_add_word(al, dict_word_id(dict, "<s>"), -1);
	alignment_add_word(al, dict_word_id(dict, "hello"), -1);
	alignment_add_word(al, dict_word_id(dict, "world"), -1);
	alignment_add_word(al, dict_word_id(dict, "</s>"), -1);
	alignment_populate(al);

	dict_free(dict);
	dict2pid_free(d2p);
	bin_mdef_free(mdef);

	return 0;
}
