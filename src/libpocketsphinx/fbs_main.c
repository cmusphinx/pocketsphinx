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
#if !defined(_WIN32) || defined(__CYGWIN__)
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/param.h>
#endif

#if defined(GNUWINCE)
#include <unistd.h>
#endif

/* SphinxBase headers */
#include <fe.h>
#include <feat.h>
#include <ckd_alloc.h>
#include <err.h>
#include <prim_type.h>

/* Local headers */
#include "strfuncs.h"
#include "cmdln_macro.h"
#include "fbs.h"
#include "pio.h"
#include "s2io.h"
#include "kb.h"
#include "uttproc.h"
#include "byteorder.h"
#include "search.h"
#include "posixwin32.h"

int32 uttproc_set_startword(char const *str);

/* Static declarations for this file. */
static search_hyp_t *run_sc_utterance(char *mfcfile, int32 sf, int32 ef,
                                      char *idspec);
static void run_ctl_file(char const *ctl_file_name);
static void init_feat(void);
static void run_ctl_file(char const *ctl_file_name);

/* Command-line arguments (actually defined in cmdln_macro.h) */
static const arg_t fbs_args_def[] = {
    input_cmdln_options(),
    waveform_to_cepstral_command_line_macro(),
    output_cmdln_options(),
    am_cmdln_options(),
    lm_cmdln_options(),
    dictionary_cmdln_options(),
    fsg_cmdln_options(),
    beam_cmdln_options(),
    search_cmdln_options(),
    CMDLN_EMPTY_OPTION
};

/* Feature and front-end parameters that may be in feat.params */
static const arg_t feat_defn[] = {
    waveform_to_cepstral_command_line_macro(),
    input_cmdln_options(),
    { NULL, 0, NULL, NULL }
};

static char utt_name[512];

arg_t const *
fbs_get_args(void)
{
    return fbs_args_def;
}

int
fbs_init(int32 argc, char **argv)
{
    E_INFO("libpocketsphinx/fbs_main.c COMPILED ON: %s, AT: %s\n\n", __DATE__, __TIME__);

    /* Parse command line arguments, unless already parsed. */
    if (argv != NULL) {
        cmd_ln_appl_enter(argc, argv,
                          "pocketsphinx.args",
                          fbs_args_def);
    }

    /* Look for a feat.params very early on, because it influences
     * everything below. */
    if (cmd_ln_str("-hmm")) {
	char *str;
        str = string_join(cmd_ln_str("-hmm"), "/feat.params", NULL);
	if (cmd_ln_parse_file(feat_defn, str, FALSE) == 0) {
	    E_INFO("Parsed model-specific feature parameters from %s\n", str);
	}
        ckd_free(str);
    }

    /* Initialize feature computation.  We have to do this first
     * because the acoustic models (loaded in kb_init()) have to match
     * the feature type and parameters. */
    init_feat();

    /* Load the language and acoustic models. */
    kb_init();

    /* FIXME FIXME FIXME: We shouldn't initialize the N-Gram search if
     * we are not going to use it, likewise for the FSG search... */
    /* Initialize the N-Gram search module */
    search_initialize();

    /* Initialize dynamic data structures needed for utterance processing */
    /* FIXME FIXME FIXME: For no good reason, this also initializes FSG search. */
    uttproc_init();

    /* FIXME: Now, because of this utter stupidity, we need to make
     * sure that we build the search tree if we are in N-Gram mode. */
    if (!uttproc_fsg_search_mode())
        search_set_current_lm();

    /* Some random stuff that doesn't have anywhere else to go. */
    if (cmd_ln_str("-rawlogdir"))
        uttproc_set_rawlogdir(cmd_ln_str("-rawlogdir"));
    if (cmd_ln_str("-mfclogdir"))
        uttproc_set_mfclogdir(cmd_ln_str("-mfclogdir"));

    E_INFO("fbs_main.c COMPILED ON: %s, AT: %s\n\n", __DATE__, __TIME__);

    /*
     * Initialization complete; If there was a control file run batch mode.
     */

    if (cmd_ln_str("-ctl")) {
        run_ctl_file(cmd_ln_str("-ctl"));
    }

    return 0;
}

int32
fbs_end(void)
{
    uttproc_end();
    search_free();
    kb_close();
    feat_free(fcb);
    fcb = NULL;
    cmd_ln_free();
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

    if (0 == strcmp(cmd_ln_str("-cmn"), "prior")) {
        char *c, *cc, *vallist;
        int32 nvals;

        vallist = ckd_salloc(cmd_ln_str("-cmninit"));
        c = vallist;
        nvals = 0;
        while (nvals < fcb->cmn_struct->veclen
               && (cc = strchr(c, ',')) != NULL) {
            *cc = '\0';
            fcb->cmn_struct->cmn_mean[nvals] = FLOAT2MFCC(atof(c));
            c = cc + 1;
            ++nvals;
        }
        if (nvals < fcb->cmn_struct->veclen && *c != '\0') {
            fcb->cmn_struct->cmn_mean[nvals] = FLOAT2MFCC(atof(c));
        }
        ckd_free(vallist);
    }

    uttproc_set_feat(fcb);
}


char *
build_uttid(char const *utt)
{
    char const *utt_id;
    int32 i;

    /* Find uttid */
    for (i = strlen(utt) - 1;
         (i >= 0) && (utt[i] != '\\') && (utt[i] != '/'); --i);
    utt_id = utt + i + 1;

    /* Copy at most sizeof(utt_name) bytes, then null-terminate. */
    strncpy(utt_name, utt_id, sizeof(utt_name));
    utt_name[sizeof(utt_name)/sizeof(utt_name[0])-1] = '\0';
    return utt_name;
}

static void
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

        if ((ctl_offset-- > 0) || (ctl_count <= 0)
            || ((line_no++ % ctl_incr) != 0))
            continue;

        E_INFO("\nUtterance: %s\n", idspec);

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

        ctl_count--;
    }

    if (ctl_fs != stdin)
        fclose(ctl_fs);
}

/*
 * Decode utterance.
 */
static search_hyp_t *
run_sc_utterance(char *mfcfile, int32 sf, int32 ef, char *idspec)
{
    int32 frmcount, ret;
    char *finalhyp;
    char utt[1024];
    search_hyp_t *hypseg;
    int32 nbest;
    char *utt_lmname_dir = cmd_ln_str("-lmnamedir");

    strcpy(utt, idspec);
    build_uttid(utt);

    nbest = cmd_ln_int32("-nbest");

    /* Select the LM for utt */
    if (utt_lmname_dir) {
        FILE *lmname_fp;
        char *lmname_ext = cmd_ln_str("-lmnameext");
        char utt_lmname_file[1000], lmname[1000];

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

    build_uttid(utt);
    if (cmd_ln_boolean("-adcin")) {
        ret = uttproc_decode_raw_file(utt, utt_name, sf, ef, 0);
    }
    else {
        ret = uttproc_decode_cep_file(utt, utt_name, sf, ef, 0);
    }

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
