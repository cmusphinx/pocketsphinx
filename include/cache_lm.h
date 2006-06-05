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
 * cache_lm.h -- Dynamic cache language model based on Roni Rosenfeld's work.
 *
 * HISTORY
 * 
 * $Log: cache_lm.h,v $
 * Revision 1.1.1.1  2006/05/23 18:45:02  dhuggins
 * re-importation
 *
 * Revision 1.7  2004/12/10 16:48:58  rkm
 * Added continuous density acoustic model handling
 *
 * 
 * 01-Apr-97	M K Ravishankar (rkm@cs) at Carnegie Mellon University
 * 		Started, based on earlier FBS6 version.
 */


#ifndef _CACHE_LM_H_
#define _CACHE_LM_H_

/*
 * Bigram cache entry.  Pointed to by a parent unigram cache entry (see below).
 */
typedef struct clm_bg_s {
    int32 w2;			/* dictionary wid */
    int32 count;		/* #times this bigram seen */
    struct clm_bg_s *next;	/* Next bigram entry for parent unigram */
} clm_bg_t;

/* 
 * Unigram cache entry.  count can be different from sum_w2count because count is
 * accumulated only for words below a given unigram probability, but sum_w2count is
 * accumulated for all bigrams observed.
 */
typedef struct {
    int32 count;		/* #times this word seen (only for words below a given
				   unigram probability) */
    int32 sum_w2count;		/* Sum of counts in w2list (different from count above) */
    clm_bg_t *w2list;		/* Successors to this word */
} clm_ug_t;

typedef struct {
    clm_ug_t *clm_ug;		/* clm_ug[w] = cache information for dictionary wid w */
    int32 sum_ugcount;		/* Sum (clm_ug[w].count) */
    int32 n_word;		/* Size of clm_ug array */

    double min_uw;		/* Min, max, and per word unigram cache LM weights.  uw
				   starts at min_uw, increases linearly as sum_ugcount
				   increases (by per_word_uw), flattens out at max_uw */
    double max_uw;
    double per_word_uw;
    double uw;			/* Currently active unigram cache LM weight */
    double bw;			/* Bigram cache LM weight */
    
    int32 uw_ugcount_limit;	/* Unigram count needed to reach max_uw */
    
    int32 ugprob_thresh;	/* (Actually logprob) above which unigrams not cached */
    int32 log_uw;		/* LOGPROB(currently effective uw) */
    int32 log_bw;		/* LOGPROB(bw) */
    int32 log_remwt;		/* LOG(remaining weight) */
} cache_lm_t;

/*
 * Interface
 */

cache_lm_t *cache_lm_init (double ug_thresh,
			   double min_uw, double max_uw, int32 nword_uwrange,
			   double bw);

void cache_lm_reset (cache_lm_t *lm);

/* Add w (dict basewid) to unigram cache */
void cache_lm_add_ug (cache_lm_t *lm, int32 w);

/* Add bigram (w1,w2) to bigram cache (w1, w2 are dict base wids) */
void cache_lm_add_bg (cache_lm_t *lm, int32 w1, int32 w2);

/*
 * Return cache LM score for the given 2-word sequence.  Also return, in remwt, the
 * remaining relative language weight to be applied to other LMs (e.g. a static LM).
 * w1, w2 are dict base wids.
 * Note that only the RELATIVE language weights (for this LM wrt other LMs) have been
 * applied by this function.  The overall "language weight" wrt the acoustic score
 * has not been applied to the returned values.  It must be applied externally.
 */
int32 cache_lm_score (cache_lm_t *lm, int32 w1, int32 w2, int32 *remwt);

void cache_lm_dump (cache_lm_t *lm, char *file);

void cache_lm_load (cache_lm_t *lm, char *file);

#endif /* _CACHE_LM_H_ */
