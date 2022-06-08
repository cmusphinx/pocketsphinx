/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */

#include <stdio.h>
#include <string.h>

#include <sphinxbase/cmd_ln.h>
#include <sphinxbase/ckd_alloc.h>

const arg_t defs[] = {
    { "-a", ARG_INTEGER, "42", "This is the first argument." },
    { "-b", ARG_STRING, NULL, "This is the second argument." },
    { "-c", ARG_BOOLEAN, "no", "This is the third argument." },
    { "-d", ARG_FLOATING, "1e-50", "This is the fourth argument." },
    { NULL, 0, NULL, NULL }
};

static char *orig_argv[] = {
    "foobie",
    "-a", "69"
};
static int orig_argc = sizeof(orig_argv) / sizeof(orig_argv[0]);

int
main(int argc, char *argv[])
{
    cmd_ln_t *config = cmd_ln_parse_r(NULL, defs, orig_argc, orig_argv, TRUE);
    cmd_ln_parse_r(config, defs, argc, argv, FALSE);
    printf("%ld %s %d %f\n",
           cmd_ln_int_r(config, "-a"),
           cmd_ln_str_r(config, "-b") ? cmd_ln_str_r(config, "-b") : "(null)",
           cmd_ln_boolean_r(config, "-c"),
           cmd_ln_float_r(config, "-d"));
    cmd_ln_free_r(config);
           
    return 0;
}
