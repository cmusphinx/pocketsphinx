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
 * @file pocketsphinx_internal.h Internal implementation of
 * PocketSphinx decoder.
 * @author David Huggins-Daines <dhuggins@cs.cmu.edu>
 */

#ifndef __POCKETSPHINX_INTERNAL_H__
#define __POCKETSPHINX_INTERNAL_H__

/* SphinxBase headers. */
#include <cmd_ln.h>
#include <logmath.h>
#include <fe.h>
#include <feat.h>

/* Local headers. */
#include "pocketsphinx.h"
#include "acmod.h"
#include "dict.h"
#include "search.h"
#include "fsg_search.h"

/**
 * Utterance processing state.
 */
typedef enum {
    UTTSTATE_UNDEF = -1,  /**< Undefined state */
    UTTSTATE_IDLE = 0,    /**< Idle, can change models, etc. */
    UTTSTATE_BEGUN = 1,   /**< Begun, can only do recognition. */
    UTTSTATE_ENDED = 2,   /**< Ended, a result is now available. */
    UTTSTATE_STOPPED = 3  /**< Stopped, can be resumed. */
} uttstate_t;

/**
 * Decoder object.
 */
struct pocketsphinx_s {
    /* Model parameters and such. */
    cmd_ln_t *config;  /**< Configuration. */
    acmod_t *acmod;    /**< Acoustic model. */
    dict_t *dict;      /**< Pronunciation dictionary. */

    /* Search modules. */
    /* ngram_search_t *ngs; */ /**< N-Gram search module. */
    fsg_search_t *fsgs;  /**< Finite-State search module. */

    /* Utterance-processing related stuff. */
    uttstate_t uttstate;/**< Current state of utterance processing. */
    int32 uttno;        /**< Utterance counter. */
    char *uttid;        /**< Utterance ID for current utterance. */
    char *uttid_prefix; /**< Prefix for automatic utterance IDs. */
    char *hypstr;       /**< Hypothesis string for current utt. */
    search_hyp_t *hyp;  /**< Hypothesis segmentation for current utt. */
    FILE *matchfp;      /**< File for writing recognition results. */
    FILE *matchsegfp;   /**< File for writing segmentation results. */
};

#endif /* __POCKETSPHINX_INTERNAL_H__ */
