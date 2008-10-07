/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 2008 Carnegie Mellon University.  All rights
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

/**
 * @file acmod.h Acoustic model structures for PocketSphinx.
 * @author David Huggins-Daines <dhuggins@cs.cmu.edu>
 */

#ifndef __ACMOD_H__
#define __ACMOD_H__

/* System headers. */
#include <stdio.h>

/* SphinxBase headers. */
#include <cmd_ln.h>
#include <logmath.h>
#include <fe.h>
#include <feat.h>
#include <bitvec.h>

/* Local headers. */
#include "bin_mdef.h"
#include "s2_semi_mgau.h"
#include "sdc_mgau.h"
#include "ms_mgau.h"
#include "tmat.h"
#include "hmm.h"

/**
 * States in utterance processing.
 */
typedef enum acmod_state_e {
    ACMOD_IDLE,		/**< Not in an utterance. */
    ACMOD_STARTED,      /**< Utterance started, no data yet. */
    ACMOD_PROCESSING,   /**< Utterance in progress. */
    ACMOD_ENDED         /**< Utterance ended, still buffering. */
} acmod_state_t;

/**
 * Function which computes one frame of GMM scores.
 */
typedef int (*frame_eval_t)(void *eval_obj,
                            int16 *senscr,
                            uint8 *senone_active,
                            int32 n_senone_active,
                            mfcc_t ** feat,
                            int32 frame,
                            int32 compallsen);

/**
 * Acoustic model structure.
 *
 * This object encapsulates all stages of acoustic processing, from
 * raw audio input to acoustic score output.  The reason for grouping
 * all of these modules together is that they all have to "agree" in
 * their parameterizations, and the configuration of the acoustic and
 * dynamic feature computation is completely dependent on the
 * parameters used to build the original acoustic model (which should
 * by now always be specified in a feat.params file).
 *
 * Because there is not a one-to-one correspondence from blocks of
 * input audio or frames of input features to frames of acoustic
 * scores (due to dynamic feature calculation), results may not be
 * immediately available after input, and the output results will not
 * correspond to the last piece of data input.
 */
struct acmod_s {
    /* Global objects, not retained. */
    cmd_ln_t *config;          /**< Configuration. */
    logmath_t *lmath;          /**< Log-math computation. */
    glist_t strings;           /**< Temporary acoustic model filenames. */

    /* Feature computation: */
    fe_t *fe;                  /**< Acoustic feature computation. */
    feat_t *fcb;               /**< Dynamic feature computation. */

    /* Model parameters: */
    bin_mdef_t *mdef;          /**< Model definition. */
    tmat_t *tmat;              /**< Transition matrices. */
    void *mgau;                /**< either s2_semi_mgau_t or
                                  ms_mgau_t, will make this more
                                  type-safe in the future. */
    void (*mgau_free)(void *); /**< Function to dealloate mgau. */

    /* Senone scoring: */
    frame_eval_t frame_eval;   /**< Function to compute GMM scores. */
    int16 *senone_scores;      /**< GMM scores for current frame. */
    bitvec_t *senone_active_vec; /**< Active GMMs in current frame. */
    uint8 *senone_active;      /**< Array of deltas to active GMMs. */
    int n_senone_active;       /**< Number of active GMMs. */
    int log_zero;              /**< Zero log-probability value. */

    /* Utterance processing: */
    mfcc_t **mfc_buf;   /**< Temporary buffer of acoustic features. */
    mfcc_t ***feat_buf; /**< Temporary buffer of dynamic features. */
    FILE *rawfh;        /**< File for writing raw audio data. */
    FILE *mfcfh;        /**< File for writing acoustic feature data. */

    /* A whole bunch of flags and counters: */
    uint8 state;        /**< State of utterance processing. */
    uint8 compallsen;   /**< Compute all senones? */
    uint8 grow_feat;    /**< Whether to grow feat_buf. */
    uint8 reserved;
    int16 output_frame; /**< Index of next frame of dynamic features. */
    int16 n_mfc_alloc;  /**< Number of frames allocated in mfc_buf */
    int16 n_mfc_frame;  /**< Number of frames active in mfc_buf */
    int16 mfc_outidx;   /**< Start of active frames in mfc_buf */
    int16 n_feat_alloc; /**< Number of frames allocated in feat_buf */
    int16 n_feat_frame; /**< Number of frames active in feat_buf */
    int16 feat_outidx;  /**< Start of active frames in feat_buf */
};
typedef struct acmod_s acmod_t;

/**
 * Initialize an acoustic model.
 *
 * @param config a command-line object containing parameters.  This
 *               pointer is not retained by this object.
 * @param lmath global log-math parameters.
 * @param fe a previously-initialized acoustic feature module to use,
 *           or NULL to create one automatically.  If this is supplied
 *           and its parameters do not match those in the acoustic
 *           model, this function will fail.  This pointer is not retained.
 * @param fe a previously-initialized dynamic feature module to use,
 *           or NULL to create one automatically.  If this is supplied
 *           and its parameters do not match those in the acoustic
 *           model, this function will fail.  This pointer is not retained.
 * @return a newly initialized acmod_t, or NULL on failure.
 */
acmod_t *acmod_init(cmd_ln_t *config, logmath_t *lmath, fe_t *fe, feat_t *fcb);

/**
 * Start logging MFCCs to a filehandle.
 *
 * @param acmod Acoustic model object.
 * @param logfh Filehandle to log to.
 * @return 0 for success, <0 on error.
 */
int acmod_set_mfcfh(acmod_t *acmod, FILE *logfh);

/**
 * Start logging raw audio to a filehandle.
 *
 * @param acmod Acoustic model object.
 * @param logfh Filehandle to log to.
 * @return 0 for success, <0 on error.
 */
int acmod_set_rawfh(acmod_t *acmod, FILE *logfh);

/**
 * Finalize an acoustic model.
 */
void acmod_free(acmod_t *acmod);

/**
 * Mark the start of an utterance.
 */
int acmod_start_utt(acmod_t *acmod);

/**
 * Mark the end of an utterance.
 */
int acmod_end_utt(acmod_t *acmod);

/**
 * Rewind the current utterance, allowing it to be rescored.
 *
 * After calling this function, the internal frame index is reset, and
 * acmod_score() will return scores starting at the first frame of the
 * current utterance.  Currently, acmod_set_grow() must have been
 * called to enable growing the feature buffer in order for this to
 * work.  In the future, senone scores may be cached instead.
 *
 * @return 0 for success, <0 for failure (if the utterance can't be
 *         rewound due to no feature or score data available)
 */
int acmod_rewind(acmod_t *acmod);

/**
 * Set memory allocation policy for utterance processing.
 *
 * @param grow_feat If non-zero, the internal dynamic feature buffer
 * will expand as necessary to encompass any amount of data fed to the
 * model.
 * @return previous allocation policy.
 */
int acmod_set_grow(acmod_t *acmod, int grow_feat);

/**
 * Feed raw audio data to the acoustic model for scoring.
 *
 * @param inout_raw In: Pointer to buffer of raw samples
 *                  Out: Pointer to next sample to be read
 * @param inout_n_samps In: Number of samples available
 *                      Out: Number of samples remaining
 * @param full_utt If non-zero, this block represents a full
 *                 utterance and should be processed as such.
 * @return Number of frames of data processed.
 */
int acmod_process_raw(acmod_t *acmod,
                      int16 const **inout_raw,
                      size_t *inout_n_samps,
                      int full_utt);

/**
 * Feed acoustic feature data into the acoustic model for scoring.
 *
 * @param inout_cep In: Pointer to buffer of features
 *                  Out: Pointer to next frame to be read
 * @param inout_n_frames In: Number of frames available
 *                      Out: Number of frames remaining
 * @param full_utt If non-zero, this block represents a full
 *                 utterance and should be processed as such.
 * @return Number of frames of data processed.
 */
int acmod_process_cep(acmod_t *acmod,
                      mfcc_t ***inout_cep,
                      int *inout_n_frames,
                      int full_utt);

/**
 * Feed dynamic feature data into the acoustic model for scoring.
 *
 * Unlike acmod_process_raw() and acmod_process_cep(), this function
 * accepts a single frame at a time.  This is because there is no need
 * to do buffering when using dynamic features as input.  However, if
 * the dynamic feature buffer is full, this function will fail, so you
 * should either always check the return value, or always pair a call
 * to it with a call to acmod_score().
 *
 * @param feat Pointer to one frame of dynamic features.
 * @return Number of frames processed (either 0 or 1).
 */
int acmod_process_feat(acmod_t *acmod,
                       mfcc_t **feat);

/**
 * Get the current frame index.
 */                       
int acmod_frame_idx(acmod_t *acmod);

/**
 * Score one frame of data.
 *
 * @param out_frame_idx  Output: frame index corresponding to this set
 *                       of scores.
 * @return Array of senone scores for this frame, or NULL if no frame
 *         is available for scoring.  The data pointed to persists only
 *         until the next call to acmod_score().
 */
int16 const *acmod_score(acmod_t *acmod,
                         int *out_frame_idx);

/**
 * Get best score and senone index for current frame.
 */
int acmod_best_score(acmod_t *acmod, int *out_best_senid);

/**
 * Clear set of active senones.
 */
void acmod_clear_active(acmod_t *acmod);

/**
 * Activate senones associated with an HMM.
 */
void acmod_activate_hmm(acmod_t *acmod, hmm_t *hmm);

#endif /* __ACMOD_H__ */
