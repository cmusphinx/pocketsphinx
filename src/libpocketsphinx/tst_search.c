/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 2009 Carnegie Mellon University.  All rights
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
 * @file tst_search.c Time-conditioned lexicon tree search ("S3")
 */

/* System headers. */
#include <string.h>
#include <assert.h>

/* SphinxBase headers. */
#include <ckd_alloc.h>
#include <listelem_alloc.h>
#include <err.h>

/* Local headers. */
#include "pocketsphinx_internal.h"
#include "ps_lattice_internal.h"
#include "tst_search.h"

static int tst_search_start(ps_search_t *search);
static int tst_search_step(ps_search_t *search, int frame_idx);
static int tst_search_finish(ps_search_t *search);
static int tst_search_reinit(ps_search_t *search);
static ps_lattice_t *tst_search_lattice(ps_search_t *search);
static char const *tst_search_hyp(ps_search_t *search, int32 *out_score);
static int32 tst_search_prob(ps_search_t *search);
static ps_seg_t *tst_search_seg_iter(ps_search_t *search, int32 *out_score);

static ps_searchfuncs_t tst_funcs = {
    /* name: */   "tst",
    /* start: */  tst_search_start,
    /* step: */   tst_search_step,
    /* finish: */ tst_search_finish,
    /* reinit: */ tst_search_reinit,
    /* free: */   tst_search_free,
    /* lattice: */  tst_search_lattice,
    /* hyp: */      tst_search_hyp,
    /* prob: */     tst_search_prob,
    /* seg_iter: */ tst_search_seg_iter,
};

static int
tst_search_start(ps_search_t *search)
{
    return -1;
}

static int
tst_search_step(ps_search_t *search, int frame_idx)
{
    return -1;
}

static int
tst_search_finish(ps_search_t *search)
{
    return -1;
}

static int
tst_search_reinit(ps_search_t *search)
{
    return -1;
}

static ps_lattice_t *
tst_search_lattice(ps_search_t *search)
{
    return NULL;
}

static char const *
tst_search_hyp(ps_search_t *search, int32 *out_score)
{
    return NULL;
}

static int32
tst_search_prob(ps_search_t *search)
{
    return 1;
}

static ps_seg_t *
tst_search_seg_iter(ps_search_t *search, int32 *out_score)
{
    return NULL;
}
