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

#include "util/byteorder.h"
#include "soundfiles.h"

int
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

#define TRY_FREAD(ptr, size, nmemb, stream)                             \
    if (fread(ptr, size, nmemb, stream) != (nmemb)) {                   \
        E_ERROR_SYSTEM("Failed to read %d bytes", size * nmemb);       \
        goto error_out;                                                 \
    }

int
read_file_header(const char *file, FILE *infh, ps_config_t *config)
{
    char header[4];

    fseek(infh, 0, SEEK_SET);
    TRY_FREAD(header, 1, 4, infh);
    fseek(infh, 0, SEEK_SET);

    if (0 == memcmp(header, "RIFF", 4)) {
        E_INFO("%s, appears to be a WAV file\n", file);
        return read_riff_header(infh, config);
    }
    else if (0 == memcmp(header, "NIST", 4)) {
        E_INFO("%s appears to be a NIST SPHERE file\n", file);
        return read_nist_header(infh, config);
    }
    else {
        E_INFO("%s appears to be raw data\n", file);
        return 0;
    }
error_out:
    return -1;
}

int
read_riff_header(FILE *infh, ps_config_t *config)
{
    char id[4];
    int32 intval, header_len;
    int16 shortval;

    /* RIFF files are little-endian by definition. */
    ps_config_set_str(config, "input_endian", "little");

    /* Read in all the header chunks and etcetera. */
    TRY_FREAD(id, 1, 4, infh);
    /* Total file length (we don't care) */
    TRY_FREAD(&intval, 4, 1, infh);
    /* 'WAVE' */
    TRY_FREAD(id, 1, 4, infh);
    if (0 != memcmp(id, "WAVE", 4)) {
        E_ERROR("This is not a WAVE file\n");
        goto error_out;
    }
    /* 'fmt ' */
    TRY_FREAD(id, 1, 4, infh);
    if (0 != memcmp(id, "fmt ", 4)) {
        E_ERROR("Format chunk missing\n");
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
        E_ERROR("WAVE file is not in PCM format\n");
        goto error_out;
    }

    /* Number of channels. */
    TRY_FREAD(&shortval, 2, 1, infh);
    SWAP_LE_16(&shortval);
    if (shortval != 1) { /* PCM */
        E_ERROR("WAVE file is not single channel\n");
        goto error_out;
    }

    /* Sampling rate (finally!) */
    TRY_FREAD(&intval, 4, 1, infh);
    SWAP_LE_32(&intval);
    if (ps_config_int(config, "samprate") == 0)
        ps_config_set_int(config, "samprate", intval);
    else if (ps_config_int(config, "samprate") != intval) {
        E_WARN("WAVE file sampling rate %d != -samprate %d\n",
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
        E_ERROR("WAVE file is not 16-bit\n");
        goto error_out;
    }

    /* Any extra parameters. */
    if (header_len > 16)
        fseek(infh, header_len - 16, SEEK_CUR);

    /* Now skip to the 'data' chunk. */
    while (1) {
        TRY_FREAD(id, 1, 4, infh);
        if (0 == memcmp(id, "data", 4)) {
            /* Total number of bytes of data (we don't care). */
            TRY_FREAD(&intval, 4, 1, infh);
            break;
        }
        else {
            /* Some other stuff... */
            /* Number of bytes of ... whatever */
            TRY_FREAD(&intval, 4, 1, infh);
            SWAP_LE_32(&intval);
            fseek(infh, intval, SEEK_CUR);
        }
    }

    /* We are ready to rumble. */
    return 0;
error_out:
    return -1;
}

int
read_nist_header(FILE *infh, ps_config_t *config)
{
    char hdr[1024];
    char *line, *c;

    TRY_FREAD(hdr, 1, 1024, infh);
    hdr[1023] = '\0';

    /* Roughly parse it to find the sampling rate and byte order
     * (don't bother with other stuff) */
    if ((line = strstr(hdr, "sample_rate")) == NULL) {
        E_ERROR("No sampling rate in NIST header!\n");
        goto error_out;
    }
    c = strchr(line, '\n');
    if (c) *c = '\0';
    c = strrchr(line, ' ');
    if (c == NULL) {
        E_ERROR("Could not find sampling rate!\n");
        goto error_out;
    }
    ++c;
    if (ps_config_int(config, "samprate") == 0)
        ps_config_set_int(config, "samprate", atoi(c));
    else if (ps_config_int(config, "samprate") != atoi(c)) {
        E_WARN("NIST file sampling rate %d != -samprate %d\n",
               atoi(c), ps_config_int(config, "samprate"));
    }

    if (line + strlen(line) < hdr + 1023)
        line[strlen(line)] = ' ';
    if ((line = strstr(hdr, "sample_byte_format")) == NULL) {
        E_ERROR("No sample byte format in NIST header!\n");
        goto error_out;
    }
    c = strchr(line, '\n');
    if (c) *c = '\0';
    c = strrchr(line, ' ');
    if (c == NULL) {
        E_ERROR("Could not find sample byte order!\n");
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
        goto error_out;
    }

    /* We are ready to rumble. */
    return 0;
error_out:
    return -1;
}
