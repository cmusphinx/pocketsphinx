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
 * @author David Huggins-Daines <dhdaines@gmail.com>
 */

#ifndef __POCKETSPHINX_INTERNAL_H__
#define __POCKETSPHINX_INTERNAL_H__

#include "fe/fe.h"
#include "feat/feat.h"
#include "util/cmd_ln.h"
#include "util/hash_table.h"
#include "util/profile.h"

#include "acmod.h"
#include "dict.h"
#include "dict2pid.h"

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

/**
 * Search algorithm structure.
 */
typedef struct ps_search_s ps_search_t;


/* Search names*/
#define PS_DEFAULT_SEARCH  "_default"
#define PS_DEFAULT_ALIGN_SEARCH  "_align"
#define PS_DEFAULT_PL_SEARCH  "_default_pl"

/* Search types */
#define PS_SEARCH_TYPE_KWS    "kws"
#define PS_SEARCH_TYPE_FSG    "fsg"
#define PS_SEARCH_TYPE_NGRAM  "ngram"
#define PS_SEARCH_TYPE_ALLPHONE  "allphone"
#define PS_SEARCH_TYPE_STATE_ALIGN  "state_align"
#define PS_SEARCH_TYPE_PHONE_LOOP  "phone_loop"

/**
 * V-table for search algorithm.
 */
typedef struct ps_searchfuncs_s {
    int (*start)(ps_search_t *search);
    int (*step)(ps_search_t *search, int frame_idx);
    int (*finish)(ps_search_t *search);
    int (*reinit)(ps_search_t *search, dict_t *dict, dict2pid_t *d2p);
    void (*free)(ps_search_t *search);

    ps_lattice_t *(*lattice)(ps_search_t *search);
    char const *(*hyp)(ps_search_t *search, int32 *out_score);
    int32 (*prob)(ps_search_t *search);
    ps_seg_t *(*seg_iter)(ps_search_t *search);
} ps_searchfuncs_t;

/**
 * Base structure for search module.
 */
struct ps_search_s {
    ps_searchfuncs_t *vt;  /**< V-table of search methods. */
    
    char *type;
    char *name;
    
    /**
     * Phoneme loop for lookahead.  Reference (not retained) to
     * phone_loop in the parent ps_decoder_t. */
    ps_search_t *pls;
    cmd_ln_t *config;      /**< Configuration. */
    acmod_t *acmod;        /**< Acoustic model. */
    dict_t *dict;        /**< Pronunciation dictionary. */
    dict2pid_t *d2p;       /**< Dictionary to senone mappings. */
    char *hyp_str;         /**< Current hypothesis string. */
    ps_lattice_t *dag;	   /**< Current hypothesis word graph. */
    ps_latlink_t *last_link; /**< Final link in best path. */
    int32 post;            /**< Utterance posterior probability. */
    int32 n_words;         /**< Number of words known to search (may
                              be less than in the dictionary) */

    /* Magical word IDs that must exist in the dictionary: */
    int32 start_wid;       /**< Start word ID. */
    int32 silence_wid;     /**< Silence word ID. */
    int32 finish_wid;      /**< Finish word ID. */
};

#define ps_search_base(s) ((ps_search_t *)s)
#define ps_search_config(s) ps_search_base(s)->config
#define ps_search_acmod(s) ps_search_base(s)->acmod
#define ps_search_dict(s) ps_search_base(s)->dict
#define ps_search_dict2pid(s) ps_search_base(s)->d2p
#define ps_search_dag(s) ps_search_base(s)->dag
#define ps_search_last_link(s) ps_search_base(s)->last_link
#define ps_search_post(s) ps_search_base(s)->post
#define ps_search_lookahead(s) ps_search_base(s)->pls
#define ps_search_n_words(s) ps_search_base(s)->n_words

#define ps_search_type(s) ps_search_base(s)->type
#define ps_search_name(s) ps_search_base(s)->name
#define ps_search_start(s) (*(ps_search_base(s)->vt->start))(s)
#define ps_search_step(s,i) (*(ps_search_base(s)->vt->step))(s,i)
#define ps_search_finish(s) (*(ps_search_base(s)->vt->finish))(s)
#define ps_search_reinit(s,d,d2p) (*(ps_search_base(s)->vt->reinit))(s,d,d2p)
#define ps_search_free(s) (*(ps_search_base(s)->vt->free))(s)
#define ps_search_lattice(s) (*(ps_search_base(s)->vt->lattice))(s)
#define ps_search_hyp(s,sc) (*(ps_search_base(s)->vt->hyp))(s,sc)
#define ps_search_prob(s) (*(ps_search_base(s)->vt->prob))(s)
#define ps_search_seg_iter(s) (*(ps_search_base(s)->vt->seg_iter))(s)

/* For convenience... */
#define ps_search_silence_wid(s) ps_search_base(s)->silence_wid
#define ps_search_start_wid(s) ps_search_base(s)->start_wid
#define ps_search_finish_wid(s) ps_search_base(s)->finish_wid

/**
 * Initialize base structure.
 */
void ps_search_init(ps_search_t *search, ps_searchfuncs_t *vt,
		    const char *type, const char *name,
                    cmd_ln_t *config, acmod_t *acmod, dict_t *dict,
                    dict2pid_t *d2p);


/**
 * Free search
 */
void ps_search_base_free(ps_search_t *search);

/**
 * Re-initialize base structure with new dictionary.
 */
void ps_search_base_reinit(ps_search_t *search, dict_t *dict,
                           dict2pid_t *d2p);

typedef struct ps_segfuncs_s {
    ps_seg_t *(*seg_next)(ps_seg_t *seg);
    void (*seg_free)(ps_seg_t *seg);
} ps_segfuncs_t;

/**
 * Base structure for hypothesis segmentation iterator.
 */
struct ps_seg_s {
    ps_segfuncs_t *vt;     /**< V-table of seg methods */
    ps_search_t *search;   /**< Search object from whence this came */
    const char *text;      /**< Textual representation of segment */
    frame_idx_t sf;        /**< Start frame. */
    frame_idx_t ef;        /**< End frame. */
    s3wid_t wid;           /**< Word ID (*not* base word ID). */
    int32 ascr;            /**< Acoustic score. */
    int32 lscr;            /**< Language model score. */
    int32 prob;            /**< Log posterior probability. */
    /* This doesn't need to be 32 bits, so once the scores above are
     * reduced to 16 bits (or less!), this will be too. */
    int32 lback;           /**< Language model backoff. */
    /* Not sure if this should be here at all. */
    float32 lwf;           /**< Language weight factor (for second-pass searches) */
};

#define ps_search_seg_next(seg) (*(seg->vt->seg_next))(seg)
#define ps_search_seg_free(s) (*(seg->vt->seg_free))(seg)


/**
 * Decoder object.
 */
struct ps_decoder_s {
    /* Model parameters and such. */
    cmd_ln_t *config;  /**< Configuration. */
    int refcount;      /**< Reference count. */

    /* Basic units of computation. */
    acmod_t *acmod;    /**< Acoustic model. */
    dict_t *dict;    /**< Pronunciation dictionary. */
    dict2pid_t *d2p;   /**< Dictionary to senone mapping. */
    logmath_t *lmath;  /**< Log math computation. */

    /* Search modules. */
    hash_table_t *searches;        /**< Set of search modules. */
    /* TODO: Convert this to a stack of searches each with their own
     * lookahead value. */
    ps_search_t *search;     /**< Currently active search module. */
    ps_search_t *phone_loop; /**< Phone loop search for lookahead. */
    int pl_window;           /**< Window size for phoneme lookahead. */

    /* Utterance-processing related stuff. */
    uint32 uttno;       /**< Utterance counter. */
    ptmr_t perf;        /**< Performance counter for all of decoding. */
    uint32 n_frame;     /**< Total number of frames processed. */
    char const *mfclogdir; /**< Log directory for MFCC files. */
    char const *rawlogdir; /**< Log directory for audio files. */
    char const *senlogdir; /**< Log directory for senone score files. */
};


struct ps_search_iter_s {
    hash_iter_t itor;
};

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __POCKETSPHINX_INTERNAL_H__ */
