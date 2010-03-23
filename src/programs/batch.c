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

/* System headers. */
#include <stdio.h>

/* SphinxBase headers. */
#include <pio.h>
#include <err.h>
#include <strfuncs.h>
#include <filename.h>
#include <byteorder.h>

/* PocketSphinx headers. */
#include <pocketsphinx.h>

/* S3kr3t headerz. */
#include "pocketsphinx_internal.h"

/* Silvio Moioli: setbuf doesn't exist in Windows CE */
#if defined(_WIN32_WCE)
	void setbuf(FILE* file, char* buf){
	}
#endif

static const arg_t ps_args_def[] = {
    POCKETSPHINX_OPTIONS,
    /* Various options specific to batch-mode processing. */
    /* Argument file. */
    { "-argfile",
      ARG_STRING,
      NULL,
      "Argument file giving extra arguments." },
    /* Control file. */
    { "-ctl",
      ARG_STRING,
      NULL,
      "Control file listing utterances to be processed" },
    { "-ctloffset",
      ARG_INT32,
      "0",
      "No. of utterances at the beginning of -ctl file to be skipped" },
    { "-ctlcount",
      ARG_INT32,
      "-1",
      "No. of utterances to be processed (after skipping -ctloffset entries)" },
    { "-ctlincr",
      ARG_INT32,
      "1",
      "Do every Nth line in the control file" },
    { "-mllrctl",
      ARG_STRING,
      NULL,
      "Control file listing MLLR transforms to use for each file" },
    { "-mllrdir",
      ARG_STRING,
      NULL,
      "Base directory for MLLR transforms" },
    { "-lmnamectl",
      ARG_STRING,
      NULL,
      "Control file listing LM name to use for each file" },

    /* Input file types and locations. */
    { "-adcin",
      ARG_BOOLEAN,
      "no",
      "Input is raw audio data" },
    { "-adchdr",
      ARG_INT32,
      "0",
      "Size of audio file header in bytes (headers are ignored)" },
    { "-cepdir",
      ARG_STRING,
      NULL,
      "Input files directory (prefixed to filespecs in control file)" },
    { "-cepext",
      ARG_STRING,
      ".mfc",
      "Input files extension (suffixed to filespecs in control file)" },

    /* Output files. */
    { "-hyp",
      ARG_STRING,
      NULL,
      "Recognition output file name" },
    { "-hypseg",
      ARG_STRING,
      NULL,
      "Recognition output with segmentation file name" },
    { "-ctm",
      ARG_STRING,
      NULL,
      "Recognition output in CTM file format (may require post-sorting)" },
    { "-outlatdir",
      ARG_STRING,
      NULL,
      "Directory for dumping word lattices" },
    { "-build_outdirs",
      ARG_BOOLEAN,
      "yes",
      "Create missing subdirectories in output directory" },
    { "-nbestdir",
      ARG_STRING,
      NULL,
      "Directory for writing N-best hypothesis lists" },
    { "-nbestext",
      ARG_STRING,
      ".hyp",
      "Extension for N-best hypothesis list files" },
    { "-nbest",
      ARG_INT32,
      "0",
      "Number of N-best hypotheses to write to -nbestdir (0 for no N-best)" },

    CMDLN_EMPTY_OPTION
};

static mfcc_t **
read_mfc_file(FILE *infh, int sf, int ef, int *out_nfr, int ceplen)
{
    long flen;
    int32 nmfc, nfr;
    float32 *floats;
    mfcc_t **mfcs;
    int swap, i;

    fseek(infh, 0, SEEK_END);
    flen = ftell(infh);
    fseek(infh, 0, SEEK_SET);
    if (fread(&nmfc, 4, 1, infh) != 1) {
        E_ERROR_SYSTEM("Failed to read 4 bytes from MFCC file");
        fclose(infh);
        return NULL;
    }
    swap = 0;
    if (nmfc != flen / 4 - 1) {
        SWAP_INT32(&nmfc);
        swap = 1;
        if (nmfc != flen / 4 - 1) {
            E_ERROR("File length mismatch: 0x%x != 0x%x\n",
                    nmfc, flen / 4 - 1);
            fclose(infh);
            return NULL;
        }
    }

    fseek(infh, sf * 4 * ceplen, SEEK_CUR);
    if (ef == -1)
        ef = nmfc / ceplen;
    nfr = ef - sf;
    mfcs = ckd_calloc_2d(nfr, ceplen, sizeof(**mfcs));
    floats = (float32 *)mfcs[0];
    if (fread(floats, 4, nfr * ceplen, infh) != nfr * ceplen) {
        E_ERROR_SYSTEM("Failed to read %d items from mfcfile");
        fclose(infh);
        ckd_free_2d(mfcs);
        return NULL;
    }
    if (swap) {
        for (i = 0; i < nfr * ceplen; ++i)
            SWAP_FLOAT32(&floats[i]);
    }
#ifdef FIXED_POINT
    for (i = 0; i < nfr * ceplen; ++i)
        mfcs[0][i] = FLOAT2MFCC(floats[i]);
#endif
    *out_nfr = nfr;
    return mfcs;
}

static int
process_mllrctl_line(ps_decoder_t *ps, cmd_ln_t *config, char const *file)
{
    char const *mllrdir;
    char *infile = NULL;
    ps_mllr_t *mllr;
    static char *lastfile;

    if (file == NULL)
        return 0;

    if (lastfile && 0 == strcmp(file, lastfile))
        return 0;

    ckd_free(lastfile);
    lastfile = ckd_salloc(file);

    if ((mllrdir = cmd_ln_str_r(config, "-mllrdir")))
        infile = string_join(mllrdir, "/", file, NULL);
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

    ckd_free(infile);
    return 0;
}

static int
process_lmnamectl_line(ps_decoder_t *ps, cmd_ln_t *config, char const *lmname)
{
    ngram_model_t *lmset = ps_get_lmset(ps);

    if (lmname == NULL)
        return 0;
    if (ngram_model_set_select(lmset, lmname) == NULL) {
        E_ERROR("No such language model: %s\n", lmname);
        return -1;
    }
    ps_update_lmset(ps, lmset);
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
    
    cepdir = cmd_ln_str_r(config, "-cepdir");
    cepext = cmd_ln_str_r(config, "-cepext");

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
    if (cmd_ln_boolean_r(config, "-adcin")) {
        
        if (ef != -1) {
            ef = (int32)((ef - sf)
                         * (cmd_ln_float32_r(config, "-samprate")
                            / cmd_ln_int32_r(config, "-frate"))
                         + (cmd_ln_float32_r(config, "-samprate")
                            * cmd_ln_float32_r(config, "-wlen")));
        }
        sf = (int32)(sf
                     * (cmd_ln_float32_r(config, "-samprate")
                        / cmd_ln_int32_r(config, "-frate")));
        fseek(infh, cmd_ln_int32_r(config, "-adchdr") + sf * sizeof(int16), SEEK_SET);
        ps_decode_raw(ps, infh, uttid, ef);
    }
    else {
        mfcc_t **mfcs;
        int nfr;

        if (NULL == (mfcs = read_mfc_file(infh, sf, ef, &nfr,
                                          cmd_ln_int32_r(config, "-ceplen")))) {
            fclose(infh);
            ckd_free(infile);
            return -1;
        }
        ps_start_utt(ps, uttid);
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
    char *outfile;

    if ((lat = ps_get_lattice(ps)) == NULL) {
        E_ERROR("Failed to obtain word lattice for utterance %s\n", uttid);
        return -1;
    }
    outfile = string_join(latdir, "/", uttid, ".lat", NULL);
    /* Build output directory structure if possible/requested (it is
     * by default). */
    if (cmd_ln_boolean_r(ps_get_config(ps), "-build_outdirs")) {
        char *dirname = ckd_salloc(outfile);
        path2dirname(outfile, dirname);
        build_directory(dirname);
        ckd_free(dirname);
    }
    if (ps_lattice_write(lat, outfile) < 0) {
        E_ERROR("Failed to write lattice to %s\n", outfile);
        return -1;
    }
    return 0;
}

static int
write_hypseg(FILE *fh, ps_decoder_t *ps, char const *uttid)
{
    int32 score, lscr, sf, ef;
    ps_seg_t *itor = ps_seg_iter(ps, &score);
    ngram_model_t *lm = ps_get_lmset(ps);

    /* Accumulate language model scores. */
    lscr = 0;
    while (itor) {
        int32 ascr, wlscr;
        ps_seg_prob(itor, &ascr, &wlscr, NULL);
        lscr += wlscr;
        itor = ps_seg_next(itor);
    }
    fprintf(fh, "%s S %d T %d A %d L %d", uttid,
            0, /* "scaling factor" which is mostly useless anyway */
            score, score - lscr, lscr);
    /* Now print out words. */
    itor = ps_seg_iter(ps, &score);
    while (itor) {
        char const *w = ps_seg_word(itor);
        int32 ascr, wlscr;

        ps_seg_prob(itor, &ascr, &wlscr, NULL);
        ps_seg_frames(itor, &sf, &ef);
        fprintf(fh, " %d %d %d %s",
                sf, ascr,
                /* FIXME: This is inconsistent with the total lm
                   score, but that's the way it's done in S3... */
                lm ? ngram_score_to_prob(lm, wlscr) : wlscr, w);
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
    show = dupid = ckd_salloc(uttid);
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
        if (dict_real_word(ps->dict, wid)) {
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

static void
process_ctl(ps_decoder_t *ps, cmd_ln_t *config, FILE *ctlfh)
{
    int32 ctloffset, ctlcount, ctlincr;
    int32 i;
    char *line;
    size_t len;
    FILE *hypfh = NULL, *hypsegfh = NULL, *ctmfh = NULL;
    FILE *mllrfh = NULL, *lmfh = NULL;
    double n_speech, n_cpu, n_wall;
    char const *outlatdir;
    char const *nbestdir;
    char const *str;
    int frate;

    ctloffset = cmd_ln_int32_r(config, "-ctloffset");
    ctlcount = cmd_ln_int32_r(config, "-ctlcount");
    ctlincr = cmd_ln_int32_r(config, "-ctlincr");
    outlatdir = cmd_ln_str_r(config, "-outlatdir");
    nbestdir = cmd_ln_str_r(config, "-nbestdir");
    frate = cmd_ln_int32_r(config, "-frate");

    if ((str = cmd_ln_str_r(config, "-mllrctl"))) {
        mllrfh = fopen(str, "r");
        if (mllrfh == NULL) {
            E_ERROR_SYSTEM("Failed to open MLLR control file file %s", str);
            goto done;
        }
    }
    if ((str = cmd_ln_str_r(config, "-lmnamectl"))) {
        lmfh = fopen(str, "r");
        if (lmfh == NULL) {
            E_ERROR_SYSTEM("Failed to open LM name control file file %s", str);
            goto done;
        }
    }
    if ((str = cmd_ln_str_r(config, "-hyp"))) {
        hypfh = fopen(str, "w");
        if (hypfh == NULL) {
            E_ERROR_SYSTEM("Failed to open hypothesis file %s for writing", str);
            goto done;
        }
        setbuf(hypfh, NULL);
    }
    if ((str = cmd_ln_str_r(config, "-hypseg"))) {
        hypsegfh = fopen(str, "w");
        if (hypsegfh == NULL) {
            E_ERROR_SYSTEM("Failed to open hypothesis file %s for writing", str);
            goto done;
        }
        setbuf(hypsegfh, NULL);
    }
    if ((str = cmd_ln_str_r(config, "-ctm"))) {
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
        char *mllrline = NULL, *lmname = NULL, *mllrfile = NULL;

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
            lmname = fread_line(lmfh, &len);
            if (lmname == NULL) {
                E_ERROR("File size mismatch between control and LM control\n");
                ckd_free(line);
                ckd_free(lmname);
                goto done;
            }
            lmname = string_trim(lmname, STRING_BOTH);
        }

        if (i < ctloffset) {
            i += ctlincr;
            ckd_free(mllrline);
            ckd_free(lmname);
            ckd_free(line);
            continue;
        }
        if (ctlcount != -1 && i >= ctloffset + ctlcount) {
            ckd_free(mllrline);
            ckd_free(lmname);
            ckd_free(line);
            break;
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
            char const *hyp, *file, *uttid;
            int32 score;

            file = wptr[0];
            uttid = NULL;
            if (nf > 1)
                sf = atoi(wptr[1]);
            if (nf > 2)
                ef = atoi(wptr[2]);
            if (nf > 3)
                uttid = wptr[3];
            /* Do actual decoding. */
            process_mllrctl_line(ps, config, mllrfile);
            process_lmnamectl_line(ps, config, lmname);
            process_ctl_line(ps, config, file, uttid, sf, ef);
            hyp = ps_get_hyp(ps, &score, &uttid);
            
            /* Write out results and such. */
            if (hypfh) {
                fprintf(hypfh, "%s (%s %d)\n", hyp ? hyp : "", uttid, score);
            }
            if (hypsegfh) {
                write_hypseg(hypsegfh, ps, uttid);
            }
            if (ctmfh) {
                ps_seg_t *itor = ps_seg_iter(ps, &score);
                write_ctm(ctmfh, ps, itor, uttid, frate);
            }
            if (outlatdir) {
                write_lattice(ps, outlatdir, uttid);
            }
            if (nbestdir) {
            }
            ps_get_utt_time(ps, &n_speech, &n_cpu, &n_wall);
            E_INFO("%s: %.2f seconds speech, %.2f seconds CPU, %.2f seconds wall\n",
                   uttid, n_speech, n_cpu, n_wall);
            E_INFO("%s: %.2f xRT (CPU), %.2f xRT (elapsed)\n",
                   uttid, n_cpu / n_speech, n_wall / n_speech);
        }
        ckd_free(mllrline);
        ckd_free(lmname);
        ckd_free(line);
        i += ctlincr;
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
}

int
main(int32 argc, char *argv[])
{
    ps_decoder_t *ps;
    cmd_ln_t *config;
    char const *ctl;
    FILE *ctlfh;

    /* Handle argument file as only argument. */
    if (argc == 2) {
        config = cmd_ln_parse_file_r(NULL, ps_args_def, argv[1], TRUE);
    }
    else {
        config = cmd_ln_parse_r(NULL, ps_args_def, argc, argv, TRUE);
    }
    /* Handle argument file as -argfile. */
    if (config && (ctl = cmd_ln_str_r(config, "-argfile")) != NULL) {
        config = cmd_ln_parse_file_r(config, ps_args_def, ctl, FALSE);
    }
    if (config == NULL) {
        /* This probably just means that we got no arguments. */
        return 2;
    }
    if ((ctl = cmd_ln_str_r(config, "-ctl")) == NULL) {
        E_FATAL("-ctl argument not present, nothing to do in batch mode!\n");
    }
    if ((ctlfh = fopen(ctl, "r")) == NULL) {
        E_FATAL_SYSTEM("Failed to open control file %s", ctl);
    }
    ps = ps_init(config);
    if (ps == NULL) {
        E_FATAL("PocketSphinx decoder init failed\n");
    }

    process_ctl(ps, config, ctlfh);

    fclose(ctlfh);
    ps_free(ps);
    return 0;
}

/** Silvio Moioli: Windows CE/Mobile entry point added. */
#if defined(_WIN32_WCE)
#pragma comment(linker,"/entry:mainWCRTStartup")
#include <windows.h>

//Windows Mobile has the Unicode main only
int wmain(int32 argc, wchar_t *wargv[]) {
    char** argv;
    size_t wlen;
    size_t len;
    int i;

    argv = malloc(argc*sizeof(char*));
    for (i=0; i<argc; i++){
        wlen = lstrlenW(wargv[i]);
        len = wcstombs(NULL, wargv[i], wlen);
        argv[i] = malloc(len+1);
        wcstombs(argv[i], wargv[i], wlen);
    }

    //assuming ASCII parameters
    return main(argc, argv);
}
#endif
