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
                ps_config_parse_json(NULL,
                                     "hmm: \"" MODELDIR "/en-us/en-us\", "
                                     "allphone: \"" MODELDIR "/en-us/en-us-phone.lm.bin\", "
                                     "beam: 1e-20, pbeam: 1e-10, allphone_ci: false, lw: 2.0"));
    return ps_decoder_test(config, "ALLPHONE", "SIL G OW F AO R W ER D T AE N M IY IH ZH ER Z S V SIL");
}
