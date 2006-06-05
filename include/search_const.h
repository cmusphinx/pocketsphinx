/* ====================================================================
 * Copyright (c) 1999-2001 Carnegie Mellon University.  All rights
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
/* SEARCH_CONST.H
 * 10-23-93 M K Ravishankar (rkm@cs.cmu.edu)
 * (int) cast added to WORST_SCORE to overcome Alpha compiler bug.
 */

#ifndef _SEARCH_CONST_H_
#define _SEARCH_CONST_H_

#define NUM_CODE_BOOKS		4	/* Number of codebooks used */

#define MAX_FRAMES		8000	/* Max Frames in an utterance */

#define WORST_SCORE		((int)0xE0000000)
	/* Large negative number. This number must be small enough so that
	 * 4 times WORST_SCORE will not overflow. The reason for this is
	 * that the search doesn't check the scores in a model before
	 * evaluating the model and it may require as many was 4 plies
	 * before the new 'good' score can wipe out the initial WORST_SCORE
	 * initialization.
	 */

#define HMM_5_STATE		1
	/* Set to TRUE is the is a 5 state HMM, otherwise
	 * this is a 3 state HMM
	 */

#if HMM_5_STATE
#define HMM_LAST_STATE	5
#else
#define HMM_LAST_STATE	3
#endif

#endif /* _SEARCH_CONST_H_ */

