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
#include "ms_mgau.h"
#include "tmat.h"
#include "hmm.h"

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
    /* Feature computation: */
    fe_t *fe;                  /**< Acoustic feature computation. */
    feat_t *fcb;               /**< Dynamic feature computation. */

    uint8 retain_fe;           /**< Do we own the fe pointer. */
    uint8 retain_fcb;          /**< Do we own the fcb poitner. */
    int16 output_frame;        /**< Index of last computed frame of dynamic features. */

    /* Model parameters: */
    bin_mdef_t *mdef;          /**< Model definition. */
    tmat_t *tmat;              /**< Transition matrices. */
    s2_semi_mgau_t *semi_mgau; /**< Fast semi-continuous models. */
    ms_mgau_model_t *ms_mgau;  /**< Slow generic models. */

    /* Senone scoring: */
    int32 *senone_scores;        /**< GMM scores for current frame. */
    bitvec_t *senone_active_vec; /**< Active GMMs in current frame. */
    int32 *senone_active;        /**< Array of active GMMs. */
    int32 n_senone_active;       /**< Number of active GMMs. */
    int32 log_zero;              /**< Zero log-probability value. */

    /* Feature computation: */
    mfcc_t **mfc_buf;   /**< Temporary buffer of acoustic features. */
    mfcc_t ***feat_buf; /**< Temporary buffer of dynamic features. */
    int16 n_mfc_alloc;  /**< Number of frames allocated in mfc_buf */
    int16 n_mfc_frame;  /**< Number of frames active in mfc_buf */
    int16 n_feat_alloc; /**< Number of frames allocated in feat_buf */
    int16 n_feat_frame; /**< Number of frames active in feat_buf */
    char *mfclogdir;    /**< Log directory for MFCC files. */
    char *rawlogdir;    /**< Log directory for raw audio files. */
    FILE *rawfp;        /**< File for writing raw audio data. */
    FILE *mfcfp;        /**< File for writing acoustic feature data. */
};
typedef struct acmod_s acmod_t;

/**
 * Initialize an acoustic model.
 *
 * @param config a command-line object containing parameters.  This
 *               pointer is not retained by this object.
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
acmod_t *acmod_init(cmd_ln_t *config, fe_t *fe, feat_t *fcb);

/**
 * Finalize an acoustic model.
 */
void acmod_free(acmod_t *acmod);

/**
 * Feed raw audio data to the acoustic model for scoring.
 *
 * @param inout_raw In: Pointer to buffer of raw samples
 *                  Out: Pointer to next sample to be read
 * @param inout_n_samps In: Number of samples available
 *                      Out: Number of samples remaining
 * @param full_utt If non-zero, this block represents a full
 *                 utterance and should be processed as such.
 * @return Number of frames of data processed
 */
int acmod_process_raw(acmod_t *acmod,
                      int16 const **inout_raw,
                      int *inout_n_samps,
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
 * @return Number of frames of data processed
 */
int acmod_process_cep(acmod_t *acmod,
                      mfcc_t const ***inout_cep,
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
                       mfcc_t const ***feat);
                       

/**
 * Score one frame of data.
 *
 * @param out_frame_idx  Output: frame index corresponding to this set
 *                       of scores.
 * @param out_best_score Output: best un-normalized acoustic score.
 * @param out_best_senid Output: senone ID corresponding to best score.
 * @return Array of senone scores for this frame, or NULL if no frame
 *         is available for scoring.  The data pointed to persists only
 *         until the next call to acmod_score().
 */
int32 const *acmod_score(acmod_t *acmod,
                         int *out_frame_idx,
                         int *out_best_score,
                         int *out_best_senid);

/**
 * Clear set of active senones.
 */
void acmod_clear_active(acmod_t *acmod);

/**
 * Activate senones associated with an HMM.
 */
void acmod_activate_hmm(acmod_t *acmod, hmm_t *hmm);

/**
 * Return the array of active senones.
 *
 * @param out_n_active Output: number of elements in returned array.
 * @return array of active senone IDs.
 */
int32 const *acmod_active_list(acmod_t *acmod, int32 *out_n_active);

#endif /* __ACMOD_H__ */
