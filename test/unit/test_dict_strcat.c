/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/**
 * Test dict_write
 */

#include <stdio.h>
#include <string.h>

#include <pocketsphinx.h>
#include <bin_mdef.h>
#include "dict.h"

#include "test_macros.h"

int
main(int argc, char *argv[])
{
    ps_config_t *config;
    bin_mdef_t *mdef;
    dict_t *dict;

    (void)argc;
    (void)argv;

    config = ps_config_init(NULL);
    ps_config_set_str(config, "dict", MODELDIR "/en-us/cmudict-en-us.dict");

    mdef = bin_mdef_read(NULL, MODELDIR "/en-us/en-us/mdef");
    TEST_ASSERT(mdef != NULL);

    dict = dict_init(config, mdef);
    TEST_ASSERT(dict != NULL);

    /* Test dict_write */
    TEST_ASSERT(dict_write(dict, "_test_dict.txt", NULL) == 0);
    remove("_test_dict.txt");

    dict_free(dict);
    bin_mdef_free(mdef);
    ps_config_free(config);

    return 0;
}
