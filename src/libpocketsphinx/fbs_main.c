/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 1999-2004 Carnegie Mellon University.  All rights
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
/* MAIN.C - for FBS (Fast Beam Search)
 *-----------------------------------------------------------------------*
 * USAGE
 *	fbs -help
 *
 * DESCRIPTION
 *	This program allows you to run the beam search in either single
 * sentence or batch mode.
 *
 *-----------------------------------------------------------------------*
 * HISTORY
 * 
 * 
 * 06-Jan-99	M K Ravishankar (rkm@cs) at Carnegie Mellon University
 * 		Added set_adc_input().
 * 		Fixed call to utt_file2feat to use mfcfile instead of utt.
 * 		Changed build_uttid to return the built id string.
 * 
 * 05-Jan-99	M K Ravishankar (rkm@cs) at Carnegie Mellon University
 * 		Added uttproc_parse_ctlfile_entry().
 * 
 * 21-Oct-98	M K Ravishankar (rkm@cs) at Carnegie Mellon University
 * 		Made file extension on ctlfn entries optional.
 * 
 * 19-Oct-98	M K Ravishankar (rkm@cs) at Carnegie Mellon University
 * 		Added uttproc_set_logfile().
 * 
 * 10-Sep-98	M K Ravishankar (rkm@cs) at Carnegie-Mellon University
 * 		Wrote Nbest list to stdout if failed to open .hyp file.
 * 		Added "-" (use stdin) special case to -ctlfn option.
 * 
 * 10-Sep-98	M K Ravishankar (rkm@cs) at Carnegie-Mellon University
 * 		Moved run_allphone_utt to uttproc.c as uttproc_allphone_cepfile, with
 * 		minor modifications to allow calls from outside the libraries.
 * 
 * 19-Nov-97	M K Ravishankar (rkm@cs) at Carnegie-Mellon University
 * 		Added return-value check from SCVQInitFeat().
 * 
 * 22-Jul-97	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Added -samp argument for sampling rate.
 * 
 * 22-May-97	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Included Bob Brennan's code for quoted strings in argument list.
 * 
 * 10-Feb-97	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Added -nbest option in batch mode.
 * 
 * 02-Jan-96	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Added time_align_phone and time_align_state flags.
 * 
 * 12-Jul-95	M K Ravishankar (rkm@cs) at Carnegie Mellon University
 * 		Changed use_3g_in_fwd_pass default to TRUE.
 * 
 * 02-Jul-95	M K Ravishankar (rkm@cs) at Carnegie Mellon University
 * 		Added allphone handling.
 * 
 * 16-Jun-95	M K Ravishankar (rkm@cs) at Carnegie Mellon University
 * 		Added handling of #comment lines in argument files.
 * 
 * 14-Jun-95	M K Ravishankar (rkm@cs) at Carnegie Mellon University
 * 		Added backslash option in building filenames (for PC compatibility).
 * 
 * 13-Jun-95	M K Ravishankar (rkm@cs) at Carnegie Mellon University
 * 		Modified to conform to the new, simplified uttproc interface.
 * 
 * 09-Dec-94	M K Ravishankar (rkm@cs) at Carnegie Mellon University
 *		Added code to handle flat forward pass after tree forward pass.
 * 		Added code to handle raw speech input in batch mode.
 * 		Moved the early cep preprocessing code to cep2feat.c.
 * 
 * Revision 8.8  94/10/11  12:38:45  rkm
 * Added backtrace argument.
 * 
 * Revision 8.7  94/07/29  11:56:59  rkm
 * Added arguments for ILM ug and bg cache parameters.
 * 
 * Revision 8.6  94/05/19  14:21:11  rkm
 * Minor changes to statistics format.
 * 
 * Revision 8.5  94/04/22  13:56:04  rkm
 * Cosmetic changes to various global variables and run-time arguments.
 * 
 * Revision 8.4  94/04/14  14:43:03  rkm
 * Added second pass option for lattice-rescoring.
 * 
 * Revision 8.1  94/02/15  15:09:47  rkm
 * Derived from v7.  Includes multiple start symbols for the LISTEN
 * project.  Includes multiple LMs for grammar switching.
 * 
 * Revision 6.15  94/02/11  13:13:29  rkm
 * Initial revision (going into v7).
 * Added multiple start symbols for the LISTEN project.
 * 
 * Revision 6.14  94/01/07  17:47:12  rkm
 * Added option to use trigrams in forward pass (simple implementation).
 * 
 * Revision 6.13  94/01/05  16:14:02  rkm
 * Added option to report alternative pronunciations in match file.
 * Output just the marker in match file if speech file could not be found.
 * 
 * Revision 6.12  93/12/04  16:37:57  rkm
 * Added ifndef _HPUX_SOURCE around getrusage.
 * 
 * Revision 6.11  93/11/22  11:39:55  rkm
 * *** empty log message ***
 * 
 * Revision 6.10  93/11/15  12:20:56  rkm
 * Added Mei-Yuh's handling of wsj1Sent organization.
 * 
 * Revision 6.9  93/11/03  12:42:52  rkm
 * Added -latsize option to specify BP and FP table sizes to allocate.
 * 
 * Revision 6.8  93/10/29  11:40:42  rkm
 * Added QUIT definition.
 * 
 * Revision 6.7  93/10/29  10:21:40  rkm
 * *** empty log message ***
 * 
 * Revision 6.6  93/10/28  18:06:04  rkm
 * Added -fbdumpdir flag.
 * 
 * Revision 6.5  93/10/27  17:41:00  rkm
 * *** empty log message ***
 * 
 * Revision 6.4  93/10/13  16:59:34  rkm
 * Added -ctlcount, -astaronly, and -svadapt options.
 * Added --END-OF-DOCUMENT-- option within .ctl files for Roni's ILM.
 * 
 * Revision 6.3  93/10/09  17:03:17  rkm
 * 
 * Revision 6.2  93/10/06  11:09:43  rkm
 * M K Ravishankar (rkm@cs) at Carnegie Mellon University
 * Darpa Trigram LM module added.
 * 
 *
 *	Spring, 89 - Fil Alleva (faa) at Carnegie Mellon
 *		Created
 */

/* System headers */
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#ifndef _WIN32
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/param.h>
#endif

/* SphinxBase headers */
#include <fe.h>
#include <feat.h>
#include <ckd_alloc.h>
#include <err.h>

/* Local headers */
#include "s2types.h"
#include "strfuncs.h"
#include "cmdln_macro.h"
#include "fbs.h"
#include "pio.h"
#include "s2io.h"
#include "kb.h"
#include "uttproc.h"
#include "allphone.h"
#include "byteorder.h"
#include "time_align.h"
#include "search.h"
#include "posixwin32.h"

/* Static declarations for this file. */
static search_hyp_t *run_sc_utterance(char *mfcfile, int32 sf, int32 ef,
                                      char *idspec);
static void init_feat(void);

/* Command-line arguments (actually defined in cmdln_macro.h) */
static const arg_t args_def[] = {
    input_cmdln_options(),
    waveform_to_cepstral_command_line_macro(),
    output_cmdln_options(),
    am_cmdln_options(),
    lm_cmdln_options(),
    dictionary_cmdln_options(),
    fsg_cmdln_options(),
    beam_cmdln_options(),
    search_cmdln_options(),
    time_align_cmdln_options(),
    allphone_cmdln_options(),
    CMDLN_EMPTY_OPTION
};

/* Some static variables we still need here. */
static float TotalElapsedTime;
static float TotalCPUTime;
static float TotalSpeechTime;
/* FIXME FIXME FIXME fixed size buffer */
static char utt_name[512];

int
fbs_init(int32 argc, char **argv)
{
    E_INFO("libpocketsphinx/fbs_main.c COMPILED ON: %s, AT: %s\n\n", __DATE__, __TIME__);

    /* Parse command line arguments */
    cmd_ln_appl_enter(argc, argv,
                      "pocketsphinx.args",
                      (arg_t *)args_def);

    /* Compatibility with old forwardonly flag */
    if ((!cmd_ln_boolean("-fwdtree")) && (!cmd_ln_boolean("-fwdflat")))
        E_FATAL
            ("At least one of -fwdtree and -fwdflat flags must be TRUE\n");

    /* Load the KB */
    kb_init();

    /* Initialize the search module */
    search_initialize();

    /* Initialize dynamic data structures needed for utterance processing */
    uttproc_init();

    /* Initialize feature computation. */
    init_feat();

    if (cmd_ln_str("-rawlogdir"))
        uttproc_set_rawlogdir(cmd_ln_str("-rawlogdir"));
    if (cmd_ln_str("-mfclogdir"))
        uttproc_set_mfclogdir(cmd_ln_str("-mfclogdir"));

    /* If multiple LMs present, choose the unnamed one by default */
    /* FIXME: Add a -lmname option, use it. */
    if (cmd_ln_str("-fsg") == NULL) {
        if (get_n_lm() == 1) {
            if (uttproc_set_lm(get_current_lmname()) < 0)
                E_FATAL("SetLM() failed\n");
        }
        else {
            if (uttproc_set_lm("") < 0)
                E_WARN
                    ("SetLM(\"\") failed; application must set one before recognition\n");
        }
    }
    else {
        E_INFO("/* Need to select from among multiple FSGs */\n");
    }

    /* Set the current start word to <s> (if it exists) */
    if (kb_get_word_id("<s>") >= 0)
        uttproc_set_startword("<s>");

    if (cmd_ln_boolean("-allphone"))
        allphone_init();

    E_INFO("libfbs/main COMPILED ON: %s, AT: %s\n\n", __DATE__, __TIME__);

    /*
     * Initialization complete; If there was a control file run batch
     */

    if (cmd_ln_str("-ctl")) {
        if (!cmd_ln_str("-tactl"))
            run_ctl_file(cmd_ln_str("-ctl"));
        else
            run_time_align_ctl_file(cmd_ln_str("-ctl"),
                                    cmd_ln_str("-tactl"),
                                    cmd_ln_str("-outsent"));

        uttproc_end();
        exit(0);
    }

    return 0;
}

int32
fbs_end(void)
{
    uttproc_end();
    return 0;
}

static void
init_feat(void)
{
    feat_t *fcb;

    fcb = feat_init(cmd_ln_str("-feat"),
                    cmn_type_from_str(cmd_ln_str("-cmn")),
                    cmd_ln_boolean("-varnorm"),
                    agc_type_from_str(cmd_ln_str("-agc")),
                    1, cmd_ln_int32("-ceplen"));

    if (0 != strcmp(cmd_ln_str("-agc"), "none")) {
        agc_set_threshold(fcb->agc_struct,
                          cmd_ln_float32("-agcthresh"));
    }

    uttproc_set_feat(fcb);
}

/* FIXME... use I/O stuff in sphinxbase */
static int32 adc_endian;

FILE *
adcfile_open(char const *utt)
{
    const char *adc_ext, *data_directory;
    FILE *uttfp;
    char *inputfile;
    int32 n, l, adc_hdr;

    adc_ext = cmd_ln_str("-cepext");
    adc_hdr = cmd_ln_int32("-adchdr");
    adc_endian = strcmp(cmd_ln_str("-adcendian"), "big");
    data_directory = cmd_ln_str("-cepdir");

    /* Build input filename */
    n = strlen(adc_ext);
    l = strlen(utt);
    if ((n <= l) && (0 == strcmp(utt + l - n, adc_ext)))
        adc_ext = "";          /* Extension already exists */
    inputfile = string_join(data_directory, "/", utt, adc_ext, NULL);

    if ((uttfp = fopen(inputfile, "rb")) == NULL) {
        E_FATAL("fopen(%s,rb) failed\n", inputfile);
    }
    if (adc_hdr > 0) {
        if (fseek(uttfp, adc_hdr, SEEK_SET) < 0) {
            E_ERROR("fseek(%s,%d) failed\n", inputfile, adc_hdr);
            fclose(uttfp);
            ckd_free(inputfile);
            return NULL;
        }
    }
#ifdef WORDS_BIGENDIAN
    if (adc_endian == 1)    /* Little endian adc file */
        E_INFO("Byte-reversing %s\n", inputfile);
#else
    if (adc_endian == 0)    /* Big endian adc file */
        E_INFO("Byte-reversing %s\n", inputfile);
#endif
    ckd_free(inputfile);

    return uttfp;
}

int32
adc_file_read(FILE *uttfp, int16 * buf, int32 max)
{
    int32 i, n;

    if (uttfp == NULL)
        return -1;

    if ((n = fread(buf, sizeof(int16), max, uttfp)) <= 0)
        return -1;

    /* Byte swap if necessary */
#ifdef WORDS_BIGENDIAN
    if (adc_endian == 1) {      /* Little endian adc file */
        for (i = 0; i < n; i++)
            SWAP_INT16(&buf[i]);
    }
#else
    if (adc_endian == 0) {      /* Big endian adc file */
        for (i = 0; i < n; i++)
            SWAP_INT16(&buf[i]);
    }
#endif

    return n;
}

char *
build_uttid(char const *utt)
{
    char const *utt_id;

    /* Find uttid */
#ifdef WIN32
    {
        int32 i;
        for (i = strlen(utt) - 1;
             (i >= 0) && (utt[i] != '\\') && (utt[i] != '/'); --i);
        utt_id = utt + i;
    }
#else
    utt_id = strrchr(utt, '/');
#endif
    if (utt_id)
        utt_id++;
    else
        utt_id = utt;
    strcpy(utt_name, utt_id);

    return utt_name;
}

void
run_ctl_file(char const *ctl_file_name)
/*-------------------------------------------------------------------------*
 * Sequence through a control file containing a list of utterance
 * NB. This is a one shot routine.
 */
{
    FILE *ctl_fs;
    char line[4096], mfcfile[4096], idspec[4096];
    int32 line_no = 0;
    int32 sf, ef;
    search_hyp_t *hyp;
    int32 ctl_offset, ctl_count, ctl_incr;

    if (strcmp(ctl_file_name, "-") != 0)
        ctl_fs = myfopen(ctl_file_name, "r");
    else
        ctl_fs = stdin;

    ctl_offset = cmd_ln_int32("-ctloffset");
    ctl_count = cmd_ln_int32("-ctlcount");
    ctl_incr = cmd_ln_int32("-ctlincr");
    for (;;) {
        if (ctl_fs == stdin)
            E_INFO("\nInput file(no ext): ");
        if (fgets(line, sizeof(line), ctl_fs) == NULL)
            break;

        if (uttproc_parse_ctlfile_entry(line, mfcfile, &sf, &ef, idspec) <
            0)
            continue;

        if (strcmp(mfcfile, "--END-OF-DOCUMENT--") == 0) {
            search_finish_document();
            continue;
        }
        if ((ctl_offset-- > 0) || (ctl_count <= 0)
            || ((line_no++ % ctl_incr) != 0))
            continue;

        E_INFO("\nUtterance: %s\n", idspec);

        if (!cmd_ln_boolean("-allphone")) {
            hyp = run_sc_utterance(mfcfile, sf, ef, idspec);
            if (hyp && cmd_ln_boolean("-shortbacktrace")) {
                /* print backtrace summary */
                fprintf(stdout, "SEG:");
                for (; hyp; hyp = hyp->next)
                    fprintf(stdout, "[%d %d %s]", hyp->sf, hyp->ef,
                            hyp->word);
                fprintf(stdout, " (%s %d A=%d L=%d)\n\n",
                        uttproc_get_uttid(), search_get_score(),
                        search_get_score() - search_get_lscr(),
                        search_get_lscr());
                fflush(stdout);
            }
        }
        else
            uttproc_allphone_file(mfcfile);

        ctl_count--;
    }

    if (ctl_fs != stdin)
        fclose(ctl_fs);
}

void
run_time_align_ctl_file(char const *utt_ctl_file_name,
                        char const *pe_ctl_file_name,
                        char const *out_sent_file_name)
/*-------------------------------------------------------------------------*
 * Sequence through a control file containing a list of utterance
 * NB. This is a one shot routine.
 */
{
    FILE *utt_ctl_fs;
    FILE *pe_ctl_fs;
    FILE *out_sent_fs;
    char Utt[1024];
    char time_align_spec[1024];
    int32 line_no = 0;
    char left_word[256];
    char right_word[256];
    char pe_words[1024];
    int32 begin_frame;
    int32 end_frame;
    int32 n_featfr;
    int32 align_all = 0;
    int32 ctl_offset, ctl_count, ctl_incr;

    time_align_init();
    time_align_set_beam_width(1e-9f); /* FIXME: !!!??? */
    E_INFO("****** USING WIDE BEAM ****** (1e-9)\n");

    utt_ctl_fs = myfopen(utt_ctl_file_name, "r");
    pe_ctl_fs = myfopen(pe_ctl_file_name, "r");

    if (out_sent_file_name) {
        out_sent_fs = myfopen(out_sent_file_name, "w");
    }
    else
        out_sent_fs = NULL;

    ctl_offset = cmd_ln_int32("-ctloffset");
    ctl_count = cmd_ln_int32("-ctlcount");
    ctl_incr = cmd_ln_int32("-ctlincr");
    while (fscanf(utt_ctl_fs, "%s\n", Utt) != EOF) {
        fgets(time_align_spec, 1023, pe_ctl_fs);

        if (ctl_offset) {
            ctl_offset--;
            continue;
        }
        if (ctl_count == 0)
            continue;
        if ((line_no++ % ctl_incr) != 0)
            continue;

        if (!strncmp
            (time_align_spec, "*align_all*", strlen("*align_all*"))) {
            E_INFO("Aligning whole utterances\n");
            align_all = 1;
            fgets(time_align_spec, 1023, pe_ctl_fs);
        }

        if (align_all) {
            strcpy(left_word, "<s>");
            strcpy(right_word, "</s>");
            begin_frame = end_frame = NO_FRAME;
            time_align_spec[strlen(time_align_spec) - 1] = '\0';
            strcpy(pe_words, time_align_spec);

            E_INFO("Utt %s\n", Utt);
            fflush(stdout);
        }
        else {
            sscanf(time_align_spec,
                   "%s %d %d %s %[^\n]",
                   left_word, &begin_frame, &end_frame, right_word,
                   pe_words);
            E_INFO("\nDoing  '%s %d) %s (%d %s' in utterance %s\n",
                   left_word, begin_frame, pe_words, end_frame, right_word,
                   Utt);
        }

        build_uttid(Utt);
        if ((n_featfr = uttproc_file2feat(Utt, 0, -1, 1)) < 0)
            E_ERROR("Failed to load %s\n", Utt);
        else {
            time_align_utterance(Utt,
                                 out_sent_fs,
                                 left_word, begin_frame,
                                 pe_words, end_frame, right_word);
        }

        --ctl_count;
    }

    fclose(utt_ctl_fs);
    fclose(pe_ctl_fs);
}


/*
 * Decode utterance.
 */
static search_hyp_t *
run_sc_utterance(char *mfcfile, int32 sf, int32 ef, char *idspec)
{
    char startword_filename[1000];
    FILE *sw_fp;
    int32 frmcount, ret;
    char *finalhyp;
    char utt[1024];
    search_hyp_t *hypseg;
    int32 nbest;
    char *startWord_directory, *startWord_ext;

    strcpy(utt, idspec);
    build_uttid(utt);

    startWord_directory = cmd_ln_str("-startworddir");
    startWord_ext = cmd_ln_str("-startwordext");

    nbest = cmd_ln_int32("-nbest");

    /* Select the LM for utt */
    if (get_n_lm() > 1) {
        FILE *lmname_fp;
        char utt_lmname_file[1000], lmname[1000];
        char *utt_lmname_dir = cmd_ln_str("-lmnamedir");
        char *lmname_ext = cmd_ln_str("-lmnameext");

        sprintf(utt_lmname_file, "%s/%s.%s", utt_lmname_dir, utt_name,
                lmname_ext);
        E_INFO("Looking for LM-name file %s\n", utt_lmname_file);
        if ((lmname_fp = fopen(utt_lmname_file, "r")) != NULL) {
            /* File containing LM name for this utt exists */
            if (fscanf(lmname_fp, "%s", lmname) != 1)
                E_FATAL("Cannot read lmname from file %s\n",
                        utt_lmname_file);
            fclose(lmname_fp);
        }
        else {
            /* No LM name specified for this utt; use default (with no name) */
            E_INFO("File %s not found, using default LM\n",
                   utt_lmname_file);
            lmname[0] = '\0';
        }

        uttproc_set_lm(lmname);
    }

    /* Select startword for utt (LISTEN project) */
#ifdef WIN32
    if (startWord_directory && (utt[0] != '\\') && (utt[0] != '/'))
        sprintf(startword_filename, "%s/%s.%s",
                startWord_directory, utt, startWord_ext);
    else
        sprintf(startword_filename, "%s.%s", utt, startWord_ext);
#else
    if (startWord_directory && (utt[0] != '/'))
        sprintf(startword_filename, "%s/%s.%s",
                startWord_directory, utt, startWord_ext);
    else
        sprintf(startword_filename, "%s.%s", utt, startWord_ext);
#endif
    if ((sw_fp = fopen(startword_filename, "r")) != NULL) {
        char startWord[512]; /* FIXME */
        fscanf(sw_fp, "%s", startWord);
        fclose(sw_fp);

        E_INFO("startWord: %s\n", startWord);

        uttproc_set_startword(startWord);
    }

    ret = uttproc_file2feat(mfcfile, sf, ef, 0);

    if (ret < 0)
        return NULL;

    /* Get hyp words segmentation (linked list of search_hyp_t) */
    if (uttproc_result_seg(&frmcount, &hypseg, 1) < 0) {
        E_ERROR("uttproc_result_seg(%s) failed\n", uttproc_get_uttid());
        return NULL;
    }
    search_result(&frmcount, &finalhyp);

    if (!uttproc_fsg_search_mode()) {
        /* Should the Nbest generation be in uttproc.c (uttproc_result)?? */
        if (nbest > 0) {
            FILE *nbestfp;
            char nbestfile[4096];
            search_hyp_t *h, **alt;
            int32 i, n_alt, startwid;
            char *nbest_dir = cmd_ln_str("-nbestdir");
            char *nbest_ext = cmd_ln_str("-nbestext");

            startwid = kb_get_word_id("<s>");
            search_save_lattice();
            n_alt =
                search_get_alt(nbest, 0, searchFrame(), -1, startwid,
                               &alt);

            sprintf(nbestfile, "%s/%s.%s", nbest_dir, utt_name, nbest_ext);
            if ((nbestfp = fopen(nbestfile, "w")) == NULL) {
                E_WARN("fopen(%s,w) failed; using stdout\n", nbestfile);
                nbestfp = stdout;
            }
            for (i = 0; i < n_alt; i++) {
                for (h = alt[i]; h; h = h->next)
                    fprintf(nbestfp, "%s ", h->word);
                fprintf(nbestfp, "\n");
            }
            if (nbestfp != stdout)
                fclose(nbestfp);
        }

    }

    return hypseg;              /* Linked list of hypothesis words */
}

/*
 * Run time align on a semi-continuous utterance.
 */
void
time_align_utterance(char const *utt,
                     FILE * out_sent_fp,
                     char const *left_word, int32 begin_frame,
                     /* FIXME: Gets modified in time_align.c - watch out! */
                     char *pe_words,
                     int32 end_frame, char const *right_word)
{
    int32 n_frames;
    mfcc_t ***feat;
#ifndef _WIN32
    struct rusage start, stop;
    struct timeval e_start, e_stop;
#endif

    if ((begin_frame != NO_FRAME) || (end_frame != NO_FRAME)) {
        E_ERROR("Partial alignment not implemented\n");
        return;
    }

    if ((n_frames = uttproc_get_featbuf(&feat)) < 0) {
        E_ERROR("#input speech frames = %d\n", n_frames);
        return;
    }

    time_align_set_input(feat, n_frames);

#ifndef _WIN32
#ifndef _HPUX_SOURCE
    getrusage(RUSAGE_SELF, &start);
#endif                          /* _HPUX_SOURCE */
    gettimeofday(&e_start, 0);
#endif                          /* _WIN32 */

    if (time_align_word_sequence(utt, left_word, pe_words, right_word) ==
        0) {
        char *data_directory = cmd_ln_str("-cepdir");
        char *seg_data_directory = cmd_ln_str("-segdir");
        char *seg_file_ext = cmd_ln_str("-segext");

        if (seg_file_ext) {
            unsigned short *seg;
            int seg_cnt;
            char seg_file_basename[MAXPATHLEN + 1];

            switch (time_align_seg_output(&seg, &seg_cnt)) {
            case NO_SEGMENTATION:
                E_ERROR("NO SEGMENTATION for %s\n", utt);
                break;

            case NO_MEMORY:
                E_ERROR("NO MEMORY for %s\n", utt);
                break;

            default:
                {
                    /* write the data in the same location as the cep file */
                    if (data_directory && (utt[0] != '/')) {
                        if (seg_data_directory) {
                            sprintf(seg_file_basename, "%s/%s.%s",
                                    seg_data_directory, utt, seg_file_ext);
                        }
                        else {
                            sprintf(seg_file_basename, "%s/%s.%s",
                                    data_directory, utt, seg_file_ext);
                        }
                    }
                    else {
                        if (seg_data_directory) {
                            char *spkr_dir;
                            char basename[MAXPATHLEN];
                            char *sl;

                            strcpy(basename, utt);

                            sl = strrchr(basename, '/');
                            *sl = '\0';
                            sl = strrchr(basename, '/');
                            spkr_dir = sl + 1;

                            sprintf(seg_file_basename, "%s/%s/%s.%s",
                                    seg_data_directory, spkr_dir, utt_name,
                                    seg_file_ext);
                        }
                        else {
                            sprintf(seg_file_basename, "%s.%s", utt,
                                    seg_file_ext);
                        }
                    }
                    E_INFO("Seg output %s\n", seg_file_basename);
                    awriteshort(seg_file_basename, seg, seg_cnt);
                }
            }
        }

        if (out_sent_fp) {
            char const *best_word_str = time_align_best_word_string();

            if (best_word_str) {
                fprintf(out_sent_fp, "%s (%s)\n", best_word_str, utt_name);
            }
            else {
                fprintf(out_sent_fp, "NO BEST WORD SEQUENCE for %s\n",
                        utt);
            }
        }
    }
    else {
        E_ERROR("No alignment for %s\n", utt_name);
    }

#ifndef _WIN32
#ifndef _HPUX_SOURCE
    getrusage(RUSAGE_SELF, &stop);
#endif                          /* _HPUX_SOURCE */
    gettimeofday(&e_stop, 0);

    E_INFO(" %5.2f SoS", n_frames * 0.01);
    E_INFO(", %6.2f sec elapsed", MakeSeconds(&e_start, &e_stop));
    if (n_frames > 0)
        E_INFO(", %5.2f xRT",
               MakeSeconds(&e_start, &e_stop) / (n_frames * 0.01));

#ifndef _HPUX_SOURCE
    E_INFO(", %6.2f sec CPU",
           MakeSeconds(&start.ru_utime, &stop.ru_utime));
    if (n_frames > 0)
        E_INFO(", %5.2f xRT",
               MakeSeconds(&start.ru_utime,
                           &stop.ru_utime) / (n_frames * 0.01));
#endif                          /* _HPUX_SOURCE */
    E_INFO("\n");

    TotalCPUTime += MakeSeconds(&start.ru_utime, &stop.ru_utime);
    TotalElapsedTime += MakeSeconds(&e_start, &e_stop);
    TotalSpeechTime += n_frames * 0.01;
#endif                          /* _WIN32 */
}
