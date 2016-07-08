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
                "-lm", MODELDIR "/en-us/en-us.lm.bin",
                "-dict", MODELDIR "/en-us/cmudict-en-us.dict",
                "-fwdtree", "yes",
                "-fwdflat", "yes",
                "-bestpath", "yes",
                "-mfclogdir", ".",
                "-rawlogdir", ".",
                "-samprate", "16000", NULL));
    return ps_decoder_test(config, "BESTPATH", "go forward ten meters");
}
