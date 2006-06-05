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
 * senscr.c -- 	Senone score computation module.
 * 		Hides details of s2 (semi-continuous) and s3 (continuous)
 * 		models, and computes generic "senone scores".
 *
 * HISTORY
 * 
 * $Log: senscr.c,v $
 * Revision 1.1.1.1  2006/05/23 18:45:01  dhuggins
 * re-importation
 *
 * Revision 1.1  2004/12/10 16:48:57  rkm
 * Added continuous density acoustic model handling
 *
 * 
 * 02-Dec-2004	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Added acoustic score weight (applied only to S3 continuous
 * 		acoustic models).
 * 
 * 01-Dec-2004	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University.
 * 		Added senone active flag related functions.
 * 
 * 20-Nov-2004	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University.
 * 		Started.
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include "s2types.h"
#include "s3types.h"
#include "err.h"
#include "ckd_alloc.h"
#include "logs3.h"
#include "cont_mgau.h"
#include "kb.h"
#include "scvq.h"
#include "phone.h"
#include "hmm_tied_r.h"
#include "search.h"
#include "senscr.h"

/*
 * Compute the best senone score from the given array of senone scores.
 * (All senones, not just active ones, should have been already computed
 * elsewhere.)
 * Also, compute the best senone score (CI or CD) per CIphone, and update
 * the corresponding bestpscr array (maintained in the search module).
 * 
 * Return value: the best senone score this frame.
 */
static int32 best_senscr_all (int32 *senscr)
{
  int32 b, i, j, k;
  int32 n_ci;		/* #CI phones */
  int32 *n_psen;	/* #Senones (CI+CD) for each CIphone */
  int32 *bestpscr;	/* Best senone score (CI or CD) for each CIphone */
  
  n_ci = phoneCiCount();
  n_psen = hmm_get_psen();
  bestpscr = search_get_bestpscr();
  
  b = (int32) 0x80000000;
  
  for (i = 0; i < n_ci; i++) {
    k = (int32) 0x80000000;
    
    /* Senones (CI+CD) for CIphone i are in one contiguous block */
    for (j = n_psen[i]; j > 0; --j, senscr++)
      if (k < *senscr)
	k = *senscr;
    
    bestpscr[i] = k;
    
    if (b < k)
      b = k;
  }
  
  return b;
}

/* Like best_senscr_all, but using Sphinx3 model ordering. */
static int32 best_senscr_all_s3 (int32 *senscr)
{
  int32 b, i, j, ci;
  int32 *bestpscr;
  bin_mdef_t *mdef;
  
  bestpscr = search_get_bestpscr();
  mdef = kb_mdef();
  
  b = (int32) 0x80000000;

  /* Initialize bestpscr with CI phones */
  for (i = 0; i < bin_mdef_n_ciphone(mdef); ++i) {
    bestpscr[i] = (int32) 0x80000000;
    for (j = 0; j < bin_mdef_n_emit_state_phone(mdef, i); ++j, ++senscr) {
      /* This assumes that CI phones have contiguous senones at the
	 beginning of the mdef, which is *almost* certainly true. */
      if (bestpscr[i] < *senscr)
	bestpscr[i] = *senscr;
      if (b < bestpscr[i])
	b = bestpscr[i];
    }
  }

  /* Now do the rest of the senones */
  for (i = mdef->n_ci_sen; i < mdef->n_sen; ++i, ++senscr) {
    /* NOTE: This assumes that each CD senone corresponds to at most
       one CI phone.  This is not always true, but we hope that taking
       the first one will work okay. */
    ci = mdef->sen2cimap[i];
    if (bestpscr[ci] < *senscr) {
      bestpscr[ci] = *senscr;
      if (b < bestpscr[ci])
	b = bestpscr[ci];
    }
  }
  
  return b;
}


/*
 * Like best_senscr_all, but computed only from the active senones in
 * the current frame.  The bestpscr array is not updated since all senone
 * scores are not available.
 * 
 * Return value: the best senone score this frame.
 */
static int32 best_senscr_active (int32 *senscr)
{
  int32 b, i, s;
  
  b = (int32) 0x80000000;

  for (i = 0; i < n_senone_active; i++) {
    s = senone_active[i];
    
    if (b < senscr[s])
      b = senscr[s];
  }
  
  return b;
}


/*
 * Compute s3 feature vector from the given input vectors; note that
 * cep, dcep and ddcep include c0, dc0, and ddc0, which should be
 * omitted.
 */
static void s3feat_build (float *s3feat,
			  mfcc_t *cep,
			  mfcc_t *dcep,
			  mfcc_t *pcep,
			  mfcc_t *ddcep)
{
  int32 i, j;

#if 1
  /* 1s_c_d_dd */
  for (i = 0, j = 0; i < CEP_VECLEN; i++, j++)
    s3feat[j] = MFCC2FLOAT(cep[i]);
  for (i = 0; i < CEP_VECLEN; i++, j++)
    s3feat[j] = MFCC2FLOAT(dcep[i]);
  for (i = 0; i < CEP_VECLEN; i++, j++)
    s3feat[j] = MFCC2FLOAT(ddcep[i]);
#else
  /* s3_1x39 */
  for (i = 1, j = 0; i < CEP_VECLEN; i++, j++)	/* Omit cep[0] */
    s3feat[j] = MFCC2FLOAT(cep[i]);
  for (i = 1; i < CEP_VECLEN; i++, j++)		/* Omit dcep[0] */
    s3feat[j] = MFCC2FLOAT(dcep[i]);
  for (i = 0; i < POW_VECLEN; i++, j++)
    s3feat[j] = MFCC2FLOAT(pcep[i]);
  for (i = 1; i < CEP_VECLEN; i++, j++)		/* Omit ddcep[0] */
    s3feat[j] = MFCC2FLOAT(ddcep[i]);
#endif
}


static int32 senscr_compute (int32 *senscr,
			     mfcc_t *cep,
			     mfcc_t *dcep,
			     mfcc_t *dcep_80ms,
			     mfcc_t *pcep,
			     mfcc_t *ddcep,
			     int32 all)
{
  mgau_model_t *g;
  
  g = kb_s3model();
  
  if (g != NULL) {	/* Use S3 acoustic model */
    int32 i, sid, best;
    float32 *s3feat;
    int32 ascr_sf;

    ascr_sf = kb_get_ascr_scale();
    s3feat = kb_s3feat();
    assert (s3feat != NULL);
    
    /* Build S3 format (single-stream) feature vector */
    s3feat_build (s3feat, cep, dcep, pcep, ddcep);
    
    best = (int32)0x80000000;
    
    if (all) {
      /* Evaluate all senones */
      for (sid = 0; sid < mgau_n_mgau(g); sid++) {
	senscr[sid] = mgau_eval (g, sid, NULL, s3feat);
	if (ascr_sf != 0)
	  senscr[sid] >>= ascr_sf;
	
	if (best < senscr[sid])
	  best = senscr[sid];
      }
      /* Normalize scores */
      for (sid = 0; sid < mgau_n_mgau(g); sid++)
	senscr[sid] -= best;
    }
    else {
      /* Evaluate only active senones */
      for (i = 0; i < n_senone_active; i++) {
	sid = senone_active[i];
	
	senscr[sid] = mgau_eval (g, sid, NULL, s3feat);
	if (ascr_sf != 0)
	  senscr[sid] >>= ascr_sf;
	
	if (best < senscr[sid])
	  best = senscr[sid];
      }
      
      /* Normalize scores */
      for (i = 0; i < n_senone_active; i++) {
	sid = senone_active[i];
	senscr[sid] -= best;
      }
    }
    if (all)
      return best_senscr_all_s3(senscr);
    else
      return best_senscr_active(senscr);
  } else {
    /* S2 (semi-continuous) senone scores */
    if (all) {
      SCVQScores_all(senscr, cep, dcep, dcep_80ms, pcep, ddcep);
      if (kb_mdef())
	return best_senscr_all_s3 (senscr);
      else
	return best_senscr_all (senscr);
    } else {
      SCVQScores(senscr, cep, dcep, dcep_80ms, pcep, ddcep);
      return best_senscr_active (senscr);
    }
  }
}


int32 senscr_all (int32 *senscr,
		  mfcc_t *cep,
		  mfcc_t *dcep,
		  mfcc_t *dcep_80ms,
		  mfcc_t *pcep,
		  mfcc_t *ddcep)
{
  return senscr_compute (senscr, cep, dcep, dcep_80ms, pcep, ddcep, TRUE);
}


int32 senscr_active (int32 *senscr,
		     mfcc_t *cep,
		     mfcc_t *dcep,
		     mfcc_t *dcep_80ms,
		     mfcc_t *pcep,
		     mfcc_t *ddcep)
{
  return senscr_compute (senscr, cep, dcep, dcep_80ms, pcep, ddcep, FALSE);
}

void sen_active_clear ( void )
{
  memset (senone_active_vec, 0,
	  (kb_get_total_dists() + BITVEC_WIDTH-1)
	  / BITVEC_WIDTH * sizeof(bitvec_t));
  n_senone_active = 0;
}

/* Some rather nasty macros for doing hmm_sen_active() in assembly. */
#ifdef __arm__
  /* Sadly, this isn't actually faster, even if it looks nicer. */
#define ASM_BITVEC_SET					\
      "mov r3, r1, lsr #5\n\t" /* sid/32 (word) */	\
      "ldr r4, [%0, r3, lsl #2]\n\t" /* word in vector */	\
      "and r1, r1, #31\n\t" /* sid%32 (bit) */		\
      "orr r4, r4, r2, lsl r1\n\t" /* set bit */	\
      "str r4, [%0, r3, lsl #2]\n\t" /* store it back */
#define BITVEC_SET_NONMPX				\
  asm("mov r2, #1\n\t" /* one bit (for use below) */	\
      "ldr r1, [%1]\n\t" /* dist[0] */			\
      ASM_BITVEC_SET					\
      "ldr r1, [%1, #12]\n\t" /* dist[3] */		\
      ASM_BITVEC_SET					\
      "ldr r1, [%1, #24]\n\t" /* dist[6] */		\
      ASM_BITVEC_SET					\
      "ldr r1, [%1, #36]\n\t" /* dist[9] */		\
      ASM_BITVEC_SET					\
      "ldr r1, [%1, #48]\n\t" /* dist[12] */		\
      ASM_BITVEC_SET					\
      : 						\
      : "r" (senone_active_vec), "r" (dist)		\
      : "r1", "r2", "r3", "r4", "memory");
#elif defined(BFIN)
  /* This is somewhat faster due to parallel issue (still needs work). */
#define BITVEC_SET_NONMPX								\
  asm("R0 = 1 (X);\n\t" /* one bit (for use below) */					\
      "R4 = 31 (X);\n\t" /* five bits (for use below) */				\
      "M0 = 12;\n\t" /* 3 words */							\
      "R1 = [%1++M0];\n\t" /* dist[0] */						\
      "P1 = 5;\n\t"									\
      "LSETUP (0f,1f) LC0 = P1;\n\t"							\
      "0:\n\t"										\
      "R2 = R1 >> 5;\n\t" /* sid/32 (word) */						\
      "P2 = R2;\n\t"									\
      "P1 = %0 + (P2 << 2);\n\t"							\
      "R1 = R1 & R4;\n\t" /* sid%32 (bit) */						\
      "R2 = LSHIFT R0 BY R1.L || R3 = [P1] || R1 = [%1++M0];\n\t" /* shift, load */	\
      "R3 = R3 | R2;\n\t"  /* set bit */						\
      "1:\n\t"										\
      "[P1] = R3;\n\t" /* store it back */						\
      : 										\
      : "a" (senone_active_vec), "b" (dist)						\
      : "M0", "P1", "P2", "R1", "R2", "R3", "R4", "memory", "cc");
#else
#define BITVEC_SET_NONMPX			\
  BITVEC_SET(senone_active_vec, dist[0]);	\
  BITVEC_SET(senone_active_vec, dist[3]);	\
  BITVEC_SET(senone_active_vec, dist[6]);	\
  BITVEC_SET(senone_active_vec, dist[9]);	\
  BITVEC_SET(senone_active_vec, dist[12]);
#endif


void rhmm_sen_active(ROOT_CHAN_T *rhmm)
{
  /* Not using kb_get_models() because this function is time-critical */
  extern SMD *smds;
  
  if (rhmm->mpx) {
    BITVEC_SET(senone_active_vec, smds[rhmm->sseqid[0]].dist[0]);
    BITVEC_SET(senone_active_vec, smds[rhmm->sseqid[1]].dist[3]);
    BITVEC_SET(senone_active_vec, smds[rhmm->sseqid[2]].dist[6]);
    BITVEC_SET(senone_active_vec, smds[rhmm->sseqid[3]].dist[9]);
    BITVEC_SET(senone_active_vec, smds[rhmm->sseqid[4]].dist[12]);
  } else {
    int32 *dist = smds[rhmm->sseqid[0]].dist;
    BITVEC_SET_NONMPX;
  }
}


void hmm_sen_active(CHAN_T *hmm)
{
  /* Not using kb_get_models() because this function is time-critical */
  extern SMD *smds;
  int32 *dist;
  
  dist = smds[hmm->sseqid].dist;
  BITVEC_SET_NONMPX;
}

#ifdef BITVEC_SEN_ACTIVE
int32 sen_active_flags2list ( void )
{
  int32 i, j, total_dists, total_bits;
  bitvec_t *flagptr, bits;
  
  total_dists = kb_get_total_dists();
  
  j = 0;
  total_bits = total_dists & -BITVEC_WIDTH;
  for (i = 0, flagptr = senone_active_vec; i < total_bits; flagptr++) {
    bits = *flagptr;

    if (bits == 0) {
      i += BITVEC_WIDTH;
      continue;
    }

    if (bits & (1<<0))
      senone_active[j++] = i;
    ++i;
    if (bits & (1<<1))
      senone_active[j++] = i;
    ++i;
    if (bits & (1<<2))
      senone_active[j++] = i;
    ++i;
    if (bits & (1<<3))
      senone_active[j++] = i;
    ++i;
    if (bits & (1<<4))
      senone_active[j++] = i;
    ++i;
    if (bits & (1<<5))
      senone_active[j++] = i;
    ++i;
    if (bits & (1<<6))
      senone_active[j++] = i;
    ++i;
    if (bits & (1<<7))
      senone_active[j++] = i;
    ++i;
#if BITVEC_WIDTH > 8
    if (bits & (1<<8))
      senone_active[j++] = i;
    ++i;
    if (bits & (1<<9))
      senone_active[j++] = i;
    ++i;
    if (bits & (1<<10))
      senone_active[j++] = i;
    ++i;
    if (bits & (1<<11))
      senone_active[j++] = i;
    ++i;
    if (bits & (1<<12))
      senone_active[j++] = i;
    ++i;
    if (bits & (1<<13))
      senone_active[j++] = i;
    ++i;
    if (bits & (1<<14))
      senone_active[j++] = i;
    ++i;
    if (bits & (1<<15))
      senone_active[j++] = i;
    ++i;
#if BITVEC_WIDTH == 32
    if (bits & (1<<16))
      senone_active[j++] = i;
    ++i;
    if (bits & (1<<17))
      senone_active[j++] = i;
    ++i;
    if (bits & (1<<18))
      senone_active[j++] = i;
    ++i;
    if (bits & (1<<19))
      senone_active[j++] = i;
    ++i;
    if (bits & (1<<20))
      senone_active[j++] = i;
    ++i;
    if (bits & (1<<21))
      senone_active[j++] = i;
    ++i;
    if (bits & (1<<22))
      senone_active[j++] = i;
    ++i;
    if (bits & (1<<23))
      senone_active[j++] = i;
    ++i;
    if (bits & (1<<24))
      senone_active[j++] = i;
    ++i;
    if (bits & (1<<25))
      senone_active[j++] = i;
    ++i;
    if (bits & (1<<26))
      senone_active[j++] = i;
    ++i;
    if (bits & (1<<27))
      senone_active[j++] = i;
    ++i;
    if (bits & (1<<28))
      senone_active[j++] = i;
    ++i;
    if (bits & (1<<29))
      senone_active[j++] = i;
    ++i;
    if (bits & (1<<30))
      senone_active[j++] = i;
    ++i;
    if (bits & (1<<31))
      senone_active[j++] = i;
    ++i;
#endif /* BITVEC_WIDTH == 32 */
#endif /* BITVEC_WIDTH > 8 */
  }

  for (; i < total_dists; ++i)
    if (*flagptr & (1<<(i % BITVEC_WIDTH)))
      senone_active[j++] = i;
  
  n_senone_active = j;
  
  return j;
}
#else
int32 sen_active_flags2list ( void )
{
  int32 i, j, total_dists, total_words, bits;
  uint8 *flagptr;
  
  total_dists = kb_get_total_dists();
  
  j = 0;
  total_words = total_dists & ~3;
  for (i = 0, flagptr = senone_active_vec; i < total_words;) {
    bits = *(int32 *)flagptr;
    if (bits == 0) {
      flagptr += 4;
      i += 4;
      continue;
    }
    if (*flagptr++)
      senone_active[j++] = i;
    ++i;
    if (*flagptr++)
      senone_active[j++] = i;
    ++i;
    if (*flagptr++)
      senone_active[j++] = i;
    ++i;
    if (*flagptr++)
      senone_active[j++] = i;
    ++i;
  }

  for (; i < total_dists; ++i, ++flagptr)
    if (*flagptr)
      senone_active[j++] = i;

  n_senone_active = j;
  
  return j;
}
#endif
