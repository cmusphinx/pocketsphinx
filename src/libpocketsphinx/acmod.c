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
 * @file acmod.c Acoustic model structures for PocketSphinx.
 * @author David Huggins-Daines <dhuggins@cs.cmu.edu>
 */

/* System headers. */

/* SphinxBase headers. */
#include <cmd_ln.h>

/* Local headers. */
#include "cmdln_macro.h"
#include "acmod.h"

/* Feature and front-end parameters that may be in feat.params */
static const arg_t feat_defn[] = {
    waveform_to_cepstral_command_line_macro(),
    input_cmdln_options(),
    CMDLN_EMPTY_OPTION
};

acmod_t *
acmod_init(cmd_ln_t *config, fe_t *fe, feat_t *fcb)
{
    acmod_t *acmod;

    acmod = ckd_calloc(1, sizeof(*acmod));
    if (fe) {
        /* Verify parameter match. */
        acmod->fe = fe;
    }
    if (fcb) {
        acmod->fcb = fcb;
    }
    return acmod;
}

void
acmod_free(acmod_t *acmod)
{
}

int
acmod_process_raw(acmod_t *acmod,
		  int16 const **inout_raw,
		  int *inout_n_samps,
		  int full_utt)
{
    return 0;
}

int
acmod_process_cep(acmod_t *acmod,
		  mfcc_t const ***inout_cep,
		  int *inout_n_frames,
		  int full_utt)
{
    return 0;
}

int
acmod_process_feat(acmod_t *acmod,
		   mfcc_t const ***feat)
{
    return 0;
}

int32 const *
acmod_score(acmod_t *acmod,
	    int *out_frame_idx,
	    int *out_best_score,
	    int *out_best_senid)
{
    int32 best_score = acmod->log_zero;
    int32 best_idx = -1;

    /* If senone scores haven't been generated for this frame,
     * compute them. */

    if (out_frame_idx) *out_frame_idx = acmod->output_frame;
    if (out_best_score) *out_best_score = best_score;
    if (out_best_senid) *out_best_senid = best_idx;
    return acmod->senone_scores;
}

void
acmod_clear_active(acmod_t *acmod)
{
}

void
acmod_activate_hmm(acmod_t *acmod, hmm_t *hmm)
{
}

int32 const *
acmod_active_list(acmod_t *acmod, int32 *out_n_active)
{
    /* If active list has not been generated for this frame,
     * generate it from the active bitvector. */

    if (out_n_active) *out_n_active = acmod->n_senone_active;
    return acmod->senone_active;
}
