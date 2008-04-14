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
 * @file fbs.h "Fast Beam Search" interface to PocketSphinx.
 */

#ifndef _FBS_H_
#define _FBS_H_

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

/* SphinxBase headers. */
#include <cmd_ln.h>
#include <ngram_model.h>
#include <prim_type.h>

/* PocketSphinx headers. */
#include <pocketsphinx_export.h>

/**
 * Recognition result (hypothesis) with word segmentation information.
 */
typedef struct search_hyp_s {
    char const *word;	/**< READ-ONLY */
    int32 wid;		/**< For internal use of decoder */
    int32 sf, ef;	/**< Start, end frames within utterance for this word */
    int32 ascr, lscr;	/**< Acoustic, LM scores (not always used!) */
    int32 fsg_state;	/**< At which this entry terminates (FSG mode only) */
    float conf;		/**< Confidence measure (roughly prob(correct)) for this word;
			   NOT FILLED IN BY THE RECOGNIZER at the moment!! */
    struct search_hyp_s *next;	/**< Next word segment in the hypothesis; NULL if none */
    int32 latden;	/**< Average lattice density in segment.  Larger values imply
			   more confusion and less certainty about the result.  To use
			   it for rejection, cutoffs must be found independently */
} search_hyp_t;


/**
 * Initialize the speech recognition engine.
 *
 * Called once at initialization with the list of arguments to
 * initialize to initialize the decoder.  If the <tt>-ctlfn</tt>
 * argument is given, it will process the argument file in batch mode
 * and exit the application.
 * 
 * If <code>argv</code> is <code>NULL</code>, then it is assumed that
 * the arguments (which are available by calling fbs_get_args()) were
 * already parsed using the SphinxBase <code>cmd_ln_*()</code>
 * interface, as in Sphinx3.
 *
 * @return 0 if successful, -1 otherwise.
 */
POCKETSPHINX_EXPORT
int32 fbs_init(int32 argc, char **argv);

/**
 * Returns the argument definitions used in fbs_init().
 *
 * This is here to avoid exporting global data, which is problematic
 * on Win32 and Symbian (and possibly other platforms).
 */
POCKETSPHINX_EXPORT
arg_t const *fbs_get_args(void);

/**
 * Finalize the speech recognition engine.
 *
 * This releases all memory and other resources associated with the
 * engine.  This allows you to switch acoustic models at runtime.
 *
 * @return 0 if successful, -1 otherwise.
 */
POCKETSPHINX_EXPORT
int32 fbs_end(void);


/**
 * Begins processing an utterance.
 *
 * @param uttid An string identifying the utterance; utterance data
 *              (raw or mfc files, if any) logged under this name.
 *              The recognition result in the "match" file also
 *              identified with this id.  If uttid is NULL, an
 *              automatically generated running sequence number (of
 *              the form %08d) is used instead.
 *
 * @return 0 if successful, else -1.
 */
POCKETSPHINX_EXPORT
int32 uttproc_begin_utt(char const *uttid);


/**
 * Decode the next block of input samples in the current utterance.
 *
 * The "block" argument specifies whether the decoder should block
 * until all pending data have been processed.  If 0, it is
 * "non-blocking".  That is, the decoder steps through only a few
 * pending frames (at least 1), and the remaining input data is queued
 * up internally for later processing.  In particular, this function
 * can be called with 0-length data to simply process internally
 * queued up frames.
 * 
 * @note The decoder will not actually process the input data if any
 * of the processing depends on the entire utterance.  (For example,
 * if CMN/AGC is based on entire current utterance.)  The data are
 * queued up internally for processing after uttproc_end_utt is
 * called.  Also, one cannot combine uttproc_rawdata and
 * uttproc_cepdata within the same utterance.
 * 
 *
 * @param raw Block of samples in native-endian linear PCM format
 * @param nsample Number of samples in <code>raw</code>, can be 0.
 * @param block Whether to process data immediately.
 * @return Number of frames internally queued up and remaining to be
 *         decoded; -1 if any error occurs.
 */
POCKETSPHINX_EXPORT
int32 uttproc_rawdata(int16 *raw,
		      int32 nsample,
		      int32 block);

/**
 * Decode the next block of input cepstra in the current utterance.
 *
 * Like uttproc_rawdata, but the input data are cepstrum vectors
 * rather than raw samples.  One cannot combine uttproc_cepdata and
 * uttproc_rawdata within the same utterance.
 *
 * @param cep Array of pointers to cepstrum vectors.
 * @param nsample Number of frames in <code>cep</code>, can be 0.
 * @param block Whether to process data immediately.
 * @return Number of frames internally queued up and remaining to be
 *         decoded; -1 if any error occurs.
 */
POCKETSPHINX_EXPORT
int32 uttproc_cepdata(float32 **cep,
		      int32 nfrm,
		      int32 block);


/**
 * Finish processing an utterance.
 *
 * For bookkeeping purposes, this function marks that no more data is
 * forthcoming in the current utterance.  It should be followed by
 * uttproc_result to obtain the final recognition result.
 *
 * @return 0 if successful, else -1.
 */
POCKETSPHINX_EXPORT
int32 uttproc_end_utt(void);

/**
 * Decode an entire utterance worth of raw data from a file.
 *
 * If you have set the <tt>-cepdir</tt> and <tt>-cepext</tt> options
 * when initializing the decoder in fbs_init(), then the filename will
 * be extended using them.
 *
 * @param filename Pathname of the raw audio file to decode.
 * @param uttid A string identifying the utterance.
 * @param sf Index of frame number to begin processing.  The frame
 *           size is determined by the <tt>-frate</tt> parameter, but
 *           it is usually 10ms (100 frames per second).
 * @param ef Index of frame number at which to stop processing, or -1
 *           to process the entire utterance.
 * @param nosearch If nonzero, don't actually try to recognize anything.
 * @return The number of frames processed if successful, else -1.
 **/
POCKETSPHINX_EXPORT
int32 uttproc_decode_raw_file(const char *filename, 
			      const char *uttid,
			      int32 sf, int32 ef, int32 nosearch);

/**
 * Decode an entire utterance worth of cepstrum data from a file.
 *
 * If you have set the <tt>-cepdir</tt> and <tt>-cepext</tt> options,
 * then the filename will be extended using them.
 *
 * @param filename Pathname of the cepstrum file to decode.
 * @param uttid A string identifying the utterance.
 * @param sf Index of frame number to begin processing.  The frame
 *           size is determined by the <tt>-frate</tt> parameter, but
 *           it is usually 10ms (100 frames per second).
 * @param ef Index of frame number at which to stop processing.
 * @param nosearch If nonzero, don't actually try to recognize anything.
 * @return The number of frames processed if successful, else -1.
 **/
POCKETSPHINX_EXPORT
int32 uttproc_decode_cep_file(const char *filename,
			      const char *uttid,
			      int32 sf, int32 ef, int32 nosearch);

/**
 * Obtain recognition result for utterance after uttproc_end_utt() has
 * been called.

 * In the blocking form (i.e. if the <code>block</code> paramter is
 * non-zero), all queued up data is processed and final result
 * returned.  In the non-blocking version, only a few pending frames
 * (at least 1) are processed.  In the latter case, the function can
 * be called repeatedly to allow the decoding to complete.
 *
 * @param out_frm (output) pointer to number of frames in utterance.
 * @param out_hyp (output) pointer to READ_ONLY recognition string.
 *            The contents of this string will be clobbered by the
 *            next call to uttproc_result() or
 *            uttproc_partial_result().
 * @param block If non-zero, process all data and return final result.
 * @return Number of frames remaining to be processed.  If non-zero
 *         (non-blocking mode) the final result is not yet available.
 *         If 0, frm and hyp contain the final recognition result.  If
 *         there is any error, the function returns -1.
 */
POCKETSPHINX_EXPORT
int32 uttproc_result(int32 *out_frm,
		     char **out_hyp,
		     int32 block);

/**
 * Obtain a list of word segmentations instead of the full
 * recognition string.
 *
 * The list of word segmentations is READ-ONLY, and clobbered by the
 * next call to any of the result functions.  Use uttproc_result() or
 * uttproc_result_seg() to obtain the final result, but not both!
 *
 * @param out_frm (output) pointer to number of frames in utterance.
 * @param out_hyp (output) pointer to NULL-terminated linked list of
 *                <code>search_hyp_t</code> structures.
 * @param block If non-zero, process all data and return final result.
 * @return 0 if successful, else -1.
 */

POCKETSPHINX_EXPORT
int32 uttproc_result_seg(int32 *out_frm,
			 search_hyp_t **out_hyp,
			 int32 block);

/**
 * Obtain a partial recognition result in the middle of an utterance.
 *
 * This function can be called anytime after uttproc_begin_utt() and
 * before the final uttproc_result() or uttproc_result_seg().
 *
 * @param out_frm (output) pointer to number of frames processed
 *                corresponding to the partial result.
 * @param out_hyp (output) pointer to READ-ONLY partial recognition
 *			   string.  Contents clobbered by the next
 *			   call to uttproc_result() or
 *			   uttproc_partial_result().
 * @return 0 if successful, else -1.
 */
POCKETSPHINX_EXPORT
int32 uttproc_partial_result(int32 *out_frm,
			     char **out_hyp);

/**
 * Obtain a list of word segmentations instead of the partial
 * recognition string.
 *
 * The list of word segmentations is READ-ONLY, and clobbered by the
 * next call to any of the result functions.
 *
 * @param out_frm (output) pointer to number of frames processed
 *                corresponding to the partial result.
 * @param out_hyp (output) pointer to NULL-terminated linked list of
 *                <code>search_hyp_t</code> structures.
 * @return 0 if successful, else -1.
 */
POCKETSPHINX_EXPORT
int32 uttproc_partial_result_seg(int32 *out_frm,
				 search_hyp_t **out_hyp);

/**
 * Abort the current utterance immediately.
 *
 * Called instead of uttproc_end_utt() to abort utterance processing
 * immediately. No final recognition result is available.  Note that
 * one cannot abort an utterance after calling uttproc_end_utt().
 *
 * @return 0 if successful, else -1.
 */
POCKETSPHINX_EXPORT
int32 uttproc_abort_utt(void);

/**
 * Pause processing of the current utterance.
 *
 * The sequence uttproc_stop_utt()...uttproc_restart_utt() can be used
 * to re-recognize the current utterance.  It is typically used to
 * switch to a new language model in the middle of an utterance, for
 * example, based on a partial recognition result; the switch occurs
 * in the middle of the two calls.  uttproc_stop_utt must eventually
 * be followed by uttproc_restart_utt.  There can be no other
 * intervening calls relating to the current utterance; i.e., no
 * uttproc_begin_utt(), uttproc_rawdata(), uttproc_cepdata(),
 * uttproc_end_utt(), uttproc_result(), uttproc_partial_result(), or
 * uttproc_abort_utt().  This operation cannot be performed after
 * uttproc_end_utt().
 *
 * @return 0 if successful, else -1.
 */
POCKETSPHINX_EXPORT
int32 uttproc_stop_utt(void);
/**
 * Resume processing the current utterance.
 *
 * This function resumes processing after a call to
 * uttproc_stop_utt().
 *
 * @return 0 if successful, else -1.
 */
POCKETSPHINX_EXPORT
int32 uttproc_restart_utt(void);


/**
 * Obtain N-best list for current utterance.
 *
 * @note Should be preceded by search_save_lattice()
 * @note Clobbers any previously returned N-best hypotheses in <cod>*alt_out</code.
 *
 * @param sf Start frame within utterance for generating N-best list.
 * @param ef End frame within utterance for generating N-best list.
 * @param w1 First word of context preceding utterance, may be -1 to
 *           indicate no two-word context.
 * @param w2 Second word of context preceding utterance.  If -1, then
 *           the default start word will be used. (FIXME: Make this work)
 * @param alt_out (output) pointer to array of <code>n</code> alternatives.
 * @return Number of alternative hypotheses returned; -1 if error.
 */
POCKETSPHINX_EXPORT
int32 search_get_alt(int32 n,
		     int32 sf, int32 ef,
		     int32 w1, int32 w2,
		     search_hyp_t ***alt_out);


/**
 * Build a word lattice for the current utterance. 
 *
 * This function should be called before search_get_alt().  (FIXME:
 * make it so this isn't required!)
 */
POCKETSPHINX_EXPORT
void search_save_lattice(void);

/**
 * Read an N-gram language model file.
 *
 * This function reads in a new LM file from <code>lmfile</code>, and
 * associate it with name <code>lmname</code>.  If there is already an
 * LM with the same name, it is automatically deleted.  The current LM
 * remains the same - to use the new language model, call
 * uttproc_set_lm(lmname).
 *
 * @note If the new language model contains words that are not in the
 * main dictionary, it will not be possible to recognize them until
 * they are added.  Use uttproc_add_word() to define new words.
 *
 * @param lmfile Path to LM file to read.
 * @param lmname Name to associate with this language model.

 * @param lw Language weight.  This is the scaling factor to apply to
 *           language model probabilities when doing speech
 *           recognition.  It controls the degree to which the search
 *           will be constrained by the language model.  A typical
 *           value is between 6.5 and 9.5.
 * @param uw Unigram weight.  The probabilities of words in isolation
 *           (unigrams) are interpolated with a uniform distribution.
 *           This parameter controls the relative weight of the
 *           estimated unigram probabilities versus the uniform
 *           distribution.  The allowable range of values is between 0
 *           and 1, with a typical value being 0.5.
 * @param wip Word insertion penalty.  This is a constant factor
 *            applied to the probability of adding a new word to the
 *            hypothesis.  Lower values will favor shorter hypotheses.
 *            The allowable range of values is between 0 and 1, with a
 *            typical value being 0.65.
 * @return 0 if successful, else -1.
 */
POCKETSPHINX_EXPORT
int32 lm_read(char const *lmfile,
	      char const *lmname,
	      double lw, double uw, double wip);

/**
 * Delete the named LM from the LM collection.
 *
 * The current LM is undefined at this point.  Use uttproc_set_lm()
 * immediately afterwards.
 *
 * @param lmname name of LM to delete.
 * @return 0 if successful, else -1.
 */
POCKETSPHINX_EXPORT
int32 lm_delete(char const *lmname);

/**
 * Get the underlying language model object by name.
 *
 * See <ngram_model.h> in SphinxBase documentation for what can be
 * done with this object.
 */

POCKETSPHINX_EXPORT
ngram_model_t *lm_get(char const *lmname);

/**
 * Get the language model set object by name.
 *
 * See <ngram_model.h> in SphinxBase documentation for what can be
 * done with this object.
 */

POCKETSPHINX_EXPORT
ngram_model_t *lm_get_lmset(char const *lmname);


/**
 * Set the currently active LM.
 *
 * Multiple LMs can be loaded initially (during fbs_init()) or at run
 * time using lm_read() (see above).  This function sets the decoder in
 * N-gram decoding mode.
 *
 * @param lmname name of LM to select.
 * @return 0 if successful, else -1.
 */
POCKETSPHINX_EXPORT
int32 uttproc_set_lm(char const *lmname);

/**
 * Get the name of the currently active LM.
 */
POCKETSPHINX_EXPORT
char const *uttproc_get_lm(void);

/**
 * Indicate to the decoder that the named LM has been updated (e.g.,
 * with the addition of a new unigram).
 *
 * @param lmname name of LM which was updated.
 * @return 0 if successful, else -1.
 */
POCKETSPHINX_EXPORT
int32 uttproc_lmupdate(char const *lmname);

/**
 * Set trigram history context for the next utterance.
 *
 * Instead of the next utterance beginning with a clean slate, it is
 * begun as if the two words wd1 and wd2 have just been recognized.
 * They are used as the (trigram) language model history for the
 * utterance.  wd1 can be NULL if there is only a one word history
 * wd2, or both wd1 and wd2 can be NULL to clear any history
 * information.
 *
 * @param wd1 First word of history, or NULL for a one-word history.
 * @param wd2 Second word of history, or NULL for empty history.
 * @return 0 if successful, else -1.
 */
POCKETSPHINX_EXPORT
int32 uttproc_set_context(char const *wd1,
			  char const *wd2);

/**
 * Add a word to the recognizer.
 *
 * This function adds a word to the pronunciation dictionary and the
 * current language model.  If the word is already present in one or
 * the other, it does whatever is necessary to ensure that the word
 * can be recognized.
 *
 * @param word Word string to add.
 * @param phones Whitespace-separated list of phoneme strings
 * describing pronunciation of <code>word</code>.
 * @param update If TRUE, update the search module to recognize the
 * newly added word.  If adding multiple words, it is more efficient
 * to pass FALSE here in all but the last word. This is equivalent to
 * calling uttproc_lmupdate(uttproc_get_lm()).
 * @return Word ID (>= 0) for newly added word, or -1 for failure.
 */
POCKETSPHINX_EXPORT
int32 uttproc_add_word(char const *word,
                       char const *phones,
                       int update);

/**
 * Set the logging directory for raw audio files.
 *
 * Sets the logging directory for raw audio files.  The file names are
 * <uttid>.raw, where <uttid> is the utterance id associated with the
 * current utterance (see uttproc_begin_utt()).
 *
 * @param directory to write raw audio files in
 * @return 0 if successful, else -1.
 */
POCKETSPHINX_EXPORT
int32 uttproc_set_rawlogdir(char const *dir);

/**
 * Set the logging directory for cepstrum files.
 *
 * Sets the logging directory for cepstrum files.  The file names are
 * <uttid>.mfc, where <uttid> is the utterance id associated with the
 * current utterance (see uttproc_begin_utt()).
 *
 * @param directory to write raw audio files in
 * @return 0 if successful, else -1.
 */
POCKETSPHINX_EXPORT
int32 uttproc_set_mfclogdir(char const *dir);


/**
 * Change the log file.
 *
 * @return 0 if ok, else -1
 */
POCKETSPHINX_EXPORT
int32 uttproc_set_logfile(char const *file);


/**
 * Obtain the uttid for the most recent utterance (in progress or just finished)
 *
 * @return READ-ONLY string that is the utterance id.
 */
POCKETSPHINX_EXPORT
char const *uttproc_get_uttid(void);


/**
 * Set the utterance ID prefix for automatically generated uttids.
 *
 * For automatically generated uttid's (see uttproc_begin_utt()), also
 * use the prefix given below.  (So the uttid is formatted "%s%08d",
 * prefix, sequence_no.)
 *
 * @param prefix prefix to prepend to automatic utterance IDs.
 * @return 0 if ok, else -1
 */
POCKETSPHINX_EXPORT
int32 uttproc_set_auto_uttid_prefix(char const *prefix);

#ifdef __cplusplus
}
#endif

#endif
