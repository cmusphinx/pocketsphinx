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

#ifdef WORDS_BIGENDIAN
#define NATIVE_ENDIAN "big"
#else
#define NATIVE_ENDIAN "little"
#endif

/** Options defining speech data input */
#define input_cmdln_options()                                           \
{ "-ctl",                                                               \
      ARG_STRING,                                                       \
      NULL,                                                             \
      "Control file listing utterances to be processed" },              \
{ "-ctloffset",                                                         \
      ARG_INT32,                                                        \
      "0",                                                              \
      "No. of utterances at the beginning of -ctl file to be skipped" }, \
{ "-ctlcount",                                                          \
      ARG_INT32,                                                        \
      "-1",                                                             \
      "No. of utterances to be processed (after skipping -ctloffset entries)" }, \
{ "-ctlincr",                                                           \
      ARG_INT32,                                                        \
      "1",                                                              \
      "Do every Nth line in the control file" },                        \
{ "-adcin",                                                             \
      ARG_BOOLEAN,                                                      \
      "no",                                                             \
      "Input is raw audio data" },                                      \
{ "-adchdr",                                                            \
      ARG_INT32,                                                        \
      "0",                                                              \
      "Size of audio file header in bytes (headers are ignored)" },     \
{ "-adcdev",                                                            \
      ARG_STRING,                                                       \
      NULL,                                                             \
      "Device name for audio input (platform-specific)" },              \
{ "-cepdir",                                                            \
      ARG_STRING,                                                       \
      NULL,                                                             \
      "Input files directory (prefixed to filespecs in control file)" }, \
{ "-cepext",                                                            \
      ARG_STRING,                                                       \
      ".mfc",                                                           \
      "Input files extension (suffixed to filespecs in control file)" }, \
{ "-rawlogdir",                                                         \
      ARG_STRING,                                                       \
      NULL,                                                             \
      "Directory for dumping raw audio" },	                        \
{ "-mfclogdir",                                                         \
      ARG_STRING,                                                       \
      NULL,                                                             \
      "Directory for dumping features" },	                        \
{ "-cmn",                                                               \
      ARG_STRING,                                                       \
      "current",                                                        \
      "Cepstral mean normalization scheme ('current', 'prior', or 'none')" }, \
{ "-cmninit",                                                           \
      ARG_STRING,                                                       \
      "8.0",                                                            \
      "Initial values (comma-separated) for cepstral mean when 'prior' is used" }, \
{ "-varnorm",                                                           \
      ARG_BOOLEAN,                                                      \
      "no",                                                             \
      "Variance normalize each utterance (only if CMN == current)" },   \
{ "-agc",                                                               \
      ARG_STRING,                                                       \
      "none",                                                           \
      "Automatic gain control for c0 ('max', 'emax', 'noise', or 'none')" }, \
{ "-agcthresh",                                                         \
      ARG_FLOAT32,                                                      \
      "2.0",                                                            \
      "Initial threshold for automatic gain control" },                 \
{ "-lda",                                                               \
      ARG_STRING,                                                       \
      NULL,                                                             \
      "File containing transformation matrix to be applied to features (single-stream features only)" }, \
{ "-ldadim",                                                            \
      ARG_INT32,                                                        \
      "0",                                                              \
      "Dimensionality of output of feature transformation (0 to use entire matrix)" }

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
{ "-outlatdir",									\
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
      "7e-29",											\
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
      "7e-29",											\
      "Beam width applied to last phone in single-phone words" },				\
{ "-fwdflatbeam",										\
      ARG_FLOAT64,										\
      "1e-64",											\
      "Beam width applied to every frame in second-pass flat search" },				\
{ "-fwdflatwbeam",										\
      ARG_FLOAT64,										\
      "7e-29",											\
      "Beam width applied to word exits in second-pass flat search" }

/** Options defining other parameters for tuning the search. */
#define search_cmdln_options()                                                                  \
{ "-compallsen",                                                                                \
      ARG_BOOLEAN,                                                                              \
      "no",                                                                                     \
      "Compute all senone scores in every frame (can be faster when there are many senones)" }, \
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
{ "-latsize",                                                                                   \
      ARG_INT32,                                                                                \
      "50000",                                                                                  \
      "Lattice size" },                                                                         \
{ "-maxwpf",                                                                                    \
      ARG_INT32,                                                                                \
      "-1",                                                                                     \
      "Maximum number of distinct word exits at each frame (or -1 for no pruning)" },           \
{ "-maxhmmpf",                                                                                  \
      ARG_INT32,                                                                                \
      "-1",                                                                                     \
      "Maximum number of active HMMs to maintain at each frame (or -1 for no pruning)" },       \
{ "-maxhistpf",                                                                                 \
      ARG_INT32,                                                                                \
      "100",                                                                                    \
      "Max no. of histories to maintain at each frame (UNUSED)" },                              \
{ "-fwdflatefwid",                                                                              \
      ARG_INT32,                                                                                \
      "4",                                                                     	                \
      "Minimum number of end frames for a word to be searched in fwdflat search" },             \
{ "-fwdflatsfwin",                                                                              \
      ARG_INT32,                                                                                \
      "25",                                                                    	                \
      "Window of frames in lattice to search for successor words in fwdflat search " }

/** Command-line options for finite state grammars. */
#define fsg_cmdln_options()                                     \
    { "-fsg",                                                   \
            ARG_STRING,                                         \
            NULL,                                               \
            "Sphinx format finite state grammar file"},         \
{ "-jsgf",                                                      \
        ARG_STRING,                                             \
        NULL,                                                   \
        "JSGF grammar file" },                                  \
{ "-toprule",                                                   \
        ARG_STRING,                                             \
        NULL,                                                   \
        "Start rule for JSGF (first public rule is default)" }, \
{ "-fsgusealtpron",                                             \
        ARG_BOOLEAN,                                            \
        "yes",                                                  \
        "Add alternate pronunciations to FSG"},                 \
{ "-fsgusefiller",                                              \
        ARG_BOOLEAN,                                            \
        "yes",                                                  \
        "Insert filler words at each state."}

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
{ "-lmname",										\
      ARG_STRING,									\
      "default",									\
      "Which language model in -lmctlfn to use by default"},				\
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
{ "-silprob",										\
      ARG_FLOAT32,									\
      "0.005",										\
      "Silence word transition probability" },						\
{ "-fillprob",										\
      ARG_FLOAT32,									\
      "1e-8",										\
      "Filler word transition probability" }

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
    { "-maxnewoov",						\
      ARG_INT32,						\
      "20",							\
      "Maximum new OOVs that can be added at run time" },	\
    { "-usewdphones",						\
      ARG_BOOLEAN,						\
      "no",							\
      "Use within-word phones only" }

/** Command-line options for acoustic modeling */
#define am_cmdln_options()                                                      \
{ "-feat",                                                                      \
      ARG_STRING,                                                               \
      "s2_4x",                                                                  \
      "Feature stream type, depends on the acoustic model" },                   \
{ "-ceplen",                                                                    \
      ARG_INT32,                                                                \
      "13",                                                                     \
     "Number of components in the input feature vector" },			\
{ "-hmm",                                                                       \
      ARG_STRING,                                                               \
      NULL,                                                                     \
      "Directory containing acoustic model files."},                            \
{ "-featparams",                                                                \
      ARG_STRING,                                                               \
      NULL,                                                                     \
      "File containing feature extraction parameters."},                        \
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
      "Senone mixture weights input file (uncompressed)" },                     \
{ "-mixwfloor",                                                                 \
      ARG_FLOAT32,                                                              \
      "0.0000001",                                                              \
      "Senone mixture weights floor (applied to data from -mixw file)" },       \
{ "-senclust",                                                                  \
      ARG_STRING,                                                               \
      NULL,                                                                     \
      "Clustered mixture weight distribution input file" },                     \
{ "-sendump",                                                                   \
      ARG_STRING,                                                               \
      NULL,                                                                     \
      "Senone dump (compressed mixture weights) input file" },                  \
{ "-mmap",                                                                      \
      ARG_BOOLEAN,                                                              \
      "yes",                                                                    \
      "Use memory-mapped I/O (if possible) for model files" },                  \
{ "-ds",                                                                        \
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
{ "-logbase",                                                                   \
      ARG_FLOAT32,                                                              \
      "1.0001",                                                                 \
      "Base in which all log-likelihoods calculated" }

#define CMDLN_EMPTY_OPTION { NULL, 0, NULL, NULL }

#endif /* __PS_CMDLN_MACRO_H__ */
