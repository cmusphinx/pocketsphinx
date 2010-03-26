#include <pocketsphinx.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "pocketsphinx_internal.h"
#include "glextree.h"
#include "test_macros.h"

int
main(int argc, char *argv[])
{
	dict_t *dict;
	dict2pid_t *d2p;
	bin_mdef_t *mdef;
	glextree_t *tree;

	TEST_ASSERT(mdef = bin_mdef_read(NULL, MODELDIR "/hmm/en_US/hub4wsj_sc_8k/mdef"));
	TEST_ASSERT(dict = dict_init(cmd_ln_init(NULL, NULL, FALSE,
						   "-dict", MODELDIR "/lm/en_US/cmu07a.dic",
						   "-fdict", MODELDIR "/hmm/en_US/hub4wsj_sc_8k/noisedict",
						   "-dictcase", "no", NULL),
				       mdef));
	TEST_ASSERT(d2p = dict2pid_build(mdef, dict));
	TEST_ASSERT(tree = glextree_build(dict, d2p));

	dict_free(dict);
	dict2pid_free(d2p);
	bin_mdef_free(mdef);
	glextree_free(tree);
	return 0;
}
