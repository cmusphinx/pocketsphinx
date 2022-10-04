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

#include <string.h>
#include <pocketsphinx.h>
#include "util/byteorder.h"
#include "util/ckd_alloc.h"

#define TRY_FREAD(ptr, size, nmemb, stream)                             \
    if (fread(ptr, size, nmemb, stream) != (nmemb)) {                   \
        E_ERROR_SYSTEM("Failed to read %d bytes", size * nmemb);        \
        rv = -1;                                                        \
        goto error_out;                                                 \
    }

int
ps_config_soundfile(ps_config_t *config, FILE *infh, const char *file)
{
    char header[4];
    int rv = 0;

    if (file == NULL)
        file = "(input filehandle)";
    fseek(infh, 0, SEEK_SET);
    TRY_FREAD(header, 1, 4, infh);
    fseek(infh, 0, SEEK_SET);

    if (0 == memcmp(header, "RIFF", 4)) {
        E_INFO("%s appears to be a WAV file\n", file);
        rv = ps_config_wavfile(config, infh, file);
    }
    else if (0 == memcmp(header, "NIST", 4)) {
        E_INFO("%s appears to be a NIST SPHERE file\n", file);
        rv = ps_config_nistfile(config, infh, file);
    }
    else if (0 == memcmp(header, "OggS", 4)) {
        E_INFO("%s appears to be an UNSUPPORTED Ogg file\n", file);
        rv = -1;
        goto error_out;
    }
    else if (0 == memcmp(header, "fLaC", 4)) {
        E_INFO("%s appears to be an UNSUPPORTED FLAC file\n", file);
        rv = -1;
        goto error_out;
    }
    else if (0 == memcmp(header, "\xff\xff\xff", 3)
             || 0 == memcmp(header, "\xff\xff\xfe", 3)) {
        E_INFO("%s might be an MP3 file, but who knows really! "
               "UNSUPPORTED!\n", file);
        rv = -1;
        goto error_out;
    }
    else {
        E_INFO("%s appears to be raw data\n", file);
    }

error_out:
    return rv;
}

int
ps_config_wavfile(ps_config_t *config, FILE *infh, const char *file)
{
    char id[4];
    int32 intval, header_len;
    int16 shortval;
    int rv = 0;

    if (file == NULL)
        file = "(input filehandle)";
        
    /* RIFF files are little-endian by definition. */
    ps_config_set_str(config, "input_endian", "little");

    /* Read in all the header chunks and etcetera. */
    TRY_FREAD(id, 1, 4, infh);
    /* Total file length (we don't care) */
    TRY_FREAD(&intval, 4, 1, infh);
    /* 'WAVE' */
    TRY_FREAD(id, 1, 4, infh);
    if (0 != memcmp(id, "WAVE", 4)) {
        E_ERROR("%s is not a WAVE file\n", file);
        rv = -1;
        goto error_out;
    }
    /* 'fmt ' */
    TRY_FREAD(id, 1, 4, infh);
    if (0 != memcmp(id, "fmt ", 4)) {
        E_ERROR("Format chunk missing\n");
        rv = -1;
        goto error_out;
    }
    /* Length of 'fmt ' chunk */
    TRY_FREAD(&intval, 4, 1, infh);
    SWAP_LE_32(&intval);
    header_len = intval;

    /* Data format. */
    TRY_FREAD(&shortval, 2, 1, infh);
    SWAP_LE_16(&shortval);
    if (shortval != 1) { /* PCM */
        E_ERROR("%s is not in PCM format\n", file);
        rv = -1;
        goto error_out;
    }

    /* Number of channels. */
    TRY_FREAD(&shortval, 2, 1, infh);
    SWAP_LE_16(&shortval);
    if (shortval != 1) { /* PCM */
        E_ERROR("%s is not single channel\n", file);
        rv = -1;
        goto error_out;
    }

    /* Sampling rate (finally!) */
    TRY_FREAD(&intval, 4, 1, infh);
    SWAP_LE_32(&intval);
    if (ps_config_int(config, "samprate") == 0)
        ps_config_set_int(config, "samprate", intval);
    else if (ps_config_int(config, "samprate") != intval) {
        E_WARN("WAVE file sampling rate %d != samprate %d\n",
               intval, ps_config_int(config, "samprate"));
    }

    /* Average bytes per second (we don't care) */
    TRY_FREAD(&intval, 4, 1, infh);

    /* Block alignment (we don't care) */
    TRY_FREAD(&shortval, 2, 1, infh);

    /* Bits per sample (must be 16) */
    TRY_FREAD(&shortval, 2, 1, infh);
    SWAP_LE_16(&shortval);
    if (shortval != 16) {
        E_ERROR("%s is not 16-bit\n", file);
        rv = -1;
        goto error_out;
    }

    /* Any extra parameters. */
    if (header_len > 16) {
        /* Avoid seeking... */
        char *spam = malloc(header_len - 16);
        if (fread(spam, 1, header_len - 16, infh) != (size_t)(header_len - 16)) {
            E_ERROR_SYSTEM("%s: Failed to read extra header", file);
            rv = -1;
        }
        ckd_free(spam);
        if (rv == -1)
            goto error_out;
    }

    /* Now skip to the 'data' chunk. */
    while (1) {
        TRY_FREAD(id, 1, 4, infh);
        if (0 == memcmp(id, "data", 4)) {
            /* Total number of bytes of data (we don't care). */
            TRY_FREAD(&intval, 4, 1, infh);
            break;
        }
        else {
            char *spam;
            /* Some other stuff... */
            /* Number of bytes of ... whatever */
            TRY_FREAD(&intval, 4, 1, infh);
            SWAP_LE_32(&intval);
            /* Avoid seeking... */
            spam = malloc(intval);
            if (fread(spam, 1, intval, infh) != (size_t)intval) {
                E_ERROR_SYSTEM("%s: Failed to read %s chunk", file, id);
                rv = -1;
            }
            ckd_free(spam);
            if (rv == -1)
                goto error_out;
        }
    }

error_out:
    return rv;
}

int
ps_config_nistfile(ps_config_t *config, FILE *infh, const char *file)
{
    char hdr[1024];
    char *line, *c;
    int rv = 0;

    if (file == NULL)
        file = "(input filehandle)";

    TRY_FREAD(hdr, 1, 1024, infh);
    hdr[1023] = '\0';

    /* Roughly parse it to find the sampling rate and byte order
     * (don't bother with other stuff) */
    if ((line = strstr(hdr, "sample_rate")) == NULL) {
        E_ERROR("No sampling rate in NIST header!\n");
        rv = -1;
        goto error_out;
    }
    c = strchr(line, '\n');
    if (c) *c = '\0';
    c = strrchr(line, ' ');
    if (c == NULL) {
        E_ERROR("Could not find sampling rate!\n");
        rv = -1;
        goto error_out;
    }
    ++c;
    if (ps_config_int(config, "samprate") == 0)
        ps_config_set_int(config, "samprate", atoi(c));
    else if (ps_config_int(config, "samprate") != atoi(c)) {
        E_WARN("NIST file sampling rate %d != samprate %d\n",
               atoi(c), ps_config_int(config, "samprate"));
    }

    if (line + strlen(line) < hdr + 1023)
        line[strlen(line)] = ' ';
    if ((line = strstr(hdr, "sample_byte_format")) == NULL) {
        E_ERROR("No sample byte format in NIST header!\n");
        rv = -1;
        goto error_out;
    }
    c = strchr(line, '\n');
    if (c) *c = '\0';
    c = strrchr(line, ' ');
    if (c == NULL) {
        E_ERROR("Could not find sample byte order!\n");
        rv = -1;
        goto error_out;
    }
    ++c;
    if (0 == memcmp(c, "01", 2)) {
        ps_config_set_str(config, "input_endian", "little");
    }
    else if (0 == memcmp(c, "10", 2)) {
        ps_config_set_str(config, "input_endian", "big");
    }
    else {
        E_ERROR("Unknown byte order %s\n", c);
        rv = -1;
        goto error_out;
    }

error_out:
    return rv;
}
