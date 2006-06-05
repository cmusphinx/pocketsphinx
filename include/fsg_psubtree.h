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
 * fsg_psubtree.h -- Phone-level FSG subtree representing all transitions
 * out of a single FSG state.
 * 
 * **********************************************
 * CMU ARPA Speech Project
 *
 * Copyright (c) 2004 Carnegie Mellon University.
 * ALL RIGHTS RESERVED.
 * **********************************************
 * 
 * HISTORY
 * 
 * $Log: fsg_psubtree.h,v $
 * Revision 1.1.1.1  2006/05/23 18:45:02  dhuggins
 * re-importation
 *
 * Revision 1.1  2004/07/16 00:57:12  egouvea
 * Added Ravi's implementation of FSG support.
 *
 * Revision 1.3  2004/06/25 14:49:08  rkm
 * Optimized size of history table and speed of word transitions by maintaining only best scoring word exits at each state
 *
 * Revision 1.2  2004/05/27 14:22:57  rkm
 * FSG cross-word triphones completed (but for single-phone words)
 *
 * Revision 1.1.1.1  2004/03/01 14:30:31  rkm
 *
 *
 * Revision 1.2  2004/02/27 15:05:21  rkm
 * *** empty log message ***
 *
 * Revision 1.1  2004/02/23 15:53:45  rkm
 * Renamed from fst to fsg
 *
 * Revision 1.4  2004/02/23 15:09:50  rkm
 * *** empty log message ***
 *
 * Revision 1.3  2004/02/19 21:16:54  rkm
 * Added fsg_search.{c,h}
 *
 * Revision 1.2  2004/02/18 15:02:34  rkm
 * Added fsg_lextree.{c,h}
 *
 * Revision 1.1  2004/02/17 21:11:49  rkm
 * *** empty log message ***
 *
 * 
 * 09-Feb-2004	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon
 * 		Started.
 */


#ifndef __S2_FSG_PSUBTREE_H__
#define __S2_FSG_PSUBTREE_H__


#include <stdio.h>
#include <string.h>
#include <s2types.h>
#include <word_fsg.h>
#include <msd.h>


/*
 * **HACK-ALERT**!!  Compile-time constant determining the size of the
 * bitvector fsg_pnode_t.fsg_pnode_ctxt_t.bv.  (See below.)
 * But it makes memory allocation simpler and more efficient.
 */
#define FSG_PNODE_CTXT_BVSZ	2

typedef struct {
  uint32 bv[FSG_PNODE_CTXT_BVSZ];
} fsg_pnode_ctxt_t;


/*
 * All transitions (words) out of any given FSG state represented are by a
 * phonetic prefix lextree (except for epsilon or null transitions; they
 * are not part of the lextree).  Lextree leaf nodes represent individual
 * FSG transitions, so no sharing is allowed at the leaf nodes.  The FSG
 * transition probs are distributed along the lextree: the prob at a node
 * is the max of the probs of all leaf nodes (and, hence, FSG transitions)
 * reachable from that node.
 * 
 * To conserve memory, the underlying HMMs with state-level information are
 * allocated only as needed.  Root and leaf nodes must also account for all
 * the possible phonetic contexts, with an independent HMM for each distinct
 * context.
 */
typedef struct fsg_pnode_s {
  /*
   * If this is not a leaf node, the first successor (child) node.  Otherwise
   * the parent FSG transition for which this is the leaf node (for figuring
   * the FSG destination state, and word emitted by the transition).  A node
   * may have several children.  The succ ptr gives just the first; the rest
   * are linked via the sibling ptr below.
   */
  union {
    struct fsg_pnode_s *succ;
    word_fsglink_t *fsglink;
  } next;
  
  /*
   * For simplicity of memory management (i.e., freeing the pnodes), all
   * pnodes allocated for all transitions out of a state are maintained in a
   * linear linked list through the alloc_next pointer.
   */
  struct fsg_pnode_s *alloc_next;
  
  /*
   * The next node that is also a child of the parent of this node; NULL if
   * none.
   */
  struct fsg_pnode_s *sibling;

  /*
   * The transition (log) probability to be incurred upon transitioning to
   * this node.  (Transition probabilities are really associated with the
   * transitions.  But a lextree node has exactly one incoming transition.
   * Hence, the prob can be associated with the node.)
   * This is a logs2(prob) value, and includes the language weight.
   */
  int32 logs2prob;
  
  /*
   * The root and leaf positions associated with any transition have to deal
   * with multiple phonetic contexts.  However, different contexts may result
   * in the same SSID (senone-seq ID), and can share a single pnode with that
   * SSID.  But the pnode should track the set of context CI phones that share
   * it.  Hence the fsg_pnode_ctxt_t bit-vector set-representation.  (For
   * simplicity of implementation, its size is a compile-time constant for
   * now.)  Single phone words would need a 2-D array of context, but that's
   * too expensive.  For now, they simply use SIL as right context, so only
   * the left context is properly modelled.
   * (For word-internal phones, this field is unused, of course.)
   */
  fsg_pnode_ctxt_t ctxt;
  
  uint8 ci_ext;		/* This node's CIphone as viewed externally (context) */
  uint8 ppos;		/* Phoneme position in pronunciation */
  boolean leaf;		/* Whether this is a leaf node */
  
  /* HMM-state-level stuff here */
  CHAN_T hmm;
} fsg_pnode_t;

/* Access macros */
#define fsg_pnode_leaf(p)	((p)->leaf)
#define fsg_pnode_logs2prob(p)	((p)->logs2prob)
#define fsg_pnode_succ(p)	((p)->next.succ)
#define fsg_pnode_fsglink(p)	((p)->next.fsglink)
#define fsg_pnode_sibling(p)	((p)->sibling)
#define fsg_pnode_hmmptr(p)	(&((p)->hmm))
#define fsg_pnode_ci_ext(p)	((p)->ci_ext)
#define fsg_pnode_ppos(p)	((p)->ppos)
#define fsg_pnode_leaf(p)	((p)->leaf)
#define fsg_pnode_ctxt(p)	((p)->ctxt)

#define fsg_pnode_add_ctxt(p,c)	((p)->ctxt.bv[(c)>>5] |= (1 << ((c)&0x001f)))


/*
 * Build the phone lextree for all transitions out of state from_state.
 * Return the root node of this tree.
 * Also, return a linear linked list of all allocated fsg_pnode_t nodes in
 * *alloc_head (for memory management purposes).
 */
fsg_pnode_t *fsg_psubtree_init (word_fsg_t *fsg,
				int32 from_state,
				fsg_pnode_t **alloc_head);


/*
 * Free the given lextree.  alloc_head: head of linear list of allocated
 * nodes updated by fsg_psubtree_init().
 */
void fsg_psubtree_free (fsg_pnode_t *alloc_head);


/*
 * Dump the list of nodes in the given lextree to the given file.  alloc_head:
 * head of linear list of allocated nodes updated by fsg_psubtree_init().
 */
void fsg_psubtree_dump (fsg_pnode_t *alloc_head, FILE *fp);


/*
 * Attempt to transition into the given node with the given attributes.
 * If the node is already active in the given frame with a score better
 * than the incoming score, nothing is done.  Otherwise the transition is
 * successful.
 * Return value: TRUE if the node was newly activated for the given frame,
 * FALSE if it was already activated for that frame (whether the incoming
 * transition was successful or not).
 */
boolean fsg_psubtree_pnode_enter (fsg_pnode_t *pnode,
				  int32 score,
				  int32 frame,
				  int32 bpidx);


/*
 * Mark the given pnode as inactive (for search).
 */
void fsg_psubtree_pnode_deactivate (fsg_pnode_t *pnode);


/* Set all flags on in the given context bitvector */
void fsg_pnode_add_all_ctxt(fsg_pnode_ctxt_t *ctxt);

/*
 * Subtract bitvector sub from bitvector src (src updated with the result).
 * Return 0 if result is all 0, non-zero otherwise.
 */
uint32 fsg_pnode_ctxt_sub (fsg_pnode_ctxt_t *src, fsg_pnode_ctxt_t *sub);


#endif
