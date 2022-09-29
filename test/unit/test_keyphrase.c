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
                    "kws: \"" DATADIR "/goforward.kws\","
                    "dict: \"" MODELDIR "/en-us/cmudict-en-us.dict\""));
    return ps_decoder_test(config, "KEYPHRASE", "forward meters");
}
