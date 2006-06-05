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
/* NEWMSD.H - Model Static Dynamic
 *------------------------------------------------------------*
 * 
 * DESCRIPTION
 *	The msd structure conatins the hmm static data as well as
 * viterbi search dynamic data.
 *------------------------------------------------------------*
 * $Log: msd.h,v $
 * Revision 1.1.1.1  2006/05/23 18:45:03  dhuggins
 * re-importation
 *
 * Revision 1.8  2004/07/16 00:57:12  egouvea
 * Added Ravi's implementation of FSG support.
 *
 * Revision 1.2  2004/03/02 15:33:39  rkm
 * FSG bug fixes
 *
 * Revision 1.2  2004/03/02 04:10:14  rkm
 * FSG bugfix: need to get senscores every utt
 *
 * Revision 1.1.1.1  2003/12/03 20:05:04  rkm
 * Initial CVS repository
 *
 * Revision 1.7  2001/12/11 00:24:48  lenzo
 * Acknowledgement in License.
 *
 * Revision 1.6  2001/12/07 17:30:02  lenzo
 * Clean up and remove extra lines.
 *
 * Revision 1.5  2001/12/07 05:09:30  lenzo
 * License.xsxc
 *
 * Revision 1.4  2001/12/07 04:27:35  lenzo
 * License cleanup.  Remove conditions on the names.  Rationale: These
 * conditions don't belong in the license itself, but in other fora that
 * offer protection for recognizeable names such as "Carnegie Mellon
 * University" and "Sphinx."  These changes also reduce interoperability
 * issues with other licenses such as the Mozilla Public License and the
 * GPL.  This update changes the top-level license files and removes the
 * old license conditions from each of the files that contained it.
 * All files in this collection fall under the copyright of the top-level
 * LICENSE file.
 *
 * Revision 1.3  2001/11/20 21:22:37  lenzo
 * Win32 re-compatibility fixes.
 *
 * Revision 1.2  2000/12/05 01:45:12  lenzo
 * Restructuring, hear rationalization, warning removal, ANSIfy
 *
 * Revision 1.1.1.1  2000/01/28 22:08:59  lenzo
 * Initial import of sphinx2
 *
 *
 *  8-Sep-92  Fil Alleva (faa) at Carnegie-Mellon University
 *	dec'l of CHAN.model[] changed from int16 to int32.
 *
 * Spring 89, Fil Alleva (faa) at Carnegie Mellon
 *	Created
 *------------------------------------------------------------*
 * BUGS
 *	We assume that there are *always* 6 nodes per model and
 * never more then 3 transition out of a node. In addition to that
 * we also make use of the fact that every model has the same base
 * topology. To that end we use memory occupied by unused transitions
 * for model bookkeeping variables.
 */

#ifndef _NEWMSD_H_
#define _NEWMSD_H_

#include "s2types.h"
#include "search_const.h"
#include "basic_types.h"

#define TRANS_CNT	((HMM_LAST_STATE*3)-1)	/* No. of trans. in Sphinx HMM */
#define NODE_CNT	(HMM_LAST_STATE+1)	/* No. of nodes in Sphinx HMM */

typedef struct StaticModel
{
    int32 dist[TRANS_CNT];	/* Which distribution */
    int32 tp[TRANS_CNT];	/* Transition probabilities */
    int32 arcProb[TRANS_CNT];	/* Acoustic Prob + Transition Prob */
} SMD;				/* Static Model Data */

typedef struct StaticModelRead
{
    int32 stateCnt;		/* Number of states */
    int32 topo[2];		/* Model topology */
} SMD_R;			/* Static Model Data Read data struct*/

/*
 * HMM instance (channel) data type.  Not the first HMM for words, which multiplex HMMs
 * based on different left contexts.  This structure is used both in the dynamic
 * HMM tree structure and in the per-word last-phone right context fanout.
 */
typedef struct CHAN_S
{
    struct CHAN_S *next;	/* first descendant of this channel; or, in the
				   case of the last phone of a word, the next
				   alternative right context channel */
    struct CHAN_S *alt;		/* sibling; i.e., next descendant of parent HMM */
    int32    score[NODE_CNT];	/* dynamic score (prob) per state */
    FRAME_ID path[NODE_CNT];	/* dynamic history pointer per state */
    SSEQ_ID  sseqid;		/* parent static model (actually senone-seq id) */
    int32    ciphone;
    int32    bestscore;		/* among score[]; for pruning */
    union {
	WORD_ID penult_phn_wid;	/* list of words whose last phone follows this one;
				   this field indicates the first of the list; the
				   rest must be built up in a separate array.  Used
				   only within HMM tree.  -1 if none */
	int32 rc_id;		/* right-context id for last phone of words */
    } info;
    FRAME_ID active;		/* last frame in which this node was active */
} CHAN_T;

/*
 * HMM instance (channel) data type for the first phone (root) of each dynamic HMM tree
 * structure.  Each state may have a different parent static HMM.  Most fields are
 * similar to those in CHAN_T.
 */
typedef struct ROOT_CHAN_S
{
    CHAN_T *next;		/* first descendant of this channel */
    int32    score[NODE_CNT];
    FRAME_ID path[NODE_CNT];
    SSEQ_ID  sseqid[NODE_CNT-1];/* parent static HMM per state */
    int32    bestscore;
    WORD_ID  penult_phn_wid;
    WORD_ID  this_phn_wid;	/* list of words consisting of this single phone;
				   actually the first of the list, like penult_phn_wid;
				   -1 if none */
    int32    diphone;		/* first diphone of this node; all words rooted at this
				   node begin with this diphone */
    int32    ciphone;		/* first ciphone of this node; all words rooted at this
				   node begin with this ciphone */
    int32    mpx;		/* TRUE iff this node uses left cross-word modelling */
    FRAME_ID active;
} ROOT_CHAN_T;

void chan_dump (CHAN_T *chan, int32 frame, FILE *fp);	/* Debug dump */


#endif /* _NEWMSD_H_ */
