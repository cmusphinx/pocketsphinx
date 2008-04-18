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
				"-hmm", MODELDIR "/hmm/wsj1",
				"-lm", MODELDIR "/lm/swb/swb.lm.DMP",
				"-dict", MODELDIR "/lm/swb/swb.dic",
				"-fwdtree", "yes",
				"-fwdflat", "yes",
				"-bestpath", "yes",
				"-input_endian", "little",
				"-samprate", "16000", NULL));
	TEST_ASSERT(ps = ps_init(config));

	ps_free(ps);

	return 0;
}
