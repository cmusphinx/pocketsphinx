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
 * senscr.h -- 	Senone score computation module.
 */


#ifndef __SENSCR_H__
#define __SENSCR_H__

#include <fixpoint.h>

#include "s2types.h"
#include "msd.h"

/* Bit-vector operations for maintaining senone_active_vec */
#define BITVEC_SEN_ACTIVE
#ifdef BITVEC_SEN_ACTIVE
#define BITVEC_WIDTH 32
#define BITVEC_SET(v, e) (v)[(e)/BITVEC_WIDTH] |= (1<<((e)%BITVEC_WIDTH))
#define BITVEC_CLEAR(v, e) (v)[(e)/BITVEC_WIDTH] &= ~(1<<((e)%BITVEC_WIDTH))
#define BITVEC_ISSET(v, e) ((v)[(e)/BITVEC_WIDTH] & (1<<((e)%BITVEC_WIDTH)))
typedef uint32 bitvec_t;
#else
#define BITVEC_WIDTH 1
#define BITVEC_SET(v, e) (v)[e] = 1
#define BITVEC_CLEAR(v, e) (v)[e] = 0
#define BITVEC_ISSET(v, e) (v)[e]
typedef uint8 bitvec_t;
#endif

/* Array of senone scores for current frame */
extern int32 *senone_scores;
/*
 * Array (list) of active senones in the current frame, and the active list
 * size.  Array allocated and maintained by the search module.
 * (Extern is ugly, but there it is, for now (rkm@cs).)
 */
extern bitvec_t *senone_active_vec;	/* Active/inactive bitvec for each senone */
extern int32 *senone_active;		/* List of active senones */
extern int32 n_senone_active;		/* No. of entries in above active list */

extern int32 **past_senone_scores;  /* Senone scores for all frames */
bitvec_t **past_senone_active_vec;  /* Previously active senones */


/*
 * Compute senone scores for the given feature data.
 * The feature data is in s2_4x format (4 streams, etc, etc)
 * The function also updates an array of best senone scores for each CIphone
 * (bestpscr) maintained by the search module.
 * 
 * Return value: the best senone score overall.
 */
int32 senscr_all(mfcc_t **feat, int32 frame_idx);

/*
 * Like senscr_all above, except restricted to the currently active senones.
 * (See senone_active and n_senone_active, above.)  Further, this functions
 * does not update the bestpscr array.
 * 
 * Return value: the best senone score overall (among the active ones).
 */
int32 senscr_active(mfcc_t **feat, int32 frame_idx);


/*
 * Clear the global senone_active_flag array.
 */
void sen_active_clear(void);


/*
 * Set senone active flags for the given HMM.  (The global extern array
 * senone_active_flag is updated for the senones in the given rhmm or hmm.)
 */
void rhmm_sen_active(ROOT_CHAN_T *rhmm);
void hmm_sen_active(CHAN_T *hmm);


/*
 * Build the active senones list (the global senone_active array) from the
 * (global) senone_active_flag array.  Also update n_senone_active.
 * Return value: number of active senones in the list built.
 */
int32 sen_active_flags2list(void);


#endif
