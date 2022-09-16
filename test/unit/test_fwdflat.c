#include <pocketsphinx.h>
#include <stdio.h>
#include <string.h>

#include "pocketsphinx_internal.h"
#include "test_macros.h"
#include "test_ps.c"

int
main(int argc, char *argv[])
{
	cmd_ln_t *config;

	(void)argc;
	(void)argv;
	TEST_ASSERT(config =
		    ps_config_parse_json(
                        NULL,
                        "hmm: \"" MODELDIR "/en-us/en-us\","
                        "lm: \"" MODELDIR "/en-us/en-us.lm.bin\","
                        "dict: \"" MODELDIR "/en-us/cmudict-en-us.dict\","
                        "fwdflatlw: 6.5,"
                        "fwdtree: false,"
                        "fwdflat: true,"
                        "bestpath: false,"
                        "fwdflatbeam: 1e-30,"
                        "fwdflatwbeam: 1e-20,"
                        "samprate: 16000"));
	return ps_decoder_test(config, "FWDFLAT", "go forward ten meters");
}
