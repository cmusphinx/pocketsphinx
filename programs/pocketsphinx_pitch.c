/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 2008 Carnegie Mellon University.  All rights 
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>

#include <pocketsphinx.h>

#include "util/cmd_ln.h"
#include "util/ckd_alloc.h"
#include "util/byteorder.h"
#include "util/strfuncs.h"
#include "util/pio.h"
#include "fe/yin.h"

#include "pocketsphinx_internal.h"

static ps_arg_t defn[] = {
  { "i",
    ARG_STRING,
    NULL,
    "Single audio input file" },

  { "o",
    ARG_STRING,
    NULL,
    "Single text output file (standard output will be used if not given)" },
  
  { "c",
    ARG_STRING,
    NULL,
    "Control file for batch processing" },
  
  { "nskip",
    ARG_INTEGER,
    "0",
    "If a control file was specified, the number of utterances to skip at the head of the file" },
  
  { "runlen",
    ARG_INTEGER,
    "-1",
    "If a control file was specified, the number of utterances to process (see -nskip too)" },
  
  { "di",
    ARG_STRING,
    NULL,
    "Input directory, input file names are relative to this, if defined" },
  
  { "ei",
    ARG_STRING,
    NULL,
    "Input extension to be applied to all input files" },
  
  { "do",
    ARG_STRING,
    NULL,
    "Output directory, output files are relative to this" },
  
  { "eo",
    ARG_STRING,
    NULL,
    "Output extension to be applied to all output files" },
  
  { "nist",
    ARG_BOOLEAN,
    "no",
    "Defines input format as NIST sphere" },
  
  { "raw",
    ARG_BOOLEAN,
    "no",
    "Defines input format as raw binary data" },
  
  { "mswav",
    ARG_BOOLEAN,
    "no",
    "Defines input format as Microsoft Wav (RIFF)" },

  { "samprate",
    ARG_INTEGER,
    "0",
    "Sampling rate of audio data (will be determined automatically if 0)" },

  { "input_endian",
    ARG_STRING,
    NULL,
    "Endianness of audio data (will be determined automatically if not given)" },

  { "fshift",
    ARG_FLOATING,
    "0.01",
    "Frame shift: number of seconds between each analysis frame." },

  { "flen",
    ARG_FLOATING,
    "0.025",
    "Number of seconds in each analysis frame (needs to be greater than twice the longest period you wish to detect - to detect down to 80Hz you need a frame length of 2.0/80 = 0.025)." },

  { "smooth_window",
    ARG_INTEGER,
    "2",
    "Number of frames on either side of the current frame to use for smoothing." },

  { "voice_thresh",
    ARG_FLOATING,
    "0.1",
    "Threshold of normalized difference under which to search for the fundamental period." },

  { "search_range",
    ARG_FLOATING,
    "0.2",
    "Fraction of the best local estimate to use as a search range for smoothing." },

  { NULL, 0, NULL, NULL }
};

static int extract_pitch(const char *in, const char *out, cmd_ln_t *config);
static int run_control_file(const char *ctl, cmd_ln_t *config);

static int
guess_file_type(char const *file, FILE *infh, ps_config_t *config)
{
    char header[4];

    fseek(infh, 0, SEEK_SET);
    if (fread(header, 1, 4, infh) != 4) {
        E_ERROR_SYSTEM("Failed to read 4 byte header");
        return -1;
    }
    if (0 == memcmp(header, "RIFF", 4)) {
        E_INFO("%s appears to be a WAV file\n", file);
        if (ps_config_typeof(config, "mswav") != 0) {
            ps_config_set_bool(config, "mswav", TRUE);
            ps_config_set_bool(config, "nist", FALSE);
            ps_config_set_bool(config, "raw", FALSE);
        }
    }
    else if (0 == memcmp(header, "NIST", 4)) {
        E_INFO("%s appears to be a NIST SPHERE file\n", file);
        if (ps_config_typeof(config, "mswav") != 0) {
            ps_config_set_bool(config, "mswav", FALSE);
            ps_config_set_bool(config, "nist", TRUE);
            ps_config_set_bool(config, "raw", FALSE);
        }
    }
    else {
        E_INFO("%s appears to be raw data\n", file);
        if (ps_config_typeof(config, "mswav") != 0) {
            ps_config_set_bool(config, "mswav", FALSE);
            ps_config_set_bool(config, "nist", FALSE);
            ps_config_set_bool(config, "raw", TRUE);
        }
    }
    fseek(infh, 0, SEEK_SET);
    return 0;
}

int
main(int argc, char *argv[])
{
    cmd_ln_t *config = cmd_ln_parse_r(NULL, defn, argc, argv, TRUE);

    if (config == NULL) {
        /* This probably just means that we got no arguments. */
        err_set_loglevel(ERR_INFO);
        cmd_ln_log_help_r(NULL, defn);
        return 1;
    }

    /* Run a control file if requested. */
    if (ps_config_str(config, "c")) {
        if (run_control_file(ps_config_str(config, "c"), config) < 0)
            return 1;
    }
    else {
        if (extract_pitch(ps_config_str(config, "i"),
                          ps_config_str(config, "o"),
                          config) < 0)
            return 1;
    }

    ps_config_free(config);
    return 0;
}

static int
extract_pitch(const char *in, const char *out, cmd_ln_t *config)
{
    FILE *infh = NULL, *outfh = NULL;
    size_t flen, fshift, nsamps;
    int16 *buf = NULL;
    yin_t *yin = NULL;
    uint16 period, bestdiff;
    int32 sps;

    if (out) {
        if ((outfh = fopen(out, "w")) == NULL) {
            E_ERROR_SYSTEM("Failed to open %s for writing", out);
            goto error_out;
        }
    }
    else {
        outfh = stdout;
    }
    if ((infh = fopen(in, "rb")) == NULL) {
        E_ERROR_SYSTEM("Failed to open %s for reading", in);
        goto error_out;
    }

    /* If we weren't told what the file type is, weakly try to
     * determine it (actually it's pretty obvious) */
    if (!(ps_config_bool(config, "raw")
          || ps_config_bool(config, "mswav")
          || ps_config_bool(config, "nist"))) {
        if (guess_file_type(in, infh, config) < 0)
            goto error_out;
    }
    
    /* Grab the sampling rate and byte order from the header and also
     * make sure this is 16-bit linear PCM. */
    if (ps_config_bool(config, "mswav")) {
        if (ps_config_wavfile(config, infh, in) < 0)
            goto error_out;
    }
    else if (ps_config_bool(config, "nist")) {
        if (ps_config_nistfile(config, infh, in) < 0)
            goto error_out;
    }
    else if (ps_config_bool(config, "raw")) {
        /* Just use some defaults for sampling rate and endian. */
        if (ps_config_str(config, "input_endian") == NULL) {
            ps_config_set_str(config, "input_endian", "little");
        }
        if (ps_config_int(config, "samprate") == 0)
            ps_config_set_int(config, "samprate", 16000);
    }

    /* Now read frames and write pitch estimates. */
    sps = ps_config_int(config, "samprate");
    flen = (size_t)(0.5 + sps * ps_config_float(config, "flen"));
    fshift = (size_t)(0.5 + sps * ps_config_float(config, "fshift"));
    yin = yin_init(flen, ps_config_float(config, "voice_thresh"),
                   ps_config_float(config, "search_range"),
                   ps_config_int(config, "smooth_window"));
    if (yin == NULL) {
        E_ERROR("Failed to initialize YIN\n");
        goto error_out;
    }
    buf = ckd_calloc(flen, sizeof(*buf));
    /* Read the first full frame of data. */
    if (fread(buf, sizeof(*buf), flen, infh) != flen) {
        /* Fail silently, which is probably okay. */
    }
    yin_start(yin);
    nsamps = 0;
    while (!feof(infh)) {
        /* Process a frame of data. */
        yin_write(yin, buf);
        if (yin_read(yin, &period, &bestdiff)) {
            fprintf(outfh, "%.3f %.2f %.2f\n",
                    /* Time point. */
                    (double)nsamps/sps,
                    /* "Probability" of voicing. */
                    bestdiff > 32768 ? 0.0 : 1.0 - (double)bestdiff / 32768,
                    /* Pitch (possibly bogus) */
                    period == 0 ? sps : (double)sps / period);
            nsamps += fshift;
        }
        /* Shift it back and get the next frame's overlap. */
        memmove(buf, buf + fshift, (flen - fshift) * sizeof(*buf));
        if (fread(buf + flen - fshift, sizeof(*buf), fshift, infh) != fshift) {
            /* Fail silently (FIXME: really?) */
        }
    }
    yin_end(yin);
    /* Process trailing frames of data. */
    while (yin_read(yin, &period, &bestdiff)) {
            fprintf(outfh, "%.3f %.2f %.2f\n",
                    /* Time point. */
                    (double)nsamps/sps,
                    /* "Probability" of voicing. */
                    bestdiff > 32768 ? 0.0 : 1.0 - (double)bestdiff / 32768,
                    /* Pitch (possibly bogus) */
                    period == 0 ? sps : (double)sps / period);
    }

    if (yin)
        yin_free(yin);
    ckd_free(buf);
    fclose(infh);
    if (outfh && outfh != stdout)
        fclose(outfh);
    return 0;

error_out:
    if (yin)
        yin_free(yin);
    ckd_free(buf);
    if (infh) fclose(infh);
    if (outfh && outfh != stdout) 
        fclose(outfh);
    return -1;
}

static int
run_control_file(const char *ctl, cmd_ln_t *config)
{
    FILE *ctlfh;
    char *line;
    char *di, *dout, *ei, *eio;
    size_t len;
    int rv, guess_type, guess_sps, guess_endian;
    int32 skip, runlen;

    skip = ps_config_int(config, "nskip");
    runlen = ps_config_int(config, "runlen");

    /* Whether to guess file types */
    guess_type = !(ps_config_bool(config, "raw")
                   || ps_config_bool(config, "mswav")
                   || ps_config_bool(config, "nist"));
    /* Whether to guess sampling rate */
    guess_sps = (ps_config_int(config, "samprate") == 0);
    /* Whether to guess endian */
    guess_endian = (ps_config_str(config, "input_endian") == NULL);

    if ((ctlfh = fopen(ctl, "r")) == NULL) {
        E_ERROR_SYSTEM("Failed to open control file %s", ctl);
        return -1;
    }
    if (ps_config_str(config, "di"))
        di = string_join(ps_config_str(config, "di"), "/", NULL);
    else
        di = ckd_salloc("");
    if (ps_config_str(config, "do"))
        dout = string_join(ps_config_str(config, "do"), "/", NULL);
    else
        dout = ckd_salloc("");
    if (ps_config_str(config, "ei"))
        ei = string_join(".", ps_config_str(config, "ei"), NULL);
    else
        ei = ckd_salloc("");
    if (ps_config_str(config, "eo"))
        eio = string_join(".", ps_config_str(config, "eo"), NULL);
    else
        eio = ckd_salloc("");
    rv = 0;
    while ((line = fread_line(ctlfh, &len)) != NULL) {
        char *infile, *outfile;

        if (skip-- > 0) {
            ckd_free(line);
            continue;
        }
        if (runlen == 0) {
            ckd_free(line);
            break;
        }
        --runlen;

        if (line[len-1] == '\n')
            line[len-1] = '\0';

        infile = string_join(di, line, ei, NULL);
        outfile = string_join(dout, line, eio, NULL);

        /* Reset various guessed information */
        if (guess_type) {
            ps_config_set_bool(config, "nist", FALSE);
            ps_config_set_bool(config, "mswav", FALSE);
            ps_config_set_bool(config, "raw", FALSE);
        }
        if (guess_sps)
            ps_config_set_int(config, "samprate", 0);
        if (guess_endian)
            ps_config_set_str(config, "input_endian", NULL);

        rv = extract_pitch(infile, outfile, config);

        ckd_free(infile);
        ckd_free(outfile);
        ckd_free(line);

        if (rv != 0)
            break;
    }
    ckd_free(di);
    ckd_free(dout);
    ckd_free(ei);
    ckd_free(eio);
    fclose(ctlfh);
    ps_config_free(config);
    return rv;
}
