/* -*- c-basic-offset:4; indent-tabs-mode: nil -*- */
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
 */

/*
 * fsg_search.h -- Search structures for FSM decoding.
 * 
 * **********************************************
 * CMU ARPA Speech Project
 *
 * Copyright (c) 2004 Carnegie Mellon University.
 * ALL RIGHTS RESERVED.
 * **********************************************
 */


#ifndef __S2_FSG_SEARCH_H__
#define __S2_FSG_SEARCH_H__


/* SphinxBase headers. */
#include <glist.h>
#include <cmd_ln.h>

/* Local headers. */
#include "fbs.h"
#include "bin_mdef.h"
#include "word_fsg.h"
#include "dict.h"
#include "tmat.h"

/* Forward declare some things. */
struct fsg_lextree_s;
struct fsg_history_s;

typedef struct fsg_search_s {
    glist_t fsglist;		/**< List of all FSGs loaded */
  
    cmd_ln_t *config;           /**< Pointer to global configuration. */
    logmath_t *lmath;		/**< Pointer to global log-math. */
    bin_mdef_t *mdef;           /**< Pointer to global model definition. */
    tmat_t *tmat;               /**< Pointer to global transition matrix. */
    dict_t *dict;		/**< Pointer to global word dictoinary. */

    word_fsg_t *fsg;		/**< Currently active FSG; NULL if none.  One
				   must be made active before starting FSG
				   decoding */
    struct fsg_lextree_s *lextree;/**< Lextree structure for the currently
				   active FSG */
    struct fsg_history_s *history;/**< For storing the Viterbi search history */
  
    glist_t pnode_active;	/**< Those active in this frame */
    glist_t pnode_active_next;	/**< Those activated for the next frame */
  
    int32 beam_orig;		/**< Global pruning threshold */
    int32 pbeam_orig;		/**< Pruning threshold for phone transition */
    int32 wbeam_orig;		/**< Pruning threshold for word exit */
    float32 beam_factor;		/**< Dynamic/adaptive factor (<=1) applied to above
                                           beams to determine actual effective beams.
                                           For implementing absolute pruning. */
    int32 beam, pbeam, wbeam;	/**< Effective beams after applying beam_factor */
    int32 lw, pip, wip;         /**< Language weights */
  
    int32 frame;			/**< Current frame */

    int32 bestscore;		/**< For beam pruning */
    int32 bpidx_start;		/**< First history entry index this frame */
  
    search_hyp_t *hyp;		/**< Search hypothesis */
    int32 ascr, lscr;		/**< Total acoustic and lm score for utt */
  
    int32 n_hmm_eval;		/**< Total HMMs evaluated this utt */
    int32 n_sen_eval;		/**< Total senones evaluated this utt */
  
    int32 state;		/**< Whether IDLE or BUSY */
} fsg_search_t;


/* Access macros */
#define fsg_search_frame(s)	((s)->frame)


/*
 * Create, initialize and return a search module.
 */
fsg_search_t *fsg_search_init(cmd_ln_t *config,
                              logmath_t *lmath,
                              bin_mdef_t *mdef,
                              dict_t *dict,
                              tmat_t *tmat);


/*
 * Lookup the FSG associated with the given name and return it, or NULL if
 * no match found.
 */
word_fsg_t *fsg_search_fsgname_to_fsg(fsg_search_t *fsgs, const char *name);


/*
 * Add the given FSG to the collection of FSGs known to this search object.
 * The given fsg is simply added to the collection.  It is not automatically
 * made the currently active one.
 * The name of the new FSG must not match any of the existing ones.  If so,
 * FALSE is returned.  If successfully added, TRUE is returned.
 */
boolean fsg_search_add_fsg(fsg_search_t *fsgs, word_fsg_t *wfsg);


/*
 * Delete the given FSG from the known collection.  Free the FSG itself,
 * and if it was the currently active FSG, also free the associated search
 * structures and leave the current FSG undefined.
 */
boolean fsg_search_del_fsg(fsg_search_t *fsgs, word_fsg_t *wfsg);


/* Like fsg_search_del_fsg(), but identifies the FSG by its name */
boolean fsg_search_del_fsg_byname(fsg_search_t *fsgs, char *name);


/*
 * Switch to a new FSG (identified by its string name).  Must not be invoked
 * when search is busy (ie, in the midst of an utterance.  That's an error
 * and FALSE is returned.  If successful, returns TRUE.
 */
boolean fsg_search_set_current_fsg(fsg_search_t *fsgs, const char *name);


/*
 * Deallocate search structure.
 */
void fsg_search_free(fsg_search_t *fsgs);


/*
 * Prepare the FSG search structure for beginning decoding of the next
 * utterance.
 */
void fsg_search_utt_start(fsg_search_t *fsgs);


/*
 * Windup and clean the FSG search structure after utterance.  Fill in the
 * results of search: fsg_search_t.{hyp,ascr,lscr,frame}.  (But some fields
 * of hyp are left unfilled for now: conf, latden, phone_perp.)
 */
void fsg_search_utt_end(fsg_search_t *fsgs);


/*
 * Step one frame forward through the Viterbi search.
 */
void fsg_search_frame_fwd(fsg_search_t *fsgs);


/*
 * Compute the partial or final Viterbi backtrace result.  (The result can
 * be retrieved using the API functions seach_result or search_get_hyp().)
 * If "check_fsg_final_state" is TRUE, the backtrace starts from the best
 * history entry ending in the final state (if it exists).  Otherwise it
 * starts from the best entry, regardless of the terminating state (usually
 * used for partial results).
 */
void fsg_search_history_backtrace(fsg_search_t *search,
				   boolean check_fsg_final_state);

/*
 * Return the start (or final) state of the currently active FSG, if any.
 * Otherwise return -1.
 */
int32 fsg_search_get_start_state(fsg_search_t *);
int32 fsg_search_get_final_state(fsg_search_t *);


/*
 * Set the start (or final) state of the current active FSG, if any, to the
 * given state.  This operation can only be done in between utterances, not
 * in the midst of one.  Return the previous start (or final) state if
 * successful.  Return -1 if any error.
 */
int32 fsg_search_set_start_state(fsg_search_t *, int32 state);
int32 fsg_search_set_final_state(fsg_search_t *, int32 state);


/*
 * Update the global senone_active list using the list of active pnodes.
 */
void fsg_search_sen_active(fsg_search_t *fsgs);


#endif
