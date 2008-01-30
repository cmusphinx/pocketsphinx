#include <pocketsphinx.h>
#include <stdio.h>
#include <string.h>

#include "test_macros.h"

static char *my_argv[] = {
	"test_ps_init",
	"-hmm", MODELDIR "/hmm/wsj1",
	"-lm", MODELDIR "/lm/swb/swb.lm.DMP",
	"-dict", MODELDIR "/lm/swb/swb.dic",
	"-cepext", "",
	"-samprate", "16000"
};
static const int my_argc = sizeof(my_argv) / sizeof(my_argv[0]);

int
main(int argc, char *argv[])
{
	pocketsphinx_t *ps;
	cmd_ln_t *config;

	TEST_ASSERT(config = cmd_ln_parse_r(NULL, pocketsphinx_args(),
					    my_argc, my_argv, TRUE));
	TEST_ASSERT(ps = pocketsphinx_init(config));

	pocketsphinx_free(ps);

	return 0;
}
