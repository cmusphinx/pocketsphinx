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
#include "pocketsphinx_internal.h"

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

static void
output_hyp(ps_endpointer_t *ep, ps_decoder_t *decoder)
{
    logmath_t *lmath;
    char *hyp_json, *ptr;
    ps_seg_t *itor;
    int frate;
    int maxlen, len;
    double st;

    maxlen = format_hyp(NULL, 0, ep, decoder);
    maxlen += 2; /* ",{" */
    lmath = ps_get_logmath(decoder);
    frate = ps_config_int(ps_get_config(decoder), "frate");
    if (ep == NULL)
        st = 0.0;
    else
        st = ps_endpointer_speech_start(ep);
    for (itor = ps_seg_iter(decoder); itor; itor = ps_seg_next(itor)) {
        maxlen += format_seg(NULL, 0, itor, st, frate, lmath);
        maxlen++; /* "," or "}" at end */
    }
    maxlen++; /* final } */
    maxlen++; /* trailing \0 */

    ptr = hyp_json = ckd_calloc(maxlen, 1);
    len = maxlen;
    len = format_hyp(hyp_json, len, ep, decoder);
    ptr += len;
    maxlen -= len;

    assert(maxlen > 2);
    strcpy(ptr, ",{");
    ptr += 2;
    maxlen -= 2;

    for (itor = ps_seg_iter(decoder); itor; itor = ps_seg_next(itor)) {
        assert(maxlen > 0);
        len = format_seg(ptr, maxlen, itor, st, frate, lmath);
        ptr += len;
        maxlen -= len;
        *ptr++ = ',';
        maxlen--;
    }
    --ptr;
    *ptr++ = '}';
    assert(maxlen == 2);
    *ptr++ = '}';
    --maxlen;
    *ptr = '\0';
    puts(hyp_json);
    ckd_free(hyp_json);
}

static int
live(ps_config_t *config)
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
                                 3, ps_config_int(config, "samprate"),
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
                         frame_size, stdin)) != frame_size) {
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
                output_hyp(ep, decoder);
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
single(ps_config_t *config)
{
    ps_decoder_t *decoder = NULL;
    short *data, *ptr;
    size_t data_size, block_size;

    if ((decoder = ps_init(config)) == NULL) {
        E_FATAL("PocketSphinx decoder init failed\n");
        goto error_out;
    }
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
        len = fread(ptr, sizeof(*ptr), block_size, stdin);
        if (len == 0) {
            if (feof(stdin))
                break;
            else
                E_ERROR_SYSTEM("Failed to read %d bytes\n",
                               sizeof(*ptr) * block_size);
        }
        ptr += len;
    }
    ps_start_utt(decoder);
    if (ps_process_raw(decoder, data, ptr - data, FALSE, TRUE) < 0) {
        E_ERROR("ps_process_raw() failed\n");
        goto error_out;
    }
    ps_end_utt(decoder);
    output_hyp(NULL, decoder);
    ckd_free(data);
    ps_free(decoder);
    return 0;

error_out:
    if (data)
        ckd_free(data);
    if (decoder)
        ps_free(decoder);
    return -1;
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

static const char *
find_command(int argc, char *argv[])
{
    /* Very unsophisticated, assume that it is at the end of the
     * key/value arguments, we will maybe get a real argument parser
     * one of these days. */
    if (argc % 2) {
        /* No extra argument */
        return "live";
    }
    else {
        return argv[argc-1];
    }
}

int
main(int argc, char *argv[])
{
    ps_config_t *config;
    const char *command;
    int rv;

    if ((config = cmd_ln_parse_r(NULL, ps_args(), argc, argv, TRUE)) == NULL) {
        cmd_ln_log_help_r(NULL, ps_args());
        return 1;
    }
    ps_default_search_args(config);
    command = find_command(argc, argv);
    if (0 == strcmp(command, "soxflags")) {
        rv = soxflags(config);
    }
    else if (0 == strcmp(command, "live")) {
        rv = live(config);
    }
    else if (0 == strcmp(command, "single")) {
        rv = single(config);
    }
    else {
        E_ERROR("Unknown command \"%s\"\n", command);
        return 1;
    }

    ps_config_free(config);
    return rv;
}
