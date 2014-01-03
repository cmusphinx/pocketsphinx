
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "pocketsphinx.h"

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

static char *
replace_str(char *str, char *orig, char *rep)
{
    static char buffer[4096];
    char *p;

    if (!(p = strstr(str, orig)))
        return str;

    strncpy(buffer, str, p - str);
    buffer[p - str] = '\0';

    sprintf(buffer + (p - str), "%s%s", rep, p + strlen(orig));

    return buffer;
}

int
main(int argc, char *argv[])
{
    cmd_ln_t *config;
    ps_decoder_t *ps;

    int32 n_detect;
    float32 threshold;
    char *out_uttid;
    char *audio_file_path;
    const char *cfg;

    FILE *audioFile;
    FILE *hypFile;
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
    ps_default_search_args(config);
    ps = ps_init(config);

    threshold = cmd_ln_float32_r(config, "-kws_threshold");
    audio_file_path = ckd_salloc(cmd_ln_str_r(config, "-infile"));
    audioFile = fopen(audio_file_path, "rb");
    fread(buf, 1, 44, audioFile);

    ps_start_utt(ps, NULL);
    while ((k = fread(buf, sizeof(int16), 2048, audioFile)) > 0) {
        ps_process_raw(ps, buf, k, FALSE, FALSE);
    }
    ps_end_utt(ps);
    ps_get_hyp(ps, &n_detect, &out_uttid);

    fclose(audioFile);

    hypFile =
        fopen(replace_str(audio_file_path, ".wav", ".kws.hyp"), "wb");
    fprintf(hypFile, "%d\n", n_detect);
    fclose(hypFile);

    ps_free(ps);
    cmd_ln_free_r(config);
    ckd_free(audio_file_path);

    return 0;
}
