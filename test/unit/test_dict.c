#include <stdio.h>
#include <string.h>

#include <pocketsphinx.h>
#include <bin_mdef.h>

#include "dict.h"
#include "test_macros.h"

int
main(int argc, char *argv[])
{
	bin_mdef_t *mdef;
	dict_t *dict;

	TEST_ASSERT(mdef = bin_mdef_read(NULL, MODELDIR "/hmm/en_US/hub4wsj_sc_8k/mdef"));
	TEST_ASSERT(dict = dict_init(cmd_ln_init(NULL, NULL, FALSE,
						   "-dict", MODELDIR "/lm/en_US/cmu07a.dic",
						   "-fdict", MODELDIR "/hmm/en_US/hub4wsj_sc_8k/noisedict",
						   "-dictcase", "no", NULL),
				       mdef));

	printf("Word ID (CARNEGIE) = %d\n",
	       dict_wordid(dict, "CARNEGIE"));
	printf("Word ID (ASDFASFASSD) = %d\n",
	       dict_wordid(dict, "ASDFASFASSD"));

	TEST_EQUAL(0, dict_write(dict, "_cmu07a.dic", NULL));
	TEST_EQUAL(0, system("diff -uw " MODELDIR "/lm/en_US/cmu07a.dic _cmu07a.dic"));

	dict_free(dict);
	bin_mdef_free(mdef);

	return 0;
}
