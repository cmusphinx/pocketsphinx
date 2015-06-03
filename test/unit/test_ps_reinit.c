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
				"-hmm", DATADIR "/tidigits/hmm",
				"-dict", DATADIR "/tidigits/lm/tidigits.dic",
				NULL));
	TEST_ASSERT(ps = ps_init(config));
	cmd_ln_free_r(config);

	TEST_ASSERT(config =
		    cmd_ln_init(NULL, ps_args(), TRUE,
				"-hmm", MODELDIR "/en-us/en-us",
				"-dict", MODELDIR "/en-us/cmudict-en-us.dict",
				NULL));
	TEST_EQUAL(0, ps_reinit(ps, config));

	/* Reinit when adding words */
	ps_add_word(ps, "foobie", "F UW B IY", FALSE);
	ps_add_word(ps, "hellosomething", "HH EH L OW S", TRUE);

	/* Reinit with existing config */
	ps_reinit(ps, NULL);

	ps_free(ps);
	cmd_ln_free_r(config);

	return 0;
}
