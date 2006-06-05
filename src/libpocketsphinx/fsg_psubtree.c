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
 * fsg_psubtree.c -- Phone-level FSG subtree representing all transitions
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
 * $Log: fsg_psubtree.c,v $
 * Revision 1.1.1.1  2006/05/23 18:44:59  dhuggins
 * re-importation
 *
 * Revision 1.2  2005/01/26 17:54:52  rkm
 * Added -maxhmmpf absolute pruning parameter in FSG mode
 *
 * Revision 1.1  2004/07/16 00:57:11  egouvea
 * Added Ravi's implementation of FSG support.
 *
 * Revision 1.3  2004/06/25 14:49:08  rkm
 * Optimized size of history table and speed of word transitions by maintaining only best scoring word exits at each state
 *
 * Revision 1.2  2004/05/27 14:22:57  rkm
 * FSG cross-word triphones completed (but for single-phone words)
 *
 * Revision 1.1.1.1  2004/03/01 14:30:30  rkm
 *
 *
 * Revision 1.3  2004/02/27 21:01:25  rkm
 * Many bug fixes in multiple FSGs
 *
 * Revision 1.2  2004/02/27 15:05:21  rkm
 * *** empty log message ***
 *
 * Revision 1.1  2004/02/23 15:53:45  rkm
 * Renamed from fst to fsg
 *
 * Revision 1.3  2004/02/23 15:09:50  rkm
 * *** empty log message ***
 *
 * Revision 1.2  2004/02/19 21:16:54  rkm
 * Added fsg_search.{c,h}
 *
 * Revision 1.1  2004/02/17 21:11:49  rkm
 * *** empty log message ***
 *
 * 
 * 10-Feb-2004	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon
 * 		Started.
 */


#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <ckd_alloc.h>
#include <err.h>
#include <fsg_psubtree.h>
#include <kb.h>
#include <dict.h>
#include <phone.h>
#include <log.h>
#include <hmm_tied_r.h>
#include <search.h>


void fsg_pnode_add_all_ctxt(fsg_pnode_ctxt_t *ctxt)
{
  int32 i;
  
  for (i = 0; i < FSG_PNODE_CTXT_BVSZ; i++)
    ctxt->bv[i] = 0xffffffff;
}


uint32 fsg_pnode_ctxt_sub (fsg_pnode_ctxt_t *src, fsg_pnode_ctxt_t *sub)
{
  int32 i;
  uint32 non_zero;
  
  non_zero = 0;
  
  for (i = 0; i < FSG_PNODE_CTXT_BVSZ; i++) {
    src->bv[i] = ~(sub->bv[i]) & src->bv[i];
    non_zero |= src->bv[i];
  }
  
  return non_zero;
}


/*
 * Add the word emitted by the given transition (fsglink) to the given lextree
 * (rooted at root), and return the new lextree root.  (There may actually be
 * several root nodes, maintained in a linked list via fsg_pnode_t.sibling.
 * "root" is the head of this list.)
 * lclist, rclist: sets of left and right context phones for this link.
 * alloc_head: head of a linear list of all allocated pnodes for the parent
 * FSG state, kept elsewhere and updated by this routine.
 * 
 * NOTE: No lextree structure for now; using a flat representation.
 */
static fsg_pnode_t *psubtree_add_trans (fsg_pnode_t *root,
					word_fsglink_t *fsglink,
					int8 *lclist, int8 *rclist,
					fsg_pnode_t **alloc_head)
{
  dictT *dict;
  int32 **lcfwd;	/* Uncompressed left cross-word context map;
			   lcfwd[left-diphone][p] = SSID for p.left-diphone */
  int32 **lcbwd;	/* Compressed left cross-word context map;
			   lcbwd[left-diphone] = array of unique SSIDs for all
			   possible left contexts */
  int32 **lcbwdperm;	/* For CIphone p, lcbwdperm[d][p] = index in lcbwd[d]
			   containing the SSID for triphone p.d */
  int32 **rcbwd;	/* Uncompressed right cross-word context map;
			   rcbwd[right-diphone][p] = SSID for right-diphone.p */
  int32 **rcfwd;	/* Compressed right cross-word context map; similar to
			   lcbwd */
  int32 **rcfwdperm;
  
  int32 silcipid;	/* Silence CI phone ID */
  int32 wip;		/* Word Insertion Penalty */
  int32 pip;		/* Phone Insertion Penalty */
  int32 pronlen;	/* Pronunciation length */
  float32 lw;		/* Language weight */
  int32 wid;		/* Word ID */
  int32 did;		/* Diphone ID */
  int32 ssid;		/* Senone Sequence ID */
  gnode_t *gn;
  fsg_pnode_t *pnode, *pred, *head;
  int32 n_ci, p, lc, rc;
  glist_t lc_pnodelist;	/* Temp pnodes list for different left contexts */ 
  glist_t rc_pnodelist;	/* Temp pnodes list for different right contexts */ 
  fsg_pnode_t **ssid_pnode_map;	/* Temp array of ssid->pnode mapping */
  int32 i, j;
  
  dict = kb_get_word_dict();
  lw = kb_get_lw();
  pip = (int32)(LOG(kb_get_pip()) * lw);
  wip = (int32)(LOG(kb_get_wip()) * lw);
  silcipid = kb_get_silence_ciphone_id();
  n_ci = phoneCiCount();
  
  lcfwd     = dict_left_context_fwd();
  lcbwd     = dict_left_context_bwd();
  lcbwdperm = dict_left_context_bwd_perm();
  rcbwd     = dict_right_context_bwd();
  rcfwd     = dict_right_context_fwd();
  rcfwdperm = dict_right_context_fwd_perm();
  
  wid = word_fsglink_wid(fsglink);
  assert (wid >= 0);	/* Cannot be a null transition */
  
  pronlen = dict_pronlen(dict, wid);
  assert (pronlen >= 1);
  if (pronlen > 255) {
    E_FATAL("Pronlen too long (%d); cannot use int8 for fsg_pnode_t.ppos\n",
	    pronlen);
  }

  assert (lclist[0] >= 0);	/* At least one phonetic context provided */
  assert (rclist[0] >= 0);
  
  head = *alloc_head;
  pred = NULL;
  
  if (pronlen == 1) {			/* Single-phone word */
    did = dict_phone(dict, wid, 0);	/* Diphone ID or SSID */
    if (dict_mpx(dict, wid)) {		/* Only non-filler words are mpx */
      /*
       * Left diphone ID for single-phone words already assumes SIL is right
       * context; only left contexts need to be handled.
       */
      lc_pnodelist = NULL;
      
      for (i = 0; lclist[i] >= 0; i++) {
	lc = lclist[i];
	ssid = lcfwd[did][lc];	/* Use lcfwd for single-phone word, not lcbwd,
				   as lcbwd would use only SIL as context */
	/* Check if this ssid already allocated for some other context */
	for (gn = lc_pnodelist; gn; gn = gnode_next(gn)) {
	  pnode = (fsg_pnode_t *) gnode_ptr(gn);
	  
	  if (pnode->hmm.sseqid == ssid) {
	    /* already allocated; share it for this context phone */
	    fsg_pnode_add_ctxt(pnode, lc);
	    break;
	  }
	}
	
	if (! gn) {	/* ssid not already allocated */
	  pnode = (fsg_pnode_t *) ckd_calloc (1, sizeof(fsg_pnode_t));
	  pnode->hmm.sseqid = ssid;
	  pnode->next.fsglink = fsglink;
	  pnode->logs2prob = word_fsglink_logs2prob(fsglink) + wip + pip;
	  pnode->ci_ext = (int8) dict_ciphone(dict, wid, 0);
	  pnode->ppos = 0;
	  pnode->leaf = TRUE;
	  pnode->sibling = root;	/* All root nodes linked together */
	  fsg_pnode_add_ctxt(pnode,lc);	/* Initially zeroed by calloc above */
	  pnode->alloc_next = head;
	  head = pnode;
	  root = pnode;
	  
	  search_chan_deactivate(&(pnode->hmm));
	  
	  lc_pnodelist = glist_add_ptr (lc_pnodelist, (void *)pnode);
	}
      }
      
      glist_free (lc_pnodelist);
    } else {		/* Filler word; no context modelled */
      ssid = did;	/* dict_phone() already has the right CIphone ssid */
      
      pnode = (fsg_pnode_t *) ckd_calloc (1, sizeof(fsg_pnode_t));
      pnode->hmm.sseqid = ssid;
      pnode->next.fsglink = fsglink;
      pnode->logs2prob = word_fsglink_logs2prob(fsglink) + wip + pip;
      pnode->ci_ext = silcipid;	/* Presents SIL as context to neighbors */
      pnode->ppos = 0;
      pnode->leaf = TRUE;
      pnode->sibling = root;
      fsg_pnode_add_all_ctxt(&(pnode->ctxt));
      pnode->alloc_next = head;
      head = pnode;
      root = pnode;
      
      search_chan_deactivate(&(pnode->hmm));	/* Mark HMM inactive */
    }
  } else {				/* Multi-phone word */
    assert (dict_mpx(dict, wid));	/* S2 HACK: pronlen>1 => mpx?? */
    
    ssid_pnode_map = (fsg_pnode_t **) ckd_calloc (n_ci, sizeof(fsg_pnode_t *));
    lc_pnodelist = NULL;
    rc_pnodelist = NULL;
    
    for (p = 0; p < pronlen; p++) {
      did = ssid = dict_phone(dict, wid, p);
      
      if (p == 0) {	/* Root phone, handle required left contexts */
	for (i = 0; lclist[i] >= 0; i++) {
	  lc = lclist[i];
	  
	  j = lcbwdperm[did][lc];
	  ssid = lcbwd[did][j];
	  pnode = ssid_pnode_map[j];
	  
	  if (! pnode) {	/* Allocate pnode for this new ssid */
	    pnode = (fsg_pnode_t *) ckd_calloc (1, sizeof(fsg_pnode_t));
	    pnode->hmm.sseqid = ssid;
	    pnode->logs2prob = word_fsglink_logs2prob(fsglink) + wip + pip;
	    pnode->ci_ext = (int8) dict_ciphone(dict, wid, 0);
	    pnode->ppos = 0;
	    pnode->leaf = FALSE;
	    pnode->sibling = root;	/* All root nodes linked together */
	    pnode->alloc_next = head;
	    head = pnode;
	    root = pnode;
	    
	    search_chan_deactivate(&(pnode->hmm));	/* Mark HMM inactive */
	    
	    lc_pnodelist = glist_add_ptr (lc_pnodelist, (void *)pnode);
	    ssid_pnode_map[j] = pnode;
	  } else {	
	    assert (pnode->hmm.sseqid == ssid);
	  }
	  fsg_pnode_add_ctxt(pnode, lc);
	}
      } else if (p != pronlen-1) {	/* Word internal phone */
	pnode = (fsg_pnode_t *) ckd_calloc (1, sizeof(fsg_pnode_t));
	pnode->hmm.sseqid = ssid;
	pnode->logs2prob = pip;
	pnode->ci_ext = (int8) dict_ciphone(dict, wid, p);
	pnode->ppos = p;
	pnode->leaf = FALSE;
	pnode->sibling = NULL;
	if (p == 1) {	/* Predecessor = set of root nodes for left ctxts */
	  for (gn = lc_pnodelist; gn; gn = gnode_next(gn)) {
	    pred = (fsg_pnode_t *) gnode_ptr(gn);
	    pred->next.succ = pnode;
	  }
	} else {	/* Predecessor = word internal node */
	  pred->next.succ = pnode;
	}
	pnode->alloc_next = head;
	head = pnode;
	
	search_chan_deactivate(&(pnode->hmm));	/* Mark HMM inactive */
	
	pred = pnode;
      } else {		/* Leaf phone, handle required right contexts */
	memset ((void *) ssid_pnode_map, 0, n_ci * sizeof(fsg_pnode_t *));
	
	for (i = 0; rclist[i] >= 0; i++) {
	  rc = rclist[i];
	  
	  j = rcfwdperm[did][rc];
	  ssid = rcfwd[did][j];
	  pnode = ssid_pnode_map[j];
	  
	  if (! pnode) {	/* Allocate pnode for this new ssid */
	    pnode = (fsg_pnode_t *) ckd_calloc (1, sizeof(fsg_pnode_t));
	    pnode->hmm.sseqid = ssid;
	    pnode->logs2prob = pip;
	    pnode->ci_ext = (int8) dict_ciphone(dict, wid, p);
	    pnode->ppos = p;
	    pnode->leaf = TRUE;
	    pnode->sibling = rc_pnodelist ?
	      (fsg_pnode_t *) gnode_ptr (rc_pnodelist) : NULL;
	    pnode->next.fsglink = fsglink;
	    pnode->alloc_next = head;
	    head = pnode;
	    
	    search_chan_deactivate(&(pnode->hmm));	/* Mark HMM inactive */
	    
	    rc_pnodelist = glist_add_ptr (rc_pnodelist, (void *)pnode);
	    ssid_pnode_map[j] = pnode;
	  } else {	
	    assert (pnode->hmm.sseqid == ssid);
	  }
	  fsg_pnode_add_ctxt(pnode, rc);
	}
	
	if (p == 1) {	/* Predecessor = set of root nodes for left ctxts */
	  for (gn = lc_pnodelist; gn; gn = gnode_next(gn)) {
	    pred = (fsg_pnode_t *) gnode_ptr(gn);
	    pred->next.succ = (fsg_pnode_t *) gnode_ptr (rc_pnodelist);
	  }
	} else {	/* Predecessor = word internal node */
	  pred->next.succ = (fsg_pnode_t *) gnode_ptr (rc_pnodelist);
	}
      }
    }
    
    ckd_free ((void *) ssid_pnode_map);
    glist_free (lc_pnodelist);
    glist_free (rc_pnodelist);
  }
  
  *alloc_head = head;
  
  return root;
}


/*
 * For now, this "tree" will be "flat"
 */
fsg_pnode_t *fsg_psubtree_init (word_fsg_t *fsg, int32 from_state,
				fsg_pnode_t **alloc_head)
{
  int32 dst;
  gnode_t *gn;
  word_fsglink_t *fsglink;
  fsg_pnode_t *root;
  int32 n_ci;
  
  root = NULL;
  assert (*alloc_head == NULL);
  
  n_ci = phoneCiCount();
  if (n_ci > (FSG_PNODE_CTXT_BVSZ * 32)) {
    E_FATAL("#phones > %d; increase FSG_PNODE_CTXT_BVSZ and recompile\n",
	    FSG_PNODE_CTXT_BVSZ * 32);
  }
  
  for (dst = 0; dst < word_fsg_n_state(fsg); dst++) {
    /* Add all links from from_state to dst */
    for (gn = word_fsg_trans(fsg,from_state,dst); gn; gn = gnode_next(gn)) {
      /* Add word emitted by this transition (fsglink) to lextree */
      fsglink = (word_fsglink_t *) gnode_ptr(gn);
      
      assert (word_fsglink_wid(fsglink) >= 0);	/* Cannot be a null trans */
      
      root = psubtree_add_trans (root, fsglink,
				 word_fsg_lc(fsg, from_state),
				 word_fsg_rc(fsg, dst),
				 alloc_head);
    }
  }
  
  return root;
}


void fsg_psubtree_free (fsg_pnode_t *head)
{
  fsg_pnode_t *next;
  
  while (head) {
    next = head->alloc_next;
    
    ckd_free ((void *) head);
    
    head = next;
  }
}


void fsg_psubtree_dump (fsg_pnode_t *head, FILE *fp)
{
  int32 i;
  word_fsglink_t *tl;
  
  for (; head; head = head->alloc_next) {
    /* Indentation */
    for (i = 0; i <= head->ppos; i++)
      fprintf (fp, "  ");
    
    fprintf (fp, "%08x.@", (int32)head);	/* Pointer used as node ID */
    fprintf (fp, " %5d.SS", head->hmm.sseqid);
    fprintf (fp, " %10d.LP", head->logs2prob);
    fprintf (fp, " %08x.SIB", (int32)head->sibling);
    fprintf (fp, " %s.%d", phone_from_id(head->ci_ext), head->ppos);
    if ((head->ppos == 0) || head->leaf) {
      fprintf (fp, " [");
      for (i = 0; i < FSG_PNODE_CTXT_BVSZ; i++)
	fprintf (fp, "%08x", head->ctxt.bv[i]);
      fprintf (fp, "]");
    }
    if (head->leaf) {
      tl = head->next.fsglink;
      fprintf (fp, " {%s[%d->%d](%d)}",
	       kb_get_word_str(tl->wid),
	       tl->from_state, tl->to_state, tl->logs2prob);
    } else {
      fprintf (fp, " %08x.NXT", (int32)head->next.succ);
    }
    fprintf (fp, "\n");
  }
  
  fflush (fp);
}


#if 0
boolean fsg_psubtree_pnode_enter (fsg_pnode_t *pnode,
				  int32 score, int32 frame, int32 bpidx) {
  boolean activate;
  
  assert (pnode->hmm.active <= frame);
  
  score += pnode->logs2prob;
  
  activate = FALSE;
  
  if (pnode->hmm.score[0] < score) {
    if (pnode->hmm.active < frame) {
      activate = TRUE;
      pnode->hmm.active = frame;
    }
    
    pnode->hmm.score[0] = score;
    pnode->hmm.path[0] = bpidx;
  }
  
  return activate;
}
#endif


void fsg_psubtree_pnode_deactivate (fsg_pnode_t *pnode)
{
  search_chan_deactivate(&(pnode->hmm));
}
