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
 * searchlat.c -- construct word lattice and search all paths for best path.
 * 
 * HISTORY
 * 
 * $Log: searchlat.c,v $
 * Revision 1.1.1.1  2006/05/23 18:45:01  dhuggins
 * re-importation
 *
 * Revision 1.12  2004/12/10 16:48:57  rkm
 * Added continuous density acoustic model handling
 *
 * 
 * 12-Aug-2004	M K Ravishankar (rkm@cs) at Carnegie Mellon University
 * 		Bugfix: Obtained current start_wid at the start of lattice_rescore().
 * 
 * 03-Apr-98	M K Ravishankar (rkm@cs) at Carnegie Mellon University
 * 		Added searchlat_set_rescore_lm().
 * 
 * 24-Mar-98	M K Ravishankar (rkm@cs) at Carnegie Mellon University
 * 		Added phone perplexity measure into search_hyp_t structure hypothesis
 * 		for each utterance.
 * 
 * 08-Mar-98	M K Ravishankar (rkm@cs) at Carnegie Mellon University
 * 		Added lattice density measure into search_hyp_t structure generated
 * 		as hypothesis for each utterance.
 * 
 * 27-May-97	M K Ravishankar (rkm@cs) at Carnegie Mellon University
 * 		Added Bob Brennan's code for handling multiple saved lattices.
 * 
 * 04-Apr-97	M K Ravishankar (rkm@cs) at Carnegie Mellon University
 * 		Added search_remove_context() call at the end of lattice_rescore.
 * 
 * 30-Oct-96	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University.
 * 		Added dump_lattice() similar to Sphinx-III format lattices (almost).
 * 
 * 08-Dec-95	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University.
 * 		Changed hyp[] result to contain actual frame ids instead of
 * 		post-silence-compression ids.
 * 
 * 08-Dec-95	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University.
 * 		In building the lattice, added a check to ignore BPTable words not
 * 		in the current LM.
 * 
 * Revision 8.3  94/10/11  12:41:26  rkm
 * Print back trace conditionally, depending on -backtrace argument.
 * Use actual pronunciation in shortest-path search, depending on
 * -reportpron argument.
 * Added acoustic score for </s> to final path score.
 * 
 * Revision 8.2  94/07/29  11:59:06  rkm
 * Added code to limit search for n-best alternatives.
 * Added search_save_lattice().
 * 
 * Revision 8.1  94/05/26  16:55:56  rkm
 * Non-recursive implementation of lattice rescoring using trigrams.
 * Created.
 * 
 */

#ifndef TRUE
/* For those infuriating HPs */
#define TRUE	1
#define FALSE	0
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "s2types.h"
#include "CM_macros.h"
#include "basic_types.h"
#include "strfuncs.h"
#include "linklist.h"
#include "list.h"
#include "hash.h"
#include "err.h"
#include "dict.h"
#include "lm.h"
#include "lmclass.h"
#include "lm_3g.h"
#include "search_const.h"
#include "msd.h"
#include "kb.h"
#include "fbs.h"
#include "search.h"

#define QUIT(x)		{fprintf x; exit(-1);}

#ifdef USE_ILM
#define lm_bg_score		ilm_bg_score
#define lm_tg_score		ilm_tg_score
#endif

/* Externally obtained variables */
static int32 finish_wid, start_wid, sil_wid, last_frame, sil_pen, filler_pen;
static dictT *dict;
static BPTBL_T *bptbl;
static int32 *BScoreStack;
static int32 **rc_fwdperm;
static search_hyp_t *hyp;
static int32 altpron = FALSE;

extern BPTBL_T *search_get_bptable ();
extern int32 *search_get_bscorestack ();
extern lw_t search_get_lw ();
extern search_hyp_t *search_get_hyp ();

#if 0
extern char *query_utt_id(void);
#endif

static latnode_t *latnode_list = NULL;

/* All paths start and end between these two lattice nodes */
static latnode_t *start_node, *final_node;
int32 final_node_ascr;	/* Acoustic score for the final </s> node */

/* Structure to hold a lattice (semi-)permanently (for dictation/correction...) */
static struct permanent_lattice_s {
    latnode_t *latnode_list;
    latnode_t *start_node, *final_node;
} lattice;

/* Structure added by Bob Brennan for maintaining several lattices at a time */
#define	MAX_LAT_QUEUE		20
static	struct	lattice_queue_s	{
    struct	permanent_lattice_s		lattice;
    char	lmName[256];
    char	uttid[256];
    int32	*comp2rawfr;
    int32	n_featfr;
    int32	addIndex;
} latQueue[MAX_LAT_QUEUE];
static	int32	latQueueInit = 0;
static	int32	latQueueAddIndex = 0;
static	char	altLMName[256];

static int32 n_node, n_link;

#define ISA_FILLER_WORD(x)	((x)>=sil_wid)
#define ISA_REAL_WORD(x)	((x)<finish_wid)

static lw_t lw_factor;	/* Lang-weight factor (lw(2nd pass)/lw(1st pass)) */
static char *rescore_lmname = NULL;

void
searchlat_set_rescore_lm (char const *lmname)
{
    if (rescore_lmname)
	free (rescore_lmname);
    if ((rescore_lmname = (char *)salloc(lmname)) == NULL)
	E_ERROR("salloc('%s') failed\n", lmname);
}

/*
 * Create a directed link between "from" and "to" nodes, but if a link already exists,
 * choose one with the best link_scr.
 */
static void link_latnodes (latnode_t *from, latnode_t *to, int32 score, int32 ef)
{
    latlink_t *link;
    
    assert (to->reachable);
    
    /* Look for an existing link between "from" and "to" nodes */
    for (link = from->links; link && (link->to != to); link = link->next);
    
    if (! link) {
	/* No link between the two nodes; create a new one */
	link = (latlink_t *) listelem_alloc (sizeof(latlink_t));

	link->from = from;
	link->to = to;
	link->link_scr = score;
	link->ef = ef;
	link->best_prev = NULL;
	
	link->next = from->links;
	from->links = link;
    } else {
	/* Link already exists; just retain the best link_scr */
	if (link->link_scr < score) {
	    link->link_scr = score;
	    link->ef = ef;
	}
    }
}

static void bypass_filler_nodes ( void )
{
    latnode_t *node, *to, *from, *prev_node, *t_node;
    latlink_t *link, *f_link, *t_link, *prev_link;
    rev_latlink_t *revlink, *t_revlink;
    int32 score;
    
    /* Create reverse links for all links pointing to filler nodes */
    for (node = latnode_list; node; node = node->next) {
	for (link = node->links; link; link = link->next) {
	    to = link->to;
	    if (ISA_FILLER_WORD(to->wid)) {
		revlink = (rev_latlink_t *) listelem_alloc (sizeof (rev_latlink_t));
		revlink->link = link;
		revlink->next = to->revlinks;
		to->revlinks = revlink;
	    }
	}
    }

    /* Bypass filler nodes */
    for (node = latnode_list; node; node = node->next) {
	if (! ISA_FILLER_WORD(node->wid))
	    continue;
	
	/* Replace each link entering filler node with links to all its successors */
	for (revlink = node->revlinks; revlink; revlink = revlink->next) {
	    link = revlink->link;	/* link entering filler node */
	    from = link->from;
	    
	    score = (node->wid == sil_wid) ? sil_pen : filler_pen;
	    score += link->link_scr;
	    
	    /*
	     * Make links from predecessor of filler (from) to successors of filler.
	     * But if successor is a filler, it has already been eliminated since it
	     * appears earlier in latnode_list (see build...).  So it can be skipped.
	     * Likewise, no reverse links needed for the new links; none of them
	     * points to a filler node.
	     */
	    for (f_link = node->links; f_link; f_link = f_link->next) {
		if (! ISA_FILLER_WORD(f_link->to->wid))
		    link_latnodes (from, f_link->to, score + f_link->link_scr, link->ef);
	    }
	}
    }

    /* Delete filler nodes and all links and reverse links from it */
    prev_node = NULL;
    for (node = latnode_list; node; node = t_node) {
	t_node = node->next;
	if (ISA_FILLER_WORD(node->wid)) {
	    for (revlink = node->revlinks; revlink; revlink = t_revlink) {
		t_revlink = revlink->next;
		revlink->link->to = NULL;
		listelem_free (revlink, sizeof(rev_latlink_t));
	    }

	    for (link = node->links; link; link = t_link) {
		t_link = link->next;
		listelem_free (link, sizeof(latlink_t));
	    }

	    if (prev_node)
		prev_node->next = t_node;
	    else
		latnode_list = t_node;
		
	    listelem_free (node, sizeof(latnode_t));
	} else
	    prev_node = node;
    }

    /* Reclaim links pointing nowhere */
    for (node = latnode_list; node; node = node->next) {
	prev_link = NULL;
	for (link = node->links; link; link = t_link) {
	    t_link = link->next;
	    if (link->to == NULL) {
		if (prev_link)
		    prev_link->next = t_link;
		else
		    node->links = t_link;
		listelem_free (link, sizeof(latlink_t));
	    } else
		prev_link = link;
	}
    }
}

static void mark_reachable (latnode_t *from)
{
    latlink_t *link;
    
    from->reachable = TRUE;
    for (link = from->links; link; link = link->next) {
	if (! link->to->reachable)
	    mark_reachable (link->to);
    }
}

    
static void delete_unreachable ( void )
{
    latnode_t *node, *t_node, *prev_node;
    latlink_t *link, *t_link;
    
    prev_node = NULL;
    for (node = latnode_list; node; node = t_node) {
	t_node = node->next;
	if (! node->reachable) {
	    /* Node and its links can be removed */
	    if (prev_node)
		prev_node->next = node->next;
	    else
		latnode_list = node->next;
	    for (link = node->links; link; link = t_link) {
		t_link = link->next;
		listelem_free (link, sizeof(latlink_t));
	    }
	    listelem_free (node, sizeof(latnode_t));
	} else
	    prev_node = node;
    }
}

static int32 latnode_seqid (latnode_t *target)
{
    int32 i;
    latnode_t *d;
    
    for (i = 0, d = latnode_list; d && (d != target); d = d->next, i++);
    assert (d);
    return (i);
}

static int32 dump_lattice (char *filename)
{
    FILE *fp;
    int32 i;
    latnode_t *d, *initial, *final;
    /* char str[1024]; */ /* !!! */
    
    initial = start_node;
    final = final_node;

    E_INFO("Writing lattice file: %s\n", filename);
    if ((fp = fopen (filename, "w")) == NULL) {
	E_ERROR("fopen(%s,w) failed\n", filename);
	return -1;
    }
    
    fprintf (fp, "# Generated by FBS8 using Sphinx-II semicontinuous models\n");
    fprintf (fp, "# -logbase %e\n", 1.0001);
    fprintf (fp, "#\n");
    
    fprintf (fp, "Frames %d\n", last_frame+1);
    fprintf (fp, "#\n");
    
    for (i = 0, d = latnode_list; d; d = d->next, i++);
    fprintf (fp, "Nodes %d (NODEID WORD STARTFRAME FIRST-ENDFRAME LAST-ENDFRAME)\n", i);
    for (i = 0, d = latnode_list; d; d = d->next, i++) {
	fprintf (fp, "%d %s %d %d %d\n",
		 i, kb_get_word_str(d->wid), d->sf, d->fef, d->lef);
    }
    fprintf (fp, "#\n");

    fprintf (fp, "Initial %d\nFinal %d\n", latnode_seqid(initial), latnode_seqid(final));
    fprintf (fp, "#\n");
    
    /* Best score (i.e., regardless of Right Context) for word segments in word lattice */
    fprintf (fp, "BestSegAscr %d (NODEID ENDFRAME ASCORE)\n", 0 /* #BPTable entries */);
#if 0
    for (i = 0; i < n_lat_entry; i++) {
	int32 ascr, lscr;
	lat_seg_ascr_lscr (i, BAD_WID, &ascr, &lscr);
	fprintf (fp, "%d %d %d\n",
		 (lattice[i].dagnode)->seqid, lattice[i].frm, ascr);
    }
#endif
    fprintf (fp, "#\n");
    
    fprintf (fp, "Edges (FROM-NODEID TO-NODEID ASCORE)\n");
#if 0
    for (d = latnode_list; d; d = d->next) {
	latlink_t *l;
	for (l = d->links; l; l = l->next)
	    fprintf (fp, "%d %d %d\n",
		     latnode_seqid(d), latnode_seqid(l->to), l->link_scr);
    }
#endif
    fprintf (fp, "End\n");

    fclose (fp);

    return 0;
}

/*
 * Build lattice from bptable and identify the start and final nodes.
 * Return 1 if successful, 0 otherwise.
 */
static int32 build_lattice (int32 bptbl_sz)
{
    int32 i, sf, ef, lef, wid, score, bss_offset;
    BPTBL_T *bp_ptr;
    latnode_t *node, *from, *to; /* , *prev_node, *t_node; */
    latlink_t *link; /* *t_link, *prev_link; */
    int32 nn, nl;
    char const *dumplatdir;
    
    /* Create lattice nodes; not all these may be useful */
    last_frame = searchFrame ();
    latnode_list = NULL;
    for (i = 0, bp_ptr = bptbl; i < bptbl_sz; i++, bp_ptr++) {
	if (! bp_ptr->valid)
	    continue;
	
	sf = (bp_ptr->bp < 0) ? 0 : bptbl[bp_ptr->bp].frame+1;
	ef = bp_ptr->frame;
	wid = bp_ptr->wid;
	
	/* Skip non-final </s> entries; note bptable entry for final </s> */
	if ((wid == finish_wid) && (ef < last_frame))
	    continue;
	
	/* Skip if word not in LM */
	if ((! ISA_FILLER_WORD(wid)) && (! dictwd_in_lm (dict->dict_list[wid]->fwid)))
	    continue;
	
	/* See if bptbl entry <wid,sf> already in lattice */
	for (node = latnode_list; node; node = node->next) {
	    if ((node->wid == wid) && (node->sf == sf))
		break;
	}

	/* For the moment, store bptbl indices in node.{fef,lef} */
	if (node)
	    node->lef = i;
	else {
	    /* New node; link to head of list */
	    node = (latnode_t *) listelem_alloc (sizeof(latnode_t));
	    node->wid = wid;
	    node->sf = sf;
	    node->fef = node->lef = i;
	    node->reachable = FALSE;
	    node->links = NULL;
	    node->revlinks = NULL;
	    
	    node->next = latnode_list;
	    latnode_list = node;
	}
    }
    
    /*
     * At this point, latnode_list is ordered such that nodes earlier in the list
     * can follow (in time) those later in the list, but not vice versa.  Now
     * create precedence links and simultanesously mark all nodes that can reach
     * the final </s> node.  (All nodes are reached from <s>.0; no problem there.)
     */
    
    /* Find start node <s>.0 */
    for (node = latnode_list; node; node = node->next) {
	if ((node->wid == start_wid) && (node->sf == 0))
	    break;
    }
    if (! node) {
	E_ERROR("Couldn't find <s>.0\n");
	return (0);
    }
    start_node = node;
    
    /* Find final node </s>.last_frame; nothing can follow this node */
    for (node = latnode_list; node; node = node->next) {
	lef = bptbl[node->lef].frame;
	if ((node->wid == finish_wid) && (lef == last_frame))
	    break;
    }
    if (! node) {
	E_ERROR("Couldn't find </s>.%d\n", last_frame);
	return 0;
    }
    final_node = node;
    final_node_ascr = bptbl[node->lef].ascr;

    /*
     * Create precedence links between all possible pairs of nodes.  However,
     * ignore nodes from which </s>.last_frame is not reachable.
     */
    final_node->reachable = TRUE;
    for (to = final_node; to; to = to->next) {
	/* Skip if not reachable; it will never be reachable from </s>.last_frame */
	if (! to->reachable)
	    continue;
	
	/* Find predecessors of to : from->fef+1 <= to->sf <= from->lef+1 */
	for (from = to->next; from; from = from->next) {
	    ef = bptbl[from->fef].frame;
	    lef = bptbl[from->lef].frame;

	    if ((to->sf <= ef) || (to->sf > lef+1))
		continue;
	    
	    /* Find bptable entry for "from" that exactly precedes "to" */
	    i = from->fef;
	    bp_ptr = bptbl+i;
	    for (; i <= from->lef; i++, bp_ptr++) {
		if (bp_ptr->wid != from->wid)
		    continue;
		if (bp_ptr->frame >= to->sf-1)
		    break;
	    }
	    
	    if ((i > from->lef) || (bp_ptr->frame != to->sf-1))
		continue;
	    
	    /* Find acoustic score from.sf->to.sf-1 with right context = to */
	    if (bp_ptr->r_diph >= 0)
		bss_offset = rc_fwdperm[bp_ptr->r_diph][dict->dict_list[to->wid]->ci_phone_ids[0]];
	    else
		bss_offset = 0;
	    score = (BScoreStack[bp_ptr->s_idx + bss_offset] - bp_ptr->score) + bp_ptr->ascr;
	    if (score > WORST_SCORE) {
		link_latnodes (from, to, score, bp_ptr->frame);
		from->reachable = TRUE;
	    }
	}
    }
    
    /* There must be at least one path between <s>.0 and </s>.last_frame */
    if (! start_node->reachable) {
	E_ERROR ("<s>.0 isolated; unreachable\n");
	return (0);
    }

    /* Now change node->{fef,lef} from bptbl indices to frames. */
    for (node = latnode_list; node; node = node->next) {
	node->fef = bptbl[node->fef].frame;
	node->lef = bptbl[node->lef].frame;
    }

    if ((dumplatdir = query_dumplat_dir()) != NULL) {
	char latfile[1024];
	
	sprintf (latfile, "%s/%s.lat", dumplatdir, uttproc_get_uttid());
	dump_lattice (latfile);
    }

    /* Change node->wid to base wid if not reporting actual pronunciations. */
    for (node = latnode_list; node; node = node->next) {
	if (! altpron)
	    node->wid = dict->dict_list[node->wid]->fwid;
    }

    /* Remove SIL and noise nodes from DAG; extend links through them */
    bypass_filler_nodes ();
    
    /* Free nodes unreachable from </s>, and their links */
    delete_unreachable ();

    nn = nl = 0;
    for (node = latnode_list; node; node = node->next) {
	for (link = node->links; link; link = link->next)
	    nl++;
	nn++;
    }
    n_node = nn;
    n_link = nl;

    return (1);
}

static void destroy_lattice (latnode_t *node_list)
{
    latnode_t *node, *tnode;
    latlink_t *link, *tlink;
    
    for (node = node_list; node; node = tnode) {
	tnode = node->next;
	for (link = node->links; link; link = tlink) {
	    tlink = link->next;
	    listelem_free (link, sizeof(latlink_t));
	}
	listelem_free (node, sizeof(latnode_t));
    }
}

int32 bptbl2latdensity (int32 bptbl_sz, int32 *density)
{
    int32 i, f, sf, ef, wid, lastfr;
    BPTBL_T *bp_ptr;
    latnode_t *node, *node2, *pred, *node2next, *latnodes;
    
    /* Create lattice nodes; not all these may be useful */
    lastfr = searchFrame ();
    latnodes = NULL;
    
    for (i = 0, bp_ptr = bptbl; i < bptbl_sz; i++, bp_ptr++) {
	sf = (bp_ptr->bp < 0) ? 0 : bptbl[bp_ptr->bp].frame+1;
	ef = bp_ptr->frame;
	wid = bp_ptr->wid;
	
	/* Skip non-final </s> entries; note bptable entry for final </s> */
	if ((wid == finish_wid) && (ef < lastfr))
	    continue;
	
	/* Skip if word not in LM */
	if ((! ISA_FILLER_WORD(wid)) && (! dictwd_in_lm (dict->dict_list[wid]->fwid)))
	    continue;
	
	/* See if bptbl entry <wid,sf> already in lattice */
	for (node = latnodes; node; node = node->next) {
	    if ((node->wid == wid) && (node->sf == sf))
		break;
	}
	
	/* For the moment, store bptbl indices in node.{fef,lef} */
	if (node)
	    node->lef = ef;
	else {
	    /* New node; link to head of list */
	    node = (latnode_t *) listelem_alloc (sizeof(latnode_t));
	    node->wid = wid;
	    node->sf = sf;
	    node->fef = node->lef = ef;
	    node->reachable = FALSE;
	    node->links = NULL;
	    node->revlinks = NULL;
	    
	    node->next = latnodes;
	    latnodes = node;
	}
    }
    
    /*
     * At this point, latnode_list is ordered such that nodes earlier in the list
     * can follow (in time) those later in the list, but not vice versa.
     */
    
    /* Merge overlapping instances into one */
    for (node = latnodes; node; node = node->next) {
	pred = node;
	for (node2 = node->next; node2; node2 = node2next) {
	    node2next = node2->next;
	    
	    if ((node->wid == node2->wid) &&
		(node->sf <= node2->lef) && (node2->sf <= node->lef)) {
		/* Overlapping instances of a word; merge */
		if (node2->sf < node->sf)
		    node->sf = node2->sf;
		if (node2->fef < node->fef)
		    node->fef = node2->fef;
		if (node2->lef > node->lef)
		    node->lef = node2->lef;
		
		pred->next = node2next;
		listelem_free (node2, sizeof(latnode_t));
	    } else
		pred = node2;
	}
    }

    for (i = 0; i <= lastfr; i++)
	density[i] = 0;
    
    for (node = latnodes; node; node = node->next) {
	if (node->lef > node->fef + 2) {	/* A little pruning */
	    for (f = node->sf; f <= node->lef; f++)
		(density[f])++;
	}
    }
    
    while (latnodes) {
	node = latnodes;
	latnodes = latnodes->next;
	listelem_free (node, sizeof(latnode_t));
    }
    
    return 0;
}

static int32 seg;	/* for traversing hyp[] */
extern int32 print_back_trace;

static void lattice_seg_back_trace (latlink_t *link)
{
    int32 f, latden;
    int32 *lattice_density, *search_get_lattice_density();
    double *phone_perplexity, *search_get_phone_perplexity();
    double perp;
    
    lattice_density = search_get_lattice_density();
    phone_perplexity = search_get_phone_perplexity();
    
    if (link) {
	lattice_seg_back_trace (link->best_prev);

	if (ISA_REAL_WORD(link->from->wid)) {
	    if (seg >= HYP_SZ-1)
		E_FATAL("**ERROR** Increase HYP_SZ\n");
	    
	    hyp[seg].wid = link->from->wid;
	    hyp[seg].sf = uttproc_feat2rawfr(link->from->sf);
	    hyp[seg].ef = uttproc_feat2rawfr(link->ef);
	    
	    hyp[seg].latden = 0;
	    hyp[seg].phone_perp = 0.0;
	    
	    for (f = link->from->sf; f <= link->ef; f++) {
		hyp[seg].latden += lattice_density[f];
		hyp[seg].phone_perp += phone_perplexity[f];
	    }
	    if ((link->ef - link->from->sf + 1) > 0) {
		hyp[seg].latden /= (link->ef - link->from->sf + 1);
		hyp[seg].phone_perp /= (link->ef - link->from->sf + 1);
	    }
	    latden = hyp[seg].latden;
	    perp = hyp[seg].phone_perp;
	    
	    seg++;
	    hyp[seg].wid = -1;

	    if (print_back_trace)
		printf ("\t%4d %4d %10d %11d %11d %8d %6d %6.2f %s\n",
			link->from->sf, link->ef,
			(-(link->link_scr))/(link->ef - link->from->sf + 1),
			-(link->link_scr), -(link->path_scr),
			seg_topsen_score(link->from->sf, link->ef)/(link->ef-link->from->sf+1),
			latden, perp,
			dict->dict_list[link->from->wid]->word);
	}
    } else {
	seg = 0;
	hyp[0].wid = -1;

	if (print_back_trace) {
	    printf ("\t%4s %4s %10s %11s %11s %8s %6s %6s %s (Bestpath) (%s)\n",
		    "SFrm", "EFrm", "AScr/Frm", "AScr", "PathScr", "BSDiff",
		    "LatDen", "PhPerp", "Word",
		    uttproc_get_uttid());
	    printf ("\t------------------------------------------------------------------------\n");
	}
    }
}

/*
 * Lattice rescoring:  Goal: Form DAG of nodes based on unique <wid,start-fram> values,
 * and find the best path through the DAG from <<s>,0> to <</s>,last_frame>.
 * Strategy: Find the best score from <<s>,0> to end point of any link and use it to
 * update links further down the path... (reword)
 */
int32 lattice_rescore ( lw_t lwf )
{
    latnode_t *node;
    latlink_t *link;
    latlink_t *q_head, *q_tail;
    int32 score, bw0, bw1, bw2;
    int32 fr;
    char *res;
    char *orig_lmname = NULL;
    
    sil_pen = search_get_sil_penalty ();
    filler_pen = search_get_filler_penalty ();
    lw_factor = lwf;
    
    start_wid = search_get_current_startwid();
    
    if (latnode_list) {
	destroy_lattice (latnode_list);
	latnode_list = NULL;
    }
    
    if (rescore_lmname) {
	orig_lmname = get_current_lmname();
	
	if (lm_set_current (rescore_lmname) < 0) {
	    E_ERROR("lm_set_current(%s) failed\n", rescore_lmname);
	    free(rescore_lmname);
	    rescore_lmname = NULL;
	}
    }
    
    if (! build_lattice (search_get_bptable_size())) {
	E_INFO ("build_lattice() failed\n");

	destroy_lattice (latnode_list);
	latnode_list = NULL;

	if (rescore_lmname) {
	    free (rescore_lmname);
	    rescore_lmname = NULL;
	    
	    if (orig_lmname)
		lm_set_current (orig_lmname);
	}
	
	return -1;
    }
    
    /* Initialize node fanin counts and path scores */
    for (node = latnode_list; node; node = node->next)
	node->info.fanin = 0;
    for (node = latnode_list; node; node = node->next) {
	for (link = node->links; link; link = link->next) {
	    (link->to->info.fanin)++;
	    link->path_scr = (int32) 0x80000000;
	}
    }

    /*
     * Form initial queue of optimal partial paths; = links out of start_node.
     * Path score includes LM score for the "to" node of the last link in the
     * path, but not the acoustic score for that node.
     */
    q_head = q_tail = NULL;
    for (link = start_node->links; link; link = link->next) {
	assert (! (ISA_FILLER_WORD(link->to->wid)));

	if (altpron) {
	    bw1 = dict->dict_list[start_wid]->fwid;
	    bw2 = dict->dict_list[link->to->wid]->fwid;
	    
	    link->path_scr = link->link_scr + LWMUL(lm_bg_score (bw1, bw2),lwf);
	} else
	    link->path_scr = link->link_scr +
		LWMUL(lm_bg_score (start_wid, link->to->wid), lwf);

	link->best_prev = NULL;
	
	if (! q_head)
	    q_head = link;
	else
	    q_tail->q_next = link;
	q_tail = link;
    }
    q_tail->q_next = NULL;
    
    /* Extend partial paths in queue as long as queue not empty */
    while (q_head) {
	/* Update path score for all possible links out of q_head->to */
	node = q_head->to;

#if 0
	printf ("QHD %s.%d -> %s.%d (%d, %d)\n",
		dict->dict_list[q_head->from->wid]->word, q_head->from->sf,
		dict->dict_list[node->wid]->word, node->sf,
		q_head->link_scr, q_head->path_scr);
#endif
	
	for (link = node->links; link; link = link->next) {
	    assert (! (ISA_FILLER_WORD(link->to->wid)));

	    if (altpron) {
		bw0 = dict->dict_list[q_head->from->wid]->fwid;
		bw1 = dict->dict_list[node->wid]->fwid;
		bw2 = dict->dict_list[link->to->wid]->fwid;
		
		score = q_head->path_scr + link->link_scr +
		    LWMUL(lm_tg_score (bw0, bw1, bw2), lwf);
	    } else
		score = q_head->path_scr + link->link_scr +
		    LWMUL(lm_tg_score (q_head->from->wid, node->wid, link->to->wid), lwf);
	    
	    if (score > link->path_scr) {
		link->path_scr = score;
		link->best_prev = q_head;
	    }
	}
	
	if (--(node->info.fanin) == 0) {
	    /*
	     * Links out of node (q_head->to) updated wrt all incoming links at node.
	     * They all now have optimal partial path scores; insert them in optimal
	     * partial path queue.
	     */
	    for (link = node->links; link; link = link->next) {
		q_tail->q_next = link;
		q_tail = link;
	    }
	    q_tail->q_next = NULL;
	}
	
	q_head = q_head->q_next;
    }

    /*
     * Rescored all paths to </s>.last_frame; now traceback optimal path.  First,
     * find the best link entering </s>.last_frame.
     */
    {
	latlink_t *best = NULL;
	
	score = (int32) 0x80000000;
	for (node = latnode_list; node; node = node->next) {
	    for (link = node->links; link; link = link->next)
		if ((link->to == final_node) && (link->path_scr > score)) {
		    score = link->path_scr;
		    best = link;
		}
	}

	assert(best != NULL);
	lattice_seg_back_trace (best);

	search_remove_context (hyp);
	search_hyp_to_str ();

	/* NB: best->path_scr doesn't include acoustic score for the final </s>! */
	search_set_hyp_total_score (best->path_scr+final_node_ascr);
    
	search_result (&fr, &res);
	E_INFO("BESTPATH: %s (%s %d)\n",
		res, uttproc_get_uttid (), best->path_scr+final_node_ascr);
    
	E_INFO("%8d nodes, %d links searched\n\n", n_node, n_link);
    }

    if (rescore_lmname) {
	free (rescore_lmname);
	rescore_lmname = NULL;
	
	if (orig_lmname)
	    lm_set_current (orig_lmname);
    }
    
    return 0;
}

/*
 * Sort lattice nodes according to extent of end frames.
 */
void sort_lattice (void)
{
    latnode_t *nodelist, *tmplist;
    int32 m;
    
    nodelist = lattice.latnode_list;
    tmplist = NULL;

    while (nodelist) {
	latnode_t *prev_node = NULL;
	latnode_t *prev_m = NULL;
	latnode_t *node;

	m = (int32) 0x7fffffff;
	for (node = nodelist; node; node = node->next) {
	    if ((node->lef - node->fef + 1) < m) {
		m = node->lef - node->fef + 1;
		prev_m = prev_node;
	    }
	    prev_node = node;
	}

	if (prev_m) {
	    node = prev_m->next;
	    prev_m->next = node->next;
	} else {
	    node = nodelist;
	    nodelist = node->next;
	}
	
	node->next = tmplist;
	tmplist = node;
    }

    lattice.latnode_list = tmplist;
}

latnode_t *search_get_lattice ( void )
{
    return (lattice.latnode_list);
}

void searchlat_init ( void )
{
    start_wid = kb_get_word_id (kb_get_lm_start_sym());
    finish_wid = kb_get_word_id (kb_get_lm_end_sym());
    sil_wid = kb_get_word_id ("SIL");
    dict = kb_get_word_dict ();
    rc_fwdperm = dict_right_context_fwd_perm ();
    altpron = query_report_altpron ();
    
    bptbl = search_get_bptable ();
    BScoreStack = search_get_bscorestack ();

    hyp = search_get_hyp ();

    lattice.latnode_list = NULL;
    lattice.final_node = NULL;
}

/* -----------Code for n-best hypotheses, uses the same lattice structure------------- */

/*
 * Tree of all possible paths, rooted at the initial "from" node.  Each partial
 * path (latpath_t) is constructed by extending another partial path--parent--by
 * one node.
 */
typedef struct latpath_s {
    latnode_t *node;		/* Also contains score estimate to final node */
    struct latpath_s *parent;
    struct latpath_s *next;	/* All paths linked linearly, sorted by best score */
    int32 score;		/* Exact score from start node upto node->sf */
} latpath_t;
static latpath_t *path_list;	/* Partial paths to be processed */
static latpath_t *path_tail;	/* Last in path_list */
static latpath_t *paths_done;	/* Partial paths that have been processed */

static int32 n_path;		/* #partial paths in path_list at any time */
static int32 n_hyp_tried;	/* Total #hyps tried */
static int32 n_hyp_reject;	/* #hyps rejected by pruning */
static int32 n_hyp_insert;
static int32 insert_depth;

/* Parameters to prune n-best alternatives search */
#define MAX_PATHS	500	/* Max allowed active paths at any time */
#define MAX_HYP_TRIES	10000

/* Back trace from path to root */
static search_hyp_t *latpath_seg_back_trace (latpath_t *path)
{
    search_hyp_t *head, *h;
    
    head = NULL;
    for (; path; path = path->parent) {
	h = (search_hyp_t *) listelem_alloc (sizeof(search_hyp_t));
	h->wid = path->node->wid;
	h->word = kb_get_word_str (h->wid);
	h->sf = uttproc_feat2rawfr(path->node->sf);
	h->ef = uttproc_feat2rawfr(path->node->fef);	/* Approximately */

	h->next = head;
	head = h;
    }

    return head;
}

/*
 * For each node in any path between from and end of utt, find the best score
 * from "from".sf to end of utt.  (NOTE: Uses bigram probs; this is an estimate
 * of the best score from "from".
 */
static int32 best_rem_score (latnode_t *from)
{
    latlink_t *link;
    int32 bestscore, score;
    
    if (from->info.rem_score <= 0)
	return (from->info.rem_score);
    
    /* Best score from "from" to end of utt not known; compute from successors */
    bestscore = WORST_SCORE;
    for (link = from->links; link; link = link->next) {
	score = best_rem_score (link->to);
	score += link->link_scr;
	score += LWMUL(lm_bg_score (from->wid, link->to->wid), lw_factor);
	if (score > bestscore)
	    bestscore = score;
    }
    from->info.rem_score = bestscore;

    return (bestscore);
}

/*
 * Insert newpath in sorted (by path score) list of paths.  But if newpath is
 * too far down the list, drop it.
 * total_score = path score (newpath) + rem_score to end of utt.
 */
static void path_insert (latpath_t *newpath, int32 total_score)
{
    latpath_t *prev, *p;
    int32 i;

    prev = NULL;
    for (i = 0, p = path_list; (i < MAX_PATHS) && p; p = p->next, i++) {
	if ((p->score + p->node->info.rem_score) < total_score)
	    break;
	prev = p;
    }

    /* newpath should be inserted between prev and p */
    if (i < MAX_PATHS) {
	/* Insert new partial hyp */
	newpath->next = p;
	if (! prev)
	    path_list = newpath;
	else
	    prev->next = newpath;
	if (! p)
	    path_tail = newpath;

	n_path++;
	n_hyp_insert++;
	insert_depth += i;
    } else {
	/* newpath score too low; reject it and also prune paths beyond MAX_PATHS */
	path_tail = prev;
	prev->next = NULL;
	n_path = MAX_PATHS;
	listelem_free (newpath, sizeof(latpath_t));

	n_hyp_reject++;
	for (; p; p = newpath) {
	    newpath = p->next;
	    listelem_free (p, sizeof(latpath_t));
	    n_hyp_reject++;
	}
    }
}

/* Find all possible extensions to given partial path */
static void  path_extend (latpath_t *path)
{
    latlink_t *link;
    latpath_t *newpath;
    int32 total_score, tail_score;
    
    /* Consider all successors of path->node */
    for (link = path->node->links; link; link = link->next) {
	/* Skip successor if no path from it reaches the final node */
	if (link->to->info.rem_score <= WORST_SCORE)
	    continue;
	
	/* Create path extension and compute exact score for this extension */
	newpath = (latpath_t *) listelem_alloc (sizeof(latpath_t));
	newpath->node = link->to;
	newpath->parent = path;
	newpath->score = path->score + link->link_scr;
	if (path->parent)
	    newpath->score += LWMUL(lm_tg_score (path->parent->node->wid,
						 path->node->wid,
						 newpath->node->wid), lw_factor);
	else
	    newpath->score += LWMUL(lm_bg_score (path->node->wid, newpath->node->wid),
				    lw_factor);
	
	/* Insert new partial path hypothesis into sorted path_list */
	n_hyp_tried++;
	total_score = newpath->score + newpath->node->info.rem_score;

	/* First see if hyp would be worse than the worst */
	if (n_path >= MAX_PATHS) {
	    tail_score = path_tail->score + path_tail->node->info.rem_score;
	    if (total_score < tail_score) {
		listelem_free (newpath, sizeof (latpath_t));
		n_hyp_reject++;
		continue;
	    }
	}

	path_insert (newpath, total_score);
    }
}

void search_hyp_free (search_hyp_t *h)
{
    search_hyp_t *tmp;
    
    while (h) {
	tmp = h->next;
	listelem_free (h, sizeof(search_hyp_t));
	h = tmp;
    }
}

static int32 hyp_diff (search_hyp_t *hyp1, search_hyp_t *hyp2)
{
    search_hyp_t *h1, *h2;
    
    for (h1 = hyp1, h2 = hyp2;
	 h1 && h2 && (h1->wid == h2->wid);
	 h1 = h1->next, h2 = h2->next);
    return (h1 || h2) ? 1 : 0;
}

/*
 * Get n best alternatives in lattice for the given frame range [sf..ef].
 * w1 and w2 provide left context; w1 may be -1.
 * NOTE: Clobbers any previously returned hypotheses!!
 * Return values: no. of alternative hypotheses returned in alt; -1 if error.
 */
int32 search_get_alt (int32 n,			/* In: No. of alternatives to look for */
		      int32 sf, int32 ef,	/* In: Start/End frame */
		      int32 w1, int32 w2,	/* In: context words */
		      search_hyp_t ***alt_out)	/* Out: array of alternatives */
{
    latnode_t *node;
    latpath_t *path, *top;
    int32 i, j, scr, n_alt;
    static search_hyp_t **alt = NULL;
    static int32 max_alt_hyp = 0;
    char remLMName[128];
    extern char *get_current_lmname();

    if (n <= 0)
	return -1;
    
    /* Save currently active LM and switch to the one associated with the lattice */
    strcpy(remLMName, get_current_lmname ());
    uttproc_set_lm(altLMName);
    
    for (i = 0; i < max_alt_hyp; i++) {
	search_hyp_free (alt[i]);
	alt[i] = NULL;
    }

    if (n > max_alt_hyp) {
	if (alt)
	    free (alt);
	max_alt_hyp = (n+255) & 0xffffff00;
	alt = (search_hyp_t **) CM_calloc (max_alt_hyp, sizeof(search_hyp_t *));
    }
    
    *alt_out = NULL;

    /* If no permanent lattice available to be searched, return */
    if (lattice.latnode_list == NULL) {
	/* MessageBoxX("permanent lattice trouble in searchlat.c"); */
	E_ERROR("NULL lattice\n");
	uttproc_set_lm(remLMName);
	return 0;
    }

    /* Initialize rem_score to default values */
    for (node = lattice.latnode_list; node; node = node->next) {
	if (node == lattice.final_node)
	    node->info.rem_score = 0;
	else if (! node->links)
	    node->info.rem_score = WORST_SCORE;
	else
	    node->info.rem_score = 1;	/* +ve => unknown value */
    }

    /* Create initial partial hypotheses list consisting of nodes starting at sf */
    n_path = 0;
    n_hyp_reject = 0;
    n_hyp_tried = 0;
    n_hyp_insert = 0;
    insert_depth = 0;
    paths_done = NULL;	    

    path_list = path_tail = NULL;
    for (node = lattice.latnode_list; node; node = node->next) {
	if (node->sf == sf) {
	    best_rem_score (node);
	    
	    path = (latpath_t *) listelem_alloc (sizeof(latpath_t));
	    path->node = node;
	    path->parent = NULL;
	    scr = (w1 < 0) ? lm_bg_score(w2, node->wid) : lm_tg_score(w1, w2, node->wid);
	    path->score = scr;
	    
	    path_insert (path, scr + node->info.rem_score);
	}
    }
    
    /* Find n hypotheses */
    for (n_alt = 0; path_list && (n_alt < n) && (n_hyp_tried < MAX_HYP_TRIES); ) {
	/* Pop the top (best) partial hypothesis */
	top = path_list;
	path_list = path_list->next;
	if (top == path_tail)
	    path_tail = NULL;
	n_path--;
	
	if ((top->node->sf >= ef) || ((top->node == lattice.final_node) &&
				      (ef > lattice.final_node->sf))) {
	    /* Complete hypothesis; generate output, but omit last (bracketing) node */
	    alt[n_alt] = latpath_seg_back_trace (top->parent);
	    /* alt[n_alt] = latpath_seg_back_trace (top); */

	    /* Accept hyp only if non-empty */
	    if (alt[n_alt]) {
		/* Check if hypothesis already output */
		for (j = 0; (j < n_alt) && (hyp_diff (alt[j], alt[n_alt])); j++);
		if (j >= n_alt)	/* New, distinct alternative hypothesis */
		    n_alt++;
		else {
		    search_hyp_free (alt[n_alt]);
		    alt[n_alt] = NULL;
		}
	    }
#if 0
	    /* Print path here for debugging */
#endif
	} else {
	    if (top->node->fef < ef)
		path_extend (top);
	}
	
	/*
	 * Add top to paths already processed; cannot be freed because other paths
	 * point to it.
	 */
	top->next = paths_done;
	paths_done = top;
    }

#if defined(SEARCH_PROFILE)
    E_INFO("#HYP = %d, #INSERT = %d, #REJECT = %d, Avg insert depth = %.2f\n",
	   n_hyp_tried, n_hyp_insert, n_hyp_reject,
	   insert_depth/((double)n_hyp_insert+1.0));
#endif
    
    /* Clean up */
    while (path_list) {
	top = path_list;
	path_list = path_list->next;
	listelem_free (top, sizeof(latpath_t));
    }
    while (paths_done) {
	top = paths_done;
	paths_done = paths_done->next;
	listelem_free (top, sizeof(latpath_t));
    }

    *alt_out = alt;

    /* Restore original LM name */
    uttproc_set_lm(remLMName);

    return n_alt;
}

void search_save_lattice ( void )
{
    if (lattice.latnode_list)
	destroy_lattice (lattice.latnode_list);
    
    lattice.latnode_list = latnode_list;
    lattice.start_node = start_node;
    lattice.final_node = final_node;
    latnode_list = NULL;
}

void search_delete_saved_lattice ( void )
{
    if (lattice.latnode_list) {
	destroy_lattice (lattice.latnode_list);
	lattice.latnode_list = NULL;
	lattice.start_node = NULL;
	lattice.final_node = NULL;
    }
}

char	*searchGetAltLMName(void)
{
    return altLMName;
}

int32	searchSetAltUttid(char *uttid)
{
    int32		i;

    for (i = 0; i < MAX_LAT_QUEUE; i++) {
	if (strcmp(latQueue[i].uttid, uttid) == 0) {
	    lattice.latnode_list = latQueue[i].lattice.latnode_list;
	    lattice.start_node = latQueue[i].lattice.start_node;
	    lattice.final_node = latQueue[i].lattice.final_node;
	    uttprocSetcomp2rawfr(latQueue[i].n_featfr, latQueue[i].comp2rawfr);

	    /* We also need to add code to set the language model for this uttid. */
	    strcpy(altLMName, latQueue[i].lmName);
	    
	    /* return success because we found utt for this uttid. */
	    return 0;
	}
    }
	
    /* return failure because we could not find alts for the desired utt. */
    return 1;
}

void	searchSaveLatQueue(char *uttid, char *lmName)
{
    int32	i, found, addIndex, minIndex = 0; /* FIXME: good default? */
    int16	*ptr;

    if (! latQueueInit) {
	for (i = 0; i < MAX_LAT_QUEUE; i++) {
	    /* latQueue[i].lattice = NULL; */	/* Why is this commented out? */
	    latQueue[i].uttid[0] = '\0';
	    latQueue[i].lmName[0] = '\0';
	    latQueue[i].addIndex = -1;
	    latQueue[i].comp2rawfr = NULL;
	    latQueue[i].n_featfr = 0;
	}
	latQueueInit = 1;
    }
    
    /* Find the next empty queue slot or the oldest uttid if all slots are full */
    for (addIndex = 100000, found = -1, i = 0; i < MAX_LAT_QUEUE; i++) {
	if (latQueue[i].addIndex == -1) {
	    found = i;
	    break;
	} else {
	    if (latQueue[i].addIndex < addIndex) {
		addIndex = latQueue[i].addIndex;
		minIndex = i;
	    }
	}
    }
    
    /* No empty slots, so free a slot so we can reuse it. */
    if (found == -1) {
	destroy_lattice(latQueue[minIndex].lattice.latnode_list);
	latQueue[minIndex].lattice.latnode_list = NULL;
	latQueue[minIndex].lattice.start_node = NULL;
	latQueue[minIndex].lattice.final_node = NULL;
	found = minIndex;
    }
    
    /* Finally place a copy of the relevant data in the queue */
    latQueue[found].lattice.latnode_list = latnode_list;
    latQueue[found].lattice.start_node = start_node;
    latQueue[found].lattice.final_node = final_node;
    strcpy(latQueue[found].lmName, lmName);
    strcpy(latQueue[found].uttid, uttid);

    if (latQueue[found].comp2rawfr != NULL)
	free(latQueue[found].comp2rawfr);

    latQueue[found].n_featfr = uttprocGetcomp2rawfr(&ptr);
    latQueue[found].comp2rawfr = (int32 *) calloc(latQueue[found].n_featfr,
						  sizeof(int32));

    for (i = 0; i < latQueue[found].n_featfr; i++)
	latQueue[found].comp2rawfr[i] = ptr[i];

    latQueue[found].addIndex = latQueueAddIndex++;
    
    latnode_list = NULL;
}
