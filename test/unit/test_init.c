#include <pocketsphinx.h>
#include <stdio.h>
#include <string.h>
#include "util/cmd_ln.h"

#include "test_macros.h"

int
main(int argc, char *argv[])
{
	ps_decoder_t *ps;
	ps_config_t *config;

	(void)argc;
	(void)argv;
        err_set_loglevel(ERR_INFO);
	TEST_ASSERT(config =
                    ps_config_parse_json(NULL,
                                         "hmm: " MODELDIR "/en-us/en-us "
                                         "lm: " MODELDIR "/en-us/en-us.lm.bin "
                                         "dict:" MODELDIR "/en-us/cmudict-en-us.dict "
                                         "fwdtree: true, fwdflat: true, bestpath: true "
                                         "samprate: 16000"));
        cmd_ln_log_values_r(config, ps_args());
	TEST_ASSERT(ps = ps_init(config));

	ps_free(ps);
	ps_config_free(config);

	return 0;
}
