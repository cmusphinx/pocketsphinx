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
/*
 * hmm.h -- HMM data structure.
 *
 * **********************************************
 * CMU ARPA Speech Project
 *
 * Copyright (c) 1997 Carnegie Mellon University.
 * ALL RIGHTS RESERVED.
 * **********************************************
 *
 * HISTORY
 * $Log$
 * Revision 1.1  2006/04/05  20:27:30  dhdfu
 * A Great Reorganzation of header files and executables
 * 
 * Revision 1.9  2006/02/22 16:46:38  arthchan2003
 * Merged from SPHINX3_5_2_RCI_IRII_BRANCH: 1, Added function hmm_vit_eval, a wrapper of computing the hmm level scores. 2, Fixed issues in , 3, Fixed issues of dox-doc
 *
 * Revision 1.8.4.6  2005/09/25 18:53:36  arthchan2003
 * Added hmm_vit_eval, in lextree.c, hmm_dump and hmm_vit_eval is now separated.
 *
 * Revision 1.8.4.5  2005/07/26 02:17:44  arthchan2003
 * Fixed  keyword problem.
 *
 * Revision 1.8.4.4  2005/07/17 05:15:47  arthchan2003
 * Totally removed the data members in hmm_t structure
 *
 * Revision 1.8.4.3  2005/07/05 05:47:59  arthchan2003
 * Fixed dox-doc. struct level of documentation are included.
 *
 * Revision 1.8.4.2  2005/07/04 07:15:55  arthchan2003
 * Removed fsg compliant stuff from hmm_t
 *
 * Revision 1.8.4.1  2005/06/27 05:38:54  arthchan2003
 * Added changes to make libsearch/fsg_* family of code to be compiled.
 *
 * Revision 1.8  2005/06/21 18:34:41  arthchan2003
 * Log. 1, Fixed doxygen documentation for all functions. 2, Add $Log$
 * Revision 1.1  2006/04/05  20:27:30  dhdfu
 * A Great Reorganzation of header files and executables
 * 
 * Log. 1, Fixed doxygen documentation for all functions. 2, Add Revision 1.9  2006/02/22 16:46:38  arthchan2003
 * Log. 1, Fixed doxygen documentation for all functions. 2, Add Merged from SPHINX3_5_2_RCI_IRII_BRANCH: 1, Added function hmm_vit_eval, a wrapper of computing the hmm level scores. 2, Fixed issues in , 3, Fixed issues of dox-doc
 * Log. 1, Fixed doxygen documentation for all functions. 2, Add
 *
 * Revision 1.4  2005/06/13 04:02:55  archan
 * Fixed most doxygen-style documentation under libs3decoder.
 *
 * Revision 1.3  2005/03/30 01:22:46  archan
 * Fixed mistakes in last updates. Add
 *
 * 
 * 29-Feb-2000	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Modified hmm_t.state to be a run-time array instead of a compile-time
 * 		one.  Modified compile-time 3 and 5-state versions of hmm_vit_eval
 * 		into hmm_vit_eval_3st and hmm_vit_eval_5st, to allow run-time selection.
 * 		Removed hmm_init().
 * 
 * 08-Dec-1999	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University.
 * 		Added HMM_SKIPARCS compile-time option and hmm_init().
 * 
 * 10-May-1999	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University.
 * 		Started, based on an earlier version.
 */


#ifndef _S3_HMM_H_
#define _S3_HMM_H_

/* System headers. */
#include <stdio.h>

/* SphinxBase headers. */
#include <sphinx_types.h>

#ifdef __cplusplus
extern "C" {
#endif
#if 0
} /* Fool Emacs into not indenting things. */
#endif

/** Hardcoded limit on the number of states (temporary) */
#define MAX_HMM_NSTATE 5

/** Shift count for senone scores. */
#define SENSCR_SHIFT 10

#ifdef FIXED_POINT
/** Gaussian mean storage type. */
typedef fixed32 mean_t;
typedef int32 var_t;
#else
typedef float32 mean_t;
typedef float32 var_t;
#endif

/** \file hmm.h
 * \brief HMM data structure and operation
 *
 * For efficiency, this version is hardwired for two possible HMM
 * topologies, but will fall back to others:
 * 
 * 5-state left-to-right HMMs: (0 is the *emitting* entry state and E
 * is a non-emitting exit state; the x's indicate allowed transitions
 * between source and destination states):
 * 
 * <pre>
 *               0   1   2   3   4   E (destination-states)
 *           0   x   x   x
 *           1       x   x   x
 *           2           x   x   x
 *           3               x   x   x
 *           4                   x   x
 *    (source-states)
 * </pre>
 *
 * 5-state topologies that contain a subset of the above transitions should work as well.
 * 
 * 3-state left-to-right HMMs (similar notation as the 5-state topology above):
 * 
 * <pre>
 *               0   1   2   E (destination-states)
 *           0   x   x   x
 *           1       x   x   x
 *           2           x   x 
 *    (source-states)
 * </pre>
 *
 * 3-state topologies that contain a subset of the above transitions should work as well. 
 */

/**
 * @struct hmm_context_t
 * @brief Shared information between a set of HMMs.
 *
 * We assume that the initial state is emitting and that the
 * transition matrix is n_emit_state x (n_emit_state+1), where the
 * extra destination dimension correponds to the non-emitting final or
 * exit state.
 */
typedef struct hmm_context_s {
    int32 n_emit_state;     /**< Number of emitting states in this set of HMMs. */
    int32 ** const *tp;	    /**< State transition scores tp[id][from][to] (logs3 values). */
    int16 const *senscore;  /**< State emission scores senscore[senid]
                               (negated scaled logs3 values). */
    s3senid_t * const *sseq;/**< Senone sequence mapping. */
    int32 *st_sen_scr;      /**< Temporary array of senone scores (for some topologies). */
    void *udata;            /**< Whatever you feel like, gosh. */
} hmm_context_t;

/**
 * @struct hmm_state_t
 * @brief A single state in the HMM 
 */
typedef struct {
    int32 score;	/**< State score (path log-likelihood) */
    int32 history;	/**< History index */
} hmm_state_t;

/**
 * @struct hmm_t
 * @brief An individual HMM among the HMM search space.
 *
 * An individual HMM among the HMM search space.  An HMM with N
 * emitting states consists of N+2 internal states including the
 * non-emitting entry (in) and exit (out) states.
 */

typedef struct hmm_s {
    hmm_context_t *ctx;                /**< Shared context data for this HMM. */
    hmm_state_t state[MAX_HMM_NSTATE]; /**< Per-state data for emitting states */
    hmm_state_t out;                   /**< Non-emitting exit state */
    union {
        int32 *mpx_ssid; /**< Senone sequence IDs for each state (for multiplex HMMs). */
        int32 ssid;      /**< Senone sequence ID. */
    } s;
    int32 bestscore;	/**< Best [emitting] state score in current frame (for pruning). */
    s3tmatid_t tmatid;  /**< Transition matrix ID (see hmm_context_t). */
    s3frmid_t frame;	/**< Frame in which this HMM was last active; <0 if inactive */
    uint8 mpx;          /**< Is this HMM multiplex? (hoisted for speed) */
    uint8 n_emit_state; /**< Number of emitting states (hoisted for speed) */
} hmm_t;

/** Access macros. */
#define hmm_context(h) (h)->ctx
#define hmm_is_mpx(h) (h)->mpx
#define hmm_state(h,st) (h)->state[st]

#define hmm_in_score(h) hmm_state(h,0).score
#define hmm_score(h,st) hmm_state(h,st).score
#define hmm_out_score(h) (h)->out.score

#define hmm_in_history(h) hmm_state(h,0).history
#define hmm_history(h,st) hmm_state(h,st).history
#define hmm_out_history(h) (h)->out.history

#define hmm_bestscore(h) (h)->bestscore
#define hmm_frame(h) (h)->frame
#define hmm_mpx_ssid(h,st) (h)->s.mpx_ssid[st]
#define hmm_nonmpx_ssid(h) (h)->s.ssid
#define hmm_ssid(h,st) (hmm_is_mpx(h)                           \
                        ? hmm_mpx_ssid(h,st) : (h)->s.ssid)
#define hmm_senid(h,st) (hmm_ssid(h,st) == -1                           \
                         ? -1 : (h)->ctx->sseq[hmm_ssid(h,st)][st])
#define hmm_senscr(h,st) (hmm_ssid(h,st) == -1                          \
                          ? WORST_SCORE                                 \
                          : -(h)->ctx->senscore[hmm_senid(h,st)] << SENSCR_SHIFT)
#define hmm_tmatid(h) (h)->tmatid
#define hmm_tprob(h,i,j) (h)->ctx->tp[hmm_tmatid(h)][i][j]
#define hmm_n_emit_state(h) ((h)->n_emit_state)
#define hmm_n_state(h) ((h)->n_emit_state + 1)

/**
 * Create an HMM context.
 **/
hmm_context_t *hmm_context_init(int32 n_emit_state,
                                int32 ** const *tp,
                                int16 const *senscore,
                                s3senid_t * const *sseq);

/**
 * Change the senone score array for a context.
 **/
#define hmm_context_set_senscore(ctx, senscr) ((ctx)->senscore = (senscr))

/**
 * Free an HMM context.
 *
 * @note The transition matrices, senone scores, and senone sequence
 * mapping are all assumed to be allocated externally, and will NOT be
 * freed by this function.
 **/
void hmm_context_free(hmm_context_t *ctx);

/**
 * Populate a previously-allocated HMM structure, allocating internal data.
 **/
void hmm_init(hmm_context_t *ctx, hmm_t *hmm, int mpx,
              int32 ssid, s3tmatid_t tmatid);

/**
 * Free an HMM structure, releasing internal data (but not the HMM structure itself).
 */
void hmm_deinit(hmm_t *hmm);

/**
 * Reset the states of the HMM to the invalid condition.

 * i.e., scores to WORST_SCORE and hist to undefined.
 */
void hmm_clear(hmm_t *h);

/**
 * Reset the scores of the HMM.
 */
void hmm_clear_scores(hmm_t *h);

/**
 * Renormalize the scores in this HMM based on the given best score.
 */
void hmm_normalize(hmm_t *h, int32 bestscr);

/**
 * Enter an HMM with the given path score and history ID.
 **/
void hmm_enter(hmm_t *h, int32 score,
               int32 histid, int32 frame);

/**
 * Viterbi evaluation of given HMM.
 *
 * @note If this module were being used for tracking state
 * segmentations, the dummy, non-emitting exit state would have to be
 * updated separately.  In the Viterbi DP diagram, transitions to the
 * exit state occur from the current time; they are vertical
 * transitions.  Hence they should be made only after the history has
 * been logged for the emitting states.  But we're not bothered with
 * state segmentations, for now.  So, we update the exit state as
 * well.
*/
int32 hmm_vit_eval(hmm_t *hmm);
  

/**
 * Like hmm_vit_eval, but dump HMM state and relevant senscr to fp first, for debugging;.
 */
int32 hmm_dump_vit_eval(hmm_t *hmm,  /**< In/Out: HMM being updated */
                        FILE *fp /**< An output file pointer */
    );

/** 
 * For debugging, dump the whole HMM out.
 */

void hmm_dump(hmm_t *h,  /**< In/Out: HMM being updated */
              FILE *fp /**< An output file pointer */
    );


#if 0
{ /* Stop indent from complaining */
#endif
#ifdef __cplusplus
}
#endif

#endif
