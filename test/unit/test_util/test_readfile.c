/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/**
 * @file test_readfile.c: Test for the methods to read the file
 * @author David Huggins-Daines <dhdaines@gmail.com>
 */

#include "util/ckd_alloc.h"
#include "util/bio.h"
#include "test_macros.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int
main(int argc, char *argv[])
{
    size_t nsamps;
    int16 *data;
    ps_config_t *config;
    const char *filename = TESTDATADIR "/chan3.wav";
    FILE *infh;
    
    (void)argc;
    (void)argv;
    data = bio_read_wavfile(TESTDATADIR, "chan3", ".wav", 44, FALSE, &nsamps);
    TEST_EQUAL(230108, nsamps);
    
    ckd_free(data);

    config = ps_config_init(NULL);
    infh = fopen(filename, "rb");
    TEST_ASSERT(0 == ps_config_wavfile(config, infh, filename));
    fclose(infh);

    infh = fopen(DATADIR "/evil.wav", "rb");
    TEST_ASSERT(-1 == ps_config_wavfile(config, infh, filename));
    fclose(infh);

    infh = fopen(DATADIR "/bad.wav", "rb");
    TEST_ASSERT(-1 == ps_config_wavfile(config, infh, filename));
    fclose(infh);

    ps_config_free(config);

    return 0;
}
