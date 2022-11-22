/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 1999-2001 Carnegie Mellon University.  All rights
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

#include <stdio.h>

#include <pocketsphinx.h>

#include "util/ckd_alloc.h"
#include "util/strfuncs.h"
#include "util/filename.h"
#include "util/byteorder.h"
#include "util/pio.h"
#include "lm/fsg_model.h"
#include "config_macro.h"
#include "pocketsphinx_internal.h"

/* Silvio Moioli: setbuf doesn't exist in Windows CE */
#if defined(_WIN32_WCE)
    void setbuf(FILE* file, char* buf){
    }
#endif

static const ps_arg_t ps_args_def[] = {
    POCKETSPHINX_OPTIONS,
    /* Various options specific to batch-mode processing. */
    /* Argument file. */
    { "argfile",
      ARG_STRING,
      NULL,
      "Argument file giving extra arguments." },
    /* Control file. */
    { "ctl",
      ARG_STRING,
      NULL,
      "Control file listing utterances to be processed" },
    { "ctloffset",
      ARG_INTEGER,
      "0",
      "No. of utterances at the beginning of -ctl file to be skipped" },
    { "ctlcount",
      ARG_INTEGER,
      "-1",
      "No. of utterances to be processed (after skipping -ctloffset entries)" },
    { "ctlincr",
      ARG_INTEGER,
      "1",
      "Do every Nth line in the control file" },
    { "mllrctl",
      ARG_STRING,
      NULL,
      "Control file listing MLLR transforms to use for each utterance" },
    { "mllrdir",
      ARG_STRING,
      NULL,
      "Base directory for MLLR transforms" },
    { "mllrext",
      ARG_STRING,
      NULL,
      "File extension for MLLR transforms (including leading dot)" },
    { "lmnamectl",
      ARG_STRING,
      NULL,
      "Control file listing LM name to use for each utterance" },
    { "fsgctl",
      ARG_STRING,
      NULL,
      "Control file listing FSG file to use for each utterance" },
    { "fsgdir",
      ARG_STRING,
      NULL,
      "Base directory for FSG files" },
    { "fsgext",
      ARG_STRING,
      NULL,
      "File extension for FSG files (including leading dot)" },
    { "alignctl",
      ARG_STRING,
      NULL,
      "Control file listing transcript files to force-align to utts" },
    { "aligndir",
      ARG_STRING,
      NULL,
      "Base directory for transcript files" },
    { "alignext",
      ARG_STRING,
      NULL,
      "File extension for transcript files (including leading dot)" },

    /* Input file types and locations. */
    { "adcin",
      ARG_BOOLEAN,
      "no",
      "Input is raw audio data" },
    { "adchdr",
      ARG_INTEGER,
      "0",
      "Size of audio file header in bytes (headers are ignored)" },
    { "senin",
      ARG_BOOLEAN,
      "no",
      "Input is senone score dump files" },
    { "cepdir",
      ARG_STRING,
      NULL,
      "Input files directory (prefixed to filespecs in control file)" },
    { "cepext",
      ARG_STRING,
      ".mfc",
      "Input files extension (suffixed to filespecs in control file)" },

    /* Output files. */
    { "hyp",
      ARG_STRING,
      NULL,
      "Recognition output file name" },
    { "hypseg",
      ARG_STRING,
      NULL,
      "Recognition output with segmentation file name" },
    { "ctm",
      ARG_STRING,
      NULL,
      "Recognition output in CTM file format (may require post-sorting)" },
    { "outlatdir",
      ARG_STRING,
      NULL,
      "Directory for dumping word lattices" },
    { "outlatfmt",
      ARG_STRING,
      "s3",
      "Format for dumping word lattices (s3 or htk)" },
    { "outlatext",
      ARG_STRING,
      ".lat",
      "Filename extension for dumping word lattices" },
    { "outlatbeam",
      ARG_FLOATING,
      "1e-5",
      "Minimum posterior probability for output lattice nodes" },
    { "build_outdirs",
      ARG_BOOLEAN,
      "yes",
      "Create missing subdirectories in output directory" },
    { "nbestdir",
      ARG_STRING,
      NULL,
      "Directory for writing N-best hypothesis lists" },
    { "nbestext",
      ARG_STRING,
      ".hyp",
      "Extension for N-best hypothesis list files" },
    { "nbest",
      ARG_INTEGER,
      "0",
      "Number of N-best hypotheses to write to -nbestdir (0 for no N-best)" },

    CMDLN_EMPTY_OPTION
};

static float32 **
read_mfc_file(FILE *infh, int sf, int ef, int *out_nfr, int ceplen)
{
    long flen;
    int32 nmfc, nfr;
    float32 *floats;
    float32 **mfcs;
    int swap, i;

    fseek(infh, 0, SEEK_END);
    flen = ftell(infh);
    fseek(infh, 0, SEEK_SET);
    if (fread(&nmfc, 4, 1, infh) != 1) {
        E_ERROR_SYSTEM("Failed to read 4 bytes from MFCC file");
        return NULL;
    }
    swap = 0;
    if (nmfc != flen / 4 - 1) {
        SWAP_INT32(&nmfc);
        swap = 1;
        if (nmfc != flen / 4 - 1) {
            E_ERROR("File length mismatch: 0x%x != 0x%x, maybe it's not MFCC file\n",
                    nmfc, flen / 4 - 1);
            return NULL;
        }
    }
    
    if (nmfc == 0) {
	E_ERROR("Empty mfcc file\n");
	return NULL;
    }

    fseek(infh, sf * 4 * ceplen, SEEK_CUR);
    if (ef == -1)
        ef = nmfc / ceplen;
    nfr = ef - sf;
    mfcs = ckd_calloc_2d(nfr, ceplen, sizeof(**mfcs));
    floats = (float32 *)mfcs[0];
    if (fread(floats, 4, nfr * ceplen, infh) != (size_t)nfr * ceplen) {
        E_ERROR_SYSTEM("Failed to read %d items from mfcfile");
        ckd_free_2d(mfcs);
        return NULL;
    }
    if (swap) {
        for (i = 0; i < nfr * ceplen; ++i)
            SWAP_FLOAT32(&floats[i]);
    }
    *out_nfr = nfr;
    return mfcs;
}

static int
process_mllrctl_line(ps_decoder_t *ps, cmd_ln_t *config, char const *file)
{
    char const *mllrdir, *mllrext;
    char *infile = NULL;
    ps_mllr_t *mllr;
    static char *lastfile;

    if (file == NULL)
        return 0;

    if (lastfile && 0 == strcmp(file, lastfile))
        return 0;

    ckd_free(lastfile);
    lastfile = ckd_salloc(file);

    mllrext = ps_config_str(config, "mllrext");
    if ((mllrdir = ps_config_str(config, "mllrdir")))
        infile = string_join(mllrdir, "/", file, 
                             mllrext ? mllrext : "", NULL);
    else if (mllrext)
        infile = string_join(file, mllrext, NULL);
    else
        infile = ckd_salloc(file);

    if ((mllr = ps_mllr_read(infile)) == NULL) {
        ckd_free(infile);
        return -1;
    }
    if (ps_update_mllr(ps, mllr) == NULL) {
        ps_mllr_free(mllr);
        ckd_free(infile);
        return -1;
    }

    E_INFO("Using MLLR: %s\n", file);
    ckd_free(infile);
    return 0;
}

static int
process_fsgctl_line(ps_decoder_t *ps, cmd_ln_t *config, char const *fname)
{
    fsg_model_t *fsg;
    int err;
    char *path = NULL;
    const char *fsgdir = ps_config_str(config, "fsgdir");
    const char *fsgext = ps_config_str(config, "fsgext");

    if (fname == NULL)
        return 0;

    if (fsgdir)
        path = string_join(fsgdir, "/", fname, fsgext ? fsgext : "", NULL);
    else if (fsgext)
        path = string_join(fname, fsgext, NULL);
    else
        path = ckd_salloc(fname);

    fsg = fsg_model_readfile(path, ps_get_logmath(ps),
                                          ps_config_float(config, "lw"));
    err = 0;
    if (!fsg) {
        err = -1;
        goto error_out;
    }
    if (ps_add_fsg(ps, fname, fsg)) {
        err = -1;
        goto error_out;
    }

    E_INFO("Using FSG: %s\n", fname);
    if (ps_activate_search(ps, fname))
        err = -1;

error_out:
    fsg_model_free(fsg);
    ckd_free(path);

    return err;
}

static int
process_lmnamectl_line(ps_decoder_t *ps, cmd_ln_t *config, char const *lmname)
{
    (void)config;
    if (!lmname)
        return 0;

    E_INFO("Using language model: %s\n", lmname);
    if (ps_activate_search(ps, lmname)) {
        E_ERROR("No such language model: %s\n", lmname);
        return -1;
    }
    return 0;
}

static int
process_alignctl_line(ps_decoder_t *ps, cmd_ln_t *config, char const *fname)
{
    int err;
    char *path = NULL;
    const char *aligndir = ps_config_str(config, "aligndir");
    const char *alignext = ps_config_str(config, "alignext");
    char *text = NULL;
    size_t nchars, nread;
    FILE *fh;

    if (fname == NULL)
        return 0;

    if (aligndir)
        path = string_join(aligndir, "/", fname, alignext ? alignext : "", NULL);
    else if (alignext)
        path = string_join(fname, alignext, NULL);
    else
        path = ckd_salloc(fname);

    if ((fh = fopen(path, "r")) == NULL) {
        E_ERROR_SYSTEM("Failed to open transcript file %s", path);
        err = -1;
        goto error_out;
    }
    fseek(fh, 0, SEEK_END);
    nchars = ftell(fh);
    text = ckd_calloc(nchars + 1, 1);
    fseek(fh, 0, SEEK_SET);
    if ((nread = fread(text, 1, nchars, fh)) != nchars) {
        E_ERROR_SYSTEM("Failed to fully read transcript file %s", path);
        err = -1;
        goto error_out;
    }
    if ((err = fclose(fh)) != 0) {
        E_ERROR_SYSTEM("Failed to close transcript file %s", path);
        goto error_out;
    }
    if (ps_set_align_text(ps, text)) {
        err = -1;
        goto error_out;
    }
    E_INFO("Force-aligning with transcript from: %s\n", fname);

error_out:
    ckd_free(path);
    ckd_free(text);

    return err;
}

static int
build_outdir_one(cmd_ln_t *config, char const *arg, char const *uttpath)
{
    char const *dir;

    if ((dir = ps_config_str(config, arg)) != NULL) {
        char *dirname = string_join(dir, "/", uttpath, NULL);
        build_directory(dirname);
        ckd_free(dirname);
    }
    return 0;
}

static int
build_outdirs(cmd_ln_t *config, char const *uttid)
{
    char *uttpath = ckd_salloc(uttid);

    path2dirname(uttid, uttpath);
    build_outdir_one(config, "outlatdir", uttpath);
    build_outdir_one(config, "mfclogdir", uttpath);
    build_outdir_one(config, "rawlogdir", uttpath);
    build_outdir_one(config, "senlogdir", uttpath);
    build_outdir_one(config, "nbestdir", uttpath);
    ckd_free(uttpath);

    return 0;
}

static int
process_ctl_line(ps_decoder_t *ps, cmd_ln_t *config,
                 char const *file, char const *uttid, int32 sf, int32 ef)
{
    FILE *infh;
    char const *cepdir, *cepext;
    char *infile;

    if (ef != -1 && ef < sf) {
        E_ERROR("End frame %d is < start frame %d\n", ef, sf);
        return -1;
    }
    
    cepdir = ps_config_str(config, "cepdir");
    cepext = ps_config_str(config, "cepext");

    /* Build input filename. */
    infile = string_join(cepdir ? cepdir : "",
                         "/", file,
                         cepext ? cepext : "", NULL);
    if (uttid == NULL) uttid = file;

    if ((infh = fopen(infile, "rb")) == NULL) {
        E_ERROR_SYSTEM("Failed to open %s", infile);
        ckd_free(infile);
        return -1;
    }
    /* Build output directories. */
    if (ps_config_bool(config, "build_outdirs"))
        build_outdirs(config, uttid);

    if (ps_config_bool(config, "senin")) {
        /* start and end frames not supported. */
        ps_decode_senscr(ps, infh);
    }
    else if (ps_config_bool(config, "adcin")) {
        
        if (ef != -1) {
            ef = (int32)((ef - sf)
                         * ((double)ps_config_int(config, "samprate")
                            / ps_config_int(config, "frate"))
                         + ((double)ps_config_int(config, "samprate")
                            * ps_config_float(config, "wlen")));
        }
        sf = (int32)(sf
                     * ((double)ps_config_int(config, "samprate")
                        / ps_config_int(config, "frate")));
        fseek(infh, ps_config_int(config, "adchdr") + sf * sizeof(int16), SEEK_SET);
        ps_decode_raw(ps, infh, ef);
    }
    else {
        float32 **mfcs;
        int nfr;

        if (NULL == (mfcs = read_mfc_file(infh, sf, ef, &nfr,
                                          ps_config_int(config, "ceplen")))) {
            E_ERROR("Failed to read MFCC from the file '%s'\n", infile);
            fclose(infh);
            ckd_free(infile);
            return -1;
        }
        ps_start_utt(ps);
        ps_process_cep(ps, mfcs, nfr, FALSE, TRUE);
        ps_end_utt(ps);
        ckd_free_2d(mfcs);
    }
    fclose(infh);
    ckd_free(infile);
    return 0;
}

static int
write_lattice(ps_decoder_t *ps, char const *latdir, char const *uttid)
{
    ps_lattice_t *lat;
    logmath_t *lmath;
    cmd_ln_t *config;
    char *outfile;
    int32 beam;

    if ((lat = ps_get_lattice(ps)) == NULL) {
        E_ERROR("Failed to obtain word lattice for utterance %s\n", uttid);
        return -1;
    }
    config = ps_get_config(ps);
    outfile = string_join(latdir, "/", uttid,
                          ps_config_str(config, "outlatext"), NULL);
    /* Prune lattice. */
    lmath = ps_get_logmath(ps);
    beam = logmath_log(lmath, ps_config_float(config, "outlatbeam"));
    ps_lattice_posterior_prune(lat, beam);
    if (0 == strcmp("htk", ps_config_str(config, "outlatfmt"))) {
        if (ps_lattice_write_htk(lat, outfile) < 0) {
            E_ERROR("Failed to write lattice to %s\n", outfile);
            return -1;
        }
    }
    else {
        if (ps_lattice_write(lat, outfile) < 0) {
            E_ERROR("Failed to write lattice to %s\n", outfile);
            return -1;
        }
    }
    return 0;
}

static int
write_nbest(ps_decoder_t *ps, char const *nbestdir, char const *uttid)
{
    cmd_ln_t *config;
    char *outfile;
    FILE *fh;
    ps_nbest_t *nbest;
    int32 i, n, score;
    const char* hyp;

    config = ps_get_config(ps);
    outfile = string_join(nbestdir, "/", uttid,
                          ps_config_str(config, "nbestext"), NULL);
    n = ps_config_int(config, "nbest");
    fh = fopen(outfile, "w");
    if (fh == NULL) {
        E_ERROR_SYSTEM("Failed to write a lattice to file %s\n", outfile);
        return -1;
    }
    nbest = ps_nbest(ps);
    for (i = 0; i < n && nbest && (nbest = ps_nbest_next(nbest)); i++) {
        hyp = ps_nbest_hyp(nbest, &score);
        fprintf(fh, "%s %d\n", hyp, score);        
    }
    if (nbest)
	ps_nbest_free(nbest);
    fclose(fh);

    return 0;
}

static int
write_hypseg(FILE *fh, ps_decoder_t *ps, char const *uttid)
{
    int32 ascr, lscr, sf, ef;
    ps_seg_t *itor = ps_seg_iter(ps);

    /* Accumulate language model scores. */
    lscr = 0; ascr = 0;
    while (itor) {
        int32 wlascr, wlscr;
        ps_seg_prob(itor, &wlascr, &wlscr, NULL);
        lscr += wlscr;
        ascr += wlascr;
        itor = ps_seg_next(itor);
    }
    fprintf(fh, "%s S %d T %d A %d L %d", uttid,
            0, /* "scaling factor" which is mostly useless anyway */
            ascr + lscr, ascr, lscr);
    /* Now print out words. */
    itor = ps_seg_iter(ps);
    while (itor) {
        char const *w = ps_seg_word(itor);
        int32 ascr, wlscr;

        ps_seg_prob(itor, &ascr, &wlscr, NULL);
        ps_seg_frames(itor, &sf, &ef);
        fprintf(fh, " %d %d %d %s", sf, ascr, wlscr, w);
        itor = ps_seg_next(itor);
    }
    fprintf(fh, " %d\n", ef);

    return 0;
}

static int
write_ctm(FILE *fh, ps_decoder_t *ps, ps_seg_t *itor, char const *uttid, int32 frate)
{
    logmath_t *lmath = ps_get_logmath(ps);
    char *dupid, *show, *channel, *c;
    double ustart = 0.0;

    /* We have semi-standardized on comma-separated uttids which
     * correspond to the fields of the STM file.  So if there's a
     * comma in the uttid, take the first two fields as show and
     * channel, and also try to find the start time. */
    show = dupid = ckd_salloc(uttid ? uttid : "(null)");
    if ((c = strchr(dupid, ',')) != NULL) {
        *c++ = '\0';
        channel = c;
        if ((c = strchr(c, ',')) != NULL) {
            *c++ = '\0';
            if ((c = strchr(c, ',')) != NULL) {
                ustart = atof_c(c + 1);
            }
        }
    }
    else {
        channel = NULL;
    }

    while (itor) {
        int32 prob, sf, ef, wid;
        char const *w;

        /* Skip things that aren't "real words" (FIXME: currently
         * requires s3kr3t h34d3rz...) */
        w = ps_seg_word(itor);
        wid = dict_wordid(ps->dict, w);
        if (wid >= 0 && dict_real_word(ps->dict, wid)) {
            prob = ps_seg_prob(itor, NULL, NULL, NULL);
            ps_seg_frames(itor, &sf, &ef);
        
            fprintf(fh, "%s %s %.2f %.2f %s %.3f\n",
                    show,
                    channel ? channel : "1",
                    ustart + (double)sf / frate,
                    (double)(ef - sf) / frate,
                    /* FIXME: More s3kr3tz */
                    dict_basestr(ps->dict, wid),
                    logmath_exp(lmath, prob));
        }
        itor = ps_seg_next(itor);
    }
    ckd_free(dupid);

    return 0;
}

static char *
get_align_hyp(ps_decoder_t *ps, int *out_score)
{
    ps_seg_t *itor;
    char *out, *ptr;
    int len;
    
    if (ps_get_hyp(ps, out_score) == NULL)
        return ckd_salloc("");
    len = 0;
    for (itor = ps_seg_iter(ps); itor; itor = ps_seg_next(itor)) {
        /* FIXME: Need to handle tag transitions somehow... */
        if (itor->wid < 0)
            continue;
        len += strlen(itor->text);
        len++;
    }
    if (len == 0)
        return ckd_salloc("");
    ptr = out = ckd_malloc(len);
    for (itor = ps_seg_iter(ps); itor; itor = ps_seg_next(itor)) {
        if (itor->wid < 0)
            continue;
        len = strlen(itor->text);
        memcpy(ptr, itor->text, len);
        ptr += len;
        *ptr++ = ' ';
    }
    *--ptr = '\0';
    return out;
}

static void
process_ctl(ps_decoder_t *ps, cmd_ln_t *config, FILE *ctlfh)
{
    int32 ctloffset, ctlcount, ctlincr;
    int32 i;
    char *line;
    size_t len;
    FILE *hypfh = NULL, *hypsegfh = NULL, *ctmfh = NULL;
    FILE *mllrfh = NULL, *lmfh = NULL, *fsgfh = NULL, *alignfh = NULL;
    double n_speech, n_cpu, n_wall;
    char const *outlatdir;
    char const *nbestdir;
    char const *str;
    int frate;

    ctloffset = ps_config_int(config, "ctloffset");
    ctlcount = ps_config_int(config, "ctlcount");
    ctlincr = ps_config_int(config, "ctlincr");
    outlatdir = ps_config_str(config, "outlatdir");
    nbestdir = ps_config_str(config, "nbestdir");
    frate = ps_config_int(config, "frate");

    if ((str = ps_config_str(config, "mllrctl"))) {
        mllrfh = fopen(str, "r");
        if (mllrfh == NULL) {
            E_ERROR_SYSTEM("Failed to open MLLR control file file %s", str);
            goto done;
        }
    }
    if ((str = ps_config_str(config, "fsgctl"))) {
        fsgfh = fopen(str, "r");
        if (fsgfh == NULL) {
            E_ERROR_SYSTEM("Failed to open FSG control file file %s", str);
            goto done;
        }
    }
    if ((str = ps_config_str(config, "alignctl"))) {
        alignfh = fopen(str, "r");
        if (alignfh == NULL) {
            E_ERROR_SYSTEM("Failed to open alignment control file file %s", str);
            goto done;
        }
    }
    if ((str = ps_config_str(config, "lmnamectl"))) {
        lmfh = fopen(str, "r");
        if (lmfh == NULL) {
            E_ERROR_SYSTEM("Failed to open LM name control file file %s", str);
            goto done;
        }
    }
    if ((str = ps_config_str(config, "hyp"))) {
        hypfh = fopen(str, "w");
        if (hypfh == NULL) {
            E_ERROR_SYSTEM("Failed to open hypothesis file %s for writing", str);
            goto done;
        }
        setbuf(hypfh, NULL);
    }
    if ((str = ps_config_str(config, "hypseg"))) {
        hypsegfh = fopen(str, "w");
        if (hypsegfh == NULL) {
            E_ERROR_SYSTEM("Failed to open hypothesis file %s for writing", str);
            goto done;
        }
        setbuf(hypsegfh, NULL);
    }
    if ((str = ps_config_str(config, "ctm"))) {
        ctmfh = fopen(str, "w");
        if (ctmfh == NULL) {
            E_ERROR_SYSTEM("Failed to open hypothesis file %s for writing", str);
            goto done;
        }
        setbuf(ctmfh, NULL);
    }

    i = 0;
    while ((line = fread_line(ctlfh, &len))) {
        char *wptr[4];
        int32 nf, sf, ef;
        char *mllrline = NULL, *lmline = NULL, *fsgline = NULL;
        char *fsgfile = NULL, *lmname = NULL, *mllrfile = NULL;
        char *alignline = NULL, *alignfile = NULL;

        if (mllrfh) {
            mllrline = fread_line(mllrfh, &len);
            if (mllrline == NULL) {
                E_ERROR("File size mismatch between control and MLLR control\n");
                ckd_free(line);
                ckd_free(mllrline);
                goto done;
            }
            mllrfile = string_trim(mllrline, STRING_BOTH);
        }
        if (lmfh) {
            lmline = fread_line(lmfh, &len);
            if (lmline == NULL) {
                E_ERROR("File size mismatch between control and LM control\n");
                ckd_free(line);
                ckd_free(lmline);
                goto done;
            }
            lmname = string_trim(lmline, STRING_BOTH);
        }
        if (fsgfh) {
            fsgline = fread_line(fsgfh, &len);
            if (fsgline == NULL) {
                E_ERROR("File size mismatch between control and FSG control\n");
                ckd_free(line);
                ckd_free(fsgline);
                goto done;
            }
            fsgfile = string_trim(fsgline, STRING_BOTH);
        }
        if (alignfh) {
            alignline = fread_line(alignfh, &len);
            if (alignline == NULL) {
                E_ERROR("File size mismatch between control and align control\n");
                ckd_free(line);
                ckd_free(alignline);
                goto done;
            }
            alignfile = string_trim(alignline, STRING_BOTH);
        }

        if (i < ctloffset) {
            i += ctlincr;
            goto nextline;
        }
        if (ctlcount != -1 && i >= ctloffset + ctlcount) {
            goto nextline;
        }

        sf = 0;
        ef = -1;
        nf = str2words(line, wptr, 4);
        if (nf == 0) {
            /* Do nothing. */
        }
        else if (nf < 0) {
            E_ERROR("Unexpected extra data in control file at line %d\n", i);
        }
        else {
            char const *hyp = NULL, *file, *uttid;
            int32 score;

            file = wptr[0];
            if (nf > 1)
                sf = atoi(wptr[1]);
            if (nf > 2)
                ef = atoi(wptr[2]);
            if (nf > 3)
                uttid = wptr[3];
            else
        	uttid = file;

            E_INFO("Decoding '%s'\n", uttid);

            /* Do actual decoding. */
            if(process_mllrctl_line(ps, config, mllrfile) < 0)
                continue;
            if(process_lmnamectl_line(ps, config, lmname) < 0)
                continue;
            if(process_fsgctl_line(ps, config, fsgfile) < 0)
                continue;
            if(process_alignctl_line(ps, config, alignfile) < 0)
                continue;
            if(process_ctl_line(ps, config, file, uttid, sf, ef) < 0)
                continue;
            /* Special case for force-alignment: report silences and
             * alternate pronunciations.  This isn't done
             * automatically because Reasons.  (API consistency but
             * also because force-alignment is really secretly FSG
             * search at the moment). */
            if (hypfh) {
                if (alignfile) {
                    char *align_hyp = get_align_hyp(ps, &score);
                    fprintf(hypfh, "%s (%s %d)\n", align_hyp, uttid, score);
                    ckd_free(align_hyp);
                }
                else {
                    hyp = ps_get_hyp(ps, &score);
                    fprintf(hypfh, "%s (%s %d)\n", hyp ? hyp : "", uttid, score);
                }
            }
            if (hypsegfh) {
                write_hypseg(hypsegfh, ps, uttid);
            }
            if (ctmfh) {
                ps_seg_t *itor = ps_seg_iter(ps);
                write_ctm(ctmfh, ps, itor, uttid, frate);
            }
            if (outlatdir) {
                write_lattice(ps, outlatdir, uttid);
            }
            if (nbestdir) {
                write_nbest(ps, nbestdir, uttid);
            }
            ps_get_utt_time(ps, &n_speech, &n_cpu, &n_wall);
            E_INFO("%s: %.2f seconds speech, %.2f seconds CPU, %.2f seconds wall\n",
                   uttid, n_speech, n_cpu, n_wall);
            E_INFO("%s: %.2f xRT (CPU), %.2f xRT (elapsed)\n",
                   uttid, n_cpu / n_speech, n_wall / n_speech);
            /* help make the logfile somewhat less opaque (air) */
            E_INFO_NOFN("%s (%s %d)\n", hyp ? hyp : "", uttid, score); 
            E_INFO_NOFN("%s done --------------------------------------\n", uttid);
        }
        i += ctlincr;
    nextline:
        ckd_free(alignline);
        ckd_free(mllrline);
        ckd_free(fsgline);
        ckd_free(lmline);
        ckd_free(line);
    }

    ps_get_all_time(ps, &n_speech, &n_cpu, &n_wall);
    E_INFO("TOTAL %.2f seconds speech, %.2f seconds CPU, %.2f seconds wall\n",
           n_speech, n_cpu, n_wall);
    E_INFO("AVERAGE %.2f xRT (CPU), %.2f xRT (elapsed)\n",
           n_cpu / n_speech, n_wall / n_speech);

done:
    if (hypfh)
        fclose(hypfh);
    if (hypsegfh)
        fclose(hypsegfh);
    if (ctmfh)
        fclose(ctmfh);
    if (alignfh)
        fclose(alignfh);
}

int
main(int32 argc, char *argv[])
{
    ps_decoder_t *ps;
    cmd_ln_t *config;
    char const *ctl;
    FILE *ctlfh;

    config = cmd_ln_parse_r(NULL, ps_args_def, argc, argv, TRUE);

    /* Handle argument file as -argfile. */
    if (config && (ctl = ps_config_str(config, "argfile")) != NULL) {
        config = cmd_ln_parse_file_r(config, ps_args_def, ctl, FALSE);
    }
    
    if (config == NULL) {
        /* This probably just means that we got no arguments. */
        err_set_loglevel(ERR_INFO);
        cmd_ln_log_help_r(NULL, ps_args_def);
        return 1;
    }

    if ((ctl = ps_config_str(config, "ctl")) == NULL) {
        E_FATAL("-ctl argument not present, nothing to do in batch mode!\n");
    }
    if ((ctlfh = fopen(ctl, "r")) == NULL) {
        E_FATAL_SYSTEM("Failed to open control file '%s'", ctl);
    }

    ps_default_search_args(config);
    if (!(ps = ps_init(config))) {
        ps_config_free(config);
        fclose(ctlfh);
        E_FATAL("PocketSphinx decoder init failed\n");
    }

    process_ctl(ps, config, ctlfh);

    fclose(ctlfh);
    ps_free(ps);
    ps_config_free(config);
    return 0;
}

#if defined(_WIN32_WCE)
#pragma comment(linker,"/entry:mainWCRTStartup")
#include <windows.h>

/* Windows Mobile has the Unicode main only */
int wmain(int32 argc, wchar_t *wargv[]) {
    char** argv;
    size_t wlen;
    size_t len;
    int i;

    argv = malloc(argc*sizeof(char*));
    for (i = 0; i < argc; i++){
        wlen = lstrlenW(wargv[i]);
        len = wcstombs(NULL, wargv[i], wlen);
        argv[i] = malloc(len+1);
        wcstombs(argv[i], wargv[i], wlen);
    }

    /* assuming ASCII parameters */
    return main(argc, argv);
}
#endif

