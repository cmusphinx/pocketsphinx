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

/* config_macro.h - Configuration definitions for PocketSphinx */

#ifndef __PS_CMDLN_MACRO_H__
#define __PS_CMDLN_MACRO_H__

/* Public headers. */
#include <pocketsphinx.h>

/* Necessary private headers. */
#include "fe/fe.h"
#include "feat/feat.h"

/**
 * Helper macro to stringify enums and other non-string values for
 * default arguments.
 **/
#define ARG_STRINGIFY(s) ARG_STRINGIFY1(s)
#define ARG_STRINGIFY1(s) #s


/** Minimal set of command-line options for PocketSphinx. */
#define POCKETSPHINX_OPTIONS \
    POCKETSPHINX_FE_OPTIONS,         \
        POCKETSPHINX_FEAT_OPTIONS,   \
        POCKETSPHINX_ACMOD_OPTIONS,  \
        POCKETSPHINX_BEAM_OPTIONS,   \
        POCKETSPHINX_SEARCH_OPTIONS, \
        POCKETSPHINX_DICT_OPTIONS,   \
        POCKETSPHINX_NGRAM_OPTIONS,  \
        POCKETSPHINX_FSG_OPTIONS,    \
        POCKETSPHINX_KWS_OPTIONS,    \
        POCKETSPHINX_DEBUG_OPTIONS

/** Options for debugging and logging. */
#define POCKETSPHINX_DEBUG_OPTIONS                      \
    { "logfn",                                         \
            ARG_STRING,                                 \
            NULL,                                       \
            "File to write log messages in" },          \
    { "loglevel",                                       \
            ARG_STRING,                                 \
            "WARN",                                       \
            "Minimum level of log messages (DEBUG, INFO, WARN, ERROR)" },\
     { "mfclogdir",                                    \
             ARG_STRING,                                \
             NULL,                                      \
             "Directory to log feature files to"        \
             },                                         \
    { "rawlogdir",                                     \
            ARG_STRING,                                 \
            NULL,                                       \
            "Directory to log raw audio files to" },    \
    { "senlogdir",                                     \
             ARG_STRING,                                \
             NULL,                                      \
             "Directory to log senone score files to"   \
             }

/** Options defining beam width parameters for tuning the search. */
#define POCKETSPHINX_BEAM_OPTIONS                                       \
{ "beam",                                                              \
      ARG_FLOATING,                                                      \
      "1e-48",                                                          \
      "Beam width applied to every frame in Viterbi search (smaller values mean wider beam)" }, \
{ "wbeam",                                                             \
      ARG_FLOATING,                                                      \
      "7e-29",                                                          \
      "Beam width applied to word exits" },                             \
{ "pbeam",                                                             \
      ARG_FLOATING,                                                      \
      "1e-48",                                                          \
      "Beam width applied to phone transitions" },                      \
{ "lpbeam",                                                            \
      ARG_FLOATING,                                                      \
      "1e-40",                                                          \
      "Beam width applied to last phone in words" },                    \
{ "lponlybeam",                                                        \
      ARG_FLOATING,                                                      \
      "7e-29",                                                          \
      "Beam width applied to last phone in single-phone words" },       \
{ "fwdflatbeam",                                                       \
      ARG_FLOATING,                                                      \
      "1e-64",                                                          \
      "Beam width applied to every frame in second-pass flat search" }, \
{ "fwdflatwbeam",                                                      \
      ARG_FLOATING,                                                      \
      "7e-29",                                                          \
      "Beam width applied to word exits in second-pass flat search" },  \
{ "pl_window",                                                         \
      ARG_INTEGER,                                                        \
      "5",                                                              \
      "Phoneme lookahead window size, in frames" },                     \
{ "pl_beam",                                                           \
      ARG_FLOATING,                                                      \
      "1e-10",                                                          \
      "Beam width applied to phone loop search for lookahead" },        \
{ "pl_pbeam",                                                          \
      ARG_FLOATING,                                                      \
      "1e-10",                                                          \
      "Beam width applied to phone loop transitions for lookahead" },   \
{ "pl_pip",                                                            \
      ARG_FLOATING,                                                      \
      "1.0",                                                            \
      "Phone insertion penalty for phone loop" },                       \
{ "pl_weight",                                                         \
      ARG_FLOATING,                                                      \
      "3.0",                                                            \
      "Weight for phoneme lookahead penalties" }                        \

/** Options defining other parameters for tuning the search. */
#define POCKETSPHINX_SEARCH_OPTIONS \
{ "compallsen",                                                                                \
      ARG_BOOLEAN,                                                                              \
      "no",                                                                                     \
      "Compute all senone scores in every frame (can be faster when there are many senones)" }, \
{ "fwdtree",                                                                                   \
      ARG_BOOLEAN,                                                                              \
      "yes",                                                                                    \
      "Run forward lexicon-tree search (1st pass)" },                                           \
{ "fwdflat",                                                                                   \
      ARG_BOOLEAN,                                                                              \
      "yes",                                                                                    \
      "Run forward flat-lexicon search over word lattice (2nd pass)" },                         \
{ "bestpath",                                                                                  \
      ARG_BOOLEAN,                                                                              \
      "yes",                                                                                    \
      "Run bestpath (Dijkstra) search over word lattice (3rd pass)" },                          \
{ "backtrace",                                                                                 \
      ARG_BOOLEAN,                                                                              \
      "no",                                                                                     \
      "Print results and backtraces to log." },                                                 \
{ "latsize",                                                                                   \
      ARG_INTEGER,                                                                                \
      "5000",                                                                                   \
      "Initial backpointer table size" },                                                       \
{ "maxwpf",                                                                                    \
      ARG_INTEGER,                                                                                \
      "-1",                                                                                     \
      "Maximum number of distinct word exits at each frame (or -1 for no pruning)" },           \
{ "maxhmmpf",                                                                                  \
      ARG_INTEGER,                                                                                \
      "30000",                                                                                  \
      "Maximum number of active HMMs to maintain at each frame (or -1 for no pruning)" },       \
{ "min_endfr",                                                                                 \
      ARG_INTEGER,                                                                                \
      "0",                                                                                      \
      "Nodes ignored in lattice construction if they persist for fewer than N frames" },        \
{ "fwdflatefwid",                                                                              \
      ARG_INTEGER,                                                                                \
      "4",                                                                     	                \
      "Minimum number of end frames for a word to be searched in fwdflat search" },             \
{ "fwdflatsfwin",                                                                              \
      ARG_INTEGER,                                                                                \
      "25",                                                                    	                \
      "Window of frames in lattice to search for successor words in fwdflat search " }

/** Command-line options for keyphrase spotting */
#define POCKETSPHINX_KWS_OPTIONS \
{ "keyphrase",                                                 \
         ARG_STRING,                                            \
         NULL,                                                  \
         "Keyphrase to spot"},                                  \
{ "kws",                                                       \
         ARG_STRING,                                            \
         NULL,                                                  \
         "A file with keyphrases to spot, one per line"},       \
{ "kws_plp",                                                   \
      ARG_FLOATING,                                              \
      "1e-1",                                                   \
      "Phone loop probability for keyphrase spotting" },          \
{ "kws_delay",                                                 \
      ARG_INTEGER,                                                \
      "10",                                                     \
      "Delay to wait for best detection score" },               \
{ "kws_threshold",                                             \
      ARG_FLOATING,                                              \
      "1e-30",                                                  \
      "Threshold for p(hyp)/p(alternatives) ratio" }

/** Command-line options for finite state grammars. */
#define POCKETSPHINX_FSG_OPTIONS \
    { "fsg",                                                   \
            ARG_STRING,                                         \
            NULL,                                               \
            "Sphinx format finite state grammar file"},         \
{ "jsgf",                                                      \
        ARG_STRING,                                             \
        NULL,                                                   \
        "JSGF grammar file" },                                  \
{ "toprule",                                                   \
        ARG_STRING,                                             \
        NULL,                                                   \
        "Start rule for JSGF (first public rule is default)" }, \
{ "fsgusealtpron",                                             \
        ARG_BOOLEAN,                                            \
        "yes",                                                  \
        "Add alternate pronunciations to FSG"},                 \
{ "fsgusefiller",                                              \
        ARG_BOOLEAN,                                            \
        "yes",                                                  \
        "Insert filler words at each state."}

/** Command-line options for statistical language models. */
#define POCKETSPHINX_NGRAM_OPTIONS \
{ "allphone",										\
      ARG_STRING,									\
      NULL,										\
      "Perform phoneme decoding with phonetic lm (given here)" },			\
{ "allphone_ci",									\
      ARG_BOOLEAN,									\
      "yes",										\
      "Perform phoneme decoding with phonetic lm and context-independent units only" }, \
{ "lm",										\
      ARG_STRING,									\
      NULL,										\
      "Word trigram language model input file" },					\
{ "lmctl",										\
      ARG_STRING,									\
      NULL,										\
      "Specify a set of language model"},						\
{ "lmname",										\
      ARG_STRING,									\
      NULL,									\
      "Which language model in -lmctl to use by default"},				\
{ "lw",										\
      ARG_FLOATING,									\
      "6.5",										\
      "Language model probability weight" },						\
{ "fwdflatlw",										\
      ARG_FLOATING,									\
      "8.5",										\
      "Language model probability weight for flat lexicon (2nd pass) decoding" },	\
{ "bestpathlw",									\
      ARG_FLOATING,									\
      "9.5",										\
      "Language model probability weight for bestpath search" },			\
{ "ascale",										\
      ARG_FLOATING,									\
      "20.0",										\
      "Inverse of acoustic model scale for confidence score calculation" },		\
{ "wip",										\
      ARG_FLOATING,									\
      "0.65",										\
      "Word insertion penalty" },							\
{ "nwpen",										\
      ARG_FLOATING,									\
      "1.0",										\
      "New word transition penalty" },							\
{ "pip",										\
      ARG_FLOATING,									\
      "1.0",										\
      "Phone insertion penalty" },							\
{ "uw",										\
      ARG_FLOATING,									\
      "1.0",										\
      "Unigram weight" }, 								\
{ "silprob",										\
      ARG_FLOATING,									\
      "0.005",										\
      "Silence word transition probability" },						\
{ "fillprob",										\
      ARG_FLOATING,									\
      "1e-8",										\
        "Filler word transition probability" } \

/** Command-line options for dictionaries. */
#define POCKETSPHINX_DICT_OPTIONS \
    { "dict",							\
      ARG_STRING,						\
      NULL,							\
      "Main pronunciation dictionary (lexicon) input file" },	\
    { "fdict",							\
      ARG_STRING,						\
      NULL,							\
      "Noise word pronunciation dictionary input file" },	\
    { "dictcase",						\
      ARG_BOOLEAN,						\
      "no",							\
      "Dictionary is case sensitive (NOTE: case insensitivity applies to ASCII characters only)" }	\

/** Command-line options for acoustic modeling */
#define POCKETSPHINX_ACMOD_OPTIONS \
{ "hmm",                                                                       \
      REQARG_STRING,                                                            \
      NULL,                                                                     \
      "Directory containing acoustic model files."},                            \
{ "featparams",                                                                \
      ARG_STRING,                                                               \
      NULL,                                                                     \
      "File containing feature extraction parameters."},                        \
{ "mdef",                                                                      \
      ARG_STRING,                                                               \
      NULL,                                                                     \
      "Model definition input file" },                                          \
{ "senmgau", \
      ARG_STRING,                                                               \
      NULL,                                                                     \
      "Senone to codebook mapping input file (usually not needed)" }, \
{ "tmat",                                                                      \
      ARG_STRING,                                                               \
      NULL,                                                                     \
      "HMM state transition matrix input file" },                               \
{ "tmatfloor",                                                                 \
      ARG_FLOATING,                                                              \
      "0.0001",                                                                 \
      "HMM state transition probability floor (applied to -tmat file)" },       \
{ "mean",                                                                      \
      ARG_STRING,                                                               \
      NULL,                                                                     \
      "Mixture gaussian means input file" },                                    \
{ "var",                                                                       \
      ARG_STRING,                                                               \
      NULL,                                                                     \
      "Mixture gaussian variances input file" },                                \
{ "varfloor",                                                                  \
      ARG_FLOATING,                                                              \
      "0.0001",                                                                 \
      "Mixture gaussian variance floor (applied to data from -var file)" },     \
{ "mixw",                                                                      \
      ARG_STRING,                                                               \
      NULL,                                                                     \
      "Senone mixture weights input file (uncompressed)" },                     \
{ "mixwfloor",                                                                 \
      ARG_FLOATING,                                                              \
      "0.0000001",                                                              \
      "Senone mixture weights floor (applied to data from -mixw file)" },       \
{ "aw",                                                                \
    ARG_INTEGER,                                                          \
    "1", \
        "Inverse weight applied to acoustic scores." },                 \
{ "sendump",                                                                   \
      ARG_STRING,                                                               \
      NULL,                                                                     \
      "Senone dump (compressed mixture weights) input file" },                  \
{ "mllr",                                                                      \
      ARG_STRING,                                                               \
      NULL,                                                                     \
      "MLLR transformation to apply to means and variances" },                  \
{ "mmap",                                                                      \
      ARG_BOOLEAN,                                                              \
      "yes",                                                                    \
      "Use memory-mapped I/O (if possible) for model files" },                  \
{ "ds",                                                                        \
      ARG_INTEGER,                                                                \
      "1",                                                                      \
      "Frame GMM computation downsampling ratio" },                             \
{ "topn",                                                                      \
      ARG_INTEGER,                                                                \
      "4",                                                                      \
      "Maximum number of top Gaussians to use in scoring." },                   \
{ "topn_beam",                                                                 \
      ARG_STRING,                                                               \
      "0",                                                                     \
      "Beam width used to determine top-N Gaussians (or a list, per-feature)" },\
{ "logbase",                                                                   \
      ARG_FLOATING,                                                              \
      "1.0001",                                                                 \
      "Base in which all log-likelihoods calculated" }

/** Options for acoustic feature computation. */
#define POCKETSPHINX_FE_OPTIONS                                         \
    { "logspec",                                                       \
      ARG_BOOLEAN,                                                      \
      "no",                                                             \
      "Write out logspectral files instead of cepstra" },               \
    { "smoothspec",                                                    \
      ARG_BOOLEAN,                                                      \
      "no",                                                             \
      "Write out cepstral-smoothed logspectral files" },                \
    { "transform",                                                     \
      ARG_STRING,                                                       \
      "legacy",                                                         \
      "Which type of transform to use to calculate cepstra (legacy, dct, or htk)" }, \
    { "alpha",                                                         \
      ARG_FLOATING,                                                     \
      ARG_STRINGIFY(DEFAULT_PRE_EMPHASIS_ALPHA),                        \
      "Preemphasis parameter" },                                        \
    { "samprate",                                                      \
      ARG_INTEGER,                                                      \
      ARG_STRINGIFY(DEFAULT_SAMPLING_RATE),                             \
      "Sampling rate" },                                                \
    { "frate",                                                         \
      ARG_INTEGER,                                                      \
      ARG_STRINGIFY(DEFAULT_FRAME_RATE),                                \
      "Frame rate" },                                                   \
    { "wlen",                                                          \
      ARG_FLOATING,                                                     \
      ARG_STRINGIFY(DEFAULT_WINDOW_LENGTH),                             \
      "Hamming window length" },                                        \
    { "nfft",                                                          \
      ARG_INTEGER,                                                      \
      "0",                                                              \
      "Size of FFT, or 0 to set automatically (recommended)" },         \
    { "nfilt",                                                         \
      ARG_INTEGER,                                                      \
      ARG_STRINGIFY(DEFAULT_NUM_FILTERS),                               \
      "Number of filter banks" },                                       \
    { "lowerf",                                                        \
      ARG_FLOATING,                                                     \
      ARG_STRINGIFY(DEFAULT_LOWER_FILT_FREQ),                           \
      "Lower edge of filters" },                                        \
    { "upperf",                                                        \
      ARG_FLOATING,                                                     \
      ARG_STRINGIFY(DEFAULT_UPPER_FILT_FREQ),                           \
      "Upper edge of filters" },                                        \
    { "unit_area",                                                     \
      ARG_BOOLEAN,                                                      \
      "yes",                                                            \
      "Normalize mel filters to unit area" },                           \
    { "round_filters",                                                 \
      ARG_BOOLEAN,                                                      \
      "yes",                                                            \
      "Round mel filter frequencies to DFT points" },                   \
    { "ncep",                                                          \
      ARG_INTEGER,                                                      \
      ARG_STRINGIFY(DEFAULT_NUM_CEPSTRA),                               \
      "Number of cep coefficients" },                                   \
    { "doublebw",                                                      \
      ARG_BOOLEAN,                                                      \
      "no",                                                             \
      "Use double bandwidth filters (same center freq)" },              \
    { "lifter",                                                        \
      ARG_INTEGER,                                                      \
      "0",                                                              \
      "Length of sin-curve for liftering, or 0 for no liftering." },    \
    { "input_endian",                                                  \
      ARG_STRING,                                                       \
      NATIVE_ENDIAN,                                                    \
      "Endianness of input data, big or little, ignored if NIST or MS Wav" }, \
    { "warp_type",                                                     \
      ARG_STRING,                                                       \
      DEFAULT_WARP_TYPE,                                                \
      "Warping function type (or shape)" },                             \
    { "warp_params",                                                   \
      ARG_STRING,                                                       \
      NULL,                                                             \
      "Parameters defining the warping function" },                     \
    { "dither",                                                        \
      ARG_BOOLEAN,                                                      \
      "no",                                                             \
      "Add 1/2-bit noise" },                                            \
    { "seed",                                                          \
      ARG_INTEGER,                                                      \
      ARG_STRINGIFY(SEED),                                              \
      "Seed for random number generator; if less than zero, pick our own" }, \
    { "remove_dc",                                                     \
      ARG_BOOLEAN,                                                      \
      "no",                                                             \
      "Remove DC offset from each frame" },                             \
    { "remove_noise",                                                  \
      ARG_BOOLEAN,                                                      \
      "no",                                                             \
      "Remove noise using spectral subtraction" },                      \
    { "verbose",                                                       \
      ARG_BOOLEAN,                                                      \
      "no",                                                             \
      "Show input filenames" }

/** Options for dynamic feature calculation. */
#define POCKETSPHINX_FEAT_OPTIONS                        \
{ "feat",                                                              \
      ARG_STRING,                                                       \
      "1s_c_d_dd",                                                      \
      "Feature stream type, depends on the acoustic model" },           \
{ "ceplen",                                                            \
      ARG_INTEGER,                                                        \
      "13",                                                             \
     "Number of components in the input feature vector" },              \
{ "cmn",                                                               \
      ARG_STRING,                                                       \
      "live",                                                        \
      "Cepstral mean normalization scheme ('live', 'batch', or 'none')" }, \
{ "cmninit",                                                           \
      ARG_STRING,                                                       \
      "40,3,-1",                                                        \
      "Initial values (comma-separated) for cepstral mean when 'live' is used" }, \
{ "varnorm",                                                           \
      ARG_BOOLEAN,                                                      \
      "no",                                                             \
      "Variance normalize each utterance (only if CMN == current)" },   \
{ "agc",                                                               \
      ARG_STRING,                                                       \
      "none",                                                           \
      "Automatic gain control for c0 ('max', 'emax', 'noise', or 'none')" }, \
{ "agcthresh",                                                         \
      ARG_FLOATING,                                                      \
      "2.0",                                                            \
      "Initial threshold for automatic gain control" },                 \
{ "lda",                                                               \
      ARG_STRING,                                                       \
      NULL,                                                             \
      "File containing transformation matrix to be applied to features (single-stream features only)" }, \
{ "ldadim",                                                            \
      ARG_INTEGER,                                                        \
      "0",                                                              \
      "Dimensionality of output of feature transformation (0 to use entire matrix)" }, \
{ "svspec",                                                             \
     ARG_STRING,                                                        \
     NULL,                                                           \
     "Subvector specification (e.g., 24,0-11/25,12-23/26-38 or 0-12/13-25/26-38)"}

#define CMDLN_EMPTY_OPTION { NULL, 0, NULL, NULL }

#endif /* __PS_CMDLN_MACRO_H__ */
