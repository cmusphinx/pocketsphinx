#include <pocketsphinx.h>
#include <stdio.h>
#include <string.h>

#include "test_macros.h"

int
main(int argc, char *argv[])
{
	ps_decoder_t *ps;
	cmd_ln_t *config;

	TEST_ASSERT(config =
		    cmd_ln_init(NULL, ps_args(), TRUE,
				"-hmm", MODELDIR "/en-us/en-us",
				"-lm", MODELDIR "/en-us/en-us.lm.bin",
				"-dict", MODELDIR "/en-us/cmudict-en-us.dict",
				"-fwdtree", "yes",
				"-fwdflat", "yes",
				"-bestpath", "yes",
				"-samprate", "16000", NULL));
	TEST_ASSERT(ps = ps_init(config));

	ps_free(ps);
	cmd_ln_free_r(config);

	return 0;
}
