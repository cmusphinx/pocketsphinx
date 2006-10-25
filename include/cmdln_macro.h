/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 2006 Carnegie Mellon University.  All rights
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

/* cmdln_macro.h - Command line definitions for PocketSphinx */

#ifndef __PS_CMDLN_MACRO_H__
#define __PS_CMDLN_MACRO_H__

#include <cmd_ln.h>
#include <fe.h> /* For waveform_to_cepstral_command_line_macro() */

/** Options defining speech data input */
#define input_cmdln_options()								\
{ "-live",										\
      ARG_BOOLEAN,									\
      "no",										\
      "Get input from audio hardware" },						\
{ "-ctl",										\
      ARG_STRING,									\
      NULL,										\
      "Control file listing utterances to be processed" },				\
{ "-ctloffset",										\
      ARG_INT32,									\
      "0",										\
      "No. of utterances at the beginning of -ctl file to be skipped" },		\
{ "-ctlcount",										\
      ARG_INT32,									\
      "1000000000",	/* A big number to approximate the default: "until EOF" */	\
      "No. of utterances to be processed (after skipping -ctloffset entries)" },	\
{ "-ctlincr",										\
      ARG_INT32,									\
      "1",										\
      "Do every Nth line in the control file" },					\
{ "-adcin",										\
      ARG_BOOLEAN,									\
      "no",										\
      "Input is raw audio data" },							\
{ "-adcendian",										\
      ARG_STRING,									\
      "little",										\
      "Byte order for raw audio files (little/big)" },					\
{ "-adchdr",										\
      ARG_INT32,									\
      "0",										\
      "Size of audio file header in bytes (headers are ignored)" },			\
{ "-cepdir",										\
      ARG_STRING,									\
      NULL,										\
      "Input files directory (prefixed to filespecs in control file)" },		\
{ "-cepext",										\
      ARG_STRING,									\
      ".mfc",										\
      "Input files extension (prefixed to filespecs in control file)" },		\
{ "-rawlogdir",										\
      ARG_STRING,									\
      NULL,										\
      "Directory for dumping raw audio input files" },					\
{ "-mfclogdir",										\
      ARG_STRING,									\
      NULL,										\
      "Directory for dumping feature input files" },					\
{ "-cmn",										\
      ARG_STRING,									\
      "current",									\
      "Cepstral mean normalization scheme ('current', 'prior', or 'none')" },		\
{ "-varnorm",										\
      ARG_BOOLEAN,									\
      "no",										\
      "Variance normalize each utterance (only if CMN == current)" },			\
{ "-agc",										\
      ARG_STRING,									\
      "none",										\
      "Automatic gain control for c0 ('max', 'emax', 'noise', or 'none')" },		\
{ "-agcthresh",										\
      ARG_FLOAT32,									\
      "2.0",										\
      "Initial threshold for automatic gain control" }

/** Options defining recognition data output and logging */
#define output_cmdln_options()							\
{ "-logfn",									\
      ARG_STRING,								\
      NULL,									\
      "Recognition log file name" },						\
{ "-backtrace",									\
      ARG_BOOLEAN,								\
      "yes",									\
      "Print back trace of recognition results" },				\
{ "-shortbacktrace",								\
      ARG_BOOLEAN,								\
      "no",									\
      "Print short back trace of recognition results" },			\
{ "-phypdump",									\
      ARG_BOOLEAN,								\
      "no",									\
      "Report partial results every so many frames" },				\
{ "-phypsegdump",								\
      ARG_BOOLEAN,								\
      "no",									\
      "Report detailed partial results every so many frames" },			\
{ "-hyp",									\
      ARG_STRING,								\
      NULL,									\
      "Recognition output file name" },						\
{ "-hypseg",									\
      ARG_STRING,								\
      NULL,									\
      "Recognition output with segmentation file name" },			\
{ "-matchscore",								\
      ARG_BOOLEAN,								\
      "no",									\
      "Report score in hyp file" },						\
{ "-reportpron",								\
      ARG_BOOLEAN,								\
      "no",									\
      "Report alternate pronunciations in match file" },			\
{ "-pscr2lat",									\
      ARG_STRING,								\
      NULL,									\
      "Directory for writing phone lattices based on best senone scores" },	\
{ "-nbestdir",									\
      ARG_STRING,								\
      NULL,									\
      "Directory for writing N-best hypothesis lists" },			\
{ "-nbestext",									\
      ARG_STRING,								\
      "hyp",									\
      "Extension for N-best hypothesis list files" },				\
{ "-nbest",									\
      ARG_INT32,								\
      "0",									\
      "Number of N-best hypotheses to write to -nbestdir" },			\
{ "-dumplatdir",								\
      ARG_STRING,								\
      NULL,									\
      "Directory for dumping lattices" }

/** Options defining beam width parameters for tuning the search. */
#define beam_cmdln_options()									\
{ "-beam",											\
      ARG_FLOAT64,										\
      "1e-48",											\
      "Beam width applied to every frame in Viterbi search (smaller values mean wider beam)" },	\
{ "-wbeam",											\
      ARG_FLOAT64,										\
      "1e-48",											\
      "Beam width applied to word exits" },							\
{ "-pbeam",											\
      ARG_FLOAT64,										\
      "1e-48",											\
      "Beam width applied to phone transitions" },						\
{ "-lpbeam",											\
      ARG_FLOAT64,										\
      "1e-40",											\
      "Beam width applied to last phone in words" },						\
{ "-lponlybeam",										\
      ARG_FLOAT64,										\
      "1e-28",											\
      "Beam width applied to last phone in single-phone words" },				\
{ "-fwdflatbeam",										\
      ARG_FLOAT64,										\
      "1e-64",											\
      "Beam width applied to every frame in second-pass flat search" },				\
{ "-fwdflatwbeam",										\
      ARG_FLOAT64,										\
      "1e-28",											\
      "Beam width applied to word exits in second-pass flat search" }

/** Options defining other parameters for tuning the search. */
#define search_cmdln_options()                                                                  \
{ "-compallsen",                                                                                \
      ARG_BOOLEAN,                                                                              \
      "no",                                                                                     \
      "Compute all senone scores in every frame (can be faster when there are many senones)" }, \
{ "-topsenfrm",                                                                                 \
      ARG_INT32,                                                                                \
      "1",                                                                                      \
      "Number of frames' top senones to use in phoneme lookahead (needs -compallsen yes)" },    \
{ "-topsenthresh",                                                                              \
      ARG_INT32,                                                                                \
      "-60000",                                                                                 \
      "Threshold for top senones to use in phoneme lookahead" },                                \
{ "-fwdtree",                                                                                   \
      ARG_BOOLEAN,                                                                              \
      "yes",                                                                                    \
      "Run forward lexicon-tree search (1st pass)" },                                           \
{ "-fwdflat",                                                                                   \
      ARG_BOOLEAN,                                                                              \
      "yes",                                                                                    \
      "Run forward flat-lexicon search over word lattice (2nd pass)" },                         \
{ "-bestpath",                                                                                  \
      ARG_BOOLEAN,                                                                              \
      "yes",                                                                                    \
      "Run bestpath (Dijkstra) search over word lattice (3rd pass)" },                          \
{ "-fwd3g",                                                                                     \
      ARG_BOOLEAN,                                                                              \
      "yes",                                                                                    \
      "Use trigrams in first pass search" },                                                    \
{ "-skipalt",                                                                                   \
      ARG_BOOLEAN,                                                                              \
      "no",                                                                                     \
      "Skip alternate frames in exiting phones" },                                              \
{ "-latsize",                                                                                   \
      ARG_INT32,                                                                                \
      "50000",                                                                                  \
      "Lattice size" },                                                                         \
{ "-maxwpf",                                                                                    \
      ARG_INT32,                                                                                \
      "100000",                                                                                 \
      "Maximum number of distinct word exits to maintain at each frame (approx)" },             \
{ "-maxhmmpf",                                                                                  \
      ARG_INT32,                                                                                \
      "100000",                                                                                 \
      "Maximum number of active HMMs to maintain at each frame (approx)" },                     \
{ "-maxhistpf",                                                                                 \
      ARG_INT32,                                                                                \
      "100",                                                                                    \
      "Max no. of histories to maintain at each frame (UNUSED)" },                              \
{ "-fsgbfs",                                                                                    \
      ARG_BOOLEAN,                                                                              \
      "yes",                                                                                    \
      "Force backtrace from FSG final state"}

/** Command-line options for finite state grammars. */
#define fsg_cmdln_options()						\
{ "-fsg",								\
      ARG_STRING,							\
      NULL,								\
      "Finite state grammar"},						\
{ "-fsgusealtpron",							\
      ARG_BOOLEAN,							\
      "yes",								\
      "Use alternative pronunciations for FSG"},			\
{ "-fsgusefiller",							\
      ARG_BOOLEAN,							\
      "yes",								\
      "(FSG Mode (Mode 2) only) Insert filler words at each state."}, 	\
{ "-fsgctlfn", 								\
      ARG_STRING, 							\
      NULL, 								\
      "A finite state grammar control file" }

/** Command-line options for statistical language models. */
#define lm_cmdln_options()								\
{ "-lm",										\
      ARG_STRING,									\
      NULL,										\
      "Word trigram language model input file" },					\
{ "-lmctlfn",										\
      ARG_STRING,									\
      NULL,										\
      "Specify a set of language model\n"},						\
{ "-lmdumpdir",										\
      ARG_STRING,									\
      NULL,										\
      "The directory for dumping the DMP file. "},					\
{ "-lmstartsym",									\
      ARG_STRING,									\
      "<s>",										\
      "Language model start symbol" },							\
{ "-lmendsym",										\
      ARG_STRING,									\
      "</s>",										\
      "Language model end symbol" },							\
{ "-startsymfn",									\
      ARG_STRING,									\
      NULL,										\
      "Start symbols file name"},							\
{ "-lmnamedir",										\
      ARG_STRING,									\
      NULL,										\
      "Directory for LM-name file for each utt"},					\
{ "-lmnameext",										\
      ARG_STRING,									\
      NULL,										\
      "Filename extension for LM-name files"},						\
{ "-startworddir",									\
      ARG_STRING,									\
      NULL,										\
      "Directory for start-word file for each utt"},					\
{ "-startwordext",									\
      ARG_STRING,									\
      NULL,										\
      "Filename extension for start-word files"},					\
{ "-lw",										\
      ARG_FLOAT32,									\
      "6.5",										\
      "Language model probability weight" },						\
{ "-fwdflatlw",										\
      ARG_FLOAT32,									\
      "8.5",										\
      "Language model probability weight for flat lexicon (2nd pass) decoding" },	\
{ "-bestpathlw",									\
      ARG_FLOAT32,									\
      "9.5",										\
      "Language model probability weight for bestpath search" },			\
{ "-wip",										\
      ARG_FLOAT32,									\
      "0.65",										\
      "Word insertion penalty" },							\
{ "-nwpen",										\
      ARG_FLOAT32,									\
      "1.0",										\
      "New word transition penalty" },							\
{ "-pip",										\
      ARG_FLOAT32,									\
      "1.0",										\
      "Phone insertion penalty" },							\
{ "-uw",										\
      ARG_FLOAT32,									\
      "1.0",										\
      "Unigram weight" }, 								\
{ "-silpen",										\
      ARG_FLOAT32,									\
      "0.005",										\
      "Silence word transition penalty" },						\
{ "-fillpen",										\
      ARG_FLOAT32,									\
      "1e-8",										\
      "Filler word transition penalty" }

/** Command-line options for dictionaries. */
#define dictionary_cmdln_options()				\
    { "-dict",							\
      REQARG_STRING,						\
      NULL,							\
      "Main pronunciation dictionary (lexicon) input file" },	\
    { "-fdict",							\
      ARG_STRING,						\
      NULL,							\
      "Noise word pronunciation dictionary input file" },	\
    { "-oovdict",						\
      ARG_STRING,						\
      NULL,							\
      "OOV dictionary input file" },				\
    { "-perdict",						\
      ARG_STRING,						\
      NULL,							\
      "Personal dictionary input file" },			\
    { "-pdict",							\
      ARG_STRING,						\
      NULL,							\
      "Noise dictionary input file" },				\
    { "-maxnewoov",						\
      ARG_INT32,						\
      "0",							\
      "Maximum new OOVs that can be added at run time" },	\
    { "-oovugprob",						\
      ARG_FLOAT32,						\
      "-4.5",							\
      "OOV unigram log (base 10) probability" },		\
    { "-usewdphones",						\
      ARG_BOOLEAN,						\
      "no",							\
      "Use within-word phones only" }

/** Command-line options for acoustic modeling */
#define am_cmdln_options()                                                      \
{ "-feat",                                                                      \
      ARG_STRING,                                                               \
      "s2_4x",                                                                  \
      "Feature stream type: (many options, must be s2_4x for now)" },           \
{ "-ceplen",                                                                    \
      ARG_INT32,                                                                \
      "13",                                                                     \
     "Number of components in the input feature vector (must be 13 for now)" }, \
{ "-hmm",                                                                       \
      ARG_STRING,                                                               \
      NULL,                                                                     \
      "Directory containing acoustic model files."},                            \
{ "-mdef",                                                                      \
      ARG_STRING,                                                               \
      NULL,                                                                     \
      "Model definition input file" },                                          \
{ "-tmat",                                                                      \
      ARG_STRING,                                                               \
      NULL,                                                                     \
      "HMM state transition matrix input file" },                               \
{ "-tmatfloor",                                                                 \
      ARG_FLOAT32,                                                              \
      "0.0001",                                                                 \
      "HMM state transition probability floor (applied to -tmat file)" },       \
{ "-mean",                                                                      \
      ARG_STRING,                                                               \
      NULL,                                                                     \
      "Mixture gaussian means input file" },                                    \
{ "-var",                                                                       \
      ARG_STRING,                                                               \
      NULL,                                                                     \
      "Mixture gaussian variances input file" },                                \
{ "-varfloor",                                                                  \
      ARG_FLOAT32,                                                              \
      "0.0001",                                                                 \
      "Mixture gaussian variance floor (applied to data from -var file)" },     \
{ "-mixw",                                                                      \
      ARG_STRING,                                                               \
      NULL,                                                                     \
      "Senone mixture weights input file" },                                    \
{ "-mixwfloor",                                                                 \
      ARG_FLOAT32,                                                              \
      "0.0000001",                                                              \
      "Senone mixture weights floor (applied to data from -mixw file)" },       \
{ "-sendump",                                                                   \
      ARG_STRING,                                                               \
      NULL,                                                                     \
      "Senone dump (compressed mixture weights) input file" },                  \
{ "-mmap",                                                                      \
      ARG_BOOLEAN,                                                              \
      "yes",                                                                    \
      "Use memory-mapped I/O (if possible) for model files" },                  \
{ "-dcep80msweight",                                                            \
      ARG_FLOAT32,                                                              \
      "1.0",                                                                    \
      "Weight for long-time (80ms) delta-cepstrum features" },                  \
{ "-dsratio",                                                                   \
      ARG_INT32,                                                                \
      "1",                                                                      \
      "Frame GMM computation downsampling ratio" },                             \
{ "-topn",                                                                      \
      ARG_INT32,                                                                \
      "4",                                                                      \
      "Number of top Gaussians to use in scoring" },                            \
{ "-kdtree",                                                                    \
      ARG_STRING,                                                               \
      NULL,                                                                     \
      "kd-Tree file for Gaussian selection" },                                  \
{ "-kdmaxdepth",                                                                \
      ARG_INT32,                                                                \
      "0",                                                                      \
      "Maximum depth of kd-Trees to use" },                                     \
{ "-kdmaxbbi",                                                                  \
      ARG_INT32,                                                                \
      "-1",                                                                     \
      "Maximum number of Gaussians per leaf node in kd-Trees" },                \
{ "-vqeval",                                                                    \
      ARG_INT32,                                                                \
      "3",                                                                      \
      "Number of subvectors to use for SubVQ-based"                             \
      " frame evaluation (3 for all) (UNUSED)"},                                \
{ "-logbase",                                                                   \
      ARG_FLOAT32,                                                              \
      "1.0001",                                                                 \
      "Base in which all log-likelihoods calculated (UNUSED)" }

/** Options for force alignment mode. */
#define time_align_cmdln_options()						\
{ "-tactl",									\
      ARG_STRING,								\
      NULL,									\
      "Time align control (input sentence) file" },				\
{ "-taword",									\
      ARG_BOOLEAN,								\
      "yes",									\
      "Time align words" },							\
{ "-taphone",									\
      ARG_BOOLEAN,								\
      "yes",									\
      "Time align phones" },							\
{ "-tastate",									\
      ARG_BOOLEAN,								\
      "no",									\
      "Time align states" },							\
{ "-outsent",									\
      ARG_STRING,								\
      NULL,									\
      "Output sentence file name" },						\
{ "-segdir",									\
      ARG_STRING,								\
      NULL,									\
      "Directory for writing state segmentation files from time alignment" },	\
{ "-segext",									\
      ARG_STRING,								\
      "v8_seg",									\
      "Extension for state segmentation files" },				\
{ "-phonelabdir",								\
      ARG_STRING,								\
      NULL,									\
      "Directory for writing phoneme label files from time alignment" },	\
{ "-phonelabext",								\
      ARG_STRING,								\
      "lab",									\
      "Extension for phoneme label files" },					\
{ "-phonelabtype",								\
      ARG_STRING,								\
      "xlabel",									\
      "Type of phoneme label files to write" }

/** Options for allphone mode. */
#define allphone_cmdln_options()					\
{ "-allphone",								\
      ARG_BOOLEAN,							\
      "no",								\
      "Do phoneme recognition" },					\
{"-phonetp",								\
     ARG_STRING,							\
     NULL,								\
     "Phone transition probabilities inputfile (default: flat probs)"},	\
{"-ptplw",								\
     ARG_FLOAT32,							\
     "5.0",								\
     "Weight (exponent) applied to phone transition probabilities"},	\
{"-uptpwt",								\
     ARG_FLOAT32,							\
     "0.001",								\
     "Uniform phone transition prob weight"}

#define CMDLN_EMPTY_OPTION { NULL, 0, NULL, NULL }

#endif /* __PS_CMDLN_MACRO_H__ */
