#include <pocketsphinx.h>
#include "pocketsphinx_internal.h"
#include "test_macros.h"

#define LEN(array) (sizeof(array) / sizeof(array[0]))
static char *good_argv[] = {
    "pocketsphinx",
    "-hmm",
    "en-us",
    "-samprate",
    "16000",
    "-beam",
    "0.005",
    "-backtrace",
    "yes"
};
static int good_argc = LEN(good_argv);
static char *good_json = \
    "{ \"hmm\": \"en-us\",\n"
    "  \"samprate\": 16000,\n"
    "  \"beam\": 5e-3,\n"
    "  \"backtrace\": true\n"
    "}";
static char *cmd_argv[] = {
    "pocketsphinx",
    "-hmm",
    "en-us",
    "-samprate",
    "16000",
    "-beam",
    "0.005",
    "-backtrace",
    "yes"
};
static int cmd_argc = LEN(cmd_argv);
static char *bad_argv[] = {
    "pocketsphinx",
    "-hmm",
    "en-us",
    "-samprate",
    "forty-two",
    "beam",
    "1e-80",
    "-backtrace",
    "WTF"
};
static int bad_argc = LEN(bad_argv);
static char *bad_json = \
    "{ \"hmm\": en us,\n"
    "  \"samprate\": 16000,\n"
    "  \"beam\": 5e-3,\n"
    "  \"backtrace\": true\n"
    "}";
static char *ugly_json = \
    "hmm: en-us samprate: 16000 beam: 0.005 backtrace: true";

static void
check_live_args(ps_config_t *config)
{
    /* Check parsed values */
    TEST_EQUAL(0, strcmp("en-us", ps_config_str(config, "hmm")));
    TEST_EQUAL(16000, ps_config_int(config, "samprate"));
    TEST_EQUAL_FLOAT(5e-3, ps_config_float(config, "beam"));
    
    /* Set new values */
    TEST_ASSERT(ps_config_set_str(config, "hmm", "fr-fr"));
    TEST_EQUAL(0, strcmp("fr-fr", ps_config_str(config, "hmm")));
    TEST_ASSERT(ps_config_set_int(config, "samprate", 8000));
    TEST_EQUAL(8000, ps_config_int(config, "samprate"));
    TEST_ASSERT(ps_config_set_float(config, "beam", 1e-40));
    TEST_EQUAL_FLOAT(1e-40, ps_config_float(config, "beam"));
}

static void
test_config_init(void)
{
    ps_config_t *config;

    TEST_ASSERT(config = ps_config_init(NULL));
    ps_config_retain(config);
    TEST_EQUAL(1, ps_config_free(config));
    TEST_EQUAL(0, ps_config_free(config));

    TEST_ASSERT(config = ps_config_init(NULL));
    TEST_ASSERT(ps_config_set_str(config, "hmm", "en-us"));
    TEST_ASSERT(ps_config_set_int(config, "samprate", 16000));
    TEST_ASSERT(ps_config_set_float(config, "beam", 0.005));
    check_live_args(config);
    TEST_EQUAL(0, ps_config_free(config));
}

static void
test_config_args(void)
{
    ps_config_t *config;
    TEST_ASSERT(config = ps_config_parse_args(NULL, good_argc, good_argv));
    TEST_EQUAL(0, ps_config_free(config));
    TEST_EQUAL(NULL, ps_config_parse_args(NULL, bad_argc, bad_argv));

    TEST_ASSERT(config = ps_config_parse_args(NULL, cmd_argc, cmd_argv));
    check_live_args(config);
    TEST_EQUAL(0, ps_config_free(config));
}

static void
test_config_json(void)
{
    ps_config_t *config;
    char *json;
    
    TEST_ASSERT(config = ps_config_parse_json(NULL, good_json));
    TEST_EQUAL(0, ps_config_free(config));
    TEST_EQUAL(NULL, ps_config_parse_json(NULL, bad_json));

    TEST_ASSERT(config = ps_config_parse_json(NULL, good_json));
    check_live_args(config);
    TEST_EQUAL(0, ps_config_free(config));

    TEST_ASSERT(config = ps_config_parse_json(NULL, ugly_json));
    /* Make a copy now, before it changes in check_live_args */
    json = ckd_salloc(ps_config_serialize_json(config));
    check_live_args(config);
    TEST_EQUAL(0, ps_config_free(config));

    /* Check that serialized JSON gives the same result */
    TEST_ASSERT(config = ps_config_parse_json(NULL, json));
    check_live_args(config);
    TEST_EQUAL(0, ps_config_free(config));
    ckd_free(json);
}

int
main(int argc, char *argv[])
{
    (void)argc; (void)argv;

    test_config_init();
    test_config_args();
    test_config_json();
    
    return 0;
}
