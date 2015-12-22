#include <stdio.h>
#include <string.h>

#include <pocketsphinx.h>
#include <bin_mdef.h>

#include "dict.h"
#include "dict2pid.h"
#include "test_macros.h"

int
main(int argc, char *argv[])
{
	bin_mdef_t *mdef;
	dict_t *dict;
	dict2pid_t *d2p;
	cmd_ln_t *config;

	TEST_ASSERT(config = cmd_ln_init(NULL, NULL, FALSE,
						   "-dict", MODELDIR "/en-us/cmudict-en-us.dict",
						   "-fdict", MODELDIR "/en-us/en-us/noisedict",
						   NULL));
	TEST_ASSERT(mdef = bin_mdef_read(NULL, MODELDIR "/en-us/en-us/mdef"));
	TEST_ASSERT(dict = dict_init(config, mdef, NULL));
	TEST_ASSERT(d2p = dict2pid_build(mdef, dict));

	dict_free(dict);
	dict2pid_free(d2p);
	bin_mdef_free(mdef);
	cmd_ln_free_r(config);

	return 0;
}
