/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 1999-2022 Carnegie Mellon University.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * This work was supported in part by funding from the Defense Advanced 
 * Research Projects Agency and the National Science Foundation of the 
 * United States of America, and the CMU Sphinx Speech Consortium.
 *
 * THIS SOFTWARE IS PROVIDED BY CARNEGIE MELLON UNIVERSITY ``AS IS'' AND 
 * ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
 * NOR ITS EMPLOYEES BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ====================================================================
 *
 */
/**
 * @file pocketsphinx.c
 * @brief Simple command-line speech recognition.
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <signal.h>

#include <pocketsphinx.h>

#include "util/ckd_alloc.h"
#include "config_macro.h"
#include "pocketsphinx_internal.h"
#include "ps_alignment_internal.h"

/* Le sigh.  Didn't want to have to do this. */
static const ps_arg_t ps_main_args_def[] = {
    POCKETSPHINX_OPTIONS,
    { "config",
      ARG_STRING,
      NULL,
      "JSON file with configuration." },
    { "phone_align",
      ARG_BOOLEAN,
      "no",
      "Run a second pass to align phones and print their durations "
      "(DOES NOT WORK IN LIVE MODE)." },
    { "state_align",
      ARG_BOOLEAN,
      "no",
      "Run a second pass to align phones and states and print their durations "
      "(Implies -phone_align) "
      "(DOES NOT WORK IN LIVE MODE)." },
    CMDLN_EMPTY_OPTION
};

static int global_done = 0;
static void
catch_sig(int signum)
{
    (void)signum;
    global_done = 1;
}

#define HYP_FORMAT "{\"b\":%.3f,\"d\":%.3f,\"p\":%.3f,\"t\":\"%s\""
static int
format_hyp(char *outptr, int len, ps_endpointer_t *ep, ps_decoder_t *decoder)
{
    logmath_t *lmath;
    double prob, st, et;
    const char *hyp;

    lmath = ps_get_logmath(decoder);
    prob = logmath_exp(lmath, ps_get_prob(decoder));
    if (ep == NULL) {
        st = 0.0;
        et = (double)ps_get_n_frames(decoder)
            / ps_config_int(ps_get_config(decoder), "frate");
    }
    else {
        st = ps_endpointer_speech_start(ep);
        et = ps_endpointer_speech_end(ep);
    }
    hyp = ps_get_hyp(decoder, NULL);
    if (hyp == NULL)
        hyp = "";
    return snprintf(outptr, len, HYP_FORMAT, st, et - st, prob, hyp);
}

static int
format_seg(char *outptr, int len, ps_seg_t *seg,
           double utt_start, int frate,
           logmath_t *lmath)
{
    double prob, st, dur;
    int sf, ef;
    const char *word;

    ps_seg_frames(seg, &sf, &ef);
    st = utt_start + (double)sf / frate;
    dur = (double)(ef + 1 - sf) / frate;
    word = ps_seg_word(seg);
    if (word == NULL)
        word = "";
    prob = logmath_exp(lmath, ps_seg_prob(seg, NULL, NULL, NULL));
    len = snprintf(outptr, len, HYP_FORMAT, st, dur, prob, word);
    if (outptr) {
        outptr += len;
        *outptr++ = '}';
        *outptr = '\0';
    }
    len++;
    return len;
}

static int
format_align_iter(char *outptr, int maxlen,
                  ps_alignment_iter_t *itor, double utt_start, int frate, logmath_t *lmath)
{
    int start, duration, score;
    double prob, st, dur;
    const char *word;

    score = ps_alignment_iter_seg(itor, &start, &duration);
    st = utt_start + (double)start / frate;
    dur = (double)duration / frate;
    prob = logmath_exp(lmath, score);
    word = ps_alignment_iter_name(itor);
    if (word == NULL)
        word = "";

    return snprintf(outptr, maxlen, HYP_FORMAT, st, dur, prob, word);
}

static int
format_seg_align(char *outptr, int maxlen,
                 ps_alignment_iter_t *itor,
                 double utt_start, int frate,
                 logmath_t *lmath, int state_align)
{
    ps_alignment_iter_t *pitor;
    int len = 0, hyplen;

    hyplen = format_align_iter(outptr, maxlen,
                               itor, utt_start, frate, lmath);
    len += hyplen;
    if (outptr)
        outptr += hyplen;
    if (maxlen)
        maxlen -= hyplen;

    len += 6; /* "w":,[ */
    if (outptr) {
        memcpy(outptr, ",\"w\":[", 6);
        outptr += 6;
    }
    if (maxlen)
        maxlen -= 6;
    
    pitor = ps_alignment_iter_children(itor);
    while (pitor != NULL) {
        hyplen = format_align_iter(outptr, maxlen,
                                   pitor, utt_start, frate, lmath);
        len += hyplen;
        if (outptr)
            outptr += hyplen;
        if (maxlen)
            maxlen -= hyplen;

        /* FIXME: refactor with recursion, someday */
        if (state_align) {
            ps_alignment_iter_t *sitor = ps_alignment_iter_children(pitor);
            len += 6; /* "w":,[ */
            if (outptr) {
                memcpy(outptr, ",\"w\":[", 6);
                outptr += 6;
            }
            if (maxlen)
                maxlen -= 6;
            while (sitor != NULL) {
                hyplen = format_align_iter(outptr, maxlen,
                                           sitor, utt_start, frate, lmath);
                len += hyplen;
                if (outptr)
                    outptr += hyplen;
                if (maxlen)
                    maxlen -= hyplen;

                len++; /* } */
                if (outptr)
                    *outptr++ = '}';
                if (maxlen)
                    maxlen--;
                sitor = ps_alignment_iter_next(sitor);
                if (sitor != NULL) {
                    len++;
                    if (outptr)
                        *outptr++ = ',';
                    if (maxlen)
                        maxlen--;
                }
            }
            len++;
            if (outptr)
                *outptr++ = ']';
            if (maxlen)
                maxlen--;
        }

        len++; /* } */
        if (outptr)
            *outptr++ = '}';
        if (maxlen)
            maxlen--;
        pitor = ps_alignment_iter_next(pitor);
        if (pitor != NULL) {
            len++;
            if (outptr)
                *outptr++ = ',';
            if (maxlen)
                maxlen--;
        }
    }

    len += 2;
    if (outptr) {
        *outptr++ = ']';
        *outptr++ = '}';
        *outptr = '\0';
    }
    if (maxlen)
        maxlen--;
    
    return len;
}

static void
output_hyp(ps_endpointer_t *ep, ps_decoder_t *decoder, ps_alignment_t *alignment)
{
    logmath_t *lmath;
    char *hyp_json, *ptr;
    int frate;
    int maxlen, len;
    double st;
    int state_align = ps_config_bool(decoder->config, "state_align");

    maxlen = format_hyp(NULL, 0, ep, decoder);
    maxlen += 6; /* "w":,[ */
    lmath = ps_get_logmath(decoder);
    frate = ps_config_int(ps_get_config(decoder), "frate");
    if (ep == NULL)
        st = 0.0;
    else
        st = ps_endpointer_speech_start(ep);
    if (alignment) {
        ps_alignment_iter_t *itor = ps_alignment_words(alignment);
        if (itor == NULL)
            maxlen++; /* ] at end */
        for (; itor; itor = ps_alignment_iter_next(itor)) {
            maxlen += format_seg_align(NULL, 0, itor, st, frate,
                                       lmath, state_align);
            maxlen++; /* , or ] at end */
        }
    }
    else {
        ps_seg_t *itor = ps_seg_iter(decoder);
        if (itor == NULL)
            maxlen++; /* ] at end */
        for (; itor; itor = ps_seg_next(itor)) {
            maxlen += format_seg(NULL, 0, itor, st, frate, lmath);
            maxlen++; /* , or ] at end */
        }
    }
    maxlen++; /* final } */
    maxlen++; /* trailing \0 */

    ptr = hyp_json = ckd_calloc(maxlen, 1);
    len = maxlen;
    len = format_hyp(hyp_json, len, ep, decoder);
    ptr += len;
    maxlen -= len;

    assert(maxlen > 6);
    memcpy(ptr, ",\"w\":[", 6);
    ptr += 6;
    maxlen -= 6;

    if (alignment) {
        ps_alignment_iter_t *itor;
        for (itor = ps_alignment_words(alignment); itor;
             itor = ps_alignment_iter_next(itor)) {
            assert(maxlen > 0);
            len = format_seg_align(ptr, maxlen, itor, st, frate, lmath,
                                   state_align);
            ptr += len;
            maxlen -= len;
            *ptr++ = ',';
            maxlen--;
        }
    }
    else {
        ps_seg_t *itor = ps_seg_iter(decoder);
        if (itor == NULL) {
            *ptr++ = ']'; /* Gets overwritten below... */
            maxlen--;
        }
        for (; itor; itor = ps_seg_next(itor)) {
            assert(maxlen > 0);
            len = format_seg(ptr, maxlen, itor, st, frate, lmath);
            ptr += len;
            maxlen -= len;
            *ptr++ = ',';
            maxlen--;
        }
    }
    --ptr;
    *ptr++ = ']';
    assert(maxlen == 2);
    *ptr++ = '}';
    --maxlen;
    *ptr = '\0';
    puts(hyp_json);
    ckd_free(hyp_json);
}

static int
live(ps_config_t *config, FILE *infile)
{
    ps_decoder_t *decoder = NULL;
    ps_endpointer_t *ep = NULL;
    short *frame = NULL;
    size_t frame_size;

    if ((decoder = ps_init(config)) == NULL) {
        E_FATAL("PocketSphinx decoder init failed\n");
        goto error_out;
    }
    if ((ep = ps_endpointer_init(0, 0.0,
                                 0, ps_config_int(config, "samprate"),
                                 0)) == NULL) {
        E_ERROR("PocketSphinx endpointer init failed\n");
        goto error_out;
    }
    frame_size = ps_endpointer_frame_size(ep);
    if ((frame = ckd_calloc(frame_size, sizeof(frame[0]))) == NULL) {
        E_ERROR("Failed to allocate frame");
        goto error_out;
    }
    if (signal(SIGINT, catch_sig) == SIG_ERR)
        E_FATAL_SYSTEM("Failed to set SIGINT handler");
    while (!global_done) {
        const int16 *speech;
        int prev_in_speech = ps_endpointer_in_speech(ep);
        size_t len, end_samples;
        if ((len = fread(frame, sizeof(frame[0]),
                         frame_size, infile)) != frame_size) {
            if (len > 0) {
                speech = ps_endpointer_end_stream(ep, frame,
                                                  frame_size,
                                                  &end_samples);
            }
            else
                break;
        } else
            speech = ps_endpointer_process(ep, frame);
        if (speech != NULL) {
            if (!prev_in_speech) {
                E_INFO("Speech start at %.2f\n",
                       ps_endpointer_speech_start(ep));
                ps_start_utt(decoder);
            }
            if (ps_process_raw(decoder, speech, frame_size, FALSE, FALSE) < 0) {
                E_ERROR("ps_process_raw() failed\n");
                goto error_out;
            }
            if (!ps_endpointer_in_speech(ep)) {
                E_INFO("Speech end at %.2f\n",
                       ps_endpointer_speech_end(ep));
                ps_end_utt(decoder);
                if (ps_config_bool(decoder->config, "phone_align"))
                    E_WARN("Subword alignment not yet supported in live mode\n");
                output_hyp(ep, decoder, NULL);
            }
        }
    }
    ckd_free(frame);
    ps_endpointer_free(ep);
    ps_free(decoder);
    return 0;

error_out:
    if (frame)
        ckd_free(frame);
    if (ep)
        ps_endpointer_free(ep);
    if (decoder)
        ps_free(decoder);
    return -1;
}

static int
decode_single(ps_decoder_t *decoder, FILE *infile)
{
    ps_alignment_t *alignment = NULL;
    size_t data_size, block_size;
    short *data, *ptr;
    int rv = 0;

    data_size = 65536;
    block_size = 2048;
    ptr = data = ckd_calloc(data_size, sizeof(*data));
    if (signal(SIGINT, catch_sig) == SIG_ERR)
        E_FATAL_SYSTEM("Failed to set SIGINT handler");
    while (!global_done) {
        size_t len;
        if ((size_t)(ptr + block_size - data) > data_size) {
            len = ptr - data;
            data_size *= 2;
            data = ckd_realloc(data, data_size * sizeof(*data));
            ptr = data + len;
        }
        len = fread(ptr, sizeof(*ptr), block_size, infile);
        if (len == 0) {
            if (feof(infile))
                break;
            else {
                E_ERROR_SYSTEM("Failed to read %d bytes\n",
                               sizeof(*ptr) * block_size);
                rv = -1;
                goto error_out;
            }
        }
        ptr += len;
    }
    if ((rv = ps_start_utt(decoder)) < 0)
        goto error_out;
    if ((rv = ps_process_raw(decoder, data, ptr - data, FALSE, TRUE)) < 0) {
        E_ERROR("ps_process_raw() failed\n");
        goto error_out;
    }
    if ((rv = ps_end_utt(decoder)) < 0)
        goto error_out;
    if (ps_config_bool(decoder->config, "phone_align")) {
        if (ps_set_alignment(decoder, NULL) < 0)
            goto error_out;
        if ((rv = ps_start_utt(decoder)) < 0)
            goto error_out;
        if ((rv = ps_process_raw(decoder, data, ptr - data, FALSE, TRUE)) < 0) {
            E_ERROR("ps_process_raw() failed\n");
            goto error_out;
        }
        if ((rv = ps_end_utt(decoder)) < 0)
            goto error_out;
        if ((alignment = ps_get_alignment(decoder)) == NULL)
            goto error_out;
        ps_activate_search(decoder, NULL);
    }
    output_hyp(NULL, decoder, alignment);
    /* Fall through intentionally */
error_out:
    ckd_free(data);
    return rv;
}

static int
single(ps_config_t *config, FILE *infile)
{
    ps_decoder_t *decoder;
    int rv = 0;

    if ((decoder = ps_init(config)) == NULL) {
        E_FATAL("PocketSphinx decoder init failed\n");
        return -1;
    }
    rv = decode_single(decoder, infile);
    ps_free(decoder);
    return rv;
}

static char *
string_array_join(char **strings, int nstrings)
{
    char *joined, *ptr;
    int i, *len, jlen;

    len = ckd_malloc(nstrings * sizeof(*len));
    for (jlen = i = 0; i < nstrings; ++i) {
        len[i] = strlen(strings[i]);
        jlen += len[i] + 1;
    }
    ptr = joined = ckd_malloc(jlen);
    for (i = 0; i < nstrings; ++i) {
        memcpy(ptr, strings[i], len[i]);
        ptr += len[i];
        *ptr++ = ' ';
    }
    *--ptr = '\0';
    ckd_free(len);
    return joined;
}

static int
align(ps_config_t *config, char **inputs, int ninputs)
{
    int rv = 0, is_stdin = FALSE;
    ps_decoder_t *decoder = NULL;
    char *text = NULL;
    FILE *fh = NULL;

    if (ninputs < 2) {
        E_ERROR("Usage: pocketsphinx align INFILE TEXT...\n");
        return -1;
    }
    /* Please do not use bestpath for alignment. */
    ps_config_set_bool(config, "bestpath", FALSE);
    ps_config_set_str(config, "lm", NULL);
    if (0 == strcmp(inputs[0], "-")) {
        is_stdin = TRUE;
        fh = stdin;
    }
    else if ((fh = fopen(inputs[0], "rb")) == NULL) {
        E_ERROR_SYSTEM("Failed to open %s for input", inputs[0]);
        goto error_out;
    }
    if ((rv = ps_config_soundfile(config, fh, inputs[0])) < 0)
        goto error_out;
    if ((decoder = ps_init(config)) == NULL) {
        E_FATAL("PocketSphinx decoder init failed\n");
        rv = -1;
        goto error_out;
    }
    text = string_array_join(inputs + 1, ninputs - 1);
    if ((rv = ps_set_align_text(decoder, text)) < 0)
        goto error_out;
    rv = decode_single(decoder, fh);
    /* Fall through intentionally. */
error_out:
    if (fh && !is_stdin)
        fclose(fh);
    if (text)
        ckd_free(text);
    if (decoder)
        ps_free(decoder);
    return rv;
}

#if 0
static int sample_rates[] = {
    8000,
    11025,
    16000,
    22050,
    32000,
    44100,
    48000
};
static const int n_sample_rates = sizeof(sample_rates)/sizeof(sample_rates[0]);

static int
minimum_samprate(ps_config_t *config)
{
    double upperf = ps_config_float(config, "upperf");
    int nyquist = (int)(upperf * 2);
    int i;
    for (i = 0; i < n_sample_rates; ++i)
        if (sample_rates[i] >= nyquist)
            break;
    if (i == n_sample_rates)
        E_FATAL("Unable to find sampling rate for -upperf %f\n", upperf);
    return sample_rates[i];
}
#endif

#define SOX_FORMAT "-r %d -c 1 -b 16 -e signed-integer -t raw -"
static int
soxflags(ps_config_t *config)
{
    int maxlen, len;
    int samprate;
    char *args;

    /* Get feature extraction parameters. */
    ps_expand_model_config(config);
    samprate = ps_config_int(config, "samprate");

    maxlen = snprintf(NULL, 0, SOX_FORMAT, samprate);
    if (maxlen < 0) {
        E_ERROR_SYSTEM("Failed to snprintf()");
        return -1;
    }
    maxlen++;
    args = ckd_calloc(maxlen, 1);
    len = snprintf(args, maxlen, SOX_FORMAT, samprate);
    if (len != maxlen - 1) {
        E_ERROR_SYSTEM("Failed to snprintf()");
        return -1;
    }
    puts(args);
    fflush(stdout);
    ckd_free(args);
    return 0;
}

static char *
find_command(int *argc, char **argv)
{
    int i;
    for (i = 1; i < *argc; i += 2) {
        char *arg = argv[i];
        if (arg && arg[0] && arg[0] != '-') {
            memmove(&argv[i],
                    &argv[i + 1],
                    (*argc - i - 1) * sizeof(argv[i]));
            --*argc;
            return arg;
        }
    }
    return "live";
}

static char **
find_inputs(int *argc, char **argv, int *ninputs)
{
    char **inputs = NULL;
    int i = 1;
    *ninputs = 0;
    while (i < *argc) {
        char *arg = argv[i];
        /* Bubble-bogo-bobo-backward-sort them to the end of argv. */
        if (arg && arg[0]
            /* "-" on its own is an input, otherwise, - starts args. */
            && (arg[0] != '-' || arg[1] == '\0')) {
            memmove(&argv[i],
                    &argv[i + 1],
                    (*argc - i - 1) * sizeof(argv[i]));
            --*argc;
            argv[*argc] = arg;
            inputs = &argv[*argc];
            ++*ninputs;
        }
        else
            i += 2;
    }
    /* Now reverse them.  I won't be passing Google's coding interview
       any time soon, not that it matters in this particular case. */
    for (i = 0; i < *ninputs / 2; ++i) {
        char *tmp = inputs[i];
        inputs[i] = inputs[*ninputs - i - 1];
        inputs[*ninputs - i - 1] = tmp;
    }
    return inputs;
}

int
process_inputs(int (*func)(ps_config_t *, FILE *),
               ps_config_t *config,
               char **inputs, int ninputs)
{
    int rv = 0;
    
    if (ninputs == 0)
        return func(config, stdin);
    else {
        int i, rv_one;
        for (i = 0; i < ninputs; ++i) {
            char *file = inputs[i];
            int is_stdin = FALSE;
            FILE *fh;

            if (0 == strcmp(file, "-")) {
                is_stdin = TRUE;
                fh = stdin;
            }
            else if ((fh = fopen(file, "rb")) == NULL) {
                E_ERROR_SYSTEM("Failed to open %s for input", file);
                rv = -1;
                continue;
            }
            if ((rv_one = ps_config_soundfile(config, fh, file)) < 0) {
                fclose(fh);
                rv = rv_one;
                continue;
            }
            if ((rv_one = func(config, fh)) < 0) {
                rv = rv_one;
                E_ERROR("Recognition failed on %s\n", file);
            }
            if (!is_stdin)
                fclose(fh);
        }
    }
    return rv;
}

static int
print_config(ps_config_t *config)
{
    if (puts(ps_config_serialize_json(config)) < 0)
        return -1;
    return 0;
}

void
usage(char *name, int help_config)
{
    fprintf(stderr, "Usage: %s [PARAMS] [soxflags | config | help | help-config | live | single | align] INPUTS...\n", name);
    fprintf(stderr, "Examples:\n");
    fprintf(stderr, "\tsox input.mp3 $(%s soxflags) | %s single -\n", name, name);
    fprintf(stderr, "\tsox -qd $(%s soxflags) | %s live -\n", name, name);
    fprintf(stderr, "\t%s single INPUT\n", name);
    fprintf(stderr, "\t%s align INPUT WORDS...\n", name);
    fprintf(stderr, "\nFor detailed PARAMS values, run %s help-config\n", name);
    if (help_config) {
        err_set_loglevel(ERR_INFO);
        cmd_ln_log_help_r(NULL, ps_args());
    }
}

int
main(int argc, char *argv[])
{
    ps_config_t *config;
    const char *conffile;
    char *command;
    char **inputs;
    int rv, ninputs;

    command = find_command(&argc, argv);
    inputs = find_inputs(&argc, argv, &ninputs);
    /* Only soxflags and config take no arguments */
    if (ninputs == 0) {
        if ((0 != strcmp(command, "soxflags"))
            && 0 != strcmp(command, "config")
            && 0 != strcmp(command, "help-config")) {
            usage(argv[0], FALSE);
            return 1;
        }
    }
    /* If arg parsing fails */
    if ((config = ps_config_parse_args(ps_main_args_def, argc, argv)) == NULL) {
        usage(argv[0], FALSE);
        return 1;
    }
    ps_default_search_args(config);
    if (ps_config_bool(config, "state_align"))
        ps_config_set_bool(config, "phone_align", TRUE);
    if ((conffile = ps_config_str(config, "config")) != NULL) {
        char *json;
        FILE *infh;
        size_t len;
        if ((infh = fopen(conffile, "rt")) == NULL) {
            E_ERROR_SYSTEM("Failed to open config file %s", conffile);
            return 1;
        }
        fseek(infh, 0, SEEK_END);
        len = (size_t)ftell(infh);
        fseek(infh, 0, SEEK_SET);
        json = ckd_malloc(len + 1);
        if (fread(json, 1, len, infh) != len) {
            E_ERROR_SYSTEM("Failed to read config file %s", conffile);
            ckd_free(json);
            fclose(infh);
            return 1;
        }
        json[len] = '\0';
        fclose(infh);
        config = ps_config_parse_json(config, json);
        ckd_free(json);
        if (config == NULL)
            return 1;
        ps_config_set_str(config, "config", NULL);
    }
    if (0 == strcmp(command, "soxflags"))
        rv = soxflags(config);
    else if (0 == strcmp(command, "config"))
        rv = print_config(config);
    else if (0 == strcmp(command, "live"))
        rv = process_inputs(live, config, inputs, ninputs);
    else if (0 == strcmp(command, "single"))
        rv = process_inputs(single, config, inputs, ninputs);
    else if (0 == strcmp(command, "align"))
        rv = align(config, inputs, ninputs);
    else if (0 == strcmp(command, "help")) {
        rv = 0;
        usage(argv[0], FALSE);
    }
    else if (0 == strcmp(command, "help-config")) {
        rv = 0;
        usage(argv[0], TRUE);
    }
    else {
        E_ERROR("Unknown command \"%s\"\n", command);
        return 1;
    }

    ps_config_free(config);
    return rv;
}
