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
 * fsg_search2.h -- Search structures for FSM decoding.
 * 
 * **********************************************
 * CMU ARPA Speech Project
 *
 * Copyright (c) 2004 Carnegie Mellon University.
 * ALL RIGHTS RESERVED.
 * **********************************************
 */


#ifndef __S2_FSG_SEARCH2_H__
#define __S2_FSG_SEARCH2_H__


/* SphinxBase headers. */
#include <glist.h>
#include <cmd_ln.h>

/* Local headers. */
#include "fbs.h"
#include "pocketsphinx_internal.h"
#include "hmm.h"
#include "word_fsg.h"

/* Forward declare some things. */
struct fsg_lextree_s;
struct fsg_history_s;

typedef struct fsg_search2_s {
    ps_search_t base;

    hmm_context_t *hmmctx; /**< HMM context. */

    hash_table_t *fsgs;		/**< Table of all FSGs loaded */
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
    float32 beam_factor;	/**< Dynamic/adaptive factor (<=1) applied to above
                                     beams to determine actual effective beams.
                                     For implementing absolute pruning. */
    int32 beam, pbeam, wbeam;	/**< Effective beams after applying beam_factor */
    int32 lw, pip, wip;         /**< Language weights */
    int32 finish_wid;		/**< Finish word ID. */
  
    int16 frame;		/**< Current frame. */
    int16 final;		/**< Decoding is finished for this utterance. */

    int32 bestscore;		/**< For beam pruning */
    int32 bpidx_start;		/**< First history entry index this frame */
  
    int32 ascr, lscr;		/**< Total acoustic and lm score for utt */
  
    int32 n_hmm_eval;		/**< Total HMMs evaluated this utt */
    int32 n_sen_eval;		/**< Total senones evaluated this utt */
} fsg_search2_t;


/* Access macros */
#define fsg_search2_frame(s)	((s)->frame)


/**
 * Create, initialize and return a search module.
 */
ps_search_t *fsg_search2_init(cmd_ln_t *config,
                              acmod_t *acmod,
                              dict_t *dict);

/**
 * Deallocate search structure.
 */
void fsg_search2_free(ps_search_t *search);


/**
 * Lookup the FSG associated with the given name.
 *
 * @return The FSG associated with name, or NULL if no match found.
 */
word_fsg_t *fsg_search2_get_fsg(fsg_search2_t *fsgs, char const *name);

/**
 * Add the given FSG to the collection of FSGs known to this search object.
 *
 * The given fsg is simply added to the collection.  It is not automatically
 * made the currently active one.
 *
 * @param name Name to associate with this FSG.  If NULL, this will be
 * taken from the FSG structure.
 * @return A pointer to the FSG associated with name, or NULL on
 * failure.  If this pointer is not equal to wfsg, then there was a
 * name conflict, and wfsg was not added.
 */
word_fsg_t *fsg_search2_add(fsg_search2_t *fsgs, char const *name, word_fsg_t *wfsg);

/**
 * Delete the given FSG from the known collection.
 */
word_fsg_t *fsg_search2_remove(fsg_search2_t *fsgs, word_fsg_t *wfsg);

/**
 * Like fsg_search2_del_fsg(), but identifies the FSG by its name.
 */
word_fsg_t *fsg_search2_remove_byname(fsg_search2_t *fsgs, char const *name);

/**
 * Switch to a new FSG (identified by its string name).
 *
 * @return Pointer to new FSG, or NULL on failure.
 */
word_fsg_t *fsg_search2_select(fsg_search2_t *fsgs, const char *name);


/**
 * Prepare the FSG search structure for beginning decoding of the next
 * utterance.
 */
int fsg_search2_start(ps_search_t *search);

/**
 * Step one frame forward through the Viterbi search.
 */
int fsg_search2_step(ps_search_t *search);

/**
 * Windup and clean the FSG search structure after utterance.
 */
int fsg_search2_finish(ps_search_t *search);

/**
 * Get hypothesis string from the FSG search.
 */
char const *fsg_search2_hyp(ps_search_t *search, int32 *out_score);

#endif
