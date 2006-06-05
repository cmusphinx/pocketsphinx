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
 * lm_3g.h - darpa standard trigram language model header file
 *
 * HISTORY
 * 
 * $Log: lm_3g.h,v $
 * Revision 1.1.1.1  2006/05/23 18:45:02  dhuggins
 * re-importation
 *
 * Revision 1.8  2004/12/10 16:48:55  rkm
 * Added continuous density acoustic model handling
 *
 * 
 * 28-Oct-98	M K Ravishankar (rkm@cs) at Carnegie Mellon University
 * 		Added lm3g_access_type().
 * 
 * 14-Apr-98	M K Ravishankar (rkm@cs) at Carnegie Mellon University
 * 		Added lm3g_n_lm() and lm3g_index2name().
 * 
 * 02-Apr-97	M K Ravishankar (rkm@cs) at Carnegie Mellon University
 * 		Added lm3g_raw_score() and lm_t.invlw.
 * 		Changed lm_{u,b,t}g_score to lm3g_{u,b,t}g_score.
 * 
 * 01-Jul-95	M K Ravishankar (rkm@cs) at Carnegie Mellon University
 * 		Added tginfo_t to help speed up find_bg() and find_tg() searches.
 * 
 * Revision 1.1  2000/12/05 01:33:29  lenzo
 * files added or moved
 *
 * Revision 1.1.1.1  2000/01/28 22:08:58  lenzo
 * Initial import of sphinx2
 *
 *
 * Revision 6.5  93/10/27  17:47:04  rkm
 * *** empty log message ***
 * 
 * Revision 6.4  93/10/15  15:02:39  rkm
 * *** empty log message ***
 * 
 */

#ifndef _LM_3G_H_
#define _LM_3G_H_

#include "s2types.h"
#include "fixpoint.h"
#include "lmclass.h"
#include "hash.h"

/* Type used to represent the language model weight. */
#ifdef FIXED_POINT
#define LWMUL(a,b) FIXMUL(a,b)
#define LW2FLOAT(a) FIX2FLOAT(a)
#define FLOAT2LW(a) FLOAT2FIX(a)
#else
#define LWMUL(a,b) ((a)*(b))
#define LW2FLOAT(a) (a)
#define FLOAT2LW(a) (a)
#endif

/* log quantities represented in either floating or integer format */
typedef union {
    float f;
    int32 l;
} log_t;


typedef struct unigram_s {
    int32 mapid;	/* Mapping to dictionary, LM-class, or other id if any.
			   Without any such mapping, this id is just the index of the
			   unigram entry.  If mapped, the ID is interpreted depending
			   on the range it falls into (see lm_3g.c); and it is -ve if
			   there is no applicable mapping. */
    log_t prob1;
    log_t bo_wt1;
    int32 bigrams;	/* index of 1st entry in lm_t.bigrams[] */
} unigram_t, UniGram, *UniGramPtr;

/*
 * To conserve space, bigram info is kept in many tables.  Since the number
 * of distinct values << #bigrams, these table indices can be 16-bit values.
 * prob2 and bo_wt2 are such indices, but keeping trigram index is less easy.
 * It is supposed to be the index of the first trigram entry for each bigram.
 * But such an index cannot be represented in 16-bits, hence the following
 * segmentation scheme: Partition bigrams into segments of BG_SEG_SZ
 * consecutive entries, such that #trigrams in each segment <= 2**16 (the
 * corresponding trigram segment).  The bigram_t.trigrams value is then a
 * 16-bit relative index within the trigram segment.  A separate table--
 * lm_t.tseg_base--has the index of the 1st trigram for each bigram segment.
 */
#define BG_SEG_SZ	512	/* chosen so that #trigram/segment <= 2**16 */
#define LOG_BG_SEG_SZ	9

typedef struct bigram_s {
    uint16 wid;	/* Index of unigram entry for this.  (NOT dictionary id.) */
    uint16 prob2;	/* index into array of actual bigram probs */
    uint16 bo_wt2;	/* index into array of actual bigram backoff wts */
    uint16 trigrams;	/* index of 1st entry in lm_t.trigrams[],
			   RELATIVE TO its segment base (see above) */
} bigram_t, BiGram, *BiGramPtr;

/*
 * As with bigrams, trigram prob info kept in a separate table for conserving
 * memory space.
 */
typedef struct trigram_s {
    uint16 wid;	/* Index of unigram entry for this.  (NOT dictionary id.) */
    uint16 prob3;	/* index into array of actual trigram probs */
} trigram_t;


/*
 * The following trigram information cache eliminates most traversals of 1g->2g->3g
 * tree to locate trigrams for a given bigram (lw1,lw2).  The organization is optimized
 * for locality of access (to the same lw1), given lw2.
 */
typedef struct tginfo_s {
    int32 w1;			/* lw1 component of bigram lw1,lw2.  All bigrams with
				   same lw2 linked together (see lm_t.tginfo). */
    int32 n_tg;			/* #tg for parent bigram lw1,lw2 */
    int32 bowt;                 /* tg bowt for lw1,lw2 */
    int32 used;			/* whether used since last lm_reset */
    trigram_t *tg;		/* Trigrams for lw1,lw2 */
    struct tginfo_s *next;      /* Next lw1 with same parent lw2; NULL if none. */
} tginfo_t;


/*
 * The language model.
 * Bigrams for each unigram are contiguous.  Bigrams for unigram i+1 come
 * immediately after bigrams for unigram i.  So, no need for a separate count
 * of bigrams/unigram; it is enough to know the 1st bigram for each unigram.
 * But an extra dummy unigram entry needed at the end to terminate the last
 * real entry.
 * Similarly, trigrams for each bigram are contiguous and trigrams for bigram
 * i+1 come immediately after trigrams for bigram i, and an extra dummy bigram
 * entry is required at the end.
 */
typedef struct lm_s {
    unigram_t *unigrams;
    bigram_t  *bigrams;	/* for entire LM */
    trigram_t *trigrams;/* for entire LM */
    log_t *prob2;	/* table of actual bigram probs */
    int32 n_prob2;	/* prob2 size */
    log_t *bo_wt2;	/* table of actual bigram backoff weights */
    int32 n_bo_wt2;	/* bo_wt2 size */
    log_t *prob3;	/* table of actual trigram probs */
    int32 n_prob3;	/* prob3 size */
    int32 *tseg_base;	/* tseg_base[i>>LOG_BG_SEG_SZ] = index of 1st
			   trigram for bigram segment (i>>LOG_BG_SEG_SZ) */
    int32 *dictwid_map; /* lexicon word-id to ILM word-id map */
    int32 max_ucount;	/* To which ucount can grow with dynamic addition of words */
    int32 ucount;	/* #unigrams in LM */
    int32 bcount;	/* #bigrams in entire LM */
    int32 tcount;	/* #trigrams in entire LM */
    int32 dict_size;	/* #words in lexicon */

    lmclass_t *lmclass;	/* Array of LM classes used by this trigram LM */
    int32 n_lmclass;	/* Size of the above array */
    int32 *inclass_ugscore;	/* P(w3|w1,w2) = P(c3|c1,c2)*P(w3|c3); c1 = lmclass(w1).
				   This array has P(w3|c3) for each w3.  This value is
				   in the LMclass structure, but copied here for speed */
    lw_t lw;          /* language weight */
    lw_t invlw;       /* 1.0/language weight */
    lw_t uw;          /* unigram weight */
    int32 log_wip;      /* word insertion penalty */
    
    tginfo_t **tginfo;	/* tginfo[lw2] is head of linked list of trigram information for
			   some cached subset of bigrams (*,lw2). */
    
    hash_t HT;		/* hash table for word-string->word-id map */
} lm_t, *LM;


#define LM3G_ACCESS_ERR		0
#define LM3G_ACCESS_UG		1
#define LM3G_ACCESS_BG		2
#define LM3G_ACCESS_TG		3


/* ----Interface---- */

void	lmSetStartSym (char const *sym);
void	lmSetEndSym (char const *sym);
lm_t *	NewModel (int32 n_ug, int32 n_bg, int32 n_tg, int32 n_dict);
int32   lm_add_word (lm_t *model, int32 dictwid);
void	lm_add (char const *name, lm_t *model, double lw, double uw, double wip);
int32	lm_set_current (char const *name);
lm_t *	lm_get_current (void);
char *	get_current_lmname (void);
lm_t *  lm_name2lm (char const *name);
int32   lm_read_clm(char const *filename, char const *lmname,
		    double lw, double uw, double wip,
		    lmclass_t *lmclass, int32 n_lmclass);
void    lm_init_oov(void);
int32	get_n_lm (void);
int32   lm3g_n_lm (void);
char   *lm3g_index2name (int k);
int32	dictwd_in_lm (int32 wid);

int32	lm3g_tg_score (int32 w1, int32 w2, int32 w3);
int32	lm3g_bg_score (int32 w1, int32 w2);
int32	lm3g_ug_score (int32 w);
int32	lm3g_raw_score (int32 score);
int32	lm3g_access_type (void);

void	lm3g_cache_reset (void);
void	lm3g_cache_stats_dump (FILE *file);
void	lm_next_frame (void);

int32   lm_delete (char const *name);

#endif /* _LM_3G_H_ */
