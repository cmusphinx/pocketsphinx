/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/**
 * Test limit on FFT size
 */

#include <stdio.h>
#include <string.h>

#include <pocketsphinx.h>
#include "fe/fe.h"

#include "test_macros.h"

int
main(int argc, char *argv[])
{
    ps_config_t *config;
    FILE *infh;

    (void)argc;
    (void)argv;

    config = ps_config_init(NULL);
    infh = fopen(DATADIR "/awful.wav", "rb");
    TEST_ASSERT(0 == ps_config_wavfile(config, infh, "awful.wav"));
    TEST_ASSERT(NULL == fe_init_auto_r(config));
    ps_config_free(config);
    fclose(infh);

    return 0;
}
