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
 * lextree.h -- 
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
 * Revision 1.11  2006/02/23 15:08:24  arthchan2003
 * Merged from the branch SPHINX3_5_2_RCI_IRII_BRANCH:
 *
 * 1, Fixed memory leaks.
 * 2, Add logic for full triphone expansion.  At this point, the
 * propagation of scores in word end is still incorrect. So composite
 * triphone should still be used by default.
 * 3, Removed lextree_copies_hmm_propagate.
 *
 * Revision 1.10.4.6  2005/11/17 06:28:50  arthchan2003
 * Changed the code to used compressed triphones. Not yet correct at this point
 *
 * Revision 1.10.4.5  2005/10/17 04:53:44  arthchan2003
 * Shrub the trees so that the run-time memory could be controlled.
 *
 * Revision 1.10.4.4  2005/10/07 19:34:31  arthchan2003
 * In full cross-word triphones expansion, the previous implementation has several flaws, e.g, 1, it didn't consider the phone beam on cross word triphones. 2, Also, when the cross word triphone phone is used, children of the last phones will be regarded as cross word triphone. So, the last phone should not be evaluated at all.  Last implementation has not safe-guaded that. 3, The rescoring for language model is not done correctly.  What we still need to do: a, test the algorithm in more databases. b,  implement some speed up schemes.
 *
 * Revision 1.10.4.2  2005/09/25 19:23:55  arthchan2003
 * 1, Added arguments for turning on/off LTS rules. 2, Added arguments for turning on/off composite triphones. 3, Moved dict2pid deallocation back to dict2pid. 4, Tidying up the clean up code.
 *
 * Revision 1.10.4.1  2005/06/27 05:37:05  arthchan2003
 * Incorporated several fixes to the search. 1, If a tree is empty, it will be removed and put back to the pool of tree, so number of trees will not be always increasing.  2, In the previous search, the answer is always "STOP P I T G S B U R G H </s>"and filler words never occurred in the search.  The reason is very simple, fillers was not properly propagated in the search at all <**exculamation**>  This version fixed this problem.  The current search will give <sil> P I T T S B U R G H </sil> </s> to me.  This I think it looks much better now.
 *
 * Revision 1.10  2005/06/21 23:32:58  arthchan2003
 * Log. Introduced lextree_init and filler_init to wrap up lextree_build
 * process. Split the hmm propagation to propagation for leaves and
 * non-leaves node.  This allows an easier time for turning off the
 * rescoring stage. However, the implementation is not clever enough. One
 * should split the array to leave array and non-leave array.
 *
 * Revision 1.7  2005/06/16 04:59:10  archan
 * Sphinx3 to s3.generic, a gentle-refactored version of Dave's change in senone scale.
 *
 * Revision 1.6  2005/06/13 04:02:59  archan
 * Fixed most doxygen-style documentation under libs3decoder.
 *
 * Revision 1.5  2005/04/25 23:53:35  archan
 * 1, Some minor modification of vithist_t, vithist_rescore can now support optional LM rescoring, vithist also has its own reporting routine. A new argument -lmrescore is also added in decode and livepretend.  This can switch on and off the rescoring procedure. 2, I am reaching the final difficulty of mode 5 implementation.  That is, to implement an algorithm which dynamically decide which tree copies should be entered.  However, stuffs like score propagation in the leave nodes and non-leaves nodes are already done. 3, As briefly mentioned in 2, implementation of rescoring , which used to happened at leave nodes are now separated. The current implementation is not the most clever one. Wish I have time to change it before check-in to the canonical.
 *
 * Revision 1.4  2005/04/25 19:22:47  archan
 * Refactor out the code of rescoring from lexical tree. Potentially we want to turn off the rescoring if we need.
 *
 * Revision 1.3  2005/03/30 01:22:47  archan
 * Fixed mistakes in last updates. Add
 *
 * 
 * 29-Feb-2000	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Modified some functions to be able to deal with HMMs with any number
 * 		of states.
 * 
 * 07-Jul-1999	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Added lextree_node_t.ci and lextree_ci_active().
 * 
 * 30-Apr-1999	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Started.
 */


#ifndef _S3_LEXTREE_H_
#define _S3_LEXTREE_H_

#include <stdio.h>

#include <sphinxbase/bitvec.h>
#include <sphinxbase/ngram_model.h>
#include <sphinxbase/glist.h>

#include "s3types.h"
#include "hmm.h"
#include "tmat.h"
#include "vithist.h"
#include "s3dict.h"
#include "bin_mdef.h"
#include "fillpen.h"

#ifdef __cplusplus
extern "C" {
#endif
#if 0
} /* Fool Emacs into not indenting things. */
#endif

#define LEXTREE_OPERATION_SUCCESS 1
#define LEXTREE_OPERATION_FAILURE 0

#define LEXTREE_TYPE_FILLER -1
#define LEXTREE_TYPE_UNIGRAM 0
#define LEXTREE_TYPE_BIGRAM 1
#define LEXTREE_TYPE_TRIGRAM 2

#if 0 /*Number reserved but not used */
#define LEXTREE_TYPE_QUADGRAM 3
#define LEXTREE_TYPE_QUINGRAM 4
#endif


/** \file lextree.h
 * \brief Data structure of lexical tree. 
 *
 * A lextree can be built for a specific history (e.g., for all bigram successors of a given
 * word or trigram successors of a word-pair in a given LM).  The history provides a set of left
 * context CIphones (if the final history word has multiple pronunciations; and there is always
 * <sil>).
 * A lextree is usually a set of trees, one for each distinct root model for the given set of
 * words.  Furthermore, the root node of each tree can itself actually be a SET of nodes,
 * required by the different left contexts.  If there is no history (i.e., the unigram lextree),
 * any CIphone is a potential left-context.  But this can explode the number of root nodes.
 * So, the root nodes of the unigram lextree use COMPOSITE models (see dict2pid.h), merging
 * different contexts into one.  Similarly, the right context (at the leaves of any lextree) is
 * always unknown.  So, all leaf nodes also use composite models.
 * Lextrees are formed by sharing as much of the HMM models as possible (based on senone-seq ID),
 * before having to diverge.  But the leaf nodes are always distinct for each word.
 * Finally, each node has a (language model) probability, given its history.  It is the max. of
 * the LM probability of all the words reachable from that node.  (Strictly speaking, it should
 * be their sum instead of max, but practically it makes little difference.)
 *
 *
 *
 * ARCHAN : Two weaknesses of the code, 1, when full triphone is
 * expanded, the code loop for all CI index. This is because dict2pid,
 * unlike ctxt_table, doesn't provide a list of triphones 
 * 2, for all active node, the code has iterate two times. Rather than once, because of separation 

 * of prop_non_leaves and prop_leaves. 
 */


/*
 * \struct lextree_node_t
 * One node in a lextree.
 */
typedef struct {
    hmm_t hmm;		/**< HMM states */

    glist_t children;	/**< Its data.ptr are children (lextree_node_t *)

                        If non-leaf node, this is the list of
                        successor nodes.  

                        If leaf node, if we are in normal mode and
                        this list will not be empty. 

                        This is the list of all possible triphone.
                        If it is not allocated. The code will
                        allocate it. 

                        */

    int32 wid;		/**< <em>Dictionary</em> word-ID if a leaf node; BAD_S3WID otherwise */
    int32 prob;		/**< LM probability of this node (of all words leading from this node) */
    int32 ssid;		/**< Senone-sequence ID (or composite state-seq ID if composite) */
    s3cipid_t rc;        /**< The (compressed) right context for this node. Preferably compressed.
                          */
    s3cipid_t ci;	/**< CIphone id for this node */
    int8 composite;	/**< Whether it is a composite model (merging many left/right contexts) */
} lextree_node_t;

/* Access macros; not meant for arbitrary use */
#define lextree_node_wid(n)		((n)->wid)
#define lextree_node_prob(n)		((n)->prob)
#define lextree_node_ssid(n)		((n)->ssid)
#define lextree_node_rc(n)		((n)->rc)
#define lextree_node_composite(n)	((n)->composite)
#define lextree_node_frame(n)		((n)->frame)


/*
 * \struct lextree_lcroot_t
 * Root nodes of the lextree valid for a given left context CIphone.
 */
typedef struct {
    s3cipid_t lc;	/* Left context CIphone */
    glist_t root;	/* Its data.ptr are the root nodes (lextree_node_t *) of interest; subset
			   of the entire lextree roots */
} lextree_lcroot_t;

/*
 * \struct lextree_t 
 * Entire lextree.
 */
typedef struct {
    int32 type;		/**< For use by other modules; NOT maintained here.  For example:
                           N-gram type; 0: unigram lextree, 1: 2g, 2: 3g lextree... */
    glist_t root;	/**< The entire set of root nodes (lextree_node_t) for this lextree */
    lextree_lcroot_t *lcroot;	/**< Lists of subsets of root nodes; a list for each left context;
                                   NULL if n_lc == 0 (i.e., no specific left context) */
    int32 n_lc;		/**< No. of separate left contexts being maintained, if any */
    int32 n_node;	/**< Total No. of nodes in this lextree which is allocated in the initialization time */
    int32 n_alloc_node;   /**< Total No. of nodes in this lextree which is allocated dynamically */
    int32 n_alloc_blk_sz;   /**< Block size of each allocation */

    bin_mdef_t *mdef;     /**< Model definition (not owned by this structure) */
    s3dict_t *dict;     /**< Dictionary (not owned by this structure) */
    dict2pid_t *dict2pid;     /**< Dictionary mapping (not owned by this structure) */
    fillpen_t *fp;      /**< Filler penalties (not owned by this structure) */
    ngram_model_t *lm;  /**< Language model */
    tmat_t *tmat;       /**< Transition matrices */

    hmm_context_t *ctx;     /**< HMM context for non-composite triphones. */
    hmm_context_t *comctx; /**< HMM context for composite triphones. */

    lextree_node_t **active;		/**< Nodes active in any frame */
    lextree_node_t **next_active;	/**< Like active, but temporary space for constructing the
					   active list for the next frame using the current */
    int32 n_active;		/**< No. of nodes active in current frame */
    int32 n_next_active;	/**< No. of nodes active in current frame */
    
    int32 best;		/**< Best HMM state score in current frame (for pruning) */
    int32 wbest;	/**< Best wordexit HMM state score in current frame (for pruning) */

    char prev_word[100];      /**< This is used in WST. The previous word for a tree */
} lextree_t;

/* Access macros; not meant for arbitrary usage */
#define lextree_type(l)			((l)->type)
#define lextree_root(l)			((l)->root)
#define lextree_lcroot(l)		((l)->lcroot)
#define lextree_n_lc(l)			((l)->n_lc)
#define lextree_n_node(l)		((l)->n_node)
#define lextree_n_alloc_node(l)		((l)->n_alloc_node)
#define lextree_active(l)		((l)->active)
#define lextree_next_active(l)		((l)->next_active)
#define lextree_n_active(l)		((l)->n_active)
#define lextree_n_next_active(l)	((l)->n_next_active)


/** Initialize a lexical tree, also factor language model score through out the tree. Currently only
 * unigram look-ahead is supported. 
 */
lextree_t* lextree_init(
    bin_mdef_t *mdef,
    tmat_t *tmat,
    s3dict_t *dict,
    dict2pid_t *dict2pid,
    fillpen_t *fp,
    ngram_model_t* lm,
    const char *lmname,     /**< In: LM name */
    int32 istreeUgProb, /**< In: Decide whether LM factoring is used or not */
    int32 bReport,     /**< In: Whether to report the progress so far. */
    int32 type        /**< In: Type of the lexical tree, 0: unigram lextree, 1: 2g, 2: 3g lextree*/
    );

/** Initialize a filler tree.
 */
lextree_t* fillertree_init(bin_mdef_t *mdef, tmat_t *tmat, s3dict_t *dict,
                           dict2pid_t *dict2pid, fillpen_t *fillpen);


/** Report the lextree data structure. 
 */
void lextree_report(
    lextree_t *ltree /**< In: Report a lexical tree*/
    );

/** \struct wordprob_t
    \brief Generic structure that could be used at any n-gram level 
*/
typedef struct {
    s3wid_t wid;	/**< NOTE: <em>dictionary</em> wid; may be BAD_S3WID if not available */
    int32 prob;         /**< The probability */
} wordprob_t;

/* Free a lextree that was created by lextree_init */
void lextree_free (lextree_t *lextree);

/**
 * Reset the entire lextree (to the inactive state).  I.e., mark each HMM node as inactive,
 * (with lextree_node_t.frame = -1), and the active list size to 0.
 */
void lextree_utt_end (lextree_t *l);


/**
 * Enter root nodes of lextree for given left-context, with given incoming score/history.
 */
void lextree_enter (lextree_t *lextree,	/**< In/Out: Lextree being entered */
		    s3cipid_t lc,	/**< In: Left-context if any (can be BAD_S3CIPID) */
		    int32 frame,	/**< In: Frame from which being activated (for the next) */
		    int32 inscore,	/**< In: Incoming score */
		    int32 inhist,	/**< In: Incoming history */
		    int32 thresh	/**< In: Pruning threshold; incoming scores below this
					   threshold will not enter successfully */
    );

/**
 * Swap the active and next_active lists of the given lextree.  (Usually done at the end of
 * each frame: from the current active list, we've built the next_active list for the next
 * frame, and finally need to make the latter the current active list.)
 */
void lextree_active_swap (lextree_t *lextree /**< The lexical tree*/
    );


/**
 * Marks the active ssid and composite ssids in the given lextree.  Caller must allocate ssid[]
 * and comssid[].  Caller also responsible for clearing them before calling this function.
 */
void lextree_ssid_active (lextree_t *lextree,	/**< In: lextree->active is scanned */
			  bitvec_t *ssid,	/**< In/Out: ssid[s] is set to non-0 if senone
						   sequence ID s is active */
			  bitvec_t *comssid	/**< In/Out: comssid[s] is set to non-0 if
						   composite senone sequence ID s is active */
    );

/**
 * For each active lextree node, mark the corresponding CI phone as active.
 */
void lextree_ci_active (lextree_t *lextree,	/**< In: Lextree being traversed */
			bitvec_t *ci_active	/**< In/Out: Active/inactive flag for ciphones */
    );

/**
 * Evaluate the active HMMs in the given lextree, using the current frame senone scores.
 * Return value: The best HMM state score as a result.
 */
int32 lextree_hmm_eval (lextree_t *lextree,	/**< In/Out: Lextree with HMMs to be evaluated */

                        int16 *senscr,  /**< In: Primary senone scores */
                        int16 *comsen,  /**< In: Composite senone scores */
                        int32 f,	/**< In: Frame in which being invoked */
			FILE *fp	/**< In: If not-NULL, dump HMM state (for debugging) */
    );

/*
 * ARCHAN: Starting from Sphinx 3.6, lextree_hmm_propagate is separated to 
 * lextree_hmm_propagate_non_leaves and lextree_hmm_propagate_leaves.  This allows 
 * the rescoring routine to be more easily switched off
 */

/**
 *
 * Propagate the non-leaves nodes of HMMs in the given lextree
 * through to the start of the next frame.  Called after HMM state
 * scores have been updated.  Marks those with "good" scores as
 * active for the next frame.
 * 
 * There is a difference between this part of the code in the
 * composite mode or not in the composite mode.  When in composite
 * mode, the code will propagate the leaf node as if it is just a
 * simple node.  In that case, composite senone-sequence index will
 * be used.
 *
 * (Warning! Grandpa is the true daddy! ) In the case when full
 * triphone is expanded, the code will propagate to all possible
 * contexts expansion which is stored in the "children" list of the
 * leaf nodes.  Notice, conceptually the list of all possible
 * contexts should be the children of the parent of the leave nodes
 * rather than the children of the leave node. However, physically,
 * the expansion was part of the children list.  So, there will be 
 * subtle difference in propagation. 
 *
 * @return SRCH_FAILURE if it failed, SRCH_SUCCESS if it succeeded.
 */
int32 lextree_hmm_propagate_non_leaves (lextree_t *lextree,	/**< In/Out: Propagate scores across HMMs in
								   this lextree */
					int32 cf,		/**< In: Current frame index. */
					int32 th,		/**< In: General (HMM survival) pruning thresh */
					int32 pth,		/**< In: Phone transition pruning threshold */
					int32 wth		/**< In: Word exit pruning threshold */
    ); 


/**
 * Propagate the leaves nodes of HMMs in the given lextree through to
 * the start of the next frame.  Called after HMM state scores have
 * been updated.  Propagates HMM exit scores through to successor HMM
 * entry states. It should be called right after
 * lextree_hmm_propagate_leaves. 
 *
 * @return SRCH_FAILURE if it failed, SRCH_SUCCESS if it succeeded.
 */

int32 lextree_hmm_propagate_leaves (lextree_t *lextree,	/**< In/Out: Propagate scores across HMMs in
							   this lextree */
				    vithist_t *vh,	/**< In/Out: Viterbi history structure to be
							   updated with word exits */
				    int32 cf,            /**< In: Current frame index */
				    int32 wth		/**< In: Word exit pruning threshold */
    ); 


/**
 * In order to use histogram pruning, get a histogram of the bestscore of each active HMM in
 * the given lextree.  For a given HMM, its bin is determined by:
 * 	(bestscr - hmm->bestscore) / bw.
 * Invoked right after all active HMMs are evaluated.
 */
void lextree_hmm_histbin (lextree_t *lextree,	/**< In: Its active HMM bestscores are used */
			  int32 bestscr,	/**< In: Overall bestscore in current frame */
			  int32 *bin,		/**< In/Out: The histogram bins; caller allocates
						   this array */
			  int32 nbin,		/**< In: Size of bin[] */
			  int32 bw		/**< In: Bin width; i.e., score range per bin */
    );

/** For debugging, dump the whole lexical tree*/
void lextree_dump (lextree_t *lextree,  /**< In: A lexical tree*/
		   FILE *fp,           /**< A file pointer */
		   int32 fmt           /**< fmt=1, Ravi's format, fmt=2, Dot's format*/ 
    );

/** Utility function that count the number of links */
int32 num_lextree_links(lextree_t *ltree /**< In: A lexical tree */
    );
#if 0
{ /* Stop indent from complaining */
#endif
#ifdef __cplusplus
}
#endif

#endif
