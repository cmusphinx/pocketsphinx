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

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#if defined(WIN32) && !defined(GNUWINCE) && !defined(CYGWIN)
#include <fcntl.h>
#include <io.h>
#else
#include <sys/file.h>
#include <sys/errno.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>

#include <stdlib.h>

#include "c.h"
#include "s2types.h"
#include "pio.h"
#include "basic_types.h"
#include "strfuncs.h"
#include "list.h"
#include "search_const.h"
#include "msd.h"
#include "pconf.h"
#include "s2_semi_mgau.h"
#include "dict.h"
#include "err.h"
#include "lmclass.h"
#include "lm_3g.h"
#include "lm.h"
#include "kb.h"
#include "time_align.h"
#include "fbs.h"
#include "search.h"
#include "cepio.h"
#include "byteorder.h"
#include "s2params.h"
#include "feat.h"
#include "ckd_alloc.h"
#include "fe.h"

/*
 * #define QUIT(x)		{fprintf x; exit(-1);}
 */

/* Default parameter initialization
 *----------------------------------*/
static char *ctl_file_name = 0;
static char *match_file_name = NULL;
static char *matchseg_file_name = NULL;
static char *logfn_arg = NULL;
char *data_directory = 0;
static char *seg_data_directory = 0;
static char const *sent_directory = ".";
static int32 phone_conf = 0;
static char *pscr2lat = NULL;   /* Directory for phone lattice files */

static int32 nbest = 0;         /* #N-best hypotheses to generate/utterance */
static char const *nbest_dir = ".";
static char const *nbest_ext = "hyp";


static int32 ctl_offset = 0;    /* No. of lines to skip at start of ctlfile */
static int32 ctl_incr = 1;      /* Do every nth line in the ctl file */
static int32 ctl_count = 0x7fffffff;    /* #lines to be processed */

char const *cep_ext = ".mfc";
static char const *sent_ext = "sent";
static float beam_width = 1e-6f;
static float new_phone_beam_width = 1e-6f;
static float last_phone_beam_width = 1e-5f;
static float lastphone_alone_beam_width = 3e-4f;
static float new_word_beam_width = 3e-4f;
static float fwdflat_beam_width = 1e-8f;
static float fwdflat_new_word_beam_width = 3e-4f;
static float filler_word_penalty = 1e-8f;
static float silence_word_penalty = 0.005f;
static float phone_insertion_penalty = 1.0f;
static float insertion_penalty = 0.65f;
static float fwdtree_lw = 6.5f;
static float fwdflat_lw = 8.5f;
static float bestpath_lw = 9.5f;
static float nw_pen = 1.0f;

static int32 fwdtree_flag = TRUE;
static int32 fwdflat_flag = TRUE;
static int32 bestpath_flag = TRUE;
static int32 forward_only = FALSE;

static int32 live = FALSE;

static int32 agcNoise = FALSE;
static int32 agcMax = FALSE;
static int32 agcEMax = FALSE;
static int32 normalizeMean = TRUE;
static int32 normalizeMeanPrior = FALSE;
static float agcThresh = 0.2f;

static int32 writeScoreInMatchFile = TRUE;

/* if (skip_alt_frm) skip alternate frames in cross-phone transitions */
static int32 skip_alt_frm = 0;

/* BPTable size to allocate, use some default if max-int */
static int32 lattice_size = 50000;

/* # LM cache lines to allocate */
static int32 lm_cache_lines = 100;

extern int32 use_3g_in_fwd_pass;
int32 use_3g_in_fwd_pass = TRUE;

/* Phone Eval state */
static char *time_align_ctl_file_name = NULL;
int32 time_align_word = TRUE;
int32 time_align_phone = TRUE;
int32 time_align_state = FALSE;

/* State segmentation file (seg file) extension */
static char *seg_file_ext = NULL;

/* For saving phone labels in alignment */
char const *phonelabdirname = NULL;
char const *phonelabextname = "lab";
char const *phonelabtype = "xlabel";

/* For saving word labels in alignment */
char const *wordlabdirname = NULL;
char const *wordlabextname = "wrd";
char const *wordlabtype = "xlabel";

/* "Best" alternative word sent output file */
static char *out_sent_filename = NULL;

static int32 allphone_mode = FALSE;

/*
 * Top senones window (#frames) for predicting phone transitions.
 * If 1, do not use top senones to predict phones, transition to all.
 */
static int32 topsen_window = 1;
static int32 topsen_thresh = -60000;

/*
 * In FSG mode, set to FALSE to force backtrace from FSG state that has best
 * path score even if it is not the final state.
 */
static int32 fsg_backtrace_finalstate = TRUE;

/* Local Storage
 *---------------*/
static float TotalElapsedTime;
static float TotalCPUTime;
static float TotalSpeechTime;

/* Report actual pronunciation in output; default = report base pronunciation */
static int32 report_altpron = FALSE;

/* If > 0, report partial result string every so many frames, starting frame 1 */
static int32 report_partial_result = 0;
/*
 * If > 0, report partial results including segmentation every so many frames,
 * starting at frame 1.
 */
static int32 report_partial_result_seg = 0;

/* If (! compute_all_senones) compute only those needed by active channels */
static int32 compute_all_senones = FALSE;

/* Directory/Filenames for raw A/D and mfc data (written in live mode) */
static char *rawlogdir = NULL;
static char *mfclogdir = NULL;

static int32 sampling_rate = 16000;
/* Let all these be uninitialized (to zero) because we may determine
 * them based on sampling_rate if the user has not specified them. */
static int32 n_mel_filt;
static float lower_filt = -1.0f;        /* Someone might want this to be zero. */
static float upper_filt = 0.0f;
static float pre_emphasis_alpha = 0.0f;
static int32 frame_rate;
static int32 n_fft;
static float window_length = 0.0f;

static int32 adc_input = FALSE; /* TRUE <=> input utterances are raw A/D data */
static char const *adc_ext = "raw";     /* Default format: raw */
static int32 adc_endian = 1;    /* Default endian: little */
static int32 adc_hdr = 0;       /* Default adc file header size */
static int32 blocking_ad_read_p = FALSE;
static int32 doublebw = 0;      /* Whether we're using double bandwidth for mel filters, default no */

/* For LISTEN project: LM and startword names for different utts */
char const *utt_lmname_dir = ".";
char const *lmname_ext = "lmname";
static char const *startWord_directory = ".";
static char const *startWord_ext = "start";
static char startWord[1024] = "";

static char *dumplat_dir = NULL;

static char utt_name[1024] = "";

static int32 maxwpf = 100000000;        /* Max words recognized per frame */
static int32 maxhmmpf = 1000000000;     /* Max active HMMs per frame */

int32 print_back_trace = TRUE;
static int32 print_short_back_trace = FALSE;
static char *arg_file = NULL;
int32 verbosity_level = 9;      /* rkm: Was 0 */

/* FIXME: This is a hack until there is a fixed-point version of it. */
int32 calc_phone_perp = TRUE;   /* whether to calculate phone perplexity */

#ifndef WIN32
extern double MakeSeconds(struct timeval *, struct timeval *);
#endif

extern int32 uttproc_set_feat(feat_t *fcb);

extern int awriteshort(char const *file, short *data, int length);

extern void allphone_init(double, double, double);      /* dubya, dubya, dubya? */

/* Parameters for the search
 *---------------------------*/
extern config_t kb_param[];
config_t param[] = {
    /*
     * LongName, Documentation, Switch, TYPE, Address
     */
    {"ArgFile", "Cmd line argument file", "-argfile",
     STRING, (caddr_t) & arg_file},

    {"AllPhoneMode", "All Phone Mode", "-allphone",
     BOOL, (caddr_t) & allphone_mode},

    {"AgcMax", "Use max based AGC", "-agcmax",
     BOOL, (caddr_t) & agcMax},

    {"AgcEMax", "Use another max based AGC", "-agcemax",
     BOOL, (caddr_t) & agcEMax},

    {"AgcNoise", "Use Noise based AGC", "-agcnoise",
     BOOL, (caddr_t) & agcNoise},

    {"AgcThreshold", "Threshold for Noise based AGC", "-agcthresh",
     FLOAT, (caddr_t) & agcThresh},

    {"NormalizeMean", "Normalize the feature means to 0.0", "-normmean",
     BOOL, (caddr_t) & normalizeMean},

    {"NormalizeMeanPrior", "Normalize feature means with prior mean",
     "-nmprior",
     BOOL, (caddr_t) & normalizeMeanPrior},

    {"LiveData", "Get input from A/D hardware", "-live",
     BOOL, (caddr_t) & live},

    {"A/D blocks on read", "A/D blocks on read", "-blockingad",
     BOOL, (caddr_t) & blocking_ad_read_p},

    {"CtlFileName", "Control file name", "-ctlfn",
     STRING, (caddr_t) & ctl_file_name},

    {"CtlLineOffset", "Number of Lines to skip in ctl file", "-ctloffset",
     INT, (caddr_t) & ctl_offset},

    {"CtlCount", "Number of lines to process in ctl file", "-ctlcount",
     INT, (caddr_t) & ctl_count},

    {"CtlLineIncr", "Do every nth line in the ctl file", "-ctlincr",
     INT, (caddr_t) & ctl_incr},

    {"ComputeAllSenones", "Compute all senone scores every frame",
     "-compallsen",
     BOOL, (caddr_t) & compute_all_senones},

    {"TopSenonesFrames", "#frames top senones for predicting phones",
     "-topsenfrm",
     INT, (caddr_t) & topsen_window},

    {"TopSenonesThresh", "Top senones threshold for predicting phones",
     "-topsenthresh",
     INT, (caddr_t) & topsen_thresh},

    {"ReportAltPron", "Report actual pronunciation in match file",
     "-reportpron",
     BOOL, (caddr_t) & report_altpron},

    {"ReportPartialResult", "Report partial results every so many frames",
     "-partial",
     INT, (caddr_t) & report_partial_result},

    {"ReportPartialResultSeg",
     "Report detailed partial results every so many frames", "-partialseg",
     INT, (caddr_t) & report_partial_result_seg},

    {"MatchFileName", "Recognition output file name", "-matchfn",
     STRING, (caddr_t) & match_file_name},

    {"MatchSegFileName", "Recognition output with segmentation",
     "-matchsegfn",
     STRING, (caddr_t) & matchseg_file_name},

    {"PhoneConfidence", "Phone confidence", "-phoneconf",
     BOOL, (caddr_t) & phone_conf},

    {"PhoneLat",
     "Directory for writing phone lattices based on best senone scores",
     "-pscr2lat",
     STRING, (caddr_t) & pscr2lat},

    {"LogFileName", "Recognition ouput file name", "-logfn",
     STRING, (caddr_t) & logfn_arg},

    {"DataDirectory", "Data directory", "-datadir",
     STRING, (caddr_t) & data_directory},

    {"DataDirectory", "Data directory", "-cepdir",
     STRING, (caddr_t) & data_directory},

    {"DataDirectory", "Data directory", "-vqdir",
     STRING, (caddr_t) & data_directory},

    {"SegDataDirectory", "Data directory", "-segdir",
     STRING, (caddr_t) & seg_data_directory},

    {"CepExt", "Cepstrum File Extension", "-cepext",
     STRING, (caddr_t) & cep_ext},

    {"SentDir", "Sentence directory", "-sentdir",
     STRING, (caddr_t) & sent_directory},

    {"SentExt", "Sentence File Extension", "-sentext",
     STRING, (caddr_t) & sent_ext},

    {"PhoneLabDir", "Phone Label Directory", "-phonelabdir",
     STRING, (caddr_t) & phonelabdirname},

    {"PhoneLabExt", "Phone Label Extension (default lab)", "-phonelabext",
     STRING, (caddr_t) & phonelabextname},

    {"PhoneLabType", "Phone Label Type (default xlabel)", "-phonelabtype",
     STRING, (caddr_t) & phonelabtype},

    {"WordLabDir", "Word Label Directory", "-wordlabdir",
     STRING, (caddr_t) & wordlabdirname},

    {"WordLabExt", "Word Label Extension (default wrd)", "-wordlabext",
     STRING, (caddr_t) & wordlabextname},

    {"WordLabType", "Word Label Type (default xlabel)", "-wordlabtype",
     STRING, (caddr_t) & wordlabtype},

    {"LMNamesDir", "Directory for LM-name file for each utt", "-lmnamedir",
     STRING, (caddr_t) & utt_lmname_dir},

    {"LMNamesExt", "Filename extension for LM-name files", "-lmnameext",
     STRING, (caddr_t) & lmname_ext},

    {"StartWordDir", "Startword directory", "-startworddir",
     STRING, (caddr_t) & startWord_directory},

    {"StartWordExt", "StartWord File Extension", "-startwordext",
     STRING, (caddr_t) & startWord_ext},

    {"NbestDir", "N-best Hypotheses Directory", "-nbestdir",
     STRING, (caddr_t) & nbest_dir},

    {"NbestCount", "No. N-best Hypotheses", "-nbest",
     INT, (caddr_t) & nbest},

    {"NbestExt", "N-best Hypothesis File Extension", "-nbestext",
     STRING, (caddr_t) & nbest_ext},

    {"BeamWidth", "Beam Width", "-beam",
     FLOAT, (caddr_t) & beam_width},

    {"NewWordBeamWidth", "New Word Beam Width", "-nwbeam",
     FLOAT, (caddr_t) & new_word_beam_width},

    {"FwdFlatBeamWidth", "FwdFlat Beam Width", "-fwdflatbeam",
     FLOAT, (caddr_t) & fwdflat_beam_width},

    {"FwdFlatNewWordBeamWidth", "FwdFlat New Word Beam Width",
     "-fwdflatnwbeam",
     FLOAT, (caddr_t) & fwdflat_new_word_beam_width},

    {"LastPhoneAloneBeamWidth", "Beam Width for Last Phones Only",
     "-lponlybw",
     FLOAT, (caddr_t) & lastphone_alone_beam_width},

    {"LastPhoneAloneBeamWidth", "Beam Width for Last Phones Only",
     "-lponlybeam",
     FLOAT, (caddr_t) & lastphone_alone_beam_width},

    {"NewPhoneBeamWidth", "New Phone Beam Width", "-npbeam",
     FLOAT, (caddr_t) & new_phone_beam_width},

    {"LastPhoneBeamWidth", "Last Phone Beam Width", "-lpbeam",
     FLOAT, (caddr_t) & last_phone_beam_width},

    {"PhoneInsertionPenalty", "Penalty for each phone used", "-phnpen",
     FLOAT, (caddr_t) & phone_insertion_penalty},

    {"InsertionPenalty", "Penalty for word transitions", "-inspen",
     FLOAT, (caddr_t) & insertion_penalty},

    {"NewWordPenalty", "Penalty for new word transitions", "-nwpen",
     FLOAT, (caddr_t) & nw_pen},

    {"SilenceWordPenalty", "Penalty for silence word transitions",
     "-silpen",
     FLOAT, (caddr_t) & silence_word_penalty},

    {"FillerWordPenalty", "Penalty for filler word transitions",
     "-fillpen",
     FLOAT, (caddr_t) & filler_word_penalty},

    {"LanguageWeight", "Weighting on Language Probabilities", "-langwt",
     FLOAT, (caddr_t) & fwdtree_lw},

    {"RescoreLanguageWeight", "LM prob weight for rescoring pass",
     "-rescorelw",
     FLOAT, (caddr_t) & bestpath_lw},

    {"FwdFlatLanguageWeight",
     "FwdFlat Weighting on Language Probabilities", "-fwdflatlw",
     FLOAT, (caddr_t) & fwdflat_lw},

    {"FwdTree", "Fwd tree search (1st pass)", "-fwdtree",
     BOOL, (caddr_t) & fwdtree_flag},

    {"FwdFlat", "Flat fwd search over fwdtree lattice", "-fwdflat",
     BOOL, (caddr_t) & fwdflat_flag},

    {"ForwardOnly", "Run only the forward pass", "-forwardonly",
     BOOL, (caddr_t) & forward_only},

    {"Bestpath", "Shortest path search over lattice", "-bestpath",
     BOOL, (caddr_t) & bestpath_flag},

    {"TrigramInFwdPass", "Use trigram (if available) in forward pass",
     "-fwd3g",
     BOOL, (caddr_t) & use_3g_in_fwd_pass},

    {"SkipAltFrames", "Skip alternate frames in exiting phones",
     "-skipalt",
     INT, (caddr_t) & skip_alt_frm},

    {"WriteScoreInMatchFile", "write score in the match file",
     "-matchscore",
     BOOL, (caddr_t) & writeScoreInMatchFile},

    {"LatticeSizes", "BP and FP Tables Sizes", "-latsize",
     INT, (caddr_t) & lattice_size},

    {"LMCacheNumLines", "No. lines in LM cache", "-lmcachelines",
     INT, (caddr_t) & lm_cache_lines},
    {"DumpLattice", "Dump Lattice", "-dumplatdir",
     STRING, (caddr_t) & dumplat_dir},

    {"SamplingRate", "Sampling rate", "-samp",
     INT, (caddr_t) & sampling_rate},

    {"NumMelFilters", "Number of mel filters", "-nfilt",
     INT, (caddr_t) & n_mel_filt},

    {"LowerMelFilter", "Lower edge of mel filters", "-lowerf",
     FLOAT, (caddr_t) & lower_filt},

    {"UpperMelFilter", "Upper edge of mel filters", "-upperf",
     FLOAT, (caddr_t) & upper_filt},

    {"PreEmphasisAlpha", "Alpha coefficient for pre-emphasis", "-alpha",
     FLOAT, (caddr_t) & pre_emphasis_alpha},

    {"FrameRate", "Frame rate (number of frames per second)", "-frate",
     INT, (caddr_t) & frame_rate},

    {"NFFT", "Number of points for FFT", "-nfft",
     INT, (caddr_t) & n_fft},

    {"WindowLength", "Window length (in seconds) for FFT", "-wlen",
     FLOAT, (caddr_t) & window_length},

    {"UseADCInput", "Use raw ADC input", "-adcin",
     BOOL, (caddr_t) & adc_input},

    {"ADCFileExt", "ADC file extension", "-adcext",
     STRING, (caddr_t) & adc_ext},

    {"ADCByteOrder", "ADC file byte order (0:BIG/1:LITTLE)", "-adcendian",
     INT, (caddr_t) & adc_endian},

    {"ADCHdrSize", "ADC file header size", "-adchdr",
     INT, (caddr_t) & adc_hdr},

    {"UseDoubleBW", "Double bandwidth mel filter", "-doublebw",
     BOOL, (caddr_t) & doublebw},

    {"RawLogDir", "Log directory for raw output files)", "-rawlogdir",
     STRING, (caddr_t) & rawlogdir},

    {"MFCLogDir", "Log directory for MFC output files)", "-mfclogdir",
     STRING, (caddr_t) & mfclogdir},

    {"TimeAlignCtlFile", "Time align control file", "-tactlfn",
     STRING, (caddr_t) & time_align_ctl_file_name},

    {"TimeAlignWord", "Time Align Phone", "-taword",
     BOOL, (caddr_t) & time_align_word},

    {"TimeAlignPhone", "Time Align Phone", "-taphone",
     BOOL, (caddr_t) & time_align_phone},

    {"TimeAlignState", "Time Align State", "-tastate",
     BOOL, (caddr_t) & time_align_state},

    {"SegFileExt", "Seg file extension", "-segext",
     STRING, (caddr_t) & seg_file_ext},

    {"OutSentFile", "output sentence file name", "-osentfn",
     STRING, (caddr_t) & out_sent_filename},

    {"PrintBackTrace", "Print Back Trace", "-backtrace",
     BOOL, (caddr_t) & print_back_trace},

    {"PrintBackTrace", "Print Back Trace", "-shortbacktrace",
     BOOL, (caddr_t) & print_short_back_trace},

    {"MaxWordsPerFrame", "Limit words exiting per frame to this number",
     "-maxwpf",
     INT, (caddr_t) & maxwpf},

    {"MaxHMMPerFrame",
     "Limit HMMs evaluated per frame to this number (approx)", "-maxhmmpf",
     INT, (caddr_t) & maxhmmpf},

    {"VerbosityLevel", "Verbosity Level", "-verbose",
     INT, (caddr_t) & verbosity_level},

    {"FSGForceFinalState", "Force backtrace from FSG final state",
     "-fsgbfs",
     BOOL, (caddr_t) & fsg_backtrace_finalstate},

    {"CalcPhonePerplexity", "Calculate phone perplexity (uses FPU)",
     "-phperp",
     BOOL, (caddr_t) & calc_phone_perp},

    {0, 0, 0, NOTYPE, 0}
};

search_hyp_t *run_sc_utterance(char *mfcfile, int32 sf, int32 ef,
                               char *idspec);

static int32
nextarg(char *line, int32 * start, int32 * len, int32 * next)
{
    int32 i, lineLen;

    lineLen = strlen(line);

    /* Find first non-space character */
    for (i = 0; isspace((unsigned char) line[i]) && i < lineLen; i++);

    if (i == lineLen)
        return 1;

    if (line[i] == '"') {
        /* NOTE: No escape characters mechanism within quoted string */
        i++;
        for (*start = i; line[i] != '"' && i < lineLen; i++);
        if (line[i] != '"')
            return 1;
        *len = i - *start;
        *next = i + 1;
        return 0;
    }
    else {
        for (*start = i; !isspace((unsigned char) line[i]) && i < lineLen;
             i++);
        *len = i - *start;
        *next = i;
        return 0;
    }
}

/*
 * Read arguments from file and append to command line arguments.
 * NOTE: Spaces inside arguments, and quote marks around arguments not handled correctly.
 */
static int32
argfile_read(const int32 argc, char ***argv, const char *argfile)
{
    FILE *fp;
    int32 i, narg, len;
    char argstr[1024];
    char argline[4096];
    char **newargv;
    char *lp;
    int32 start, next;

    if ((fp = fopen(argfile, "r")) == NULL)
        E_FATAL("fopen(%s,r) failed\n", argfile);

    /* Count arguments */
    narg = 0;
    while (fgets(argline, sizeof(argline), fp) != NULL) {
        if (argline[0] == '#')
            continue;

        lp = argline;

        while (!nextarg(lp, &start, &len, &next)) {
            lp = lp + next;
            narg++;
        }
    }
    rewind(fp);

    /* Allocate space for arguments */
    narg += argc;
    newargv = (char **) malloc(narg * sizeof(char *));
    if (newargv == NULL)
        E_FATAL("malloc failed\n");

    /* Read arguments */
    newargv[0] = (*argv)[0];
    i = 1;
    while (fgets(argline, sizeof(argline), fp) != NULL) {
        if (argline[0] == '#')
            continue;

        lp = argline;
        while (!nextarg(lp, &start, &len, &next)) {
            assert(i < narg);
            strncpy(argstr, lp + start, len);
            argstr[len] = '\0';
            lp = lp + next;
            newargv[i] = ckd_salloc(argstr);

            i++;
        }
    }
    fclose(fp);
    assert(i == narg - argc + 1);

    /* Copy initial argument list at the end */
    for (i = 1, narg -= (argc - 1); i < argc; i++, narg++)
        newargv[narg] = (*argv)[i];

    *argv = newargv;
    return (narg);
}

static void
init_feat(void)
{
    feat_t *fcb;
    agc_type_t agc;
    cmn_type_t norm;

    agc = AGC_NONE;
    if (agcNoise)
        agc = AGC_NOISE;
    else if (agcMax)
        agc = AGC_MAX;
    else if (agcEMax)
        agc = AGC_EMAX;
    if ((!ctl_file_name) && live && (agc != AGC_NONE) && (agc != AGC_EMAX)) {
        agc = AGC_EMAX;
        E_INFO("Live mode; AGC set to AGC_EMAX\n");
    }

    norm = CMN_NONE;
    if (normalizeMean)
        norm = normalizeMeanPrior ? CMN_PRIOR : CMN_CURRENT;
    if ((!ctl_file_name) && live && (norm == CMN_CURRENT)) {
        norm = CMN_PRIOR;
        E_INFO("Live mode; CMN set to CMN_PRIOR\n");
    }

    fcb = feat_init("s2_4x", norm, 0, agc, 1);

    if (agcNoise || agcMax) {
        agc_set_threshold(fcb->agc_struct, agcThresh);
    }

    uttproc_set_feat(fcb);
}


static int32 final_argc;
static char **final_argv;
static FILE *logfp = NULL;
static char logfile[4096];      /* Hack!! Hardwired constant 4096 */

static void
log_arglist(FILE * fp, int32 argc, char *argv[])
{
    int32 i;

    /* Log the arguments */
    for (i = 0; i < argc; i++) {
        if (argv[i][0] == '-')
            fprintf(fp, "\\\n ");
        fprintf(fp, "%s ", argv[i]);
    }
    fprintf(fp, "\n\n");
    fflush(fp);
}

/* Should be in uttproc.c, but ... */
int32
uttproc_set_logfile(char const *file)
{
    FILE *fp;

    E_INFO("uttproc_set_logfile(%s)\n", file);

    if ((fp = fopen(file, "w")) == NULL) {
        E_ERROR("fopen(%s,w) failed; logfile unchanged\n", file);
        return -1;
    }
    else {
        if (logfp)
            fclose(logfp);

        logfp = fp;
        /* 
         * Rolled back the dup2() bug fix for windows only. In
         * Microsoft Visual C, dup2 seems to cause problems in some
         * applications: the files are opened, but nothing is written
         * to it.
         */
#ifdef WIN32
        *stdout = *logfp;
        *stderr = *logfp;
#else
        dup2(fileno(logfp), 1);
        dup2(fileno(logfp), 2);
#endif

        E_INFO("Previous logfile: '%s'\n", logfile);
        strcpy(logfile, file);

        log_arglist(logfp, final_argc, final_argv);
    }

    return 0;
}

int
fbs_init(int32 argc, char **argv)
{
    /* Parse command line arguments */
    pconf(argc, argv, param, 0, 0, 0);
    if (arg_file) {
        /* Read arguments from argfile */
        argc = argfile_read(argc, &argv, arg_file);
        pconf(argc, argv, param, 0, 0, 0);
    }
    final_argc = argc;
    final_argv = argv;

    /* Open logfile if specified; else log to stdout/stderr */
    logfile[0] = '\0';
    if (logfn_arg) {
        if ((logfp = fopen(logfn_arg, "w")) == NULL) {
            E_ERROR("fopen(%s,w) failed; logging to stdout/stderr\n",
                    logfn_arg);
        }
        else {
            strcpy(logfile, logfn_arg);
            /* 
             * Rolled back the dup2() bug fix for windows only. In
             * Microsoft Visual C, dup2 seems to cause problems in
             * some applications: the files are opened, but nothing is
             * written to it.
             */
#ifdef WIN32
            *stdout = *logfp;
            *stderr = *logfp;
#else
            dup2(fileno(logfp), 1);
            dup2(fileno(logfp), 2);
#endif
        }
    }

    /* These hardwired verbosity level constants should be changed! */
    if (verbosity_level >= 2)
        log_arglist(stdout, argc, argv);

#ifndef WIN32
    if (verbosity_level >= 2) {
        system("hostname");
        system("date");
        printf("\n\n");
    }
#endif

    E_INFO("libfbs/main COMPILED ON: %s, AT: %s\n\n", __DATE__, __TIME__);

    /* Compatibility with old forwardonly flag */
    if (forward_only)
        bestpath_flag = FALSE;
    if ((!fwdtree_flag) && (!fwdflat_flag))
        E_FATAL
            ("At least one of -fwdtree and -fwdflat flags must be TRUE\n");

    /* Load the KB */
    kb(argc, argv, insertion_penalty, fwdtree_lw, phone_insertion_penalty);

    search_initialize();

    search_set_beam_width(beam_width);
    search_set_new_word_beam_width(new_word_beam_width);
    search_set_new_phone_beam_width(new_phone_beam_width);
    search_set_last_phone_beam_width(last_phone_beam_width);
    search_set_lastphone_alone_beam_width(lastphone_alone_beam_width);
    search_set_silence_word_penalty(silence_word_penalty,
                                    phone_insertion_penalty);
    search_set_filler_word_penalty(filler_word_penalty,
                                   phone_insertion_penalty);
    search_set_newword_penalty(nw_pen);
    search_set_lw(fwdtree_lw, fwdflat_lw, bestpath_lw);
    search_set_ip(insertion_penalty);
    search_set_skip_alt_frm(skip_alt_frm);
    search_set_fwdflat_bw(fwdflat_beam_width, fwdflat_new_word_beam_width);

    /* Initialize dynamic data structures needed for utterance processing */
    uttproc_init();

    /* Initialize feature computation. */
    init_feat();

    if (rawlogdir)
        uttproc_set_rawlogdir(rawlogdir);
    if (mfclogdir)
        uttproc_set_mfclogdir(mfclogdir);

    /* If multiple LMs present, choose the unnamed one by default */
    if (kb_get_fsg_file_name() == NULL) {
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

    if (allphone_mode)
        allphone_init(beam_width, new_word_beam_width,
                      phone_insertion_penalty);

    E_INFO("libfbs/main COMPILED ON: %s, AT: %s\n\n", __DATE__, __TIME__);

    /*
     * Initialization complete; If there was a control file run batch
     */

    if (ctl_file_name) {
        if (!time_align_ctl_file_name)
            run_ctl_file(ctl_file_name);
        else
            run_time_align_ctl_file(ctl_file_name,
                                    time_align_ctl_file_name,
                                    out_sent_filename);

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

FILE *
adcfile_open(char const *utt)
{
    char inputfile[MAXPATHLEN];
    FILE *uttfp;
    int32 n, l;

    n = strlen(adc_ext);
    l = strlen(utt);
    if ((l > n + 1) && (utt[l - n - 1] == '.')
        && (strcmp(utt + l - n, adc_ext) == 0))
        adc_ext = "";          /* Extension already exists */

    /* Build input filename */
#ifdef WIN32
    if (data_directory && (utt[0] != '/') && (utt[0] != '\\') &&
        ((utt[0] != '.') || ((utt[1] != '/') && (utt[1] != '\\'))))
        sprintf(inputfile, "%s/%s.%s", data_directory, utt, adc_ext);
    else
        sprintf(inputfile, "%s.%s", utt, adc_ext);
#else
    if (data_directory && (utt[0] != '/')
        && ((utt[0] != '.') || (utt[1] != '/')))
        sprintf(inputfile, "%s/%s.%s", data_directory, utt, adc_ext);
    else
        sprintf(inputfile, "%s.%s", utt, adc_ext);
#endif

    if ((uttfp = fopen(inputfile, "rb")) == NULL) {
        E_FATAL("fopen(%s,rb) failed\n", inputfile);
    }
    if (adc_hdr > 0) {
        if (fseek(uttfp, adc_hdr, SEEK_SET) < 0) {
            E_ERROR("fseek(%s,%d) failed\n", inputfile, adc_hdr);
            fclose(uttfp);
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

    if (strcmp(ctl_file_name, "-") != 0)
        ctl_fs = myfopen(ctl_file_name, "r");
    else
        ctl_fs = stdin;

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

        if (!allphone_mode) {
            hyp = run_sc_utterance(mfcfile, sf, ef, idspec);
            if (hyp && print_short_back_trace) {
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

    time_align_init();
    beam_width = 1e-9f;
    time_align_set_beam_width(beam_width);
    E_INFO("****** USING WIDE BEAM ****** (1e-9)\n");

    utt_ctl_fs = myfopen(utt_ctl_file_name, "r");
    pe_ctl_fs = myfopen(pe_ctl_file_name, "r");

    if (out_sent_file_name) {
        out_sent_fs = myfopen(out_sent_file_name, "w");
    }
    else
        out_sent_fs = NULL;

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
search_hyp_t *
run_sc_utterance(char *mfcfile, int32 sf, int32 ef, char *idspec)
{
    char startword_filename[1000];
    FILE *sw_fp;
    int32 frmcount, ret;
    char *finalhyp;
    char utt[1024];
    search_hyp_t *hypseg;

    strcpy(utt, idspec);
    build_uttid(utt);

    if (nbest > 0)
        bestpath_flag = 1;      /* Force creation of DAG */

    /* Select the LM for utt */
    if (get_n_lm() > 1) {
        FILE *lmname_fp;
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
#ifndef WIN32
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

#ifndef WIN32
#ifndef _HPUX_SOURCE
    getrusage(RUSAGE_SELF, &start);
#endif                          /* _HPUX_SOURCE */
    gettimeofday(&e_start, 0);
#endif                          /* WIN32 */

    if (time_align_word_sequence(utt, left_word, pe_words, right_word) ==
        0) {
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

#ifndef WIN32
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
#endif                          /* WIN32 */
}

char const *
get_current_startword(void)
{
    return startWord;
}

int32
query_compute_all_senones(void)
{
    return compute_all_senones;
}

int32
query_lm_cache_lines(void)
{
    return (lm_cache_lines);
}

int32
query_report_altpron(void)
{
    return (report_altpron);
}

int32
query_lattice_size(void)
{
    return (lattice_size);
}

char const *
query_match_file_name(void)
{
    return (match_file_name);
}

char const *
query_matchseg_file_name(void)
{
    return (matchseg_file_name);
}

int32
query_fwdtree_flag(void)
{
    return fwdtree_flag;
}

int32
query_fwdflat_flag(void)
{
    return fwdflat_flag;
}

int32
query_bestpath_flag(void)
{
    return bestpath_flag;
}

int32
query_topsen_window(void)
{
    return topsen_window;
}

int32
query_topsen_thresh(void)
{
    return topsen_thresh;
}

int32
query_blocking_ad_read(void)
{
    return (blocking_ad_read_p);
}

char const *
query_dumplat_dir(void)
{
    return dumplat_dir;
}

int32
query_adc_input(void)
{
    return adc_input;
}

int32
set_adc_input(int32 value)
{
    int32 old_value;

    old_value = adc_input;
    adc_input = value;
    return old_value;
}

void
query_fe_params(param_t * param)
{
    param->SAMPLING_RATE = sampling_rate;
    param->PRE_EMPHASIS_ALPHA = DEFAULT_PRE_EMPHASIS_ALPHA;
    param->WINDOW_LENGTH = DEFAULT_WINDOW_LENGTH;
    /* NOTE! This could be 256 for 8000Hz, but that requires that
     * the acoustic models be retrained, and most 8k models are
     * using 512 points.  So we will use DEFAULT_FFT_SIZE (512)
     * everywhere. */
    param->FFT_SIZE = DEFAULT_FFT_SIZE;
    param->doublebw = doublebw;
    if (param->doublebw) {
        E_INFO("Will use double bandwidth in mel filter\n");
    }
    else {
        E_INFO("Will not use double bandwidth in mel filter\n");
    }

    /* Provide some defaults based on sampling rate if the user
     * hasn't. */
    switch (sampling_rate) {
    case 16000:
        param->FRAME_RATE = 100;
        param->NUM_FILTERS = DEFAULT_BB_NUM_FILTERS;
        param->LOWER_FILT_FREQ = DEFAULT_BB_LOWER_FILT_FREQ;
        param->UPPER_FILT_FREQ = DEFAULT_BB_UPPER_FILT_FREQ;
        break;
    case 11025:
        /* Numbers from CALO project meeting data. */
        param->FRAME_RATE = 105;
        param->NUM_FILTERS = 36;
        param->LOWER_FILT_FREQ = 130;
        param->UPPER_FILT_FREQ = 5400;
        break;
    case 8000:
        param->FRAME_RATE = 100;
        param->NUM_FILTERS = DEFAULT_NB_NUM_FILTERS;
        param->LOWER_FILT_FREQ = DEFAULT_NB_LOWER_FILT_FREQ;
        param->UPPER_FILT_FREQ = DEFAULT_NB_UPPER_FILT_FREQ;
        break;
    }

    if (n_mel_filt != 0)
        param->NUM_FILTERS = n_mel_filt;
    if (lower_filt != -1.0f)
        param->LOWER_FILT_FREQ = lower_filt;
    if (upper_filt != 0.0f)
        param->UPPER_FILT_FREQ = upper_filt;
    if (pre_emphasis_alpha != 0.0f)
        param->PRE_EMPHASIS_ALPHA = pre_emphasis_alpha;
    if (window_length != 0.0f)
        param->WINDOW_LENGTH = window_length;
    if (frame_rate != 0)
        param->FRAME_RATE = frame_rate;
    if (n_fft != 0)
        param->FFT_SIZE = n_fft;
}

int32
query_sampling_rate(void)
{
    return sampling_rate;
}

int32
query_doublebw(void)
{
    return doublebw;
}

char const *
query_ctlfile_name(void)
{
    return ctl_file_name;
}

int32
query_phone_conf(void)
{
    return phone_conf;
}

char *
query_pscr2lat(void)
{
    return pscr2lat;
}

int32
query_maxwpf(void)
{
    return maxwpf;
}

int32
query_maxhmmpf(void)
{
    return maxhmmpf;
}

int32
query_back_trace(void)
{
    return print_back_trace;
}

int32
query_ctl_offset(void)
{
    return ctl_offset;
}

int32
query_ctl_count(void)
{
    return ctl_count;
}

int32
query_report_partial_result(void)
{
    return report_partial_result;
}

int32
query_report_partial_result_seg(void)
{
    return report_partial_result_seg;
}

int32
query_fsg_backtrace_finalstate(void)
{
    return fsg_backtrace_finalstate;
}
