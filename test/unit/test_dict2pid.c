#include <stdio.h>
#include <string.h>

#include <pocketsphinx.h>

#include "bin_mdef.h"
#include "dict.h"
#include "dict2pid.h"
#include "test_macros.h"

int
main(int argc, char *argv[])
{
	bin_mdef_t *mdef;
	dict_t *dict;
	dict2pid_t *d2p;
	ps_config_t *config;

	(void)argc;
	(void)argv;
	TEST_ASSERT(config = ps_config_parse_json(NULL,
                                                  "dict:" MODELDIR "/en-us/cmudict-en-us.dict "
                                                  "fdict:" MODELDIR "/en-us/en-us/noisedict"));
	TEST_ASSERT(mdef = bin_mdef_read(NULL, MODELDIR "/en-us/en-us/mdef"));
	TEST_ASSERT(dict = dict_init(config, mdef));
	TEST_ASSERT(d2p = dict2pid_build(mdef, dict));

	dict_free(dict);
	dict2pid_free(d2p);
	bin_mdef_free(mdef);
	ps_config_free(config);

	return 0;
}
