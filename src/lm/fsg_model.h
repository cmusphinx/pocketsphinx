/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 1999-2004 Carnegie Mellon University.  All rights
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
 * **********************************************
 * CMU ARPA Speech Project
 *
 * Copyright (c) 2003 Carnegie Mellon University.
 * ALL RIGHTS RESERVED.
 * **********************************************
 *
 */

/**
 * @file fsg_model.h
 * @brief Finite-state grammars for recognition
 */


#ifndef __FSG_MODEL_H__
#define __FSG_MODEL_H__

#include <stdio.h>
#include <string.h>

#include <pocketsphinx.h>
#include <pocketsphinx/export.h>

#include "util/glist.h"
#include "util/bitvec.h"
#include "util/hash_table.h"
#include "util/listelem_alloc.h"

#ifdef __cplusplus
extern "C" {
#endif
#if 0
/* Fool Emacs. */
}
#endif

/**
 * @struct fsg_link_t
 * @brief A single transition in the FSG.
 */
typedef struct fsg_link_s {
    int32 from_state;
    int32 to_state;
    int32 logs2prob;	/**< log(transition probability)*lw */
    int32 wid;		/**< Word-ID; <0 if epsilon or null transition */
} fsg_link_t;

/* Access macros */
#define fsg_link_from_state(l)	((l)->from_state)
#define fsg_link_to_state(l)	((l)->to_state)
#define fsg_link_wid(l)		((l)->wid)
#define fsg_link_logs2prob(l)	((l)->logs2prob)

/**
 * @struct trans_list_t
 * @brief Adjacency list for a state in an FSG.
 *
 * Actually we use hash tables so that random access is a bit faster.
 * Plus it allows us to make the lookup code a bit less ugly.
 */
typedef struct trans_list_s {
    hash_table_t *null_trans;   /* Null transitions keyed by state. */
    hash_table_t *trans;        /* Lists of non-null transitions keyed by state. */
} trans_list_t;

/**
 * @struct fsg_model_t
 * @brief Word level FSG definition.
 *
 * States are simply integers 0..n_state-1.
 * A transition emits a word and has a given probability of being taken.
 * There can also be null or epsilon transitions, with no associated emitted
 * word.
 */
typedef struct fsg_model_s {
    int refcount;       /**< Reference count. */
    char *name;		/**< A unique string identifier for this FSG */
    int32 n_word;       /**< Number of unique words in this FSG */
    int32 n_word_alloc; /**< Number of words allocated in vocab */
    char **vocab;       /**< Vocabulary for this FSG. */
    bitvec_t *silwords; /**< Indicates which words are silence/fillers. */
    bitvec_t *altwords; /**< Indicates which words are pronunciation alternates. */
    logmath_t *lmath;	/**< Pointer to log math computation object. */
    int32 n_state;	/**< number of states in FSG */
    int32 start_state;	/**< Must be in the range [0..n_state-1] */
    int32 final_state;	/**< Must be in the range [0..n_state-1] */
    float32 lw;		/**< Language weight that's been applied to transition
			   logprobs */
    trans_list_t *trans; /**< Transitions out of each state, if any. */
    listelem_alloc_t *link_alloc; /**< Allocator for FSG links. */
} fsg_model_t;

/* Access macros */
#define fsg_model_name(f)		((f)->name)
#define fsg_model_n_state(f)		((f)->n_state)
#define fsg_model_start_state(f)	((f)->start_state)
#define fsg_model_final_state(f)	((f)->final_state)
#define fsg_model_log(f,p)		logmath_log((f)->lmath, p)
#define fsg_model_lw(f)			((f)->lw)
#define fsg_model_n_word(f)		((f)->n_word)
#define fsg_model_word_str(f,wid)       (wid == -1 ? "(NULL)" : (f)->vocab[wid])

/**
 * Iterator over arcs.
 * Implementation of arc iterator.
 */
typedef struct fsg_arciter_s {
    hash_iter_t *itor, *null_itor;
    gnode_t *gn;
} fsg_arciter_t;

/**
 * Have silence transitions been added?
 */
#define fsg_model_has_sil(f)            ((f)->silwords != NULL)

/**
 * Have alternate word transitions been added?
 */
#define fsg_model_has_alt(f)            ((f)->altwords != NULL)

#define fsg_model_is_filler(f,wid) \
    (fsg_model_has_sil(f) ? bitvec_is_set((f)->silwords, wid) : FALSE)
#define fsg_model_is_alt(f,wid) \
    (fsg_model_has_alt(f) ? bitvec_is_set((f)->altwords, wid) : FALSE)

/**
 * Create a new FSG.
 */
fsg_model_t *fsg_model_init(char const *name, logmath_t *lmath,
                            float32 lw, int32 n_state);

/**
 * Add a word to the FSG vocabulary.
 *
 * @return Word ID for this new word.
 */
int fsg_model_word_add(fsg_model_t *fsg, char const *word);

/**
 * Look up a word in the FSG vocabulary.
 *
 * @return Word ID for this word
 */
int fsg_model_word_id(fsg_model_t *fsg, char const *word);

/**
 * Add the given transition to the FSG transition matrix.
 *
 * Duplicates (i.e., two transitions between the same states, with the
 * same word label) are flagged and only the highest prob retained.
 */
void fsg_model_trans_add(fsg_model_t * fsg,
                         int32 from, int32 to, int32 logp, int32 wid);

/**
 * Add a null transition between the given states.
 *
 * There can be at most one null transition between the given states;
 * duplicates are flagged and only the best prob retained.  Transition
 * probs must be <= 1 (i.e., logprob <= 0).
 *
 * @return 1 if a new transition was added, 0 if the prob of an existing
 * transition was upgraded; -1 if nothing was changed.
 */
int32 fsg_model_null_trans_add(fsg_model_t * fsg, int32 from, int32 to, int32 logp);

/**
 * Add a "tag" transition between the given states.
 *
 * A "tag" transition is a null transition with a non-null word ID,
 * which corresponds to a semantic tag or other symbol to be output
 * when this transition is taken.
 *
 * As above, there can be at most one null or tag transition between
 * the given states; duplicates are flagged and only the best prob
 * retained.  Transition probs must be <= 1 (i.e., logprob <= 0).
 *
 * @return 1 if a new transition was added, 0 if the prob of an existing
 * transition was upgraded; -1 if nothing was changed.
 */
int32 fsg_model_tag_trans_add(fsg_model_t * fsg, int32 from, int32 to,
                              int32 logp, int32 wid);

/**
 * Obtain transitive closure of null transitions in the given FSG.
 *
 * @param nulls List of null transitions, or NULL to find them automatically.
 * @return Updated list of null transitions.
 */
POCKETSPHINX_EXPORT
glist_t fsg_model_null_trans_closure(fsg_model_t * fsg, glist_t nulls);

/**
 * Get the list of transitions (if any) from state i to j.
 */
glist_t fsg_model_trans(fsg_model_t *fsg, int32 i, int32 j);

/**
 * Get an iterator over the outgoing transitions from state i.
 */
fsg_arciter_t *fsg_model_arcs(fsg_model_t *fsg, int32 i);

/**
 * Get the current arc from the arc iterator.
 */
fsg_link_t *fsg_arciter_get(fsg_arciter_t *itor);

/**
 * Move the arc iterator forward.
 */
fsg_arciter_t *fsg_arciter_next(fsg_arciter_t *itor);

/**
 * Free the arc iterator (early termination)
 */
void fsg_arciter_free(fsg_arciter_t *itor);
/**
 * Get the null transition (if any) from state i to j.
 */
fsg_link_t *fsg_model_null_trans(fsg_model_t *fsg, int32 i, int32 j);

/**
 * Add silence word transitions to each state in given FSG.
 *
 * @param state state to add a self-loop to, or -1 for all states.
 * @param silprob probability of silence transition.
 */
int fsg_model_add_silence(fsg_model_t * fsg, char const *silword,
                          int state, float32 silprob);

/**
 * Add alternate pronunciation transitions for a word in given FSG.
 */
int fsg_model_add_alt(fsg_model_t * fsg, char const *baseword,
                      char const *altword);


#ifdef __cplusplus
}
#endif

#endif /* __FSG_MODEL_H__ */
