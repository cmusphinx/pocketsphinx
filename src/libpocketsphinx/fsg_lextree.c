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
 * fsg_lextree.c -- The collection of all the lextrees for the entire FSM.
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
 * $Log: fsg_lextree.c,v $
 * Revision 1.1.1.1  2006/05/23 18:44:59  dhuggins
 * re-importation
 *
 * Revision 1.1  2004/07/16 00:57:11  egouvea
 * Added Ravi's implementation of FSG support.
 *
 * Revision 1.3  2004/06/23 20:32:16  rkm
 * *** empty log message ***
 *
 * Revision 1.2  2004/05/27 14:22:57  rkm
 * FSG cross-word triphones completed (but for single-phone words)
 *
 * Revision 1.1.1.1  2004/03/01 14:30:30  rkm
 *
 *
 * Revision 1.2  2004/02/27 15:05:21  rkm
 * *** empty log message ***
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


#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <ckd_alloc.h>
#include <err.h>
#include <fsg_lextree.h>


#define __FSG_DBG__		0


/*
 * For now, allocate the entire lextree statically.
 */
fsg_lextree_t *fsg_lextree_init (word_fsg_t *fsg)
{
  int32 s;
  fsg_lextree_t *lextree;
  fsg_pnode_t *pn;
  
  /* Allocate "this" structure */
  lextree = (fsg_lextree_t *) ckd_calloc (1, sizeof(fsg_lextree_t));
  
  lextree->fsg = fsg;
  
  /* Allocate ptrs for lextree root, and allocated list for each state */
  lextree->root = (fsg_pnode_t **) ckd_calloc (word_fsg_n_state(fsg),
					       sizeof(fsg_pnode_t *));
  lextree->alloc_head = (fsg_pnode_t **) ckd_calloc (word_fsg_n_state(fsg),
						     sizeof(fsg_pnode_t *));
  
  /* Create lextree for each state */
  lextree->n_pnode = 0;
  for (s = 0; s < word_fsg_n_state(fsg); s++) {
    lextree->root[s] = fsg_psubtree_init (fsg, s, &(lextree->alloc_head[s]));
    
    for (pn = lextree->alloc_head[s]; pn; pn = pn->alloc_next)
      lextree->n_pnode++;
  }
  E_INFO("%d HMM nodes in lextree\n", lextree->n_pnode);
  
#if __FSG_DBG__
  fsg_lextree_dump (lextree, stdout);
#endif

  return lextree;
}


void fsg_lextree_dump (fsg_lextree_t *lextree, FILE *fp)
{
  int32 s;
  
  for (s = 0; s < word_fsg_n_state(lextree->fsg); s++) {
    fprintf (fp, "State %5d root %08x\n", s, (int32)lextree->root[s]);
    fsg_psubtree_dump (lextree->alloc_head[s], fp);
  }
  fflush (fp);
}


void fsg_lextree_free (fsg_lextree_t *lextree)
{
  int32 s;
  
  for (s = 0; s < word_fsg_n_state(lextree->fsg); s++)
    fsg_psubtree_free (lextree->alloc_head[s]);
  
  ckd_free ((void *) lextree->root);
  ckd_free ((void *) lextree->alloc_head);
  ckd_free ((void *) lextree);
}


void fsg_lextree_utt_start (fsg_lextree_t *lextree)
{  
}


void fsg_lextree_utt_end (fsg_lextree_t *lextree)
{
}
