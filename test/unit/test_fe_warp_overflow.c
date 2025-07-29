/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/**
 * Test safe parameter handling in feature warping
 */

#include <stdio.h>
#include <string.h>

#include <pocketsphinx.h>
#include "util/ckd_alloc.h"
#include "fe/fe.h"

#include "test_macros.h"

int
main(int argc, char *argv[])
{
    char *long_str;
    fe_t *fe;
    ps_config_t *config;
    int i;

    (void)argc;
    (void)argv;

    config = ps_config_init(NULL);
    ps_config_set_float(config, "samprate", 16000);

    /* Create string longer than 256 chars */
    long_str = ckd_malloc(512);
    for (i = 0; i < 511; i++)
        long_str[i] = '0' + (i % 10);
    long_str[511] = '\0';

    /* Test all warp types with long parameter */
    ps_config_set_str(config, "warp_type", "affine");
    ps_config_set_str(config, "warp_params", long_str);
    fe = fe_init_auto_r(config);
    TEST_ASSERT(fe != NULL);
    fe_free(fe);

    ps_config_set_str(config, "warp_type", "inverse_linear");
    ps_config_set_str(config, "warp_params", long_str);
    fe = fe_init_auto_r(config);
    TEST_ASSERT(fe != NULL);
    fe_free(fe);

    ps_config_set_str(config, "warp_type", "piecewise_linear");
    ps_config_set_str(config, "warp_params", long_str);
    fe = fe_init_auto_r(config);
    TEST_ASSERT(fe != NULL);
    fe_free(fe);

    ckd_free(long_str);
    ps_config_free(config);

    return 0;
}
