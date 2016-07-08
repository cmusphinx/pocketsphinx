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

    TEST_ASSERT(config =
            cmd_ln_init(NULL, ps_args(), TRUE,
                "-hmm", MODELDIR "/en-us/en-us",
                "-allphone", MODELDIR "/en-us/en-us-phone.lm.bin",
                "-beam", "1e-20", "-pbeam", "1e-10", "-allphone_ci", "no", "-lw", "2.0",
                NULL));
    return ps_decoder_test(config, "ALLPHONE", "SIL G OW F AO R W ER D T AE N M IY IH ZH ER Z S V SIL");
}
