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
 * fsg_lextree.h -- The collection of all the lextrees for the entire FSM.
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
 * $Log: fsg_lextree.h,v $
 * Revision 1.1.1.1  2006/05/23 18:45:02  dhuggins
 * re-importation
 *
 * Revision 1.1  2004/07/16 00:57:12  egouvea
 * Added Ravi's implementation of FSG support.
 *
 * Revision 1.3  2004/06/23 20:32:16  rkm
 * *** empty log message ***
 *
 * Revision 1.2  2004/05/27 14:22:57  rkm
 * FSG cross-word triphones completed (but for single-phone words)
 *
 * Revision 1.1.1.1  2004/03/01 14:30:31  rkm
 *
 *
 * Revision 1.1  2004/02/23 15:53:45  rkm
 * Renamed from fst to fsg
 *
 * Revision 1.2  2004/02/19 21:16:54  rkm
 * Added fsg_search.{c,h}
 *
 * Revision 1.1  2004/02/18 15:02:34  rkm
 * Added fsg_lextree.{c,h}
 *
 * 
 * 18-Feb-2004	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon
 * 		Started.
 */


#ifndef __S2_FSG_LEXTREE_H__
#define __S2_FSG_LEXTREE_H__


#include <word_fsg.h>
#include <fsg_psubtree.h>


typedef struct fsg_lextree_s {
  word_fsg_t *fsg;	/* The fsg for which this lextree is built */
  fsg_pnode_t **root;	/* root[s] = lextree representing all transitions
			   out of state s.  Note that the "tree" for each
			   state is actually a collection of trees, linked
			   via fsg_pnode_t.sibling (root[s]->sibling) */
  fsg_pnode_t **alloc_head;	/* alloc_head[s] = head of linear list of all
				   pnodes allocated for state s */
  int32 n_pnode;	/* #HMM nodes in search structure */
} fsg_lextree_t;

/* Access macros */
#define fsg_lextree_root(lt,s)	((lt)->root[s])
#define fsg_lextree_n_pnode(lt)	((lt)->n_pnode)


/*
 * Create, initialize, and return a new phonetic lextree for the given FSM.
 */
fsg_lextree_t *fsg_lextree_init (word_fsg_t *);


void fsg_lextree_free (fsg_lextree_t *);


void fsg_lextree_dump (fsg_lextree_t *, FILE *);


void fsg_lextree_utt_start (fsg_lextree_t *);
void fsg_lextree_utt_end (fsg_lextree_t *);


#endif
