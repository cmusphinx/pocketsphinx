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
				"-lm", MODELDIR "/en-us/en-us.lm.dmp",
				"-dict", MODELDIR "/en-us/cmudict-en-us.dict",
				"-fwdtree", "yes",
				"-fwdflat", "yes",
				"-bestpath", "yes",
				"-input_endian", "little",
				"-samprate", "16000", NULL));
	TEST_ASSERT(ps = ps_init(config));
	cmd_ln_free_r(config);

	TEST_ASSERT(config =
		    cmd_ln_init(NULL, ps_args(), TRUE,
				"-hmm", DATADIR "/tidigits/hmm",
				"-lm", DATADIR "/tidigits/lm/tidigits.lm.dmp",
				"-dict", DATADIR "/tidigits/lm/tidigits.dic",
				"-fwdtree", "yes",
				"-fwdflat", "yes",
				"-bestpath", "yes",
				"-input_endian", "little",
				"-samprate", "16000", NULL));
	TEST_EQUAL(0, ps_reinit(ps, config));

	ps_free(ps);
	cmd_ln_free_r(config);

	return 0;
}
