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

	TEST_ASSERT(mdef = bin_mdef_read(NULL, MODELDIR "/hmm/en_US/wsj1/mdef"));
	TEST_ASSERT(dict = dict_init(cmd_ln_init(NULL, NULL, FALSE,
						   "-dict", MODELDIR "/lm/en_US/cmudict.0.6d",
						   "-fdict", MODELDIR "/hmm/en_US/wsj1/noisedict",
						   "-dictcase", "no", NULL),
				       mdef));

	printf("Word ID (CARNEGIE) = %d\n",
	       dict_wordid(dict, "CARNEGIE"));
	printf("Word ID (ASDFASFASSD) = %d\n",
	       dict_wordid(dict, "ASDFASFASSD"));

	TEST_EQUAL(0, dict_write(dict, "_cmudict.0.6d", NULL));
	TEST_EQUAL(0, system("diff -uw " MODELDIR "/lm/en_US/cmudict.0.6d _cmudict.0.6d"));

	dict_free(dict);
	bin_mdef_free(mdef);

	return 0;
}
