#include <pocketsphinx.h>
#include <stdio.h>
#include <string.h>

#include "pocketsphinx_internal.h"
#include "test_macros.h"
#include "ps_test.c"

int
main(int argc, char *argv[])
{
	cmd_ln_t *config;

	TEST_ASSERT(config =
		    cmd_ln_init(NULL, pocketsphinx_args(), TRUE,
				"-hmm", MODELDIR "/hmm/wsj1",
				"-lm", MODELDIR "/lm/swb/swb.lm.DMP",
				"-dict", MODELDIR "/lm/swb/swb.dic",
				"-fwdtree", "yes",
				"-fwdflat", "yes",
				"-bestpath", "no",
				"-input_endian", "little",
				"-samprate", "16000", NULL));
	return pocketsphinx_test(config, "FWDFLAT", "GO FORWARD TEN YEARS");
}
