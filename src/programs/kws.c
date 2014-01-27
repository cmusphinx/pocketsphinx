
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "pocketsphinx.h"
#include <sphinxbase/err.h>

static const arg_t cont_args_def[] = {
    POCKETSPHINX_OPTIONS,
    {"-argfile",
     ARG_STRING,
     NULL,
     "Argument file giving extra arguments."},
    {"-infile",
     ARG_STRING,
     NULL,
     "Audio file to transcribe."},
    CMDLN_EMPTY_OPTION
};

int
main(int argc, char *argv[])
{
    cmd_ln_t *config;
    ps_decoder_t *ps;

    int32 n_detect;

    const char *input_file_path;
    const char *cfg;
    const char *utt_id;
    FILE *input_file;
    int16 buf[2048];
    int k;

    if (argc == 2) {
        config = cmd_ln_parse_file_r(NULL, cont_args_def, argv[1], TRUE);
    }
    else {
        config = cmd_ln_parse_r(NULL, cont_args_def, argc, argv, FALSE);
    }

    /* Handle argument file as -argfile. */
    if (config && (cfg = cmd_ln_str_r(config, "-argfile")) != NULL) {
        config = cmd_ln_parse_file_r(config, cont_args_def, cfg, FALSE);
    }
    if (config == NULL)
        return 1;

    if (cmd_ln_str_r(config, "-kws") == NULL) {
        E_ERROR("Keyword is missing. Use -kws <keyphrase> to specify the phrase to look for.\n");
        return 1;
    }

    input_file_path = cmd_ln_str_r(config, "-infile");
    if (input_file_path == NULL) {
        E_ERROR("Input file is missing. Use -infile <input_file> to specify the file to look in.\n");
        return 1;
    }

    ps_default_search_args(config);
    ps = ps_init(config);
    
    if (ps == NULL) {
        E_ERROR("Failed to create the decoder\n");
        return 1;
    }
    
    input_file = fopen(input_file_path, "rb");
    if (input_file == NULL) {
        E_FATAL_SYSTEM("Failed to open input file '%s'", input_file_path);
    }
    
    fread(buf, 1, 44, input_file);

    ps_start_utt(ps, NULL);
    while ((k = fread(buf, sizeof(int16), 2048, input_file)) > 0) {
        ps_process_raw(ps, buf, k, FALSE, FALSE);
    }
    ps_end_utt(ps);
    ps_get_hyp(ps, &n_detect, &utt_id);
    E_INFO("Detected %d times\n", n_detect);

    fclose(input_file);
    ps_free(ps);
    cmd_ln_free_r(config);

    return 0;
}
