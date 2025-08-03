/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/**
 * Test that pocketsphinx_lm_convert works without lw/wip parameters
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <pocketsphinx.h>
#include "util/cmd_ln.h"
#include "lm/ngram_model.h"

#include "test_macros.h"

static const ps_arg_t defn[] = {
    { "logbase",
      ARG_FLOATING,
      "1.0001",
      "Base in which all log-likelihoods calculated" },
    { "mmap",
      ARG_BOOLEAN,
      "no",
      "Use memory-mapped I/O for reading binary LM files"},
    { NULL, 0, NULL, NULL }
};

int
main(int argc, char *argv[])
{
    ps_config_t *config;
    ngram_model_t *lm;
    logmath_t *lmath;

    (void)argc;
    (void)argv;

    /* Create a minimal config without lw/wip parameters */
    config = ps_config_init(defn);
    TEST_ASSERT(config != NULL);

    /* Create log math object */
    lmath = logmath_init(1.0001, 0, 0);
    TEST_ASSERT(lmath != NULL);

    /* Try to read a language model - this should work without errors */
    lm = ngram_model_read(config, DATADIR "/turtle.lm.bin",
                          NGRAM_AUTO, lmath);
    TEST_ASSERT(lm != NULL);

    /* Clean up */
    ngram_model_free(lm);
    logmath_free(lmath);
    ps_config_free(config);

    return 0;
}
