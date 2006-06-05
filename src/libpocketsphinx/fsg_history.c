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
 * fsg_history.c -- FSG Viterbi decode history
 * 
 * **********************************************
 * CMU ARPA Speech Project
 *
 * Copyright (c) 1999 Carnegie Mellon University.
 * ALL RIGHTS RESERVED.
 * **********************************************
 * 
 * HISTORY
 * 
 * $Log: fsg_history.c,v $
 * Revision 1.1.1.1  2006/05/23 18:44:59  dhuggins
 * re-importation
 *
 * Revision 1.3  2004/12/10 16:48:56  rkm
 * Added continuous density acoustic model handling
 *
 * 
 * 25-Feb-2004	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Started..
 */


#include <s2types.h>
#include <err.h>
#include <ckd_alloc.h>
#include <assert.h>
#include <fsg_history.h>
#include <kb.h>
#include <phone.h>
#include <search.h>


#define __FSG_DBG__	0


fsg_history_t *fsg_history_init (word_fsg_t *fsg)
{
  fsg_history_t *h;
  
  h = (fsg_history_t *) ckd_calloc (1, sizeof(fsg_history_t));
  h->fsg = fsg;
  h->entries = blkarray_list_init();
  
  if (fsg) {
    h->frame_entries = (glist_t **) ckd_calloc_2d (word_fsg_n_state(fsg),
						   phoneCiCount(),
						   sizeof(glist_t));
  } else {
    h->frame_entries = NULL;
  }
  
  return h;
}


void fsg_history_set_fsg (fsg_history_t *h, word_fsg_t *fsg)
{
  if (blkarray_list_n_valid(h->entries) != 0) {
    E_WARN("Switching FSG while history not empty; history cleared\n");
    blkarray_list_reset (h->entries);
  }
  
  if (h->frame_entries)
    ckd_free_2d ((void **) h->frame_entries);
  h->frame_entries = NULL;
  
  h->fsg = fsg;
  
  if (fsg) {
    h->frame_entries = (glist_t **) ckd_calloc_2d (word_fsg_n_state(fsg),
						   phoneCiCount(),
						   sizeof(glist_t));
  }
}


void fsg_history_entry_add (fsg_history_t *h,
			    word_fsglink_t *link,
			    int32 frame, int32 score, int32 pred,
			    int32 lc, fsg_pnode_ctxt_t rc)
{
  fsg_hist_entry_t *entry, *new_entry;
  int32 s;
  gnode_t *gn, *prev_gn;
  
  /* Skip the optimization for the initial dummy entries; always enter them */
  if (frame < 0) {
    new_entry = (fsg_hist_entry_t *) ckd_calloc(1, sizeof(fsg_hist_entry_t));
    new_entry->fsglink = link;
    new_entry->frame = frame;
    new_entry->score = score;
    new_entry->pred = pred;
    new_entry->lc = lc;
    new_entry->rc = rc;
    
    blkarray_list_append(h->entries, (void *)new_entry);
    return;
  }
  
  s = word_fsglink_to_state(link);
  
  /* Locate where this entry should be inserted in frame_entries[s][lc] */
  prev_gn = NULL;
  for (gn = h->frame_entries[s][lc]; gn; gn = gnode_next(gn)) {
    entry = (fsg_hist_entry_t *) gnode_ptr(gn);
    
    if (entry->score < score)
      break;	/* Found where to insert new entry */
    
    /* Existing entry score not worse than new score */
    if (fsg_pnode_ctxt_sub(&rc, &(entry->rc)) == 0)
      return;	/* rc set reduced to 0; new entry can be ignored */
    
    prev_gn = gn;
  }
  
  /* Create new entry after prev_gn (if prev_gn is NULL, at head) */
  new_entry = (fsg_hist_entry_t *) ckd_calloc(1, sizeof(fsg_hist_entry_t));
  new_entry->fsglink = link;
  new_entry->frame = frame;
  new_entry->score = score;
  new_entry->pred = pred;
  new_entry->lc = lc;
  new_entry->rc = rc;	/* Note: rc set must be non-empty at this point */
  
  if (! prev_gn) {
    h->frame_entries[s][lc] = glist_add_ptr (h->frame_entries[s][lc],
					     (void *) new_entry);
    prev_gn = h->frame_entries[s][lc];
  } else
    prev_gn = glist_insert_ptr (prev_gn, (void *) new_entry);
  
  /*
   * Update the rc set of all the remaining entries in the list.  At this
   * point, gn is the entry, if any, immediately following new entry.
   */
  while (gn) {
    entry = (fsg_hist_entry_t *) gnode_ptr(gn);
    
    if (fsg_pnode_ctxt_sub(&(entry->rc), &rc) == 0) {
      /* rc set of entry reduced to 0; can prune this entry */
      ckd_free ((void *)entry);
      gn = gnode_free (gn, prev_gn);
    } else {
      prev_gn = gn;
      gn = gnode_next(gn);
    }
  }
}


/*
 * Transfer the surviving history entries for this frame into the permanent
 * history table.
 */
void fsg_history_end_frame (fsg_history_t *h)
{
  int32 s, lc, ns, np;
  gnode_t *gn;
  fsg_hist_entry_t *entry;
  
  ns = word_fsg_n_state(h->fsg);
  np = phoneCiCount();
  
  for (s = 0; s < ns; s++) {
    for (lc = 0; lc < np; lc++) {
      for (gn = h->frame_entries[s][lc]; gn; gn = gnode_next(gn)) {
	entry = (fsg_hist_entry_t *) gnode_ptr(gn);
	blkarray_list_append(h->entries, (void *) entry);
      }
      
      glist_free(h->frame_entries[s][lc]);
      h->frame_entries[s][lc] = NULL;
    }
  }
}


fsg_hist_entry_t *fsg_history_entry_get (fsg_history_t *h, int32 id)
{
  blkarray_list_t *entries;
  int32 r, c;
  
  entries = h->entries;
  
  if (id >= blkarray_list_n_valid(entries))
    return NULL;
  
  r = id / blkarray_list_blksize(entries);
  c = id - (r * blkarray_list_blksize(entries));
  
  return ((fsg_hist_entry_t *) blkarray_list_ptr(entries, r, c));
}


void fsg_history_reset (fsg_history_t *h)
{
  blkarray_list_reset (h->entries);
}


int32 fsg_history_n_entries (fsg_history_t *h)
{
  return (blkarray_list_n_valid(h->entries));
}


int32 fsg_history_entry_hyp_extract (fsg_history_t *h, int32 id,
				     search_hyp_t *hyp)
{
  fsg_hist_entry_t *entry, *pred_entry;
  word_fsglink_t *fl;
  
  if (id <= 0)
    return -1;
  
  entry = fsg_history_entry_get(h, id);
  fl = entry->fsglink;
  
  hyp->wid = word_fsglink_wid(fl);
  hyp->word = (hyp->wid >= 0) ? kb_get_word_str(hyp->wid) : "";
  hyp->ef = entry->frame;
  hyp->lscr = word_fsglink_logs2prob(fl);
  hyp->fsg_state = word_fsglink_to_state(fl);
  hyp->conf = 0.0;		/* Not known */
  hyp->latden = 0;		/* Not known */
  hyp->phone_perp = 0.0;	/* Not known */
  
  /* hyp->sf and hyp->ascr depends on the predecessor entry */
  if (hyp->wid < 0) {		/* NULL transition */
    hyp->sf = hyp->ef;
    hyp->ascr = 0;
  } else {			/* Non-NULL transition */
    if (entry->pred < 0) {	/* Predecessor is dummy root entry */
      hyp->sf = 0;
      hyp->ascr = entry->score - hyp->lscr;
    } else {
      pred_entry = fsg_history_entry_get(h, entry->pred);
      hyp->sf = pred_entry->frame + 1;
      hyp->ascr = entry->score - pred_entry->score - hyp->lscr;
    }
  }
  
  assert (hyp->sf <= hyp->ef);
  
  return 1;
}


void fsg_history_dump (fsg_history_t *h, char const *uttid, FILE *fp)
{
  int32 i, r, nf;
  fsg_hist_entry_t *entry;
  word_fsglink_t *fl;
  search_hyp_t hyp;
  
  fprintf (fp, "# Hist-Begin %s\n", uttid ? uttid : "");
  fprintf (fp, "# Dummy root entry ID = 0\n");
  
  fprintf (fp, "# %5s %5s %5s %7s %11s %10s %11s %8s %8s %6s %4s %8s\n",
	   "Index", "SFrm", "EFrm", "Pred", "PathScr", "Lscr", "Ascr", "Ascr/Frm", "A-BS/Frm",
	   "FsgSt", "LC", "RC-set");
  
  for (i = 1; i < fsg_history_n_entries(h); i++) {
    entry = fsg_history_entry_get (h, i);
    
    if (fsg_history_entry_hyp_extract (h, i, &hyp) > 0) {
      nf = hyp.ef - hyp.sf + 1;
      fl = entry->fsglink;
      
      fprintf (fp, "%7d %5d %5d %7d %11d %10d %11d %8d %8d %6d %4d ",
	       i,
	       hyp.sf, hyp.ef,
	       entry->pred,
	       entry->score,
	       hyp.lscr, hyp.ascr,
	       (hyp.wid >= 0) ? hyp.ascr/nf : 0,
	       (hyp.wid >= 0) ? (seg_topsen_score(hyp.sf, hyp.ef) - hyp.ascr) / nf : 0,
	       word_fsglink_to_state(fl),
	       entry->lc);
      
      for (r = FSG_PNODE_CTXT_BVSZ-1; r > 0; r--)
	fprintf (fp, "%08x.", entry->rc.bv[r]);
      fprintf (fp, "%08x", entry->rc.bv[0]);
      
      fprintf (fp, "  %s\n", hyp.word);
    }
  }
  
  fprintf (fp, "# Hist-End %s\n", uttid ? uttid : "");
  fflush (fp);
}


void fsg_history_utt_start (fsg_history_t *h)
{
  int32 s, lc, ns, np;
  
  assert (blkarray_list_n_valid(h->entries) == 0);
  assert (h->frame_entries);
  
  ns = word_fsg_n_state(h->fsg);
  np = phoneCiCount();
  
  for (s = 0; s < ns; s++) {
    for (lc = 0; lc < np; lc++) {
      assert (h->frame_entries[s][lc] == NULL);
    }
  }
}


void fsg_history_utt_end (fsg_history_t *h)
{
}
