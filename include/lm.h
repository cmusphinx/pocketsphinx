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

/*
 * lm.h - Wrapper for all language models under one integrated look (ideally!).
 *
 * HISTORY
 * 
 * $Log: lm.h,v $
 * Revision 1.1.1.1  2006/05/23 18:45:02  dhuggins
 * re-importation
 *
 * Revision 1.7  2004/12/10 16:48:58  rkm
 * Added continuous density acoustic model handling
 *
 * 
 * 03-Apr-97	M K Ravishankar (rkm@cs) at Carnegie Mellon University
 * 		Started.
 */


#ifndef _LM_H_
#define _LM_H_

/* LM scores, given sequences of dictionary base wid */
int32	lm_tg_score (int32 w1, int32 w2, int32 w3);
int32	lm_bg_score (int32 w1, int32 w2);
int32	lm_ug_score (int32 w);

/* One-time initialization of cache LM */
void lm_cache_lm_init ( void );

/*
 * Add a unigram (dictionary word id w) to cache LM (if doesn't exceed ugprob thresh).
 * The LM (and decoder) must be quiescent during this operation.
 */
void lm_cache_lm_add_ug (int32 w);

/*
 * Add a bigram (dictionary word id w1,w2) to cache LM.
 * The LM (and decoder) must be quiescent during this operation.
 */
void lm_cache_lm_add_bg (int32 w1, int32 w2);

void lm_cache_lm_dump (char *file);
void lm_cache_lm_load (char *file);

#endif
