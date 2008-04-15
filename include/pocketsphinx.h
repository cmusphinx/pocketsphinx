/* -*- c-basic-offset:4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 1999-2008 Carnegie Mellon University.  All rights
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
/**
 * @file pocketsphinx.h Main header file for the PocketSphinx decoder.
 */

#ifndef __POCKETSPHINX_H__
#define __POCKETSPHINX_H__

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

/* SphinxBase headers we need. */
#include <cmd_ln.h>
#include <logmath.h>
#include <fe.h>
#include <feat.h>
#include <ngram_model.h>

/* PocketSphinx headers (not many of them!) */
#include <pocketsphinx_export.h>
#include <cmdln_macro.h>
#include <fsg_set.h>

/**
 * PocketSphinx speech recognizer object.
 */
typedef struct pocketsphinx_s pocketsphinx_t;

/**
 * PocketSphinx N-best hypothesis iterator object.
 */
typedef struct ps_nbest_s ps_nbest_t;

/**
 * PocketSphinx segmentation iterator object.
 */
typedef struct ps_seg_s ps_seg_t;

/**
 * Initialize the decoder from a configuration object.
 *
 * @note The decoder retains ownership of the pointer
 * <code>config</code>, so you must not attempt to free it manually.
 *
 * @param config a command-line structure, as created by
 * cmd_ln_parse_r() or cmd_ln_parse_file_r().
 */
POCKETSPHINX_EXPORT
pocketsphinx_t *pocketsphinx_init(cmd_ln_t *config);

/**
 * Reinitialize the decoder with updated configuration.
 *
 * This function allows you to switch the acoustic model, dictionary,
 * or other configuration without creating an entirely new decoding
 * object.
 *
 * @param ps Decoder.
 * @param config An optional new configuration to use.  If this is
 *               NULL, the previous configuration will be reloaded,
 *               with any changes applied.
 * @return 0 for success, <0 for failure.
 */
POCKETSPHINX_EXPORT
int pocketsphinx_reinit(pocketsphinx_t *ps, cmd_ln_t *config);

/**
 * Returns the argument definitions used in pocketsphinx_init().
 *
 * This is here to avoid exporting global data, which is problematic
 * on Win32 and Symbian (and possibly other platforms).
 */
POCKETSPHINX_EXPORT
arg_t const *pocketsphinx_args(void);

/**
 * Finalize the decoder.
 *
 * This releases all resources associated with the decoder, including
 * any language models or grammars which have been added to it, and
 * the initial configuration object passed to pocketsphinx_init().
 *
 * @param ps Decoder to be freed.
 */
POCKETSPHINX_EXPORT
void pocketsphinx_free(pocketsphinx_t *ps);

/**
 * Get the configuration object for this decoder.
 *
 * @return The configuration object for this decoder.  The decoder
 *         retains ownership of this pointer, so you should not
 *         attempt to free it manually.
 */
POCKETSPHINX_EXPORT
cmd_ln_t *pocketsphinx_get_config(pocketsphinx_t *ps);

/**
 * Get the log-math computation object for this decoder.
 *
 * @return The log-math object for this decoder.  The decoder
 *         retains ownership of this pointer, so you should not
 *         attempt to free it manually.
 */
POCKETSPHINX_EXPORT
logmath_t *pocketsphinx_get_logmath(pocketsphinx_t *ps);

/**
 * Get the language model set object for this decoder.
 *
 * If N-Gram decoding is not enabled, this will return NULL.  You will
 * need to enable it using pocketsphinx_update_lmset().
 *
 * @return The language model set object for this decoder.  The
 *         decoder retains ownership of this pointer, so you should
 *         not attempt to free it manually.
 */
POCKETSPHINX_EXPORT
ngram_model_t *pocketsphinx_get_lmset(pocketsphinx_t *ps);

/**
 * Update the language model set object for this decoder.
 *
 * This function does several things.  Most importantly, it enables
 * N-Gram decoding if not currently enabled.  It also updates internal
 * data structures to reflect any changes made to the language model
 * set (e.g. switching language models, adding words, etc).
 *
 * @return The current language model set object for this decoder, or
 *         NULL on failure.
 */
POCKETSPHINX_EXPORT
ngram_model_t *pocketsphinx_update_lmset(pocketsphinx_t *ps);

/**
 * Get the finite-state grammar set object for this decoder.
 *
 * If FSG decoding is not enabled, this returns NULL.  Call
 * pocketsphinx_update_fsgset() to enable it.
 *
 * @return The current FSG set object for this decoder, or
 *         NULL if none is available.
 */
POCKETSPHINX_EXPORT
fsg_set_t *pocketsphinx_get_fsgset(pocketsphinx_t *ps);

/**
 * Update the finite-state grammar set object for this decoder.
 *
 * This function does several things.  Most importantly, it enables
 * FSG decoding if not currently enabled.  It also updates internal
 * data structures to reflect any changes made to the FSG set.
 *
 * @return The current FSG set object for this decoder, or
 *         NULL on failure.
 */
POCKETSPHINX_EXPORT
fsg_set_t *pocketsphinx_update_fsgset(pocketsphinx_t *ps);

/**
 * Start utterance processing.
 *
 * This function should be called before any utterance data is passed
 * to the decoder.  It marks the start of a new utterance and
 * reinitializes internal data structures.
 *
 * @param ps Decoder to be started.
 * @return 0 for success, <0 on error.
 */
POCKETSPHINX_EXPORT
int pocketsphinx_start_utt(pocketsphinx_t *ps);

/**
 * Decode raw audio data.
 *
 * @param ps Decoder.
 * @param no_search If non-zero, perform feature extraction but don't
 * do any recognition yet.  This may be necessary if your processor
 * has trouble doing recognition in real-time.
 * @param full_utt If non-zero, this block of data is a full utterance
 * worth of data.  This may allow the recognizer to produce more
 * accurate results.
 * @return Number of frames of data searched, or <0 for error.
 */
POCKETSPHINX_EXPORT
int pocketsphinx_process_raw(pocketsphinx_t *ps,
                             int16 const *data,
                             size_t n_samples,
                             int no_search,
                             int full_utt);

/**
 * Decode acoustic feature data.
 *
 * @param ps Decoder.
 * @param no_search If non-zero, perform feature extraction but don't
 * do any recognition yet.  This may be necessary if your processor
 * has trouble doing recognition in real-time.
 * @param full_utt If non-zero, this block of data is a full utterance
 * worth of data.  This may allow the recognizer to produce more
 * accurate results.
 * @return Number of frames of data searched, or <0 for error.
 */
POCKETSPHINX_EXPORT
int pocketsphinx_process_cep(pocketsphinx_t *ps,
                             mfcc_t **data,
                             int n_frames,
                             int no_search,
                             int full_utt);

/**
 * End utterance processing.
 *
 * @param ps Decoder.
 * @return 0 for success, <0 on error
 */
POCKETSPHINX_EXPORT
int pocketsphinx_end_utt(pocketsphinx_t *ps);

/**
 * Get hypothesis string and path score.
 *
 * @param out_best_score Output: path score corresponding to returned string.
 * @return String containing best hypothesis at this point in
 *         decoding.  NULL if no hypothesis is available.
 */
POCKETSPHINX_EXPORT
char const *pocketsphinx_get_hyp(pocketsphinx_t *ps, int32 *out_best_score);

/**
 * Get an iterator over the word segmentation for the best hypothesis.
 *
 * @param out_best_score Output: path score corresponding to hypothesis.
 * @return Iterator over the best hypothesis at this point in
 *         decoding.  NULL if no hypothesis is available.
 */
POCKETSPHINX_EXPORT
ps_seg_t *pocketsphinx_seg_iter(pocketsphinx_t *ps, int32 *out_best_score);

/**
 * Get the next segment in a word segmentation.
 *
 * @return Updated iterator with the next segment.  NULL at end of
 *         utterance (the iterator will be freed in this case).
 */
POCKETSPHINX_EXPORT
ps_seg_t *pocketsphinx_seg_next(ps_seg_t *seg);

/**
 * Get word string from a segmentation iterator.
 */
POCKETSPHINX_EXPORT
char const *pocketsphinx_seg_word(ps_seg_t *seg);

/**
 * Get start and end frames from a segmentation iterator.
 */
POCKETSPHINX_EXPORT
void pocketsphinx_seg_frames(ps_seg_t *seg, int *out_sf, int *out_ef);

/**
 * Get log posterior probability from a segmentation iterator.
 *
 * @note This is currently not implemented.
 *
 * @param out_pprob Output: log posterior probability of current
 *                  segment.  Log is expressed in the log-base used in
 *                  the decoder.  To convert to linear floating-point,
 *                  use pocketsphinx_exp().
 */
POCKETSPHINX_EXPORT
void pocketsphinx_seg_prob(ps_seg_t *seg, int32 *out_pprob);

/**
 * Finish iterating over a word segmentation early, freeing resources.
 */
POCKETSPHINX_EXPORT
void pocketsphinx_seg_free(ps_seg_t *seg);

/**
 * Get an iterator over the best hypotheses, optionally within a
 * selected region of the utterance.
 *
 * @param ps Decoder.
 * @param sf Start frame for N-best search (0 for whole utterance) 
 * @param ef End frame for N-best search (-1 for whole utterance) 
 * @param ctx1 First word of trigram context (NULL for whole utterance)
 * @param ctx2 First word of trigram context (NULL for whole utterance)
 * @return Iterator over N-best hypotheses.
 */
POCKETSPHINX_EXPORT
ps_nbest_t *pocketsphinx_nbest(pocketsphinx_t *ps, int sf, int ef,
			       char const *ctx1, char const *ctx2);

/**
 * Move an N-best list iterator forward.
 *
 * @param nbest N-best iterator.
 * @return Updated N-best iterator, or NULL if no more hypotheses are
 *         available (iterator is freed ni this case).
 */
POCKETSPHINX_EXPORT 
ps_nbest_t *pocketsphinx_nbest_next(ps_nbest_t *nbest);

/**
 * Get the hypothesis string from an N-best list iterator.
 *
 * @param nbest N-best iterator.
 * @param out_score Output: Path score for this hypothesis.
 * @return String containing next best hypothesis.
 */
POCKETSPHINX_EXPORT
char const *pocketsphinx_nbest_hyp(ps_nbest_t *nbest, int32 *out_score);

/**
 * Get the word segmentation from an N-best list iterator.
 *
 * @param nbest N-best iterator.
 * @param out_score Output: Path score for this hypothesis.
 * @return Iterator over the next best hypothesis.
 */
POCKETSPHINX_EXPORT
ps_seg_t *pocketsphinx_nbest_seg(ps_nbest_t *nbest, int32 *out_score);

/**
 * Finish N-best search early, releasing resources.
 *
 * @param nbest N-best iterator.
 */
POCKETSPHINX_EXPORT
void pocketsphinx_nbest_free(ps_nbest_t *nbest);

/**
 * Get performance information for the current utterance.
 *
 * @param ps Decoder.
 * @param out_nspeech Output: Number of seconds of speech.
 * @param out_ncpu    Output: Number of seconds of CPU time used.
 * @param out_nwall   Output: Number of seconds of wall time used.
 */
POCKETSPHINX_EXPORT
void pocketsphinx_get_utt_time(pocketsphinx_t *ps, double *out_nspeech,
			       double *out_ncpu, double *out_nwall);

/**
 * Get overall performance information.
 *
 * @param ps Decoder.
 * @param out_nspeech Output: Number of seconds of speech.
 * @param out_ncpu    Output: Number of seconds of CPU time used.
 * @param out_nwall   Output: Number of seconds of wall time used.
 */
POCKETSPHINX_EXPORT
void pocketsphinx_get_all_time(pocketsphinx_t *ps, double *out_nspeech,
			       double *out_ncpu, double *out_nwall);

/**
 * @mainpage PocketSphinx API Documentation
 * @author David Huggins-Daines <dhuggins@cs.cmu.edu>
 * @version 0.5
 * @date April, 2008
 */

#endif /* __POCKETSPHINX_H__ */
