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
 */

#ifndef __ACMOD_H__
#define __ACMOD_H__

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

/**
 * Acoustic model structure.
 */
struct acmod_s {
    bin_mdef_t *mdef;          /**< Model definition. */
    tmat_t *tmat;              /**< Transition matrices. */
    /* Means, variances, and mixture weights: */
    s2_semi_mgau_t *semi_mgau; /**< Fast semi-continuous models. */
    ms_mgau_model_t *ms_mgau;  /**< Slow generic models. */

    int32 *senone_scores;        /**< GMM scores for current frame. */
    bitvec_t *senone_active_vec; /**< Active GMMs in current frame. */
    int32 *senone_active;        /**< Array of active GMMs. */
    int32 n_senone_active;       /**< Number of active GMMs. */
};
typedef struct acmod_s acmod_t;

#endif /* __ACMOD_H__ */
