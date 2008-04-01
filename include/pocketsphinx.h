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
#include <fbs.h>

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
pocketsphinx_t *pocketsphinx_init(cmd_ln_t *config);

/**
 * Returns the argument definitions used in pocketsphinx_init().
 *
 * This is here to avoid exporting global data, which is problematic
 * on Win32 and Symbian (and possibly other platforms).
 */
arg_t const *pocketsphinx_args(void);

/**
 * Finalize the decoder.
 */
void pocketsphinx_free(pocketsphinx_t *ps);

/**
 * Get the configuration object for this decoder.
 */
cmd_ln_t *pocketsphinx_get_config(pocketsphinx_t *ps);

/**
 * Load a finite-state grammar (FSG or JSGF format).
 *
 * FIXME: Actually this functionality will be in SphinxBase, the only
 * thing necessary is that we have an FSG switching capability here.
 *
 * The type of file will be auto-detected based on the file contents.
 *
 * FSG files contain a single grammar which is identified by a name
 * field in the header of the file.  To select a grammar, call
 * pocketsphinx_set_fsg() with this name.
 *
 * JSGF files can import an arbitrary number of rules which all live
 * inside a global namespace.  Each 'public' rule is actually a
 * grammar unto itself.  To select a rule, call pocketsphinx_set_fsg()
 * with the fully-qualified rule name.
 *
 * @return For FSG files, name of the grammar.  For JSGF files, the
 * name of the first public rule in the file specified.
 */
const char *pocketsphinx_load_fsgfile(pocketsphinx_t *ps,
				      const char *fsgfile);

/**
 * Load a list of FSG files.
 */
const char *pocketsphinx_load_fsgctl(pocketsphinx_t *ps,
				     const char *fsgctlfile,
				     int set_default);

/**
 * Run a control file in batch mode.
 */
int pocketsphinx_run_ctl_file(pocketsphinx_t *ps,
                              char const *ctlfile);

/**
 * Start utterance processing.
 */
int pocketsphinx_start_utt(pocketsphinx_t *ps);

/**
 * Decode raw audio data.
 *
 * @param no_search If non-zero, perform feature extraction but don't
 * do any recognition yet.  This may be necessary if your processor
 * has trouble doing recognition in real-time.
 * @param full_utt If non-zero, this block of data is a full utterance
 * worth of data.  This may allow the recognizer to produce more
 * accurate results.
 * @return Number of frames of data searched, or <0 for error.
 */
int pocketsphinx_process_raw(pocketsphinx_t *ps,
                             int16 const *data,
                             size_t n_samples,
                             int no_search,
                             int full_utt);

/**
 * Decode acoustic feature data.
 *
 * @param no_search If non-zero, perform feature extraction but don't
 * do any recognition yet.  This may be necessary if your processor
 * has trouble doing recognition in real-time.
 * @param full_utt If non-zero, this block of data is a full utterance
 * worth of data.  This may allow the recognizer to produce more
 * accurate results.
 * @return Number of frames of data searched, or <0 for error.
 */
int pocketsphinx_process_cep(pocketsphinx_t *ps,
                             mfcc_t **data,
                             int n_frames,
                             int no_search,
                             int full_utt);

/**
 * End utterance processing.
 */
int pocketsphinx_end_utt(pocketsphinx_t *ps);

/**
 * Get hypothesis string and path score.
 *
 * @param out_best_score Output: path score corresponding to returned string.
 * @return String containing best hypothesis at this point in
 *         decoding.  NULL if no hypothesis is available.
 */
char const *pocketsphinx_get_hyp(pocketsphinx_t *ps, int32 *out_best_score);

/**
 * Get an iterator over the word segmentation for the best hypothesis.
 *
 * @param out_best_score Output: path score corresponding to hypothesis.
 * @return Iterator over the best hypothesis at this point in
 *         decoding.  NULL if no hypothesis is available.
 */
ps_seg_t *pocketsphinx_seg_iter(pocketsphinx_t *ps, int32 *out_best_score);

/**
 * Get the next segment in a word segmentation.
 *
 * @return Updated iterator with the next segment.  NULL at end of
 *         utterance (the iterator will be freed in this case).
 */
ps_seg_t *pocketsphinx_seg_next(ps_seg_t *seg);

/**
 * Get word string from a segmentation iterator.
 */
char const *pocketsphinx_seg_word(ps_seg_t *seg);

/**
 * Get start and end frames from a segmentation iterator.
 */
void pocketsphinx_seg_frames(ps_seg_t *seg, int *out_sf, int *out_ef);

/**
 * Get posterior probability from a segmentation iterator
 */
void pocketsphinx_seg_prob(ps_seg_t *seg, float32 *out_pprob);

/**
 * Finish iterating over a word segmentation early, freeing resources.
 */
void pocketsphinx_seg_free(ps_seg_t *seg);

/**
 * Get an iterator over the best hypotheses, optionally within a
 * selected region of the utterance.
 *
 * @param sf Start frame for N-best search (0 for whole utterance) 
 * @param ef End frame for N-best search (-1 for whole utterance) 
 * @param ctx1 First word of trigram context (NULL for whole utterance)
 * @param ctx2 First word of trigram context (NULL for whole utterance)
 */
ps_nbest_t *pocketsphinx_nbest(pocketsphinx_t *ps, int sf, int ef,
			       char const *ctx1, char const *ctx2);

/**
 * Get the next best hypothesis string in an N-best list.
 *
 * @return String containing next best hypothesis.  NULL if no more
 * hypotheses are available (iterator will be freed in this case).
 */
char const *pocketsphinx_nbest_next(ps_nbest_t *nbest);

/**
 * Get the next best word segmentation in an N-best list.
 *
 * @return Iterator over the next best hypothesis.  NULL if no more
 * hypotheses are available (iterator will be freed in this case).
 */
ps_seg_t *pocketsphinx_nbest_next_seg(ps_nbest_t *nbest);

/**
 * Finish N-best search early, releasing resources.
 */
void pocketsphinx_nbest_free(ps_nbest_t *nbest);

/**
 * Get performance information for the current utterance.
 *
 * @param out_nspeech Output: Number of seconds of speech.
 * @param out_ncpu    Output: Number of seconds of CPU time used.
 * @param out_nwall   Output: Number of seconds of wall time used.
 */
void pocketsphinx_get_utt_time(pocketsphinx_t *ps, double *out_nspeech,
			       double *out_ncpu, double *out_nwall);

/**
 * Get overall performance information.
 *
 * @param out_nspeech Output: Number of seconds of speech.
 * @param out_ncpu    Output: Number of seconds of CPU time used.
 * @param out_nwall   Output: Number of seconds of wall time used.
 */
void pocketsphinx_get_all_time(pocketsphinx_t *ps, double *out_nspeech,
			       double *out_ncpu, double *out_nwall);

/**
 * @mainpage PocketSphinx API Documentation
 * @author David Huggins-Daines <dhuggins@cs.cmu.edu>
 * @author Mosur Ravishankar <rkm@cs.cmu.edu>
 * @author Evandro Gouvêa <egouvea@cs.cmu.edu>
 * @author Arthur Chan <archan@cs.cmu.edu>
 * @version 0.5
 * @date January, 2008
 */

#endif /* __POCKETSPHINX_H__ */
