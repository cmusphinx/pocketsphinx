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
#include <prim_type.h>
#include <cmd_ln.h>
#include <strfuncs.h>
#include <string.h>

/* Local headers. */
#include "cmdln_macro.h"
#include "acmod.h"

/* Feature and front-end parameters that may be in feat.params */
static const arg_t feat_defn[] = {
    waveform_to_cepstral_command_line_macro(),
    input_cmdln_options(),
    CMDLN_EMPTY_OPTION
};

static int
acmod_init_am(acmod_t *acmod)
{
    char *mdeffn, *tmatfn, *featparams;

    /* Look for feat.params in acoustic model dir. */
    if ((featparams = cmd_ln_str_r(acmod->config, "-featparams"))) {
        if (cmd_ln_parse_file_r(acmod->config, feat_defn, featparams, FALSE) != NULL) {
	    E_INFO("Parsed model-specific feature parameters from %s\n", featparams);
        }
    }

    /* Read model definition. */
    if ((mdeffn = cmd_ln_str_r(acmod->config, "-mdef")) == NULL) {
        E_ERROR("Must specify -mdef or -hmm\n");
        return -1;
    }

    if ((acmod->mdef = bin_mdef_read(mdeffn)) == NULL) {
        E_ERROR("Failed to read model definition from %s\n", mdeffn);
        return -1;
    }

    /* Read transition matrices. */
    if ((tmatfn = cmd_ln_str_r(acmod->config, "-tmat")) == NULL) {
        E_ERROR("No tmat file specified\n");
        return -1;
    }
    acmod->tmat = tmat_init(tmatfn, acmod->lmath,
                            cmd_ln_float32_r(acmod->config, "-tmatfloor"),
                            TRUE);

    /* Read the acoustic models. */
    if ((cmd_ln_str_r(acmod->config, "-mean") == NULL)
        || (cmd_ln_str_r(acmod->config, "-var") == NULL)
        || (cmd_ln_str_r(acmod->config, "-tmat") == NULL)) {
        E_ERROR("No mean/var/tmat files specified\n");
        return -1;
    }

    E_INFO("Attempting to use SCGMM computation module\n");
    acmod->mgau
        = s2_semi_mgau_init(acmod->config, acmod->lmath, acmod->mdef);
    if (acmod->mgau) {
        char *kdtreefn = cmd_ln_str_r(acmod->config, "-kdtree");
        if (kdtreefn)
            s2_semi_mgau_load_kdtree(acmod->mgau, kdtreefn,
                                     cmd_ln_int32_r(acmod->config, "-kdmaxdepth"),
                                     cmd_ln_int32_r(acmod->config, "-kdmaxbbi"));
        acmod->frame_eval = (frame_eval_t)&s2_semi_mgau_frame_eval;
    }
    else {
        E_INFO("Falling back to general multi-stream GMM computation\n");
        acmod->mgau =
            ms_mgau_init(acmod->config, acmod->lmath);
        acmod->frame_eval = (frame_eval_t)&ms_cont_mgau_frame_eval;
    }

    return 0;
}

static int
acmod_init_feat(acmod_t *acmod)
{
    acmod->fcb = 
        feat_init(cmd_ln_str_r(acmod->config, "-feat"),
                  cmn_type_from_str(cmd_ln_str_r(acmod->config,"-cmn")),
                  cmd_ln_boolean_r(acmod->config, "-varnorm"),
                  agc_type_from_str(cmd_ln_str_r(acmod->config, "-agc")),
                  1, cmd_ln_int32_r(acmod->config, "-ceplen"));
    if (acmod->fcb == NULL)
        return -1;

    if (cmd_ln_exists_r(acmod->config, "-agcthresh")
        && 0 != strcmp(cmd_ln_str_r(acmod->config, "-agc"), "none")) {
        agc_set_threshold(acmod->fcb->agc_struct,
                          cmd_ln_float32_r(acmod->config, "-agcthresh"));
    }

    if (cmd_ln_exists_r(acmod->config, "-cmninit")
        && 0 == strcmp(cmd_ln_str_r(acmod->config, "-cmn"), "prior")) {
        char *c, *cc, *vallist;
        int32 nvals;

        vallist = ckd_salloc(cmd_ln_str_r(acmod->config, "-cmninit"));
        c = vallist;
        nvals = 0;
        while (nvals < acmod->fcb->cmn_struct->veclen
               && (cc = strchr(c, ',')) != NULL) {
            *cc = '\0';
            acmod->fcb->cmn_struct->cmn_mean[nvals] = FLOAT2MFCC(atof(c));
            c = cc + 1;
            ++nvals;
        }
        if (nvals < acmod->fcb->cmn_struct->veclen && *c != '\0') {
            acmod->fcb->cmn_struct->cmn_mean[nvals] = FLOAT2MFCC(atof(c));
        }
        ckd_free(vallist);
    }
    return 0;
}

int
acmod_fe_mismatch(acmod_t *acmod, fe_t *fe)
{
    /* Output vector dimension needs to be the same. */
    if (cmd_ln_int32_r(acmod->config, "-ceplen") != fe_get_output_size(fe))
        return TRUE;
    /* Feature parameters need to be the same. */
    /* ... */
    return FALSE;
}

int
acmod_feat_mismatch(acmod_t *acmod, feat_t *fcb)
{
    /* Feature type needs to be the same. */
    if (0 != strcmp(cmd_ln_str_r(acmod->config, "-feat"), feat_name(fcb)))
        return TRUE;
    /* Input vector dimension needs to be the same. */
    if (cmd_ln_int32_r(acmod->config, "-ceplen") != feat_cepsize(fcb))
        return TRUE;
    /* FIXME: Need to check LDA and stuff too. */
    return FALSE;
}

acmod_t *
acmod_init(cmd_ln_t *config, logmath_t *lmath, fe_t *fe, feat_t *fcb)
{
    acmod_t *acmod;

    acmod = ckd_calloc(1, sizeof(*acmod));
    acmod->config = config;
    acmod->lmath = lmath;
    acmod->state = ACMOD_IDLE;

    /* Load acoustic model parameters. */
    if (acmod_init_am(acmod) < 0)
        goto error_out;

    if (fe) {
        if (acmod_fe_mismatch(acmod, fe))
            goto error_out;
        acmod->fe = fe;
    }
    else {
        /* Initialize a new front end. */
        acmod->retain_fe = TRUE;
        acmod->fe = fe_init_auto_r(config);
        if (acmod->fe == NULL)
            goto error_out;
    }
    if (fcb) {
        if (acmod_feat_mismatch(acmod, fcb))
            goto error_out;
        acmod->fcb = fcb;
    }
    else {
        /* Initialize a new fcb. */
        acmod->retain_fcb = TRUE;
        if (acmod_init_feat(acmod) < 0)
            goto error_out;
    }

    /* The MFCC buffer needs to be at least as large as the dynamic
     * feature window.  */
    acmod->n_mfc_alloc = acmod->fcb->window_size * 2 + 1;
    acmod->mfc_buf = (mfcc_t **)
        ckd_calloc_2d(acmod->n_mfc_alloc, acmod->fcb->cepsize,
                      sizeof(**acmod->mfc_buf));

    /* Feature buffer has to be at least as large as MFCC buffer. */
    acmod->n_feat_alloc = acmod->n_mfc_alloc;
    acmod->feat_buf = feat_array_alloc(acmod->fcb, acmod->n_feat_alloc);

    /* Senone computation stuff. */
    acmod->senone_scores = ckd_calloc(bin_mdef_n_sen(acmod->mdef),
                                                     sizeof(*acmod->senone_scores));
    acmod->senone_active_vec = bitvec_alloc(bin_mdef_n_sen(acmod->mdef));
    acmod->senone_active = ckd_calloc(bin_mdef_n_sen(acmod->mdef),
                                                     sizeof(*acmod->senone_active));
    acmod->log_zero = logmath_get_zero(acmod->lmath);
    acmod->compallsen = cmd_ln_boolean_r(config, "-compallsen");
    return acmod;

error_out:
    acmod_free(acmod);
    return NULL;
}

void
acmod_free(acmod_t *acmod)
{
    if (acmod->retain_fcb)
        feat_free(acmod->fcb);
    if (acmod->retain_fe)
        fe_close(acmod->fe);
    ckd_free_2d((void **)acmod->mfc_buf);
    feat_array_free(acmod->feat_buf);
    ckd_free(acmod);
}

int
acmod_start_utt(acmod_t *acmod)
{
    fe_start_utt(acmod->fe);
    acmod->state = ACMOD_STARTED;
    acmod->n_mfc_frame = 0;
    acmod->n_feat_frame = 0;
    acmod->output_frame = 0;
    return 0;
}

int
acmod_end_utt(acmod_t *acmod)
{
    int32 nfr;

    acmod->state = ACMOD_ENDED;
    if (acmod->n_mfc_frame < acmod->n_mfc_alloc) {
        fe_end_utt(acmod->fe, acmod->mfc_buf[acmod->n_mfc_frame], &nfr);
    }
    acmod->n_mfc_frame += nfr;
    return 0;
}

int
acmod_process_raw(acmod_t *acmod,
		  int16 const **inout_raw,
		  size_t *inout_n_samps,
		  int full_utt)
{
    /* If this is a full utterance, process it all at once. */
    if (full_utt) {
        int32 nfr, ntail;

        /* Resize mfc_buf to fit. */
        if (fe_process_frames(acmod->fe, NULL, inout_n_samps, NULL, &nfr) < 0)
            return -1;
        if (acmod->n_mfc_alloc < nfr + 1) {
            ckd_free_2d(acmod->mfc_buf);
            acmod->mfc_buf = ckd_calloc_2d(nfr + 1, fe_get_output_size(acmod->fe),
                                           sizeof(**acmod->mfc_buf));
            acmod->n_mfc_alloc = nfr + 1;
        }
        acmod->n_mfc_frame = 0;
        fe_start_utt(acmod->fe);
        if (fe_process_frames(acmod->fe, inout_raw, inout_n_samps,
                              acmod->mfc_buf, &nfr) < 0)
            return -1;
        fe_end_utt(acmod->fe, acmod->mfc_buf[nfr], &ntail);
        nfr += ntail;
        acmod->n_mfc_frame = nfr;

        /* Resize feat_buf to fit. */
        if (acmod->n_feat_alloc < nfr) {
            feat_array_free(acmod->feat_buf);
            acmod->feat_buf = feat_array_alloc(acmod->fcb, nfr);
            acmod->n_feat_alloc = nfr;
            acmod->n_feat_frame = 0;
        }
        /* Make dynamic features. */
        acmod->n_feat_frame =
            feat_s2mfc2feat_live(acmod->fcb, acmod->mfc_buf, &nfr,
                                 TRUE, TRUE, acmod->feat_buf);
        /* Mark all MFCCs as consumed. */
        acmod->n_mfc_frame = 0;
        return nfr;
    }
    else {
        int32 nfeat, ncep;

        /* Append MFCCs to the end of any that are previously in there
         * (in practice, there will probably be none) */
        if (inout_n_samps && *inout_n_samps) {
            ncep = acmod->n_mfc_alloc - acmod->n_mfc_frame;
            if (fe_process_frames(acmod->fe, inout_raw, inout_n_samps,
                                  acmod->mfc_buf + acmod->n_mfc_frame, &ncep) < 0)
                return -1;
            acmod->n_mfc_frame += ncep;
        }

        /* Number of input frames to generate features. */
        ncep = acmod->n_mfc_frame;
        /* Don't overflow the output feature buffer. */
        if (ncep > acmod->n_feat_alloc - acmod->n_feat_frame)
            ncep = acmod->n_feat_alloc - acmod->n_feat_frame;
        nfeat = feat_s2mfc2feat_live(acmod->fcb, acmod->mfc_buf, &ncep,
                                     (acmod->state == ACMOD_STARTED),
                                     (acmod->state == ACMOD_ENDED),
                                     acmod->feat_buf + acmod->n_feat_frame);
        acmod->n_feat_frame += nfeat;
        /* Free up space in the MFCC buffer. */
        /* FIXME: we should use circular buffers here instead. */
        /* At the end of the utterance we get more features out than
         * we put in MFCCs. */
        acmod->n_mfc_frame -= ncep;
        memmove(acmod->mfc_buf[0],
                acmod->mfc_buf[ncep],
                (acmod->n_mfc_frame
                 * fe_get_output_size(acmod->fe)
                 * sizeof(**acmod->mfc_buf)));
        acmod->state = ACMOD_PROCESSING;
        return ncep;
    }
}

int
acmod_process_cep(acmod_t *acmod,
		  mfcc_t ***inout_cep,
		  int *inout_n_frames,
		  int full_utt)
{
    /* If this is a full utterance, process it all at once. */
    if (full_utt) {
        int32 nfr;

        /* Resize feat_buf to fit. */
        if (acmod->n_feat_alloc < *inout_n_frames) {
            feat_array_free(acmod->feat_buf);
            acmod->feat_buf = feat_array_alloc(acmod->fcb, *inout_n_frames);
            acmod->n_feat_alloc = *inout_n_frames;
            acmod->n_feat_frame = 0;
        }
        /* Make dynamic features. */
        nfr = feat_s2mfc2feat_live(acmod->fcb, *inout_cep, inout_n_frames,
                                   TRUE, TRUE, acmod->feat_buf);
        acmod->n_feat_frame = nfr;
        *inout_cep += *inout_n_frames;
        *inout_n_frames = 0;
        return nfr;
    }
    else {
        int32 nfeat, ncep;

        /* Number of input frames to generate features. */
        ncep = acmod->n_mfc_frame;
        /* Don't overflow the output feature buffer. */
        if (ncep > acmod->n_feat_alloc - acmod->n_feat_frame)
            ncep = acmod->n_feat_alloc - acmod->n_feat_frame;
        nfeat = feat_s2mfc2feat_live(acmod->fcb, *inout_cep,
                                     &ncep,
                                     (acmod->state == ACMOD_STARTED),
                                     (acmod->state == ACMOD_ENDED),
                                     acmod->feat_buf + acmod->n_feat_frame);
        acmod->n_feat_frame += nfeat;
        *inout_n_frames -= ncep;
        *inout_cep += ncep;
        acmod->state = ACMOD_PROCESSING;
        return ncep;
    }
}

int
acmod_process_feat(acmod_t *acmod,
		   mfcc_t **feat)
{
    if (acmod->n_feat_frame == acmod->n_feat_alloc)
        return 0;
    else {
        /* Just copy it in. */
        int i;
        for (i = 0; i < feat_n_stream(acmod->fcb); ++i)
            memcpy(acmod->feat_buf[acmod->n_feat_frame][i],
                   feat[i], feat_stream_len(acmod->fcb, i) * sizeof(**feat));
        return 1;
    }
}

int32 const *
acmod_score(acmod_t *acmod,
	    int *out_frame_idx,
	    int *out_best_score,
	    int *out_best_senid)
{
    /* No frames available to score. */
    if (acmod->n_feat_frame == 0)
        return NULL;

    /* Generate scores for the next available frame */
    *out_best_score = 
        (*acmod->frame_eval)(acmod->mgau,
                             acmod->senone_scores,
                             acmod->senone_active,
                             acmod->n_senone_active,
                             acmod->feat_buf[0],
                             acmod->output_frame, TRUE,
                             out_best_senid);
    /* Shift back the rest of the feature buffer (FIXME: we should
     * really use circular buffers here) */
    --acmod->n_feat_frame;
    memmove(acmod->feat_buf[0][0], acmod->feat_buf[1][0],
            (acmod->n_feat_frame
             * feat_dimension(acmod->fcb)
             * sizeof(***acmod->feat_buf)));

    *out_frame_idx = acmod->output_frame;
    ++acmod->output_frame;

    return acmod->senone_scores;
}

void
acmod_clear_active(acmod_t *acmod)
{
    bitvec_clear_all(acmod->senone_active_vec, bin_mdef_n_sen(acmod->mdef));
    acmod->n_senone_active = 0;
}

#define MPX_BITVEC_SET(a,h,i)                                           \
    if ((h)->s.mpx_ssid[i] != -1)                                       \
        bitvec_set((a)->senone_active_vec,                              \
                   bin_mdef_sseq2sen((a)->mdef, (h)->s.mpx_ssid[i], (i)));
#define NONMPX_BITVEC_SET(a,h,i)                                        \
    bitvec_set((a)->senone_active_vec,                                  \
               bin_mdef_sseq2sen((a)->mdef, (h)->s.ssid, (i)));

void
acmod_activate_hmm(acmod_t *acmod, hmm_t *hmm)
{
    int i;

    if (hmm_is_mpx(hmm)) {
        switch (hmm_n_emit_state(hmm)) {
        case 5:
            MPX_BITVEC_SET(acmod, hmm, 4);
            MPX_BITVEC_SET(acmod, hmm, 3);
        case 3:
            MPX_BITVEC_SET(acmod, hmm, 2);
            MPX_BITVEC_SET(acmod, hmm, 1);
            MPX_BITVEC_SET(acmod, hmm, 0);
            break;
        default:
            for (i = 0; i < hmm_n_emit_state(hmm); ++i) {
                MPX_BITVEC_SET(acmod, hmm, i);
            }
        }
    }
    else {
        switch (hmm_n_emit_state(hmm)) {
        case 5:
            NONMPX_BITVEC_SET(acmod, hmm, 4);
            NONMPX_BITVEC_SET(acmod, hmm, 3);
        case 3:
            NONMPX_BITVEC_SET(acmod, hmm, 2);
            NONMPX_BITVEC_SET(acmod, hmm, 1);
            NONMPX_BITVEC_SET(acmod, hmm, 0);
            break;
        default:
            for (i = 0; i < hmm_n_emit_state(hmm); ++i) {
                NONMPX_BITVEC_SET(acmod, hmm, i);
            }
        }
    }
}

int32
acmod_flags2list(acmod_t *acmod)
{
    int32 i, j, total_dists, total_bits;
    bitvec_t flagptr;

    total_dists = bin_mdef_n_sen(acmod->mdef);

    j = 0;
    total_bits = total_dists & -32;
    for (i = 0, flagptr = acmod->senone_active_vec; i < total_bits; flagptr++) {
        int32 bits = *flagptr;

        if (bits == 0) {
            i += 32;
            continue;
        }

        if (bits & (1 << 0))
            acmod->senone_active[j++] = i;
        ++i;
        if (bits & (1 << 1))
            acmod->senone_active[j++] = i;
        ++i;
        if (bits & (1 << 2))
            acmod->senone_active[j++] = i;
        ++i;
        if (bits & (1 << 3))
            acmod->senone_active[j++] = i;
        ++i;
        if (bits & (1 << 4))
            acmod->senone_active[j++] = i;
        ++i;
        if (bits & (1 << 5))
            acmod->senone_active[j++] = i;
        ++i;
        if (bits & (1 << 6))
            acmod->senone_active[j++] = i;
        ++i;
        if (bits & (1 << 7))
            acmod->senone_active[j++] = i;
        ++i;
        if (bits & (1 << 8))
            acmod->senone_active[j++] = i;
        ++i;
        if (bits & (1 << 9))
            acmod->senone_active[j++] = i;
        ++i;
        if (bits & (1 << 10))
            acmod->senone_active[j++] = i;
        ++i;
        if (bits & (1 << 11))
            acmod->senone_active[j++] = i;
        ++i;
        if (bits & (1 << 12))
            acmod->senone_active[j++] = i;
        ++i;
        if (bits & (1 << 13))
            acmod->senone_active[j++] = i;
        ++i;
        if (bits & (1 << 14))
            acmod->senone_active[j++] = i;
        ++i;
        if (bits & (1 << 15))
            acmod->senone_active[j++] = i;
        ++i;
        if (bits & (1 << 16))
            acmod->senone_active[j++] = i;
        ++i;
        if (bits & (1 << 17))
            acmod->senone_active[j++] = i;
        ++i;
        if (bits & (1 << 18))
            acmod->senone_active[j++] = i;
        ++i;
        if (bits & (1 << 19))
            acmod->senone_active[j++] = i;
        ++i;
        if (bits & (1 << 20))
            acmod->senone_active[j++] = i;
        ++i;
        if (bits & (1 << 21))
            acmod->senone_active[j++] = i;
        ++i;
        if (bits & (1 << 22))
            acmod->senone_active[j++] = i;
        ++i;
        if (bits & (1 << 23))
            acmod->senone_active[j++] = i;
        ++i;
        if (bits & (1 << 24))
            acmod->senone_active[j++] = i;
        ++i;
        if (bits & (1 << 25))
            acmod->senone_active[j++] = i;
        ++i;
        if (bits & (1 << 26))
            acmod->senone_active[j++] = i;
        ++i;
        if (bits & (1 << 27))
            acmod->senone_active[j++] = i;
        ++i;
        if (bits & (1 << 28))
            acmod->senone_active[j++] = i;
        ++i;
        if (bits & (1 << 29))
            acmod->senone_active[j++] = i;
        ++i;
        if (bits & (1 << 30))
            acmod->senone_active[j++] = i;
        ++i;
        if (bits & (1 << 31))
            acmod->senone_active[j++] = i;
        ++i;
    }

    for (; i < total_dists; ++i)
        if (*flagptr & (1 << (i % 32)))
            acmod->senone_active[j++] = i;

    acmod->n_senone_active = j;

    return j;
}

int const *
acmod_active_list(acmod_t *acmod, int32 *out_n_active)
{
    /* If active list has not been generated for this frame,
     * generate it from the active bitvector. */

    if (out_n_active) *out_n_active = acmod->n_senone_active;
    return acmod->senone_active;
}
