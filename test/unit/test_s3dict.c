#include <stdio.h>
#include <string.h>

#include <pocketsphinx.h>
#include <bin_mdef.h>

#include "s3dict.h"
#include "test_macros.h"

int
main(int argc, char *argv[])
{
	bin_mdef_t *mdef;
	s3dict_t *dict;

	TEST_ASSERT(mdef = bin_mdef_read(NULL, MODELDIR "/hmm/wsj1/mdef"));
	TEST_ASSERT(dict = s3dict_init(mdef, MODELDIR "/lm/cmudict.0.6d",
				       MODELDIR "/hmm/wsj1/noisedict",
				       TRUE, TRUE));

	printf("Word ID (CARNEGIE) = %d\n",
	       s3dict_wordid(dict, "CARNEGIE"));
	printf("Word ID (ASDFASFASSD) = %d\n",
	       s3dict_wordid(dict, "ASDFASFASSD"));

	s3dict_free(dict);
	bin_mdef_free(mdef);

	return 0;
}
