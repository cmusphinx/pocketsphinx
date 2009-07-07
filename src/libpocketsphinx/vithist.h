/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
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
 * vithist.h -- Viterbi history
 * 
 * **********************************************
 * CMU ARPA Speech Project
 *
 * Copyright (c) 1999 Carnegie Mellon University.
 * ALL RIGHTS RESERVED.
 * **********************************************
 * 
 * HISTORY
 * $Log$
 * Revision 1.1  2006/04/05  20:27:30  dhdfu
 * A Great Reorganzation of header files and executables
 * 
 * Revision 1.12  2006/02/23 16:56:13  arthchan2003
 * Merged from the branch SPHINX3_5_2_RCI_IRII_BRANCH
 * 1, Split latticehist_t from flat_fwd.c to  here.
 * 2, Introduced vithist_entry_cp.  This is much better than the direct
 * copy we have been using. (Which could cause memory problem easily)
 *
 * Revision 1.11.4.9  2006/01/16 18:11:39  arthchan2003
 * 1, Important Bug fixes, a local pointer is used when realloc is needed.  This causes invalid writing of the memory, 2, Acoustic scores of the last segment in IBM lattice generation couldn't be found in the past.  Now, this could be handled by the optional acoustic scores in the node of lattice.  Application code is not yet checked-in
 *
 * Revision 1.11.4.8  2005/11/17 06:46:02  arthchan2003
 * 3 changes. 1, Code was added for full triphone implementation, not yet working. 2, Senone scale is removed from vithist table. This was a bug introduced during some fixes in CALO.
 *
 * Revision 1.11.4.7  2005/10/17 04:58:30  arthchan2003
 * vithist.c is the true source of memory leaks in the past for full cwtp expansion.  There are two changes made to avoid this happen, 1, instead of using ve->rc_info as the indicator whether something should be done, used a flag bFullExpand to control it. 2, avoid doing direct C-struct copy (like *ve = *tve), it becomes the reason of why memory are leaked and why the code goes wrong.
 *
 * Revision 1.11.4.6  2005/10/07 20:05:05  arthchan2003
 * When rescoring in full triphone expansion, the code should use the score for the word end with corret right context.
 *
 * Revision 1.11.4.5  2005/09/26 06:37:33  arthchan2003
 * Before anyone get hurt, quickly change back to using SINGLE_RC_HISTORY.
 *
 * Revision 1.11.4.4  2005/09/25 19:23:55  arthchan2003
 * 1, Added arguments for turning on/off LTS rules. 2, Added arguments for turning on/off composite triphones. 3, Moved dict2pid deallocation back to dict2pid. 4, Tidying up the clean up code.
 *
 * Revision 1.11.4.3  2005/09/11 03:00:15  arthchan2003
 * All lattice-related functions are not incorporated into vithist. So-called "lattice" is essentially the predecessor of vithist_t and fsg_history_t.  Later when vithist_t support by right context score and history.  It should replace both of them.
 *
 * Revision 1.11.4.2  2005/07/26 02:20:39  arthchan2003
 * merged hyp_t with srch_hyp_t.
 *
 * Revision 1.11.4.1  2005/07/04 07:25:22  arthchan2003
 * Added vithist_entry_display and vh_lmstate_display in vithist.
 *
 * Revision 1.11  2005/06/22 02:47:35  arthchan2003
 * 1, Added reporting flag for vithist_init. 2, Added a flag to allow using words other than silence to be the last word for backtracing. 3, Fixed doxygen documentation. 4, Add  keyword.
 *
 * Revision 1.10  2005/06/16 04:59:10  archan
 * Sphinx3 to s3.generic, a gentle-refactored version of Dave's change in senone scale.
 *
 * Revision 1.9  2005/06/13 04:02:59  archan
 * Fixed most doxygen-style documentation under libs3decoder.
 *
 * Revision 1.8  2005/05/26 00:46:59  archan
 * Added functionalities that such that <sil> will not be inserted at the end of the utterance.
 *
 * Revision 1.7  2005/04/25 23:53:35  archan
 * 1, Some minor modification of vithist_t, vithist_rescore can now support optional LM rescoring, vithist also has its own reporting routine. A new argument -lmrescore is also added in decode and livepretend.  This can switch on and off the rescoring procedure. 2, I am reaching the final difficulty of mode 5 implementation.  That is, to implement an algorithm which dynamically decide which tree copies should be entered.  However, stuffs like score propagation in the leave nodes and non-leaves nodes are already done. 3, As briefly mentioned in 2, implementation of rescoring , which used to happened at leave nodes are now separated. The current implementation is not the most clever one. Wish I have time to change it before check-in to the canonical.
 *
 * Revision 1.6  2005/04/21 23:50:26  archan
 * Some more refactoring on the how reporting of structures inside kbcore_t is done, it is now 50% nice. Also added class-based LM test case into test-decode.sh.in.  At this moment, everything in search mode 5 is already done.  It is time to test the idea whether the search can really be used.
 *
 * Revision 1.5  2005/04/20 03:46:30  archan
 * factor dag header writer into vithist.[ch], do the corresponding change for lm_t
 *
 * Revision 1.4  2005/03/30 01:22:47  archan
 * Fixed mistakes in last updates. Add
 *
 * 
 * 20.Apr.2001  RAH (rhoughton@mediasite.com, ricky.houghton@cs.cmu.edu)
 *              Added vithist_free() to free allocated memory
 * 
 * 30-Sep-1999	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 *		Added vithist_entry_t.ascr.
 * 
 * 13-Aug-1999	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Added maxwpf handling.
 * 
 * 24-May-1999	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Started.
 */


#ifndef _S3_VITHIST_H_
#define _S3_VITHIST_H_

#include <stdio.h>

#include <s3types.h>
#include <cmd_ln.h>
#include <logmath.h>
#include <glist.h>
#include "kbcore.h"
#include "search.h"
#include "dict.h"
#include "lm.h"
#include "fillpen.h"

#include "dag.h"  
#include "ctxt_table.h"

/** \file vithist.h 

\brief Viterbi history structures.  Mainly vithist_t, also its
slightly older brother latticehist_t. They are respectively used
by decode (mode 4 and 5) and decode_anytopo (mode 3).  The curent
arrangement is temporary. 
*/

#ifdef __cplusplus
extern "C" {
#endif
#if 0
} /* Fool Emacs into not indenting things. */
#endif

/**
 * LM state.  Depending on type of LM (word-ngram, class-ngram, FSG, etc.), the contents
 * of LM state will vary.  Accommodate them with a union.  For now, only trigram LM in it.
 * (Not completely thought out; some of this might have to change later.)
 */
typedef union vh_lmstate_u {
    struct {
        s3lmwid32_t lwid[2];	/**< 2-word history; [0] is most recent */
    } lm3g;
} vh_lmstate_t;


typedef struct backpointer_s {
    int32 score;
    int32 pred;
} backpointer_t;

/**
 * Viterbi history entry.
 */
typedef struct {
    backpointer_t path;         /**< Predecessor word and best path score including it */
    vh_lmstate_t lmstate;	/**< LM state */
    s3wid_t wid;		/**< Dictionary word ID; exact word that just exited */
    s3frmid_t sf, ef;		/**< Start and end frames for this entry */
    int32 ascr;			/**< Acoustic score for this node */
    int32 lscr;			/**< LM score for this node, given its Viterbi history */
    int16 type;			/**< >=0: regular n-gram word; <0: filler word entry */
    int16 valid;		/**< Whether it should be a valid history for LM rescoring */
    backpointer_t *rc;          /**< Individual score/history for different right contexts */
    int32 n_rc;                 /**< Number of right contexts */
} vithist_entry_t;

/** Return the word ID of an entry */
#define vithist_entry_wid(ve)	((ve)->wid)

/** Return the starting frame of an entry */
#define vithist_entry_sf(ve)	((ve)->sf)

/** Return the ending frame of an entry */
#define vithist_entry_ef(ve)	((ve)->ef)

/** Return the acoustic score of an entry */
#define vithist_entry_ascr(ve)	((ve)->ascr)

/** Return the language score of an entry */
#define vithist_entry_lscr(ve)	((ve)->lscr)

/** Return the total score of an entry */
#define vithist_entry_score(ve)	((ve)->path.score)
#define vithist_entry_pred(ve)	((ve)->path.pred)
#define vithist_entry_valid(ve)	((ve)->valid)


/**
 * \struct vh_lms2vh_t
 * In each frame, there are several word exits.  There can be several exit instances of the
 * same word, corresponding to different LM histories.  Generally, each exit is associated
 * with an LM state.  We only need to retain the best entry for each LM state.  The following
 * structure is for this purpose.
 * For all exits in the current frame, all n-word histories (assuming an N-gram LM) ending in
 * a given word are arranged in a tree, with the most recent history word at the root.  The
 * leaves of the tree point to the (current best) vithist entry with that history in the
 * current frame.
 */
typedef struct {		/**< Mapping from LM state to vithist entry */
    int32 state;		/**< (Part of) the state information */
    int32 vhid;			/**< Associated vithist ID (only for leaf nodes) */
    vithist_entry_t *ve;	/**< Entry ptr corresponding to vhid (only for leaf nodes) */
    glist_t children;		/**< Children of this node in the LM state tree; data.ptr of
                                   type (vh_lms2vh_t *) */
} vh_lms2vh_t;


/**
 * \struct vithist_t 
 * Memory management of Viterbi history entries done in blocks.  Initially, one block of
 * VITHIST_BLKSIZE entries allocated.  If exhausted, another block allocated, and so on.
 * So we can have several discontiguous blocks allocated.  Entries are identified by a
 * global, running sequence no.
 */
typedef struct {
    vithist_entry_t **entry;	/**< entry[i][j]= j-th entry in the i-th block allocated */
    int32 *frame_start;		/**< For each frame, the first vithist ID in that frame; (the
                                   last is just before the first of the next frame) */
    int32 n_entry;		/**< Total #entries used (generates global seq no. or ID) */
    int32 n_frm;		/**< No. of frames processed so far in this utterance */
    int32 n_ci;                 /**< No. of CI phones */
    int32 bghist;		/**< If TRUE (bigram-mode) only one entry/word/frame; otherwise
				   multiple entries allowed, one per distinct LM state */
    
    int32 wbeam;		/**< Pruning beamwidth */
    
    int32 *bestscore;		/**< Best word exit score in each frame */
    int32 *bestvh;		/**< Vithist entry ID with the best exit score in each frame */
    
    vh_lms2vh_t **lms2vh_root;	/**< lms2vh[w]= Root of LM states ending in w in current frame */
    glist_t lwidlist;		/**< List of LM word IDs with entries in lms2vh_root */
} vithist_t;


#define VITHIST_BLKSIZE		16384	/* (1 << 14) */
#define VITHIST_MAXBLKS		256
#define VITHIST_ID2BLK(i)	((i) >> 14)
#define VITHIST_ID2BLKOFFSET(i)	((i) & 0x00003fff)	/* 14 LSB */

/** Access macros; not meant for arbitrary use */

/** Return a pointer to the entry with the given ID. */
#define vithist_id2entry(vh,id) ((vh)->entry[VITHIST_ID2BLK(id)] + VITHIST_ID2BLKOFFSET(id))

/** Return the number of entry in the Viterbi history */
#define vithist_n_entry(vh)		((vh)->n_entry)

/** Return the best score of the Viterbi history */
#define vithist_bestscore(vh)		((vh)->bestscore)

/** Return the best viterbi history entry ID of the Viterbi history */
#define vithist_bestvh(vh)		((vh)->bestvh)

/** Return lms2vh */
#define vithist_lms2vh_root(vh,w)	((vh)->lms2vh_root[w])

/** Return the language word ID list */
#define vithist_lwidlist(vh)		((vh)->lwidlist)

/** Return the first entry for the frame f */
#define vithist_first_entry(vh,f)	((vh)->frame_start[f])

/** Return the last entry for the frame f */
#define vithist_last_entry(vh,f)	((vh)->frame_start[f+1] - 1)


/** 
 * One-time intialization: Allocate and return an initially empty
 * vithist module 
 * @return An initialized vithist_t
*/

vithist_t *vithist_init (kbcore_t *kbc,  /**< Core search data structure */
			 int32 wbeam,    /**< Word exit beam width */
			 int32 bghist,   /**< If only bigram history is used */
			 int32 report    /**< Whether to report the progress  */
    );


/**
 * Invoked at the beginning of each utterance; vithist initialized with a root <s> entry.
 * @return Vithist ID of the root <s> entry.
 */
int32 vithist_utt_begin (vithist_t *vh, /**< In: a Viterbi history data structure*/
			 kbcore_t *kbc  /**< In: a KBcore */
    );


/**
 * Invoked at the end of each utterance; append a final </s> entry that results in the best
 * path score (i.e., LM including LM transition to </s>).
 * Return the ID of the appended entry if successful, -ve if error (empty utterance).
 */
int32 vithist_utt_end (vithist_t *vh, /**< In: a Viterbi history data structure*/
		       kbcore_t *kbc  /**< In: a KBcore */
    );


/**
 * Invoked at the end of each block of a live decode.
 * Returns viterbi histories of partial decodes
 */
int32 vithist_partialutt_end (vithist_t *vh, /**< In: a Viterbi history data structure*/
			      kbcore_t *kbc  /**< In: a KBcore */
    );

/* Invoked at the end of each utterance to clear up and deallocate space */
void vithist_utt_reset (vithist_t *vh  /**< In: a Viterbi history data structure*/
    );


/**
 * Viterbi backtrace.  Return value: List of hyp_t pointer entries for the individual word
 * segments.  Caller responsible for freeing the list.
 */
glist_t vithist_backtrace (vithist_t *vh,       /**< In: a Viterbi history data structure*/
			   int32 id,		/**< ID from which to begin backtrace */
			   dict_t *dict         /**< a dictionary for look up the ci phone of a word*/
    );


/**
 * Add an entry to the Viterbi history table without rescoring.  Any
 * entry having the same LM state will be replaced with the one given.
 */
void vithist_enter(vithist_t * vh,              /**< The history table */
                   kbcore_t * kbc,              /**< a KB core */
                   vithist_entry_t * tve,       /**< an input vithist element */
                   int32 comp_rc                /**< a compressed rc. If it is the actual rc, it won't work. FIXME: WHAT DOES THIS MEAN?!!?!? */
    );

/**
 * Like vithist_enter, but LM-rescore this word exit wrt all histories that ended at the
 * same time as the given, tentative pred.  Create a new vithist entry for each predecessor
 * (but, of course, only the best for each distinct LM state will be retained; see above).
 * 
 * ARCHAN: Precisely speaking, it is a full trigram rescoring. 
 */
void vithist_rescore (vithist_t *vh,    /**< In: a Viterbi history data structure*/
		      kbcore_t *kbc,    /**< In: a kb core. */
		      s3wid_t wid,      /**< In: a word ID */
		      int32 ef,		/**< In: End frame for this word instance */
		      int32 score,	/**< In: Does not include LM score for this entry */
		      int32 pred,	/**< In: Tentative predecessor */
		      int32 type,       /**< In: Type of lexical tree */
		      int32 rc          /**< In: The compressed rc. So if you use the actual rc, it doesn't work.  */
    );


/** Invoked at the end of each frame */
void vithist_frame_windup (vithist_t *vh,	/**< In/Out: Vithist module to be updated */
			   int32 frm,		/**< In: Frame in which being invoked */
			   FILE *fp,		/**< In: If not NULL, dump vithist entries
						   this frame to the file (for debugging) */
			   kbcore_t *kbc	/**< In: Used only for dumping to fp, for
						   debugging */
    );

/**
 * Mark up to maxwpf best words, and variants within beam of best frame score as valid,
 * and the remaining as invalid.
 */
void vithist_prune (vithist_t *vh,      /**< In: a Viterbi history data structure*/
		    dict_t *dict,	/**< In: Dictionary, for distinguishing filler words */
		    int32 frm,		/**< In: Frame in which being invoked */
		    int32 maxwpf,	/**< In: Max unique words per frame to be kept valid */
		    int32 maxhist,	/**< In: Max histories to maintain per frame */
		    int32 beam	/**< In: Entry score must be >= frame bestscore+beam */
    );

/**
 * Dump the Viterbi history data to the given file (for debugging/diagnostics).
 */
void vithist_dump (vithist_t *vh,     /**< In: a Viterbi history data structure */
		   int32 frm,		/**< In: If >= 0, print only entries made in this frame,
					   otherwise print all entries */
		   kbcore_t *kbc,       /**< In: a KBcore */
		   FILE *fp             /**< Out: File to be written */
    );

/**
 * Build a word graph (DAG) from Viterbi history.
 */
dag_t *vithist_dag_build(vithist_t * vh, glist_t hyp, dict_t * dict, int32 endid,
                         cmd_ln_t *config, logmath_t *logmath);

/**
 * Write a word graph (DAG) built from Viterbi history (temporary function)
 */
int32 vithist_dag_write(vithist_t *vithist,
                        const char *filename,
                        dag_t * dag,
                        lm_t * lm,
                        dict_t * dict);

/** 
 * Free a Viterbi history data structure 
 */

void vithist_free(vithist_t *vh         /**< In: a Viterbi history data structure */
    );


/**
 * Report a Viterbi history architecture 
 */
  
void vithist_report(vithist_t *vh       /**< In: a Viterbi history data structure */
    );

/**
 * Display the lmstate of an entry. 
 */

void vh_lmstate_display(vh_lmstate_t *vhl, /**< In: An lmstate data structure */
			dict_t *dict /**< In: If specified, the word string of lm IDs would also be translated */
    );

/**
 * Display the vithist_entry structure. 
 */
void vithist_entry_display(vithist_entry_t *ve, /**< In: An entry of vithist */
			   dict_t* dict  /**< In: If specified, the word string of lm IDs would also be translated */
    );


/**
 * \struct lattice_t 
 * Word lattice for recording decoded hypotheses.
 * 
 * lattice[i] = entry for a word ending at a particular frame.  There can be at most one
 * entry for a word in a given frame.
 * NOTE: lattice array allocated statically.  Need a more graceful way to grow without
 * such an arbitrary internal limit.
 */
typedef struct lattice_s {
    s3wid_t   wid;	/**< Decoded word */
    s3frmid_t frm;	/**< End frame for this entry */
    s3latid_t history;	/**< Index of predecessor lattice_t entry */

    int32 ascr;         /**< Acoustic score for this node */
    int32 lscr;         /**< Language score for this node */

    int32     score;	/**< Best path score upto the end of this entry */
    int32    *rcscore;	/**< Individual path scores for different right context ciphones */
    dagnode_t *dagnode;	/**< DAG node representing this entry */
} lattice_t;


#define LAT_ALLOC_INCR		32768

#define LATID2SF(hist,l)	(IS_S3LATID(hist->lattice[l].history) ? \
			 hist->lattice[hist->lattice[l].history].frm + 1 : 0)

/** 
 * \struct latticehist_t 
 *
 * Encapsulation of all memory management of the lattice structure.
 * The name of lattice_t and latticehist_t is slighly confusing.
 * lattice_t is actually just one entry. One should also
 * differentiate the term "lattice" as a general graph-like
 * structure which either represent the search graph or as a
 * constraint of a search graph
 */

/* FIXME, 1, Either replace all latticehist_t by vithist_t or 
   2, there is no need to always put dict_t, ctxt_table_t , lm_t and fillpen_t as arguments 
*/

typedef struct {

    lattice_t *lattice;  /**< An array of lattice entries. frm_lat_start[f] represnet the first entry for the frame f */
    s3latid_t *frm_latstart;	/**< frm_latstart[f] = first lattice entry in frame f */

    int32 lat_alloc;     /**< Number of allocated entries */
    int32 n_lat_entry;   /**< Number of lattice entries */
    int32 n_cand;        /**< If number of candidate is larger than
                            zero, The then decoder is workin in
                            "lattice-mode", which means using the
                            candidate in the lattice to constrain the
                            search? */
    int32 n_frms_alloc;  /**< Number of frames allocated in frm_latstart */
    int32 n_frm;         /**< Number of frames searched */

} latticehist_t;


#define latticehist_n_cand(hist)		((hist)->n_cand)
#define latticehist_lat_alloc(hist)		((hist)->lat_alloc)
#define latticehist_n_lat_entry(hist)		((hist)->n_lat_entry)

  
/**
 * Initialization of lattice history table 
 */
  
latticehist_t *latticehist_init(int32 init_alloc_size, /**<Initial allocation size */
				int32 num_frames       /**<Number of frames in represented in the lattice*/
    );

/**
   Free lattice history table 
*/
void latticehist_free(latticehist_t *lat /**< The latticie history table */
    );

/*
 * Reset a lattice history table 
 */
   
void latticehist_reset(latticehist_t *lat /**< The lattice history table*/
    );

/**
 * Dump the lattice history table
 */
void latticehist_dump (latticehist_t *lathist,  /**< A table of lattice entries */
		       FILE *fp,                /**< File pointer where one would want to dump the lattice */
		       dict_t *dict,            /**< The dictionary*/
		       ctxt_table_t *ct,        /**< Context table */
		       int32 dumpRC             /**< Whether we whould dump all the scores and histories for right context */
    );

/**
 * Enter a entry into lattice 
 */
void lattice_entry (latticehist_t *lathist, /**< A table of lattice entries */
		    s3wid_t w,              /**< Word ID to enter. */
		    int32 f,                /**< current frame number */
		    int32 score,            /**< The score to enter, usually it is the score of the final state of hmm */
		    s3latid_t history,      /**< The last lattice entry to enter, usually the entry of the final state of hmm is used. */
		    int32 rc,                /**< Right context of the HMM*/ 
		    ctxt_table_t *ct,       /**< A context table */
		    dict_t *dict            /**< A dictionary */
    );


void two_word_history (latticehist_t *lathist, 
		       s3latid_t l, s3wid_t *w0, s3wid_t *w1, dict_t *dict);

/**
 * Find path score for lattice entry l for the given right context word.
 * If context word is BAD_S3WID it's a wild card; return the best path score.
 */

int32 lat_pscr_rc (latticehist_t *lathist, /**< A table of lattice entries */
		   s3latid_t l,            /**< lattice ID */
		   s3wid_t w_rc,           /**< The right context word */
		   ctxt_table_t *ct,       /**< Context table */
		   dict_t *dict            /**< The dictionary */
    );

/**
 * Find path history for lattice entry l for the given right context word.
 * If context word is BAD_S3WID it's a wild card; return the phone history.
 */
s3latid_t lat_pscr_rc_history (latticehist_t *lathist, /**< A table of lattice entries */
			       s3latid_t l,  /**< lattice ID */
			       s3wid_t w_rc,  /**< The right context word */
			       ctxt_table_t *ct,  /**< Context table */
			       dict_t *dict     /** The dictionary */
    );

int32 lat_seg_lscr (latticehist_t *lathist, 
		    s3latid_t l, 
		    lm_t *lm, 
		    dict_t *dict, 
		    ctxt_table_t *ct, 
		    fillpen_t *fillpen, 
		    int32 isCand);


/**
 * Find LM score for transition into lattice entry l.
 */

void lat_seg_ascr_lscr (latticehist_t *lathist, /**< A table of lattice entries */
			s3latid_t l,  /**< lattice ID */
			s3wid_t w_rc, /**< The right context word */
			int32 *ascr,  /**< Out: Acoustic score */
			int32 *lscr,  /**< Out: language score */
			lm_t *lm,     /**< LM */
			dict_t *dict,  /**< Dictionary */
			ctxt_table_t *ct,  /** Context table */
			fillpen_t *fillpen /**< filler penalty */
    );


/**
   Get the final entry of the lattice
*/
s3latid_t lat_final_entry ( latticehist_t* lathist, /**< A table of lattice entries */
			    dict_t* dict, /** The dictioanry */
			    int32 curfrm, /**< The current  frame */
			    char* uttid /** Utterance ID */
    );
			      
/**
 * Backtrace the lattice and get back a search hypothesis. 
 */
srch_hyp_t *lattice_backtrace (latticehist_t *lathist, /**< A table of lattice entries */
			       s3latid_t l,   /**< The lattice ID */
			       s3wid_t w_rc,   /**< The word on the right */
			       srch_hyp_t **hyp, /**< Output: final hypothesis */ 
			       lm_t *lm,       /**< LM */
			       dict_t *dict,   /**< Dictionary */
			       ctxt_table_t *ct,  /**< Context table */
			       fillpen_t *fillpen /**< filler penalty struct */
    );

/**
 * Build a DAG from the lattice: each unique <word-id,start-frame> is a node, i.e. with
 * a single start time but it can represent several end times.  Links are created
 * whenever nodes are adjacent in time.
 * dagnodes_list = linear list of DAG nodes allocated, ordered such that nodes earlier
 * in the list can follow nodes later in the list, but not vice versa:  Let two DAG
 * nodes d1 and d2 have start times sf1 and sf2, and end time ranges [fef1..lef1] and
 * [fef2..lef2] respectively.  If d1 appears later than d2 in dag.list, then
 * fef2 >= fef1, because d2 showed up later in the word lattice.  If there is a DAG
 * edge from d1 to d2, then sf1 > fef2.  But fef2 >= fef1, so sf1 > fef1.  Reductio ad
 * absurdum.
 */
dag_t * latticehist_dag_build(latticehist_t * vh, glist_t hyp, dict_t * dict,
                              lm_t *lm, ctxt_table_t *ctxt, fillpen_t *fpen,
                              int32 endid, cmd_ln_t *config, logmath_t *logmath);

/** 
 * Write a dag from latticehist_t
 */
int32 latticehist_dag_write (latticehist_t *lathist,  /**< A table off lattice entries */
                             const char *filename,
			     dag_t *dag,
			     lm_t *lm, 
			     dict_t *dict, 
			     ctxt_table_t *ct, 
			     fillpen_t *fillpen
    );

#if 0
{ /* Stop indent from complaining */
#endif
#ifdef __cplusplus
}
#endif

#endif
