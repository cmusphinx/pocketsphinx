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
 * time_align.c
 *
 * Description:
 *   These routines will time align a given word string to a given acoustic input stream.
 *   The steps involved in this process are:
 *   	- The given word string is converted into a triphone model DAG.
 *	- The viterbi algorithm is applied to find the best state sequence through the DAG
 *	- State, phone and word level backpointers are maintained in order to retrieve
 *	  the best state, phone and word level segmentations of the acoustic input stream.
 *
 *   Optional silence is allowed between words.
 *
 * To Do:
 *   - allow for arbitrary left and right context begin and end phones.  This would allow
 *     these routines to do a detailed phonetic analysis in an error region for instance.
 *   - allow optional filler word sequence between words (e.g. ++COUGH++ ++SNIFF++ SIL ++SNIFF++).
 *
 * Revision History
 * 
 * $Log: time_align.c,v $
 * Revision 1.1.1.1  2006/05/23 18:45:01  dhuggins
 * re-importation
 *
 * Revision 1.17  2005/07/21 20:05:52  egouvea
 * Fixed handling of compound word in the time aligner. Before the fix,
 * the code expected alternate pronunciations to be indicated by a single
 * digit. Changed it so the alternate marker could be anything
 * (e.g. A(MANUAL) instead of A(2)).
 *
 * Revision 1.16  2004/12/10 16:48:57  rkm
 * Added continuous density acoustic model handling
 *
 * 
 * 22-Nov-2004	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Modified to use senscr module for senone score evaluation.
 * 
 * Revision 1.15  2004/11/13 00:38:43  egouvea
 * Replaced most printf with E_INFO (or E_WARN or...). Changed the output
 * of the time_align code so it's consistent with the other decoder modes
 * (allphone, normal decoding etc). Added the file utt id to the
 * time_align output.
 *
 * Revision 1.14  2004/07/16 00:57:11  egouvea
 * Added Ravi's implementation of FSG support.
 *
 * Revision 1.2  2004/05/27 14:22:57  rkm
 * FSG cross-word triphones completed (but for single-phone words)
 *
 * Revision 1.13  2001/12/11 00:24:48  lenzo
 * Acknowledgement in License.
 *
 * Revision 1.12  2001/12/07 17:30:02  lenzo
 * Clean up and remove extra lines.
 *
 * Revision 1.11  2001/12/07 05:09:30  lenzo
 * License.xsxc
 *
 * Revision 1.10  2001/12/07 04:27:35  lenzo
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
 * Revision 1.9  2001/12/07 00:51:49  lenzo
 * Quiet warnings.
 *
 * Revision 1.8  2001/10/23 22:20:30  lenzo
 * Change error logging and reporting to the E_* macros that call common
 * functions.  This will obsolete logmsg.[ch] and they will be removed
 * or changed in future versions.
 *
 * Revision 1.7  2001/07/02 16:47:12  lenzo
 * Fixed triphone lookup fallback case.
 *
 * Revision 1.6  2001/02/13 19:51:38  lenzo
 * *** empty log message ***
 *
 * Revision 1.5  2001/01/25 19:36:29  lenzo
 * Fixing some memory leaks
 *
 * Revision 1.4  2000/12/12 23:01:42  lenzo
 * Rationalizing libs and names some more.  Split a/d and fe libs out.
 *
 * Revision 1.3  2000/12/05 01:45:12  lenzo
 * Restructuring, hear rationalization, warning removal, ANSIfy
 *
 * Revision 1.2  2000/03/29 14:30:28  awb
 * *** empty log message ***
 *
 * Revision 1.1.1.1  2000/01/28 22:08:57  lenzo
 * Initial import of sphinx2
 *
 *
 * 
 * 02-Jan-96	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Added time_align_word flag to determine whether word segmentation is
 * 		output.  Implemented printing of word and phone segmentations.
 * 
 * 02-Jan-96	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Added time_align_phone and time_align_state flags for determining at run
 * 		time whether phone-level and state-level backtrace are printed.
 * 
 * Revision 1.4  1995/01/27  18:04:18  eht
 * Fixed a print statement bug
 *
 * Revision 1.3  1994/10/13  11:33:12  eht
 * Fixed handling of single phone words.
 *
 * Revision 1.2  1994/09/26  15:58:12  eht
 * Have time_align control allocation/deallocation of buffers.
 * Output revision control ID at initialization time
 *
 * Revision 1.1  1994/09/26  13:24:45  eht
 * Initial revision
 *
 * Public Interface:
 *
 *   time_align_init()
 *     Initializes the system.  Should be called once before any of the following
 *     calls are made.
 *     
 *   time_align_set_input()
 *	Provide the speech input for time alignment.  A call to this must precede
 *	any of the following calls.
 *
 *   time_align_word_sequence(char *left_word, char *word_seq, char *right_word)
 *	Performs the forced recognition against the input.  A call to this routine
 *	must precede the following calls.
 *
 *   char *time_align_best_word_string()
 *	Returns the best word string associated with the best state sequence found
 *	by time_align_word_sequence().  Included in this string are filler
 *	words (sil, silb, sile, noise words).
 *
 *	BEWARE:
 *		The caller must NOT free the returned string.
 *
 *   time_align_seg_output(unsigned short **seg, int *seg_cnt)
 *     Returns the state time alignments in the form of a sequence of shorts
 *     where:
 *	        high bit set indicates the first frame of a phone
 *
 *		   x = ci_phone_id * 5 + state_id,  where state_id is either
 *			0, 1, 2, 3, 4 and ci_phone_id is the context independent
 *			from the phone file given to the system.
 *
 *	These data are used by the senone decision tree builder.
 *
 *	BEWARE:
 *		The caller must NOT free the returned seg array.
 *
 *   SEGMENT_T *time_align_get_segmentation(int kind, int *seg_cnt)
 */

#define SHOW_NOTHING		(0x0000)
#define SHOW_EVERYTHING		(0xffff)
#define SHOW_INVOKATION		(0x0001) /* print function call invokation info */
#define SHOW_MODEL_EVAL		(0x0002) /* print model evaluation trace */
#define SHOW_SUMMARY_INFO	(0x0004) /* print search summary info */
#define SHOW_ACTIVE		(0x0008) /* print the active model state before/after evaluation
					    scores */
#define SHOW_BP			(0x0010) /* show any new backpointer info for each frame */
#define SHOW_FORCED_MODEL	(0x0020) /* show the topology of the model to be used for
					    forced recognition */
#define SHOW_BEST_WORD_PATH	(0x0040) /* print the best scoring word alignment */
#define SHOW_PHONE_GRAPH	(0x0080) /* print the phone graph to be searched */
#define SHOW_PRUNING		(0x0100) /* print the models pruned per frame */
#define SHOW_MODEL_DAG		(0x0200) /* print the model dag used */
#define SHOW_NODE_EXPANSION	(0x0400) /* print ci -> triphone expansion information */
#define SHOW_SYS_INFO		(0x0800) /* print memory usage/ system performance figures */
#define SHOW_BEST_PHONE_PATH	(0x1000) /* print the best scoring phone alignment */
#define SHOW_BEST_STATE_PATH	(0x2000) /* print the best scoring state alignment */
#define SHOW_BEST_PATHS		(SHOW_BEST_STATE_PATH|SHOW_BEST_WORD_PATH|SHOW_BEST_PHONE_PATH)

#if !defined(SHOW)

#define SHOW SHOW_BEST_PATHS

#endif

/* UNIX/C stuff */
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

/* CMU Speech stuff */
#include "s2types.h"
#include "CM_macros.h"
#include "basic_types.h"
#include "strfuncs.h"
#include "list.h"
#include "hash.h"
#include "search_const.h"
#include "msd.h"
#include "dict.h"
#include "lmclass.h"
#include "lm_3g.h"
#include "lm.h"
#include "kb.h"
#include "phone.h"
#include "log.h"
#include "hmm_tied_r.h"
#include "scvq.h"
#include "senscr.h"
#include "s2params.h"
#include "fbs.h"
#include "senscr.h"
#include "search.h"
#include "err.h"

/* This module stuff */
#include "time_align.h"

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

int save_labs(SEGMENT_T *segs,
	      int num_entries,
	      const char *dirname,
	      const char *filename,
	      const char *extname,
	      const char *labtype);

static mfcc_t *cep_f = NULL;
static mfcc_t *dcep_f = NULL;
static mfcc_t *dcep_80ms_f = NULL;
static mfcc_t *pcep_f = NULL;
static mfcc_t *ddcep_f = NULL;
static int    frame_cnt = 0;

static dictT *WordDict;

static int *active_models[2];

static int *cur_active_models;
static int *boundary_active_models;
static int *pruned_active_models;

static int cur_active_cnt;
static int next_active_cnt;

static int cur_frame = 0;

#define WORD_BP_TABLE_SIZE_INCREMENT	(1 * 1000)
BACK_POINTER_T *word_bp_table = NULL;
static int word_bp_table_next_free;
static int word_bp_table_frame_start;
static int max_word_bp_table_size = WORD_BP_TABLE_SIZE_INCREMENT;

#define PHONE_BP_TABLE_SIZE_INCREMENT	(10 * 1000)
BACK_POINTER_T *phone_bp_table = NULL;
static int phone_bp_table_next_free;
static int phone_bp_table_frame_start;
static int max_phone_bp_table_size = PHONE_BP_TABLE_SIZE_INCREMENT;

#define STATE_BP_TABLE_SIZE_INCREMENT	(NODE_CNT * 10 * 1000)
BACK_POINTER_T *state_bp_table = NULL;
static int state_bp_table_next_free;
static int state_bp_table_frame_start;
static int max_state_bp_table_size = STATE_BP_TABLE_SIZE_INCREMENT;

static DYNMODEL_T *all_models = NULL;
static int all_model_cnt;

static SMD *Models;

static int32 *distScores;

static int32 totalDists;

static int32 beam_width;

static int32 n_word_segments = 0;
static int32 n_phone_segments = 0;
static int32 n_state_segments = 0;

static char *best_word_string = NULL;
static int   best_word_string_len = 0;

static int saved_final_model;

static int32 sil_word_id;
static int32 sil_phone_id;
static int32 silb_phone_id;
static int32 sile_phone_id;

static int32 start_word_id;
static int32 end_word_id;

static const char *lcl_utt_id = NULL;

static SEGMENT_T *wdseg = NULL;
static SEGMENT_T *phseg = NULL;

void
    time_align_set_utt(const char *id)
{
    lcl_utt_id = id;
}

void time_align_set_beam_width(double bw)
{
#if SHOW&SHOW_INVOKATION
    E_INFO("time_align_set_beam_width(%e) called\n", bw);
#endif
    beam_width = 8 * LOG(bw);
}

int constituent_cnt(char const *compound_word)
{
    char *uscore;
    int cnt;
    char const *rem_word;

    rem_word = compound_word;
    cnt = 1;
    uscore = strchr(rem_word, '_');
    while ((uscore = strchr(uscore+1, '_')))
	cnt++;

    ++cnt;

    return cnt;
}

char *head_word_of(int k)
{
    dict_entry_t *cmpnd_wrd = WordDict->dict_list[k];
    static char   head_word[1024];
    char         *uscore;

    strcpy(head_word, cmpnd_wrd->word);

    uscore = strchr(head_word, '_');

    *uscore = '\0';

    return head_word;
}

char *cvt_uscores_to_sp(char const *word)
{
    char *wrk = salloc(word);
    char *uscore;

    uscore = wrk;

    while ((uscore = strchr(uscore+1, '_'))) {
	*uscore = ' ';
    }

    return wrk;
}

int descending_order_by_len(const void *a, const void *b)
{
    const COMPOUND_WORD_T *a_wrd = a;
    const COMPOUND_WORD_T *b_wrd = b;

    /* sort into descending order */
    if (a_wrd->word_cnt < b_wrd->word_cnt) {
	return 1;	/* implies swap */
    }
    else if (a_wrd->word_cnt > b_wrd->word_cnt) {
	return -1;
    }
    else
	return 0;
}

COMPOUND_WORD_T *
mk_compound_word_list(int *out_cnt)
{
    int i, j;
    dict_entry_t	**dict_list = WordDict->dict_list;
    char		*word;
    char		*alt_marker;
    int			compound_word_cnt;
    int			*compound_word_id;
    COMPOUND_WORD_T	*compound_word_list;

    E_INFO("Scanning dictionary for compound words: ");

    for (i = 0, compound_word_cnt = 0; i < WordDict->dict_entry_count; i++) {
	word = dict_list[i]->word;
	if (strchr(word+1, '_') != NULL) {
	    /* there is at least one underscore in the word
	       beginning at the second character onward.  Assume this
	       is a compound word */
	    ++compound_word_cnt;
	}
    }
    E_INFO("%d compound words found\n", compound_word_cnt);

    compound_word_id = (int *)calloc(compound_word_cnt, sizeof(int));

    /* rescan the dict to find the word id's of the compound words */
    for (i = 0, j = 0; i < WordDict->dict_entry_count; i++) {
	word = dict_list[i]->word;
	if (strchr(word+1, '_') != NULL) {
	    /* there is at least one underscore in the word
	       beginning at the second character onward.  Assume this
	       is a compound word */
	    
	    alt_marker = strchr(word, '(');
	    
	    if (alt_marker == NULL) {
		compound_word_id[j++] = i;
		E_INFO("\tadding c. %s to list\n", word);
	    }
	    else {
	        if (alt_marker[strlen(alt_marker) - 1] == ')') {
		    E_INFO("skipping c. alt pron %s\n", word);
		}
		else {
		    E_WARN("unusual word format %s.  Word not added to compound list\n", word);
		}
	    }
	}
    }

    compound_word_cnt = j;

    compound_word_list = calloc(compound_word_cnt, sizeof(COMPOUND_WORD_T));
    for (i = 0; i < compound_word_cnt; i++) {
	compound_word_list[i].word_id   = compound_word_id[i];
	compound_word_list[i].word_str  = dict_list[compound_word_id[i]]->word;
	compound_word_list[i].match_str =
	    cvt_uscores_to_sp(compound_word_list[i].word_str);
	compound_word_list[i].word_cnt =
	    constituent_cnt(compound_word_list[i].word_str);
    }

    qsort(compound_word_list, compound_word_cnt, sizeof(COMPOUND_WORD_T),
	  descending_order_by_len);

    free(compound_word_id);
    
    *out_cnt = compound_word_cnt;

    return compound_word_list;
}

static COMPOUND_WORD_T *compound_word_list;
static int compound_word_cnt;

int
time_align_init(void)
{
    static char const *rcsid = "$Id: time_align.c,v 1.1.1.1 2006/05/23 18:45:01 dhuggins Exp $";

#if SHOW&SHOW_INVOKATION    
    E_INFO("time_align_init() called\n");
#endif

    E_INFO("rcsid == %s\n", rcsid);

    WordDict   = kb_get_word_dict();
    totalDists = kb_get_total_dists ();
    distScores = search_get_dist_scores();
    Models     = kb_get_models();

    sil_word_id   = kb_get_word_id("SIL");
    start_word_id = kb_get_word_id("<s>");
    end_word_id   = kb_get_word_id("</s>");

    sil_phone_id  = phone_to_id("SIL", FALSE);
    silb_phone_id = phone_to_id("SILb", FALSE);
    sile_phone_id = phone_to_id("SILe", FALSE);
    
    state_bp_table = (BACK_POINTER_T *)CM_calloc(max_state_bp_table_size, sizeof(BACK_POINTER_T));
    E_INFO("state_bp_table size %dK\n",
	   max_state_bp_table_size * sizeof(BACK_POINTER_T) / 1024);

    phone_bp_table = (BACK_POINTER_T *)CM_calloc(max_phone_bp_table_size, sizeof(BACK_POINTER_T));
    E_INFO("phone_bp_table size %dK\n",
	   max_phone_bp_table_size * sizeof(BACK_POINTER_T) / 1024);

    word_bp_table  = (BACK_POINTER_T *)CM_calloc(max_word_bp_table_size, sizeof(BACK_POINTER_T));
    E_INFO("word_bp_table size %dK\n",
	   max_word_bp_table_size * sizeof(BACK_POINTER_T) / 1024);

    compound_word_list = mk_compound_word_list(&compound_word_cnt);

    return 0;
}

/*
 * Since more that one word string will be evaluated against the input, set
 * the input first to allow the possibility of avoiding reprocessing the input
 * data.
 */
void
time_align_set_input(mfcc_t *c,
		     mfcc_t *d,
		     mfcc_t *d_80,
		     mfcc_t *p,
		     mfcc_t *dd,
		     int n_f)
{
    cep_f = c;
    dcep_f = d;
    dcep_80ms_f = d_80;
    pcep_f = p;
    ddcep_f = dd;

    frame_cnt = n_f;

#if SHOW&SHOW_INVOKATION
    E_INFO("time_align_set_input() called\n");
#endif
#if SHOW&SHOW_SUMMARY_INFO
    E_INFO("time_align_set_input() received %d of input\n", frame_cnt);
#endif
}

static int
begin_triphone(int32 phone_id,
	       int32 lc_phone_id,
	       int32 rc_phone_id)
{
    int32 out_phone_id;
    char phone_str[64];
    char const *phone_name;
    char const *lc_phone_name;
    char const *rc_phone_name;

    assert(phone_id != NO_PHONE);
    assert(lc_phone_id != NO_PHONE);
    assert(rc_phone_id != NO_PHONE);

    phone_name    = phone_from_id(phone_id);
    lc_phone_name = phone_from_id(lc_phone_id);
    rc_phone_name = phone_from_id(rc_phone_id);
    
    sprintf(phone_str, "%s(%s,%s)b", phone_name, lc_phone_name, rc_phone_name);
    out_phone_id = phone_to_id(phone_str, FALSE);
    if (out_phone_id != NO_PHONE) {
	return out_phone_id;
    }

    sprintf(phone_str, "%s(%s,%s)", phone_name, lc_phone_name, rc_phone_name);
    out_phone_id = phone_to_id(phone_str, FALSE);
    if (out_phone_id != NO_PHONE) {
	E_INFO("%s(%s,%s) approx'ed as %s\n",
	       phone_name, lc_phone_name, rc_phone_name,
	       phone_str);
	return out_phone_id;
    }

    sprintf(phone_str, "%s(SIL,%s)b", phone_name, rc_phone_name);
    out_phone_id = phone_to_id(phone_str, FALSE);
    if (out_phone_id != NO_PHONE) {
	E_INFO("%s(%s,%s) approx'ed as %s\n",
	       phone_name, lc_phone_name, rc_phone_name,
	       phone_str);
	return out_phone_id;
    }

    sprintf(phone_str, "%s(SIL,%s)", phone_name, rc_phone_name);
    out_phone_id = phone_to_id(phone_str, FALSE);
    if (out_phone_id != NO_PHONE) {
	E_INFO("%s(%s,%s) approx'ed as %s\n",
	       phone_name, lc_phone_name, rc_phone_name,
	       phone_str);
	return out_phone_id;
    }

    sprintf(phone_str, "%s", phone_name);
    out_phone_id = phone_to_id(phone_str, FALSE);
    if (out_phone_id != NO_PHONE) {
	E_INFO("%s(%s,%s) approx'ed as %s\n",
	       phone_name, lc_phone_name, rc_phone_name,
	       phone_str);
	return out_phone_id;
    }

    E_ERROR(" %s(%s,%s) returned id NO_PHONE\n", 
             phone_name, lc_phone_name, rc_phone_name);

    assert(FALSE); /* should never get here.  But if we do, there are some big problems */
    return 0;
}

static int32
single_phone_word_triphone(int32 phone_id,
			   int32 lc_phone_id,
			   int32 rc_phone_id)
{
    int32 out_phone_id;
    char phone_str[64];
    char const *phone_name;
    char const *lc_phone_name;
    char const *rc_phone_name;

    assert(phone_id != NO_PHONE);
    assert(lc_phone_id != NO_PHONE);
    assert(rc_phone_id != NO_PHONE);

    phone_name    = phone_from_id(phone_id);
    lc_phone_name = phone_from_id(lc_phone_id);
    rc_phone_name = phone_from_id(rc_phone_id);
    
    sprintf(phone_str, "%s(%s,%s)s", phone_name, lc_phone_name, rc_phone_name);
    out_phone_id = phone_to_id(phone_str, FALSE);
    if (out_phone_id != NO_PHONE)
	return out_phone_id;

    sprintf(phone_str, "%s(%s,%s)e", phone_name, lc_phone_name, rc_phone_name);
    out_phone_id = phone_to_id(phone_str, FALSE);
    if (out_phone_id != NO_PHONE)
	return out_phone_id;

    sprintf(phone_str, "%s(%s,%s)", phone_name, lc_phone_name, rc_phone_name);
    out_phone_id = phone_to_id(phone_str, FALSE);
    if (out_phone_id != NO_PHONE) {
	E_INFO("%s(%s,%s)s approxed as %s\n",
	       phone_name, lc_phone_name, rc_phone_name, phone_str);
	return out_phone_id;
    }

    sprintf(phone_str, "%s(%s,SIL)e", phone_name, lc_phone_name);
    out_phone_id = phone_to_id(phone_str, FALSE);
    if (out_phone_id != NO_PHONE) {
	E_INFO("%s(%s,%s)s approxed as %s\n",
	       phone_name, lc_phone_name, rc_phone_name,
	       phone_str);
	return out_phone_id;
    }

    sprintf(phone_str, "%s(%s,SIL)", phone_name, lc_phone_name);
    out_phone_id = phone_to_id(phone_str, FALSE);
    if (out_phone_id != NO_PHONE) {
	E_INFO("%s(%s,%s)s approxed as %s\n",
	       phone_name, lc_phone_name, rc_phone_name,
	       phone_str);
	return out_phone_id;
    }

    sprintf(phone_str, "%s", phone_name);
    out_phone_id = phone_to_id(phone_str, FALSE);
    if (out_phone_id != NO_PHONE) {
	E_INFO("%s(%s,%s)s approxed as %s\n",
	       phone_name, lc_phone_name, rc_phone_name,
	       phone_str);
	return out_phone_id;
    }

    E_ERROR("%s(%s,%s) returned id NO_PHONE\n", 
             phone_name, lc_phone_name, rc_phone_name);

    assert(FALSE); /* should never get here.  But if we do, there are some big problems */
    return 0;
}

static int32
end_triphone(int32 phone_id,
	     int32 lc_phone_id,
	     int32 rc_phone_id)
{
    int32 out_phone_id;
    char phone_str[64];
    char const *phone_name;
    char const *lc_phone_name;
    char const *rc_phone_name;

    assert(phone_id != NO_PHONE);
    assert(lc_phone_id != NO_PHONE);
    assert(rc_phone_id != NO_PHONE);

    phone_name    = phone_from_id(phone_id);
    lc_phone_name = phone_from_id(lc_phone_id);
    rc_phone_name = phone_from_id(rc_phone_id);
    
    sprintf(phone_str, "%s(%s,%s)e", phone_name, lc_phone_name, rc_phone_name);
    out_phone_id = phone_to_id(phone_str, FALSE);
    if (out_phone_id != NO_PHONE)
	return out_phone_id;

    sprintf(phone_str, "%s(%s,%s)", phone_name, lc_phone_name, rc_phone_name);
    out_phone_id = phone_to_id(phone_str, FALSE);
    if (out_phone_id != NO_PHONE) {
	E_INFO("%s(%s,%s)e approxed as %s\n",
	       phone_name, lc_phone_name, rc_phone_name,
	       phone_str);
	return out_phone_id;
    }

    sprintf(phone_str, "%s(%s,SIL)e", phone_name, lc_phone_name);
    out_phone_id = phone_to_id(phone_str, FALSE);
    if (out_phone_id != NO_PHONE) {
	E_INFO("%s(%s,%s)e approxed as %s\n",
	       phone_name, lc_phone_name, rc_phone_name,
	       phone_str);
	return out_phone_id;
    }

    sprintf(phone_str, "%s(%s,SIL)", phone_name, lc_phone_name);
    out_phone_id = phone_to_id(phone_str, FALSE);
    if (out_phone_id != NO_PHONE) {
	E_INFO("%s(%s,%s)e approxed as %s\n",
	       phone_name, lc_phone_name, rc_phone_name,
	       phone_str);
	return out_phone_id;
    }

    sprintf(phone_str, "%s", phone_name);
    out_phone_id = phone_to_id(phone_str, FALSE);
    if (out_phone_id != NO_PHONE) {
	E_INFO("%s(%s,%s)e approxed as %s\n",
	       phone_name, lc_phone_name, rc_phone_name,
	       phone_str);
	return out_phone_id;
    }

    E_ERROR("%s(%s,%s) returned id NO_PHONE\n", 
	    phone_name, lc_phone_name, rc_phone_name);

    assert(FALSE); /* should never get here.  But if we do, there are some big problems */
    return 0;
}

static int32
triphone(int32 phone_id,
	 int32 lc_phone_id,
	 int32 rc_phone_id)
{
    char phone_str[64];
    char const *phone_name;
    char const *lc_phone_name;
    char const *rc_phone_name;
    int32 out_phone_id;

    assert(phone_id != NO_PHONE);
    assert(lc_phone_id != NO_PHONE);
    assert(rc_phone_id != NO_PHONE);

    phone_name    = phone_from_id(phone_id);
    lc_phone_name = phone_from_id(lc_phone_id);
    rc_phone_name = phone_from_id(rc_phone_id);

    sprintf(phone_str, "%s(%s,%s)", phone_name, lc_phone_name, rc_phone_name);
    out_phone_id = phone_to_id(phone_str, FALSE);
    if (out_phone_id != NO_PHONE) {
	return out_phone_id;
    }

    sprintf(phone_str, "%s", phone_name);
    out_phone_id = phone_to_id(phone_str, FALSE);
    if (out_phone_id != NO_PHONE) {
	E_INFO("%s(%s,%s)e approxed as %s\n",
	       phone_name, lc_phone_name, rc_phone_name,
	       phone_str);
	return out_phone_id;
    }

    E_ERROR("%s(%s,%s) returned id NO_PHONE\n", 
            phone_name, lc_phone_name, rc_phone_name);

    assert(FALSE);	/* should never get here */
    return 0;
}

static void
add_triphone(short *amatrix, int32 *phone_id_map, int *word_id_map, char *boundary,
	     int orig, int new, int32 triphone_id)
{
    int i;
    short *orig_adjacency;
    short *new_adjacency;

    orig_adjacency = &amatrix[orig * MAX_NODES];
    new_adjacency = &amatrix[new * MAX_NODES];
    phone_id_map[new] = triphone_id;
    boundary[new] = boundary[orig];
    word_id_map[new] = word_id_map[orig];
    for (i = 0; i < MAX_NODES; i++) {
	if (orig_adjacency[i] == LEFT_ADJACENT) {
	    amatrix[ i * MAX_NODES + new ] = RIGHT_ADJACENT;
	    new_adjacency[i] = orig_adjacency[i];
	}
	else if (orig_adjacency[i] == RIGHT_ADJACENT) {
	    amatrix[ i * MAX_NODES + new ] = LEFT_ADJACENT;
	    new_adjacency[i] = orig_adjacency[i];
	}
    }
}

void
prune_invalid_adjacencies(short *amatrix,
			  int node_cnt,
			  int *ci_map,
			  int *lc_map,
			  int *rc_map)
{
    int node;
    int neighbor;
    short *node_adjacency;
    short *neighbor_adjacency;

    /*
     * sweep through the adjacency matrix pruning arcs
     * that do not satisfy left and right context constraints.
     *
     * node 0 is either a SIL, SILb or a left context node.  No need to expand.
     * node 1 is an optional sil
     */
    for (node = 2, node_adjacency = &amatrix[2*MAX_NODES];
	 node < node_cnt;
	 node++, node_adjacency += MAX_NODES) {
	
	for (neighbor = 0; neighbor < node_cnt; neighbor++) {

	    neighbor_adjacency = &amatrix[neighbor * MAX_NODES];

	    /* sil phones are not expanded into triphones.  So, no need to remove any arcs
	       them up */
	    if (ci_map[node] != sil_phone_id) {
		if (node_adjacency[neighbor] == LEFT_ADJACENT) {
		    if (lc_map[node] != ci_map[neighbor]) {
			node_adjacency[neighbor] = NOT_ADJACENT;
			neighbor_adjacency[node] = NOT_ADJACENT;
		    }
		}
		else if (node_adjacency[neighbor] == RIGHT_ADJACENT) {
		    if (rc_map[node] != ci_map[neighbor]) {
			node_adjacency[neighbor] = NOT_ADJACENT;
			neighbor_adjacency[node] = NOT_ADJACENT;
		    }
		}
	    }
	}
    }
}

static int
expand_phone_graph(short *amatrix,
		   char  *boundary,
		   int32 *phone_id_map,
		   int   *word_id_map,
		   int    in_node_cnt)
{
    int32 *lc_phone_id;
    int    lc_cnt;
    int    lc_is_end_word;
    int32 *rc_phone_id;
    int    rc_cnt;
    int    rc_is_start_word;
    int    ci_cnt = phoneCiCount();
    char   *is_in_lc;
    char   *is_in_rc;
    int     node;
    short    *amatrix_row;
    int32     phone_id;
    int     i, j;
    int     out_node_cnt;
    int     num_expanded;
    int32   triphone_id;
    int32   *ci_phone_id_map;
    int32   *lc_phone_id_map;
    int32   *rc_phone_id_map;

    out_node_cnt = in_node_cnt;

    ci_phone_id_map = calloc(MAX_NODES, sizeof(int32));
    lc_phone_id_map = calloc(MAX_NODES, sizeof(int32));
    rc_phone_id_map = calloc(MAX_NODES, sizeof(int32));
    memcpy(ci_phone_id_map, phone_id_map, in_node_cnt * sizeof(int32));

    /* Map various between word non-speech as SIL for the
       purposes of triphone creation */
    for (i = 0; i < in_node_cnt; i++) {
	if ((ci_phone_id_map[i] == sile_phone_id) ||
	    (ci_phone_id_map[i] == silb_phone_id) ||
	    (ci_phone_id_map[i] == FILLER_PHONE_SEQ)) {
	    ci_phone_id_map[i] = sil_phone_id;
	}
    }

    is_in_lc = calloc(ci_cnt, sizeof(char));
    lc_phone_id   = calloc(ci_cnt, sizeof(int32));

    is_in_rc = calloc(ci_cnt, sizeof(char));
    rc_phone_id   = calloc(ci_cnt, sizeof(int32));
    
    /* node 0 is the left context ci phone */
    /* node 1 is the optional filler phone sequence after the left context phone */
    /* node 2 is the first node which is to be expanded */
    for (node = 2, amatrix_row = &amatrix[2 * MAX_NODES];
	 node < in_node_cnt;
	 node++, amatrix_row += MAX_NODES) {

	memset(is_in_lc, 0, ci_cnt*sizeof(char));
	memset(is_in_rc, 0, ci_cnt*sizeof(char));

	amatrix_row = &amatrix[node * MAX_NODES];

	if (ci_phone_id_map[node] != sil_phone_id) {
	    /* non-filler phones are expanded */
#if SHOW&SHOW_NODE_EXPANSION
	    E_INFO("%s expands to ", phone_from_id(ci_phone_id_map[node]));
#endif
	    
	    lc_cnt = rc_cnt = 0;	
	    lc_is_end_word = UNDEFINED;

	    if (boundary[node] == TRUE)
		rc_is_start_word = TRUE;
	    else
		rc_is_start_word = FALSE;

	    for (i = 0; i < in_node_cnt; i++) {

		phone_id = ci_phone_id_map[i];

		if (amatrix_row[i] == LEFT_ADJACENT) {
		    if (lc_is_end_word == UNDEFINED) {
			if (boundary[i] == TRUE) {
			    lc_is_end_word = TRUE;
			}
			else {
			    lc_is_end_word = FALSE;
			}
		    }

		    if (!is_in_lc[phone_id]) {
			is_in_lc[phone_id] = TRUE;
			lc_phone_id[lc_cnt++] = phone_id;
		    }
		    else	/* ignore it since already there */ ;
		}
		else if (amatrix_row[i] == RIGHT_ADJACENT) {
		    if (!is_in_rc[phone_id]) {
			is_in_rc[phone_id] = TRUE;
			rc_phone_id[rc_cnt++] = phone_id;
		    }
		    else	/* ignore it since already there */ ;
		}
	    }

	    if (lc_is_end_word && rc_is_start_word) {
		/* single phone word case */
		num_expanded = 0;
		for (i = 0; i < lc_cnt; i++) {
		    for (j = 0; j < rc_cnt; j++) {
			triphone_id = single_phone_word_triphone(ci_phone_id_map[node], lc_phone_id[i], rc_phone_id[j]);
#if SHOW&SHOW_NODE_EXPANSION
			E_INFOCONT(" %s", phone_from_id(triphone_id));
#endif
			if (num_expanded == 0) {
			    phone_id_map[node] = triphone_id;
			    lc_phone_id_map[node] = lc_phone_id[i];
			    rc_phone_id_map[node] = rc_phone_id[j];
			}
			else {
			    ci_phone_id_map[out_node_cnt] = ci_phone_id_map[node];
			    lc_phone_id_map[out_node_cnt] = lc_phone_id[i];
			    rc_phone_id_map[out_node_cnt] = rc_phone_id[j];
			    add_triphone(amatrix, phone_id_map, word_id_map, boundary,
					 node, out_node_cnt++, triphone_id);
			}
			++num_expanded;
		    }
		}
#if SHOW&SHOW_NODE_EXPANSION
		E_INFOCONT("\n");
#endif
	    }
	    else if (lc_is_end_word) {
		/* multi-phone word start */
		num_expanded = 0;
		for (i = 0; i < lc_cnt; i++) {
		    for (j = 0; j < rc_cnt; j++) {
			triphone_id = begin_triphone(ci_phone_id_map[node], lc_phone_id[i], rc_phone_id[j]);
#if SHOW&SHOW_NODE_EXPANSION
			E_INFOCONT(" %s", phone_from_id(triphone_id));
#endif
			if (num_expanded == 0) {
			    phone_id_map[node] = triphone_id;
			    lc_phone_id_map[node] = lc_phone_id[i];
			    rc_phone_id_map[node] = rc_phone_id[j];
			}
			else {
			    ci_phone_id_map[out_node_cnt] = ci_phone_id_map[node];
			    lc_phone_id_map[out_node_cnt] = lc_phone_id[i];
			    rc_phone_id_map[out_node_cnt] = rc_phone_id[j];
			    add_triphone(amatrix, phone_id_map, word_id_map, boundary,
					 node, out_node_cnt++, triphone_id);
			}
			++num_expanded;
		    }
		}
#if SHOW&SHOW_NODE_EXPANSION
		E_INFOCONT("\n");
#endif
	    }
	    else if (rc_is_start_word) {
		/* multi-phone word end */
		num_expanded = 0;
		for (i = 0; i < lc_cnt; i++) {
		    for (j = 0; j < rc_cnt; j++) {
			triphone_id = end_triphone(ci_phone_id_map[node], lc_phone_id[i], rc_phone_id[j]);
#if SHOW&SHOW_NODE_EXPANSION
			E_INFOCONT(" %s", phone_from_id(triphone_id));
#endif
			if (num_expanded == 0) {
			    phone_id_map[node] = triphone_id;
			    lc_phone_id_map[node] = lc_phone_id[i];
			    rc_phone_id_map[node] = rc_phone_id[j];
			}
			else {
			    ci_phone_id_map[out_node_cnt] = ci_phone_id_map[node];
			    lc_phone_id_map[out_node_cnt] = lc_phone_id[i];
			    rc_phone_id_map[out_node_cnt] = rc_phone_id[j];
			    add_triphone(amatrix, phone_id_map, word_id_map, boundary,
					 node, out_node_cnt++, triphone_id);
			}
			++num_expanded;
		    }
		}
#if SHOW&SHOW_NODE_EXPANSION
		E_INFOCONT("\n");
#endif
	    }
	    else {
		/* multi-phone word internal */
		num_expanded = 0;
		for (i = 0; i < lc_cnt; i++) {
		    for (j = 0; j < rc_cnt; j++) {
			triphone_id = triphone(ci_phone_id_map[node], lc_phone_id[i], rc_phone_id[j]);

#if SHOW&SHOW_NODE_EXPANSION
			E_INFOCONT(" %s", phone_from_id(triphone_id));
#endif
			if (num_expanded == 0) {
			    phone_id_map[node] = triphone_id;
			    lc_phone_id_map[node] = lc_phone_id[i];
			    rc_phone_id_map[node] = rc_phone_id[j];
			}
			else {
			    ci_phone_id_map[out_node_cnt] = ci_phone_id_map[node];
			    lc_phone_id_map[out_node_cnt] = lc_phone_id[i];
			    rc_phone_id_map[out_node_cnt] = rc_phone_id[j];
			    add_triphone(amatrix, phone_id_map, word_id_map, boundary,
					 node, out_node_cnt++, triphone_id);
			}
			++num_expanded;
		    }
		}
#if SHOW&SHOW_NODE_EXPANSION
		E_INFOCONT("\n");
#endif
	    }
	}
	else {
#if SHOW&SHOW_NODE_EXPANSION
	    if (ci_phone_id_map[node] >= 0) {
		E_INFO("%s:%d not expanded\n", phone_from_id(ci_phone_id_map[node]), node);
	    }
	    else {
		E_INFO("FILL:%d not expanded\n", node);
	    }
#endif
	}
    }

    prune_invalid_adjacencies(amatrix, out_node_cnt,
			      ci_phone_id_map, lc_phone_id_map, rc_phone_id_map);
    
    free(ci_phone_id_map);
    free(lc_phone_id_map);
    free(rc_phone_id_map);

    free(is_in_lc);
    free(lc_phone_id);

    free(is_in_rc);
    free(rc_phone_id);

    return out_node_cnt;
}

static int
add_word(short  *amatrix,
	 char   *boundary,
	 int32  *phone_id_map,
	 int32  *word_id_map,

	 int    *out_new_node,
	 int    *out_word_cnt,

	 char const *cur_word_str,

	 int     *next_end,
	 int     *next_end_cnt,

	 int     *prior_end,
	 int     *prior_end_cnt)
{
    int i, k; /* If we really need a variable named 'j' we can declare one */
    int32 cur_word_id;
    dict_entry_t *cur_word;
    int new_node = *out_new_node;
    int word_cnt = *out_word_cnt;

    cur_word_id = kb_get_word_id(cur_word_str);
    if (cur_word_id < 0) {
      E_ERROR("Unknown word: %s\n", cur_word_str);
      return -1;
    }
    
    do {
	cur_word = WordDict->dict_list[cur_word_id];

	phone_id_map[new_node] = cur_word->ci_phone_ids[0];
	word_id_map[new_node] = cur_word_id;
	++word_cnt;
	if (cur_word->len > 1)
	    boundary[new_node] = FALSE;
	else {
	    boundary[new_node] = TRUE;
	    next_end[*next_end_cnt] = new_node;
	    (*next_end_cnt)++;
	}
	for (k = 0; k < *prior_end_cnt; k++) {
	    i = prior_end[k];
	    amatrix[i*MAX_NODES + new_node] = RIGHT_ADJACENT;
	    amatrix[new_node*MAX_NODES + i] = LEFT_ADJACENT;
	}

	new_node++;

	if (cur_word->len > 1) {

	    /* add the word internal nodes (if any) */
	    for (k = 1; k < cur_word->len-1; k++, new_node++) {
		phone_id_map[new_node] = cur_word->ci_phone_ids[k];
		boundary[new_node] = FALSE;
		word_id_map[new_node] = cur_word_id;
		amatrix[(new_node-1)*MAX_NODES + new_node] = RIGHT_ADJACENT;
		amatrix[new_node*MAX_NODES + new_node-1] = LEFT_ADJACENT;
	    }

	    /* add the last phone of the word */
	    phone_id_map[new_node] = cur_word->ci_phone_ids[k];
	    word_id_map[new_node] = cur_word_id;

	    boundary[new_node] = TRUE;

	    amatrix[(new_node-1)*MAX_NODES + new_node] = RIGHT_ADJACENT;
	    amatrix[new_node*MAX_NODES + new_node-1] = LEFT_ADJACENT;
	    
	    next_end[*next_end_cnt] = new_node;
	    (*next_end_cnt)++;

	    new_node++;
	}
	    
	/* get the next alternative pronunication */
	cur_word_id = cur_word->alt;

    } while (cur_word_id >= 0);

    *out_new_node = new_node;
    *out_word_cnt = word_cnt;
    return 0;
}

char *next_transcript_word(char **out_rem_word_seq)
{
    char *cur_word_str;
    char *rem_word_seq = *out_rem_word_seq;
    char *sp;
    int fchar;

    /* after first space is converted to '\0', this will be the next word */
    cur_word_str = rem_word_seq;

    sp = strchr(rem_word_seq, ' ');
    if (sp) {
	fchar = *(sp+1); /* should be the first character of the next word */
	if (isspace(fchar) || (fchar == '\0')) {
	    E_FATAL("Please remove the extra spaces in:\n|%s|\n", rem_word_seq);
	}

	*out_rem_word_seq = sp+1;
	*sp = '\0';
    }
    else
	*out_rem_word_seq = NULL;

    return cur_word_str;
}

static int
mk_phone_graph(short *amatrix,
	       char  *boundary,
	       int32 *phone_id_map,
	       int *word_id_map,
	       int *final_model,
	       char const *left_word_str,
	       char *word_seq,
	       char const *right_word_str,
	       int  *out_word_cnt)
{
    int i, k, m;
    int left_word_id = kb_get_word_id(left_word_str);
    int right_word_id = kb_get_word_id(right_word_str);
    dict_entry_t *left_word;
    dict_entry_t *right_word;
    char const *cur_word_str;
    char *rem_word_seq;
    int new_node;
    int end_node[MAX_COMPOUND_LEN][MAX_NODES];
    int end_node_cnt[MAX_COMPOUND_LEN];
    int end_list_index[MAX_COMPOUND_LEN];
    int *prior_end;
    int *prior_end_cnt;
    int *next_end;
    int *next_end_cnt;
    int word_cnt;

    if (left_word_id < 0) {
      E_ERROR("Cannot find left word %s in the dictionary\n",
	      left_word_str);
      return -1;
    }

    if (right_word_id < 0) {
      E_ERROR("Cannot find right word %s in the dictionary\n",
	      right_word_str);
      return -1;
    }

    left_word  = WordDict->dict_list[left_word_id];
    right_word = WordDict->dict_list[right_word_id];

    for (i = 0; i < MAX_COMPOUND_LEN; i++) {
	end_list_index[i] = i;
	end_node_cnt[i] = 0;
    }
    
    word_cnt = 0;
    new_node = 0;

    prior_end = end_node[end_list_index[0]];
    prior_end_cnt = &end_node_cnt[end_list_index[0]];

    phone_id_map[new_node] = left_word->ci_phone_ids[left_word->len-1];
    boundary[new_node] = TRUE;
    word_id_map[new_node] = left_word_id;
    ++word_cnt;
    amatrix[new_node * MAX_NODES + 1] = RIGHT_ADJACENT;
    prior_end[*prior_end_cnt] = new_node;
    (*prior_end_cnt)++;
    ++new_node;

    phone_id_map[new_node] = sil_phone_id; /* sil_phone_id for train, FILLER_PHONE_SEQ for test */
    word_id_map[new_node] = sil_word_id;

    boundary[new_node]     = TRUE;
    amatrix[new_node * MAX_NODES + 0] = LEFT_ADJACENT;
    prior_end[*prior_end_cnt] = new_node;
    (*prior_end_cnt)++;
    ++new_node;

    /* loop over the words in the word sequence */

    rem_word_seq = word_seq;
    
    do {
	prior_end = end_node[end_list_index[0]];
	prior_end_cnt = &end_node_cnt[end_list_index[0]];

	for (m = 0; m < compound_word_cnt; m++) {
	    if (strncmp(compound_word_list[m].match_str, rem_word_seq,
			strlen(compound_word_list[m].match_str)) == 0) {
		/* if substring matches */
		if ((rem_word_seq[strlen(compound_word_list[m].match_str)] == ' ') ||
		    (rem_word_seq[strlen(compound_word_list[m].match_str)] == '\0')) {
		    /* and end of word */
		    
		    E_INFOCONT("\t%s applies to %s\n", 
			   compound_word_list[m].match_str, rem_word_seq);

		    next_end = end_node[end_list_index[compound_word_list[m].word_cnt]];
		    next_end_cnt = &end_node_cnt[end_list_index[compound_word_list[m].word_cnt]];

		    cur_word_str = compound_word_list[m].word_str;

		    /* add compound word and alternates */
		    add_word(amatrix, boundary, phone_id_map, word_id_map,
			     &new_node, &word_cnt,
			     cur_word_str,
			     next_end,
			     next_end_cnt,
			     prior_end,
			     prior_end_cnt);

		    /* add optional filler phone sequence after words just added */
		    phone_id_map[new_node] = sil_phone_id;
		    word_id_map[new_node] = sil_word_id;
		    ++word_cnt;
		    boundary[new_node] = TRUE;
		    for (k = 0; k < *next_end_cnt; k++) {
			i = next_end[k];
			amatrix[i*MAX_NODES + new_node] = 1;
			amatrix[new_node*MAX_NODES + i ] = -1;
		    }
		    
		    next_end[*next_end_cnt] = new_node;
		    (*next_end_cnt)++;
		    
		    new_node++;
		}
	    }
	}

	next_end = end_node[end_list_index[1]];
	next_end_cnt = &end_node_cnt[end_list_index[1]];

	cur_word_str = next_transcript_word(&rem_word_seq);

	/* add word and alternates */
	add_word(amatrix, boundary, phone_id_map, word_id_map,
		 &new_node, &word_cnt,
		 cur_word_str,
		 next_end,
		 next_end_cnt,
		 prior_end,
		 prior_end_cnt);

	/* add optional filler phone sequence after words just added */
	phone_id_map[new_node] = sil_phone_id;
	word_id_map[new_node] = sil_word_id;
	++word_cnt;
	boundary[new_node] = TRUE;
	for (k = 0; k < *next_end_cnt; k++) {
	    i = next_end[k];
	    amatrix[i*MAX_NODES + new_node] = 1;
	    amatrix[new_node*MAX_NODES + i ] = -1;
	}

	next_end[*next_end_cnt] = new_node;
	(*next_end_cnt)++;

	new_node++;

	/* zero the end count for the one which will become
	   MAX_COMPOUND_LEN out */
	*prior_end_cnt = 0;

	/* advance the end node indices w/ wraparound */
	for (i = 0; i < MAX_COMPOUND_LEN; i++) {
	    end_list_index[i]++;
	    end_list_index[i] %= MAX_COMPOUND_LEN;
	}
    } while (rem_word_seq);	/* while there are words left to do */

    phone_id_map[new_node] = right_word->ci_phone_ids[0];
    word_id_map[new_node] = right_word_id;
    ++word_cnt;
    boundary[new_node] = FALSE;
    
    prior_end = end_node[end_list_index[0]];
    prior_end_cnt = &end_node_cnt[end_list_index[0]];

    for (i = 0; i < *prior_end_cnt; i++) {
	amatrix[prior_end[i] * MAX_NODES + new_node] = 1;
	amatrix[new_node * MAX_NODES + prior_end[i]] = -1;
    }

    *final_model = new_node;

    ++new_node;

    *out_word_cnt = word_cnt;

    return new_node;	/* i.e. the node count */
}

static char *model_name[MAX_NODES];
static int32 *saved_phone_id_map;

void
print_phone_graph(short *amatrix, int node_cnt, int32 *phone_id_map, int *word_id_map)
{
#if SHOW&SHOW_MODEL_DAG
    int i, j, a;

    for (i = 0; i < node_cnt; i++) {
	if (phone_id_map[i] != FILLER_PHONE_SEQ)
	    E_INFOCONT("\"%s:%d\"\t", phone_from_id(phone_id_map[i]), i);
	else
	    E_INFOCONT("\"FILL:%d\"\t", i);
	
	for (j = 0; j < node_cnt; j++) {
	    int a = amatrix[i*MAX_NODES + j];
	    
	    if (a == RIGHT_ADJACENT) {
		if (phone_id_map[j] != FILLER_PHONE_SEQ)
		    E_INFOCONT("\"%s:%d\" ", phone_from_id(phone_id_map[j]), j);
		else
		    E_INFOCONT("\"FILL:%d\" ", j);
	    }
	}
	E_INFOCONT(";\n");
    }
#endif
}

static void
mk_model(short *amatrix,
	 int node,
	 int node_cnt,
	 DYNMODEL_T *model,
	 int32 *phone_id_map,
	 int *out_next_cnt)
{
    int i, j;
    int next_cnt;
    DYNMODEL_T *cur_model;
    short *amatrix_row;
    int *next_array;

    cur_model = &model[node];
    amatrix_row = &amatrix[node * MAX_NODES];

    cur_model->model_best_score = WORST_SCORE;

    if (phone_id_map[node] == NO_EVAL)
        cur_model->sseq_id = NO_EVAL;
    else
        cur_model->sseq_id = hmm_pid2sid(phone_id_map[node]);

    for (i = 0; i < NODE_CNT; i++) {
	cur_model->score[i] = WORST_SCORE;
	cur_model->wbp[i] = NO_BP;
	cur_model->pbp[i] = NO_BP;
	cur_model->sbp[i] = NO_BP;
    }

    for (i = 0, next_cnt = 0; i < node_cnt; i++) {
	if (amatrix_row[i] == RIGHT_ADJACENT)
	    next_cnt++;
    }

    next_array = calloc(next_cnt, sizeof(int32));
    for (i = 0, j = 0; i < node_cnt; i++) {
	if (amatrix_row[i] == RIGHT_ADJACENT) {
	    next_array[j] = i;
	    ++j;
	}
    }
    cur_model->next_cnt = next_cnt;
    cur_model->next = next_array;

    *out_next_cnt = next_cnt;
}
	 
DYNMODEL_T *mk_viterbi_decode_models(short *amatrix,
				     int node_cnt,
				     int32 *phone_id_map)
{
    DYNMODEL_T *model;
    int i;
    int succ_cnt;
    int sum_succ_cnt;

    model = calloc(node_cnt, sizeof(DYNMODEL_T));
    for (i = 0, sum_succ_cnt = 0; i < node_cnt; i++) {
	model_name[i] = salloc(phone_from_id(phone_id_map[i]));
	mk_model(amatrix, i, node_cnt, model, phone_id_map, &succ_cnt);
	sum_succ_cnt += succ_cnt;
    }

    return model;
}

static DYNMODEL_T *
mk_models(int32 **out_phone_id_map,	/* mapping from graph node to associated phone id */
	  int32 **out_word_id_map,	/* mapping from graph node to associated word id */
	  char  **out_boundary,		/* mapping from graph node to TRUE if graph node is
					   before a word boundary (i.e. an end node for a word) */
	  int *out_model_cnt,	/* number of nodes in final graph */
	  int *out_word_cnt,	/* number of words (incl. alts) in final graph */
	  int *final_model,	/* final model in the dag */
	  char const *left_word_str, /* the left context word */
	  char *word_seq,            /* the sequence of words between the left context
				        and right context words exclusive */
	  char const *right_word_str)/* the right context word */
{
    short *amatrix = NULL;	/* adjacency matrix for a graph of phone models */
    char  *boundary = NULL;	/* TRUE indicates word boundary to the right */
    int32 *phone_id_map = NULL;  /* the phone id's of the models in the graph */
    int32 *word_id_map = NULL;  /* the word id's associated w/ the models in the graph */
    int    word_cnt;

    int   ci_node_cnt;
    int   node_cnt;

    DYNMODEL_T *model;

    amatrix  = (short *)calloc(MAX_NODES * MAX_NODES, sizeof(short));
    boundary =  (char *)calloc(MAX_NODES, sizeof(char));
    phone_id_map = (int32 *)calloc(MAX_NODES, sizeof(int32));
    word_id_map = (int32 *)calloc(MAX_NODES, sizeof(int32));

    ci_node_cnt = mk_phone_graph(amatrix, boundary, phone_id_map, word_id_map, final_model,
				 left_word_str, word_seq, right_word_str, &word_cnt);

    if (ci_node_cnt < 0) {
	free(amatrix);
	free(boundary);
	free(phone_id_map);
	free(word_id_map);

	return NULL;
    }

    print_phone_graph(amatrix, ci_node_cnt, phone_id_map, word_id_map);

    node_cnt = expand_phone_graph(amatrix, boundary, phone_id_map, word_id_map, ci_node_cnt);

    print_phone_graph(amatrix, node_cnt, phone_id_map, word_id_map);

    model = mk_viterbi_decode_models(amatrix, node_cnt, phone_id_map);

    free(amatrix);

    *out_boundary = boundary;
    *out_word_id_map = word_id_map;
    *out_phone_id_map = phone_id_map;
    *out_model_cnt = node_cnt;
    *out_word_cnt = word_cnt;

    return model;
}

static BACK_POINTER_T
*ck_alloc(BACK_POINTER_T *bp_table,
	  int entries_needed,
	  int *in_out_max_entries,
	  int size_increment)
{
    int max_entries = *in_out_max_entries;

    if (entries_needed > max_entries) {
	E_INFO ("Increasing BPTable size from %dK by %dK\n",
		max_entries*sizeof(BACK_POINTER_T) / 1024,
		size_increment*sizeof(BACK_POINTER_T) / 1024);

	max_entries += size_increment;
	assert(max_entries >= entries_needed);

	bp_table = (BACK_POINTER_T *) CM_recalloc(bp_table,
						  max_entries, sizeof(BACK_POINTER_T));

	*in_out_max_entries = max_entries;
    }
    
    return bp_table;
}

int new_word_bp(int32 word_id, int prior_bp, int prior_score)
{
    int i = word_bp_table_next_free;

    word_bp_table = ck_alloc (word_bp_table, i+1,
			      &max_word_bp_table_size, WORD_BP_TABLE_SIZE_INCREMENT);

    word_bp_table[i].id = word_id;
    word_bp_table[i].end_frame = cur_frame;
    word_bp_table[i].score = prior_score;
    word_bp_table[i].prev = prior_bp;

    ++word_bp_table_next_free;

    return i;
}

int new_phone_bp(int model_index, int prior_bp, int prior_score)
{
    int i = phone_bp_table_next_free;

    phone_bp_table = ck_alloc (phone_bp_table, i+1,
			       &max_phone_bp_table_size, PHONE_BP_TABLE_SIZE_INCREMENT);

    phone_bp_table[i].id = model_index;
    phone_bp_table[i].end_frame = cur_frame;
    phone_bp_table[i].score = prior_score;
    phone_bp_table[i].prev = prior_bp;

    ++phone_bp_table_next_free;

    return i;
}

int new_state_bp(int model_index, int exit_state, int prior_bp, int prior_score)
{
    int i = state_bp_table_next_free;

    state_bp_table = ck_alloc (state_bp_table, i+1,
			       &max_state_bp_table_size, STATE_BP_TABLE_SIZE_INCREMENT);

    state_bp_table[i].id = model_index * NODE_CNT + exit_state;
    state_bp_table[i].end_frame = cur_frame;
    state_bp_table[i].score = prior_score;
    state_bp_table[i].prev = prior_bp;

    ++state_bp_table_next_free;

    return i;
}

static void 
evaluate_active_models_boundary(
    int *active_models,	/* the models being actively
			   searched */
    int *n_active,	/* a pointer to the number of active
			   models in the active_models list.
			   The number may be increased as
			   new start nodes are inserted */
    int *bnd_models,	/* the set of models with phone boundary
			   paths to be explored */
    int bnd_cnt,	/* the number of such models */
    int32 *phone_id_map,
    int32 *word_id_map,
    char  *boundary,
    
    int *in_out_best_score)
{
    int i, j, k;
    int exit_score;
    int next_score;
    int *next_scores;
    int *next_bp;
    int *next;
    int  next_cnt;
    int *scores;
    int *wbp;
    int *pbp;
    int *sbp;
    int end_active;
    DYNMODEL_T *bnd_model;
    DYNMODEL_T *next_model;
    int         next_model_index;
    int new_wbp_index;
    int new_pbp_index;

    int local_best_score = *in_out_best_score;

    end_active = *n_active;

    for (i = 0; i < bnd_cnt; i++) {
	k = bnd_models[i];

	bnd_model = &all_models[k];

	scores   = bnd_model->score;

	wbp      = bnd_model->wbp;
	pbp      = bnd_model->pbp;
	sbp      = bnd_model->sbp;

	next     = bnd_model->next;
	next_cnt = bnd_model->next_cnt;

	exit_score = scores[NODE_CNT-1];

	new_pbp_index = new_phone_bp(k, pbp[NODE_CNT-1], exit_score);
	if (boundary[k]) {
	    new_wbp_index = new_word_bp(word_id_map[k], wbp[NODE_CNT-1], exit_score);
	}
	else {
	    new_wbp_index = wbp[NODE_CNT-1];	/* just copy the boundary node bp across */
	}

	for (j = 0; j < next_cnt; j++) {
	    next_model_index = next[j];
	    next_model = &all_models[next_model_index];
	    next_scores = next_model->score;
	    next_score  = next_scores[0];

	    if (exit_score > next_score) {

		if (next_score == WORST_SCORE) {
		    /* not already in the active model list */
		    
		    active_models[end_active]  = next_model_index;
		    ++end_active;
		}

		next_scores[0] = exit_score;

		if (exit_score > next_model->model_best_score)
		    next_model->model_best_score = exit_score;
		
		if (exit_score > local_best_score)
		    local_best_score = exit_score;

		/* just copy the state backpointer as no data is consumed
		   between phone boundaries */
		next_bp = next_model->sbp;
		next_bp[0] = sbp[NODE_CNT-1];

		next_bp = next_model->pbp;
		next_bp[0] = new_pbp_index;

		next_bp = next_model->wbp;
		next_bp[0] = new_wbp_index;
	    }
	}
    }
    *n_active = end_active;
    *in_out_best_score = local_best_score;
}

#define T44	t00
#define T45	t01
#define T33	t10
#define T34	t11
#define T35	t12
#define T22	t20
#define T23	t21
#define T24	t22
#define T11	t00
#define T12	t01
#define T13	t02
#define T00	t10
#define T01	t11
#define T02	t12

static void 
evaluate_active_models_internal(int *phone_bnd_models,
				int *bnd_cnt,
				int *active_model_index,
				int n_models,
				int *in_out_best_score)
{
    /* iteration variables */
    int i, j;

    int model_best_score;
    int local_best_score;

    /* local storage for a frequently used score */
    int score;

    /* this keeps track of which state to compute output scores for */
    int state;

    /* local copy of model transition probably table */
    int *tp;

    /* local copy of the senone indexes for each state of the model */
    /* fbs v6 uses int16 indices and fbs8 uses int32 */
    int32 *dist;

    /* local pointer to the best scoring path to the states in a model */
    int *scores;

    /* local pointers to the word, phone and state back pointers for a model respectively */
    int *wbp;
    int *pbp;
    int *sbp;
    int  nsbp[NODE_CNT-1];

    /* A brief description of the t?? variables below:

       		t?0 is the self transition from some state
		t?1 is the transition to the next state from some state
		t?2 is the skip-next transition out of some state

       There are three sets of these variables since for the fixed topology
       models currently used, you need to select scores from at most three
       sets of outgoing node transitions to compute the next score for the node. */
    int t00, t01, t02;
    int t10, t11, t12;
    int t20, t21, t22;

    /* local copy of the senone sequence id of the model currently being evaluated */
    int sseq_id;

    /* index of the model currently being evaluated */
    int active_model;

    local_best_score = *in_out_best_score;

    for (i = 0, j = 0; i < n_models; i++) {

	model_best_score = WORST_SCORE;

	active_model = active_model_index[i];

	sseq_id = all_models[active_model].sseq_id;
	scores  = all_models[active_model].score;
	wbp     = all_models[active_model].wbp;
	pbp     = all_models[active_model].pbp;
	sbp     = all_models[active_model].sbp;

	dist    = Models[sseq_id].dist;	/* seno associated w/ transitions */
 
	tp      = Models[sseq_id].tp;	/* transition probabilities */

#if SHOW&SHOW_MODEL_EVAL
	printf("%s: %d [%d, %d, %d, %d, %d]\n",
	       model_name[active_model], sseq_id,
	       dist[0], dist[3], dist[6], dist[9], dist[12]);
#endif

	/* compute scores of arcs out of state 4 (4->4, 4->5) */
	state = NODE_CNT-2;
#if SHOW&SHOW_MODEL_EVAL
	printf("%d(", state);
#endif
	score   = scores[state];
	if (score > WORST_SCORE) {
	    nsbp[state] = new_state_bp(active_model, state, sbp[state], score);

	    T44 = T45 = score + distScores[dist[12]];
	    T44 += tp[12];
#if SHOW&SHOW_MODEL_EVAL
	    printf("sc=%d t12= %d ", distScores[dist[12]], T44);
#endif
	    T45 += tp[13];
#if SHOW&SHOW_MODEL_EVAL
	    printf("sc=%d t13= %d ", distScores[dist[13]], T45);
#endif
	}
	else {
	    T44 = T45 = WORST_SCORE;
	}
#if SHOW&SHOW_MODEL_EVAL
	printf(")\n", state);
#endif

	/* compute scores of arcs out of state 3 (3->3, 3->4, 3->5) */
	--state;
#if SHOW&SHOW_MODEL_EVAL
	printf("%d(", state);
#endif
	score   = scores[state];
	if (score > WORST_SCORE) {
	    nsbp[state] = new_state_bp(active_model, state, sbp[state], score);

	    T33 = T34 = T35 = score + distScores[dist[9]];

	    T33 += tp[9];
#if SHOW&SHOW_MODEL_EVAL
	    printf("sc=%d t9= %d ", distScores[dist[9]], T33);
#endif
	    T34 += tp[10];
#if SHOW&SHOW_MODEL_EVAL
	    printf("sc=%d t10= %d ", distScores[dist[10]], T34);
#endif
	    T35 += tp[11];
#if SHOW&SHOW_MODEL_EVAL
	    printf("sc=%d t11= %d ", distScores[dist[11]], T35);
#endif
	}
	else {
	    T33 = T34 = T35 = WORST_SCORE;
	}
#if SHOW&SHOW_MODEL_EVAL
	printf(")\n", state);
#endif

	/* compute scores of arcs out of state 2 (2->2, 2->3, 2->4) */
	--state;
#if SHOW&SHOW_MODEL_EVAL
	printf("%d(", state);
#endif
	score   = scores[state];
	if (score > WORST_SCORE) {
	    nsbp[state] = new_state_bp(active_model, state, sbp[state], score);

	    T22 = T23 = T24 = score + distScores[dist[6]];

	    T22 += tp[6];
#if SHOW&SHOW_MODEL_EVAL
	    printf("sc=%d t6= %d ", distScores[dist[6]], T22);
#endif

	    T23 += tp[7];
#if SHOW&SHOW_MODEL_EVAL
	    printf("sc=%d t7= %d ", distScores[dist[7]], T23);
#endif

	    T24 += tp[8];
#if SHOW&SHOW_MODEL_EVAL
	    printf("sc=%d t8= %d ", distScores[dist[8]], T24);
#endif
	}
	else {
	    T22 = T23 = T24 = WORST_SCORE;
	}
#if SHOW&SHOW_MODEL_EVAL
	printf(")\n", state);
#endif

	/* determine best arc incident on state 5 */
	/* conceptually, state 5 is connected to state 0 via a null arc.
	   The score associated w/ this state will be compared to the score associated
	   with state 0 of the next model and the back pointers for state 0 of the next
	   model will be updated to reflect which score was better */
	   
	/* update backpointers based on best scoring transition */
	if (T45 > T35) { /* (4->5) > (3->5) */
	    scores[5] = T45;
	    if (T45 > model_best_score)
		model_best_score = T45;
	    phone_bnd_models[j++] = active_model;
	    wbp[5] = wbp[4]; 	/* forward word back pointer */
	    pbp[5] = pbp[4];	/* forward phone back pointer */
	    sbp[5] = nsbp[4];
	}
	else if (T35 > WORST_SCORE) { /* 3->5 best */
	    scores[5] = T35;
	    if (T35 > model_best_score)
		model_best_score = T35;
	    phone_bnd_models[j++] = active_model;
	    wbp[5] = wbp[3]; 	/* forward word back pointer */
	    pbp[5] = pbp[3]; 	/* forward phone back pointer */
	    sbp[5] = nsbp[3];
	}
	else {
	    scores[5] = WORST_SCORE;
	}

	score = T44;		/* (4->4) is best so far */
	if (score < T34)
	    score = T34;	/* (3->4) is best so far */
	if (score < T24)
	    score = T24;	/* (2->4) is best */

	if (score > WORST_SCORE) {
	    if (score == T34) { /* (3->4) is best */
		wbp[4] = wbp[3];	/* forward word back pointer along */
		pbp[4] = pbp[3];	/* forward phone back pointer along */
		sbp[4] = nsbp[3];
	    }
	    else if (score == T24) {
		/* 2->4 transition best */
		wbp[4] = wbp[2];	/* forward word back pointer along */
		pbp[4] = pbp[2];	/* forward phone back pointer along */
		sbp[4] = nsbp[2];
	    }
	    else {
		/* 4->4 transition best */
		sbp[4] = nsbp[4];
	    }
	    scores[4] = score;
	    if (score > model_best_score)
		model_best_score = score;
	}
	else {
	    scores[4] = WORST_SCORE;
	}
	
	/* compute scores of arcs out of state 1 (1->1, 1->2, 1->3) */
	--state;
#if SHOW&SHOW_MODEL_EVAL
	printf("%d(", state);
#endif
	score   = scores[state];
	if (score > WORST_SCORE) {
	    nsbp[state] = new_state_bp(active_model, state, sbp[state], score);

	    T11 = T12 = T13 = score + distScores[dist[3]];

	    T11 += tp[3];
#if SHOW&SHOW_MODEL_EVAL
	    printf("sc=%d t3= %d ", distScores[dist[3]], T11);
#endif

	    T12 += tp[4];
#if SHOW&SHOW_MODEL_EVAL
	    printf("sc=%d t4= %d ", distScores[dist[4]], T12);
#endif

	    T13 += tp[5];
#if SHOW&SHOW_MODEL_EVAL
	    printf("sc=%d t5= %d ", distScores[dist[5]], T13);
#endif
	}
	else {
	    T11 = T12 = T13 = WORST_SCORE;
	}
#if SHOW&SHOW_MODEL_EVAL
	printf(")\n");
#endif

	score = T33;	/* (3->3) is best so far */
	if (score < T23)
	    score = T23;	/* (2->3) is best so far */
	if (score < T13)
	    score = T13;	/* (1->3) is best */
	
	if (score > WORST_SCORE) {
	    /* update backpointers based on best scoring transition */
	    if (score == T23) {	/* 2->3 is best */
		wbp[3] = wbp[2];
		pbp[3] = pbp[2];
		sbp[3] = nsbp[2];
	    }
	    else if (score == T13) { /* 1->3 is best */
		wbp[3] = wbp[1];
		pbp[3] = pbp[1];
		sbp[3] = nsbp[1];
	    }
	    else { /* 3->3 is best */
		sbp[3] = nsbp[3];
	    }

	    scores[3] = score;
	    if (score > model_best_score)
		model_best_score = score;
	}
	else {
	    scores[3] = WORST_SCORE;
	}

	/* compute scores of arcs out of state 0 (0->0, 0->1, 0->2) */
	--state;
#if SHOW&SHOW_MODEL_EVAL
	printf("%d(", state);
#endif
	score   = scores[state];
	if (score > WORST_SCORE) {
	    nsbp[state] = new_state_bp(active_model, state, sbp[state], score);

	    T00 = T01 = T02 = score + distScores[dist[0]];

	    T00 += tp[0];
#if SHOW&SHOW_MODEL_EVAL
	    printf("sc=%d t0= %d ", distScores[dist[0]], T00);
#endif

	    T01 += tp[1];
#if SHOW&SHOW_MODEL_EVAL
	    printf("sc=%d t1= %d ", distScores[dist[1]], T01);
#endif

	    T02 += tp[2];
#if SHOW&SHOW_MODEL_EVAL
	    printf("sc=%d t2= %d ", distScores[dist[2]], T02);
#endif
	}
	else {
	    T00 = T01 = T02 = WORST_SCORE;
	}
#if SHOW&SHOW_MODEL_EVAL
	printf(")\n");
#endif

	score = T22;	/* (2->2) is best so far */
	if (score < T12)
	    score = T12;	/* (1->2) is best so far */
	if (score < T02)
	    score = T02;	/* (0->2) is best */

	if (score > WORST_SCORE) {
	    /* update state 2 backpointers based on best scoring transition */
	    if (score == T12) {
		wbp[2] = wbp[1];
		pbp[2] = pbp[1];
		sbp[2] = nsbp[1];
	    }
	    else if (score == T02) {
		wbp[2] = wbp[0];
		pbp[2] = pbp[0];
		sbp[2] = nsbp[0];
	    }
	    else {
		sbp[2] = nsbp[2];
	    }
	    scores[2] = score;
	    if (score > model_best_score)
		model_best_score = score;
	}
	else {
	    scores[2] = WORST_SCORE;
	}

	score = T11;		/* (1->1) is best so far */
	if (score < T01)	/* (0->1) is best */
	    score = T01;

	if (score > WORST_SCORE) {
	    /* update state 1 backpointers based on best scoring transition */
	    if (score == T01) {
		wbp[1] = wbp[0];
		pbp[1] = pbp[0];
		sbp[1] = nsbp[0];
	    }
	    else if (score > WORST_SCORE) {
		sbp[1] = nsbp[1];
	    }

	    scores[1] = score;
	    if (score > model_best_score)
		model_best_score = score;
	}
	else {
	    scores[1] = WORST_SCORE;
	}

	/* evaluate INTERNAL incident arcs on state 0.  Null between phone arc
	 * is evaluated in the between phone routine */

	/* update state 0 backpointer/score for self loop if transition out is possible */
	if (scores[0] > WORST_SCORE) {
	    sbp[0] = nsbp[0];
	    scores[0] = T00;
	    if (T00 > model_best_score)
		model_best_score = score;
	}
	else {
	    scores[0] = WORST_SCORE;
	}

	all_models[active_model].model_best_score = model_best_score;

	if (model_best_score > local_best_score)
	    local_best_score = model_best_score;
    }
    
    *bnd_cnt = j;
    
    *in_out_best_score = local_best_score;
}

/* presumably compilers are able to inline this code */
static void
make_worst_score(DYNMODEL_T *m)
{
    int i;
    int *scores = m->score;
    int *sbp = m->sbp;
    int *pbp = m->pbp;
    int *wbp = m->wbp;

    for (i = 0; i < NODE_CNT; i++) {
	scores[i] = WORST_SCORE;
	wbp[i] = pbp[i] = sbp[i] = NO_BP;
    }
}

static void
prune_active_models(int *pruned_active_models,
		    int *out_pruned_active_cnt,
		    int *active_models,
		    int  active_cnt,
		    int  pruning_threshold,
		    DYNMODEL_T *paths_in_model)
{
    int i;
    int active_model_index;
    int pruned_active_cnt;

    for (i = 0, pruned_active_cnt = 0; i < active_cnt; i++) {
	active_model_index = active_models[i];

	if (all_models[active_model_index].model_best_score > pruning_threshold) {
	    /* copy model index over to the active list for the next frame of data */

	    pruned_active_models[pruned_active_cnt++] = active_model_index;
	}
	else {
#if SHOW&SHOW_PRUNING
	    if (pruned_active_cnt == i) {
		/* first pruned model candidate path set */
		printf("pruned at frame %d: ", cur_frame);
	    }
#endif
	    make_worst_score(&all_models[active_model_index]);

#if SHOW&SHOW_PRUNING
	    printf("%s[%d] ", model_name[active_model_index], active_model_index);
#endif
	}
    }
#if SHOW&SHOW_PRUNING
    if (pruned_active_cnt < active_cnt) {
	printf("\n");
    }
#endif

    *out_pruned_active_cnt = pruned_active_cnt;

#if SHOW&SHOW_SUMMARY_INFO
    printf("pruned %d models at frame %d\n", active_cnt - pruned_active_cnt, cur_frame);
#endif
}

#if 0 /* looks like debugging only */
static void
print_active_models(int *model_index,
		    int n_models,
		    int header,
		    int pruning_threshold)
{
    int i, j, k;
    int *score;
    int *wbp;
    int *pbp;
    int *sbp;
    int id;
    char *name;

    if (n_models > 0) {
	for (i = 0; i < n_models; i++) {
	    k = model_index[i];
	    score = all_models[k].score;
	    wbp = all_models[k].wbp;
	    pbp = all_models[k].pbp;
	    sbp = all_models[k].sbp;

	    if (header)
		printf("%s[%d]:\t%d\t%d\n", model_name[k], k, all_models[k].model_best_score,
		       all_models[k].model_best_score - pruning_threshold);
	    
	    printf("score: ");
	    for (j = 0; j < NODE_CNT; j++) {
		if (score[j] != WORST_SCORE)
		    printf(" %18d", score[j]);
		else
		    printf(" ------------------");
	    }
	    printf("\n");

	    printf("word: ");
	    for (j = 0; j < NODE_CNT; j++) {
		if (score[j] != WORST_SCORE) {
		    id = word_bp_table[wbp[j]].id;
		    if (id < 0) {
			name = "<root>";
		    }
		    else {
			name = (WordDict->dict_list[id])->word;
		    }
		    printf(" %12s(%4d)",
			   name,
			   word_bp_table[wbp[j]].end_frame);
		}
		else
		    printf(" ------------------");
	    }
	    printf("\n");

	    printf("phone: ");
	    for (j = 0; j < NODE_CNT; j++) {
		if (score[j] != WORST_SCORE) {
		    id = phone_bp_table[pbp[j]].id;
		    if (id < 0) {
			name = "<root>";
		    }
		    else {
			name = model_name[id];
		    }
		    printf(" %12s(%4d)",
			   name,
			   phone_bp_table[pbp[j]].end_frame);
		}
		else
		    printf(" ------------------");
	    }
	    printf("\n");

	    printf("state: ");
	    for (j = 0; j < NODE_CNT; j++) {
		if (score[j] != WORST_SCORE) {
		    assert(sbp[j] != NO_BP);
		    id = state_bp_table[sbp[j]].id;
		    if (id < 0) {
			name = "<root>";
		    }
		    else {
			static char state_name[64];
			int mid;
			int state;

			mid = id / NODE_CNT;
			state = id % NODE_CNT;

			sprintf(state_name, "%s[%d]", model_name[mid], state);

			name = state_name;
		    }
		    printf(" %12s(%4d)",
			   name,
			   state_bp_table[sbp[j]].end_frame);
		}
		else
		    printf(" ------------------");
	    }
	    printf("\n");
	}
    }
    else {
	printf("print_active_models: no active models\n");
    }
}

static void
print_word_back_pointers(int from,       /* starting back pointer index */
			 int stop_before) /* one greater than the last index */
{
    printf("new word back pointers this frame\n");
    for (; from < stop_before; from++) {
	printf("%s[%d]: %d] %d\n",
	       (WordDict->dict_list[word_bp_table[from].id])->word,
	       word_bp_table[from].id,
	       word_bp_table[from].end_frame,
	       word_bp_table[from].score);
    }
}

static void
print_phone_back_pointers(int from,	  /* starting back pointer index */
			  int stop_before) /* one greater than the last index */
{
    printf("new phone back pointers this frame\n");
    for (; from < stop_before; from++) {
	printf("%s[%d]: %d] %d\n",
	       model_name[phone_bp_table[from].id],
	       phone_bp_table[from].id,
	       phone_bp_table[from].end_frame,
	       phone_bp_table[from].score);
    }
}

static void
print_state_back_pointers(int from,	  /* starting back pointer index */
			  int stop_before)/* one greater than the last index */
{
    int id;
    int mid;
    int state;

    printf("new state back pointers this frame\n");
    for (; from < stop_before; from++) {
	id = state_bp_table[from].id;

	mid = id / NODE_CNT;
	state = id % NODE_CNT;

	printf("(%s:%d) %d] %d\n",
	       model_name[mid],
	       state,
	       state_bp_table[from].end_frame,
	       state_bp_table[from].score
	       );
    }
}
#endif /* 0 */

static void
print_models(DYNMODEL_T *models,
	     int n_models,
	     int32 *word_id_map,
	     char  *boundary)
{
#if SHOW&SHOW_PHONE_GRAPH
    int i, j, k, m;
    char word_string[1024];

    for (i = 0; i < n_models; i++) {
	if (boundary[i]) {
	    sprintf(word_string, "[%s]", (WordDict->dict_list[word_id_map[i]])->word);
	}
	else {
	    word_string[0] = '\0';
	}

	printf("\"%s:%d%s\"\t",
	       model_name[i], i, word_string);
	for (j = 0; j < models[i].next_cnt; j++) {
	    k = models[i].next[j];
	    
	    if (boundary[k]) {
		sprintf(word_string, "[%s]", (WordDict->dict_list[word_id_map[k]])->word);
	    }
	    else {
		word_string[0] = '\0';
	    }

	    printf("\"%s:%d%s\" ",
		   model_name[k], k, word_string);
	}
	printf(";\n");
    }
#endif
}

static int
va_traverse_back_trace(BACK_POINTER_T *bp_table,
		       int bp_idx,
		       int *score,
		       void (*segment_op)(),
		       va_list ap)
{
    int prior_end;
    int prior_score;
    int cur_score;
    int segment_score;

    if (bp_idx) {
	prior_end = va_traverse_back_trace(bp_table, bp_table[bp_idx].prev, &prior_score,
					   segment_op, ap);

	cur_score = bp_table[bp_idx].score;

	segment_score = cur_score - prior_score;

	if (segment_op) {
	    segment_op(bp_table[bp_idx].id,
		       prior_end+1, bp_table[bp_idx].end_frame,
		       segment_score, ap);
	}

	if (score)
	    *score = bp_table[bp_idx].score;

	return bp_table[bp_idx].end_frame;
    }

    *score = 0;
    return -1;
}

static int
traverse_back_trace(BACK_POINTER_T *bp_table,
		    int bp_idx,
		    int *score,
		    void (*segment_op)(),
		    ...)
{
    int rv;
    va_list ap;

    va_start(ap, segment_op);
    
    rv = va_traverse_back_trace(bp_table, bp_idx, score, segment_op, ap);

    va_end(ap);
    return rv;
}

static int
time_align_word_sequence_init(
    /* output arguments */
    DYNMODEL_T **out_model,
    int32 **out_word_id_map,
    int32 **out_phone_id_map,
    char **out_boundary,
    int *out_model_cnt,
    int *final_model,

    /* input arguments */
    char const *left_word,
    /* FIXME: Actually, word_seq gets modified by
       next_transcript_word(), which is probably a bug. */
    char *word_seq,
    char const *right_word)
{
    int i, k;	/* what good routine is w/o iteration variables */
    int word_cnt;
    DYNMODEL_T *model;
    int         model_cnt;

    /* structure a set of models for forced recognition */
    model = mk_models(out_phone_id_map, out_word_id_map, out_boundary,
		      &model_cnt, &word_cnt, final_model,
		      left_word, word_seq, right_word);

    /* something prevented the model array from being built.  e.g. missing word */
    if (model == NULL)
	return -1;

#if (SHOW&SHOW_SUMMARY_INFO)
    printf("time_align_word_sequence_init: %d models and %d words (including alternates and sil) in DAG\n",
	   model_cnt, word_cnt);
#endif

#if (SHOW&SHOW_PHONE_GRAPH)
    /* shows the topology of the models to be matched to the input */
    print_models(model, model_cnt, *out_word_id_map, *out_boundary);
#endif
    
    active_models[0] = (int *)CM_calloc(model_cnt, sizeof(int));
    active_models[1] = (int *)CM_calloc(model_cnt, sizeof(int));

    word_bp_table_next_free = 0;
    word_bp_table_frame_start = 0;

    phone_bp_table_next_free = 0;
    phone_bp_table_frame_start = 0;

    state_bp_table_next_free = 0;
    state_bp_table_frame_start = 0;

    cur_frame = 0;
    next_active_cnt = 0;
    cur_active_cnt = 0;
    cur_active_models = active_models[cur_frame & 0x1];

    /* probably could use some notion of a start node set here */
    cur_active_models[cur_active_cnt++] = 0;	/* make the start node of the dag active */

    /* put the root node for all back trace trees into the first active node(s) */
    word_bp_table[word_bp_table_next_free].id        = NONE;
    word_bp_table[word_bp_table_next_free].end_frame = -1;
    word_bp_table[word_bp_table_next_free].score     = 0;
    word_bp_table[word_bp_table_next_free].prev      = NONE;

    phone_bp_table[phone_bp_table_next_free].id        = NONE;
    phone_bp_table[phone_bp_table_next_free].end_frame = -1;
    phone_bp_table[phone_bp_table_next_free].score     = 0;
    phone_bp_table[phone_bp_table_next_free].prev      = NONE;

    state_bp_table[state_bp_table_next_free].id        = NONE;
    state_bp_table[state_bp_table_next_free].end_frame = -1;
    state_bp_table[state_bp_table_next_free].score     = 0;
    state_bp_table[state_bp_table_next_free].prev      = NONE;

    for (i = 0; i < cur_active_cnt; i++) {
	k = cur_active_models[i];

	model[k].score[0] = 0;

	model[k].wbp[0] = word_bp_table_next_free;
	model[k].pbp[0] = phone_bp_table_next_free;
	model[k].sbp[0] = state_bp_table_next_free;
    }

    ++word_bp_table_next_free;
    ++phone_bp_table_next_free;
    ++state_bp_table_next_free;

    *out_model = model;
    *out_model_cnt = model_cnt;

    return 0;
}

void
print_word_segment(int word_id,
		   int begin,
		   int end,
		   int score,
		   va_list ap)
{
    char *name;
    char *utt_id = va_arg(ap, char *);

    name = WordDict->dict_list[word_id]->word;

    printf("%s:word> %20s %4d %4d %15d\n",
	   (utt_id != NULL ? utt_id : ""),
	   name,
	   begin,
	   end,
	   score);

    ++n_word_segments;
    
    best_word_string_len += (strlen(name) + 1);
}

void
build_word_segment(int word_id,
		   int begin,
		   int end,
		   int score,
		   va_list ap) /* Gack!  not used... */
{
    char *name;
    /* char *utt_id = va_arg(ap, char *); */

    name = WordDict->dict_list[word_id]->word;

    wdseg[n_word_segments].name = name;
    wdseg[n_word_segments].start = begin;
    wdseg[n_word_segments].end = end;
    wdseg[n_word_segments].score = score;

    ++n_word_segments;
    
    best_word_string_len += (strlen(name) + 1);
}

void
append_word(int word_id,
	    int begin,
	    int end,
	    int score,
	    va_list ap)
{
    char *str = va_arg(ap, char *);
    char *name;

    if (word_id != start_word_id) {
	name = WordDict->dict_list[word_id]->word;
	
	strcat((char *)str, name);
	strcat((char *)str, " ");
    }
}

void
print_phone_segment(int model_index,
		    int begin,
		    int end,
		    int score,
		    va_list ap)
{
    char *name = model_name[model_index];
    char *utt_id = va_arg(ap, char *);

    printf("%s:phone> %20s %4d %4d %15d\n",
	   (utt_id != NULL ? utt_id : ""),
	   name, begin, end, score);
    
    ++n_phone_segments;
}

void
build_phone_segment(int model_index,
		    int begin,
		    int end,
		    int score,
		    va_list ap) /* Not used (!) */
{
    char *name = model_name[model_index];
    /* char *utt_id = va_arg(ap, char *); */

    phseg[n_phone_segments].name = name;
    phseg[n_phone_segments].start = begin;
    phseg[n_phone_segments].end = end;
    phseg[n_phone_segments].score = score;
    
    ++n_phone_segments;
}

void
cnt_state_segments(int id,
		   int begin,
		   int end,
		   int score,
		   va_list ap)
{
    ++n_state_segments;
}

void
cnt_word_segments(int id,
		  int begin,
		  int end,
		  int score,
		  va_list ap)
{
    best_word_string_len += (strlen(WordDict->dict_list[id]->word) + 1);

    ++n_word_segments;
}

void
print_state_segment(int state_id,
		    int begin,
		    int end,
		    int score,
		    va_list ap)
{
    char state_name[64];
    int mid;
    int state;
    char *utt_id = va_arg(ap, char *);

    mid   = state_id / NODE_CNT;
    state = state_id % NODE_CNT;

    sprintf(state_name, "%s:%d",  model_name[mid], state);

    printf("%s:state> %20s %4d %4d %15d\n",
	   (utt_id != NULL ? utt_id : ""),
	   state_name,
	   begin,
	   end,
	   score);
    
    ++n_state_segments;
}

void
find_active_senones(DYNMODEL_T *all_models,
		    int *active_index,
		    int active_cnt)
{
    /* v6 uses int16 and v8 uses int32 */
    int32 *trans_to_senone_id;
    int i, j, k;

    sen_active_clear();
    
    for (i = 0; i < active_cnt; i++) {
	trans_to_senone_id  = Models[all_models[active_index[i]].sseq_id].dist;
	
	for (j = 0; j < 14; j += 3) {	/* HACK!!  Hardwired #transitions(14) */
	    k = trans_to_senone_id[j];
	    if (! BITVEC_ISSET(senone_active_vec, k)) {
		BITVEC_SET(senone_active_vec, k);
		senone_active[n_senone_active++] = k;
	    }
	}
    }
}

static int
lm_score(char const *left_word, char const *middle_words, char const *right_word)
{
    char *rem_word_seq;
    char *cur_word_str;
    int wid;
    int base_wid;
    int out_lm_score;
    int wn = NO_WORD, wn1 = NO_WORD, wn2 = NO_WORD;
    char word_seq[1024];

    /* FIXME: potential buffer overrun */
    sprintf(word_seq, "%s %s %s", left_word, middle_words, right_word);

    rem_word_seq = word_seq;
    out_lm_score = 0;
    do {
	cur_word_str = next_transcript_word(&rem_word_seq);

	wid = kb_get_word_id(cur_word_str);

	base_wid = WordDict->dict_list[wid]->fwid;

	wn2 = wn1;
	wn1 = wn;
	wn  = base_wid;

	/*
	 * language weight, insertion penalty and unigram weight are applied
	 * after the source language model has been read.
	 */
	if (wn1 == NO_WORD) {
	    out_lm_score += lm_ug_score(wn);
	}
	else if (wn2 == NO_WORD) {
	    out_lm_score += lm_bg_score(wn1, wn);
	}
	else {
	    out_lm_score += lm_tg_score(wn2, wn1, wn);
	}
    } while (rem_word_seq);

    return out_lm_score;
}

int
time_align_word_sequence(char const * Utt,
			 char const *left_word,
			 char *word_seq,
			 char const *right_word)
{
    int phone_bnd_cnt;
    int32 *phone_id_map;
    int32 *word_id_map;
    char *boundary;
    int   pruning_threshold;
    int   best_score;
    int   bp;
    int   final_model;
    int   probs_computed_cnt;
#ifdef   FBSVQ_V6
    int   norm;
#endif

    int   total_models_evaluated;
    int   total_model_boundaries_crossed;
    int	  total_models_pruned;

    extern int32 time_align_word;	/* Whether to output word alignment */
    extern int32 time_align_phone;	/* Whether to output phone alignment */
    extern int32 time_align_state;	/* Whether to output state alignment */
    extern char *build_uttid (const char *utt);		/* in fbs_main.c */

    assert(frame_cnt > 0);
    assert(left_word != NULL);
    assert(word_seq != NULL);
    assert(right_word != NULL);

    time_align_set_utt (build_uttid(Utt));
    
    if (all_models) {
	int32 i;
	for (i = 0; i < all_model_cnt; i++) {
	    free(all_models[i].next);
	    free(model_name[i]);
	}

	free(all_models);

    }

    if (saved_phone_id_map) {
	free(saved_phone_id_map);
    }

    if (time_align_word_sequence_init(&all_models,
				      &word_id_map,
				      &phone_id_map,
				      &boundary,
				      &all_model_cnt,
				      &final_model,
				      
				      left_word,
				      word_seq,
				      right_word) < 0)
	return -1;

    saved_phone_id_map = phone_id_map;
    saved_final_model  = final_model;

    probs_computed_cnt = 0;
    total_models_evaluated = 0;
    total_model_boundaries_crossed = 0;
    total_models_pruned = 0;
    do {

	cur_active_models      = active_models[cur_frame & 0x01];
	pruned_active_models = boundary_active_models = active_models[(cur_frame+1) & 0x01];

	best_score = WORST_SCORE;
	
	/* compute only necessary senones */
	find_active_senones(all_models, cur_active_models, cur_active_cnt);

	/* compute the output probabilities for the active shared states */
	probs_computed_cnt += senscr_active(distScores,
					    &cep_f[cur_frame*13],
					    &dcep_f[cur_frame*13],
					    &dcep_80ms_f[cur_frame*13],
					    &pcep_f[cur_frame*3],
					    &ddcep_f[cur_frame*13]);

#if SHOW&SHOW_SUMMARY_INFO
	E_INFO("in> frame %d, %d active models\n", cur_frame, cur_active_cnt);
#endif
#if SHOW&SHOW_ACTIVE
	print_active_models(cur_active_models, cur_active_cnt, TRUE, pruning_threshold);
#endif 

	word_bp_table_frame_start = word_bp_table_next_free;
	phone_bp_table_frame_start = phone_bp_table_next_free;
	state_bp_table_frame_start = state_bp_table_next_free;
	
	total_models_evaluated += cur_active_cnt;

	evaluate_active_models_internal(
					boundary_active_models,
					&phone_bnd_cnt,
					cur_active_models,
					cur_active_cnt,
					&best_score);

	total_model_boundaries_crossed += phone_bnd_cnt;

	evaluate_active_models_boundary(cur_active_models,
					&cur_active_cnt,
					boundary_active_models,
					phone_bnd_cnt,
					phone_id_map,
					word_id_map,
					boundary,
					&best_score);

	pruning_threshold = best_score + beam_width;
	if (pruning_threshold < WORST_SCORE)
	    pruning_threshold = WORST_SCORE;

	prune_active_models(pruned_active_models,
			    &next_active_cnt,
			    cur_active_models,
			    cur_active_cnt,
			    pruning_threshold,
			    all_models);

	total_models_pruned += (cur_active_cnt - next_active_cnt);

#if SHOW&SHOW_SUMMARY_INFO
	E_INFO("out> frame %d, %d active models, %d boundaries, best_score %d, pruning_threshold %d\n",
	       cur_frame, cur_active_cnt, phone_bnd_cnt, best_score, pruning_threshold);
#endif
			    
#if SHOW&SHOW_ACTIVE
	print_active_models(pruned_active_models, next_active_cnt, TRUE, pruning_threshold);
#endif
#if SHOW&SHOW_BP
	print_word_back_pointers(word_bp_table_frame_start, word_bp_table_next_free);
	print_phone_back_pointers(phone_bp_table_frame_start, phone_bp_table_next_free);
	print_state_back_pointers(state_bp_table_frame_start, state_bp_table_next_free);
#endif
	++cur_frame;
	
	cur_active_cnt = next_active_cnt;
    } while (cur_frame < frame_cnt);

    if (cur_active_cnt == 0) {
      E_WARN("all paths pruned at end of input\n");
      return -1;
    }

#if SHOW&SHOW_SYS_INFO
    E_INFO("%d eval mod/frame avg\n",
	   total_models_evaluated / frame_cnt);
    E_INFO("%d eval bnd/frame avg\n",
	   total_model_boundaries_crossed / frame_cnt);
    E_INFO("%d prun mod/frame avg\n",
	   total_models_pruned / frame_cnt);
    E_INFO("%d avg outprobs/frame\n",
	   probs_computed_cnt / frame_cnt);
#endif

    n_word_segments  = 0;
    n_phone_segments = 0;
    n_state_segments = 0;
    best_word_string_len  = 0;
    if (! wdseg) {
	wdseg = (SEGMENT_T *) CM_calloc (MAX_FRAMES+1, sizeof(SEGMENT_T));
	phseg = (SEGMENT_T *) CM_calloc (MAX_FRAMES+1, sizeof(SEGMENT_T));
    }
    
    print_models(all_models, all_model_cnt, word_id_map, boundary);
    
    bp = all_models[final_model].wbp[NODE_CNT-1];

    if (bp != NO_BP) {
	/* create a "dummy" BP to record the score at the last state since there has been
	 * no transition out of it done in the evaluation above. */
	bp = new_word_bp(word_id_map[final_model], bp, all_models[final_model].score[NODE_CNT-1]);

#if SHOW&SHOW_BEST_WORD_PATH
      /*
	printf("%20s %4s %4s %s\n",
	       "Word", "Beg", "End", "Acoustic Score");
      */
	traverse_back_trace(word_bp_table,
			    bp, 
			    NULL,
			    build_word_segment, lcl_utt_id);
	if (time_align_word) {
	    int32 i;
	    printf ("\n\t%5s %5s %10s %11s %s (Word-falign) (%s)\n",
		    "SFrm", "EFrm", "AScr/Frm", "AScr", "Word",
		    lcl_utt_id);
	    printf ("\t------------------------------------------------------------\n");
	    for (i = 0; i < n_word_segments; i++) {
	      /*
		printf ("%20s %4d %4d %12d\n",
			wdseg[i].name, wdseg[i].start, wdseg[i].end, wdseg[i].score);
	      */
	      printf ("\t%5d %5d %10d %11d %s\n",
		      wdseg[i].start, wdseg[i].end, 
		      wdseg[i].score/(wdseg[i].end -wdseg[i].start + 1), 
		      wdseg[i].score, wdseg[i].name);
	    }
	    printf ("\t------------------------------------------------------------\n");
	    printf ("\t%5d %5d %10d %11d %s(TOTAL)\n",
		    wdseg[0].start, wdseg[n_word_segments - 1].end, 
		    all_models[final_model].score[NODE_CNT-1]/(wdseg[n_word_segments - 1].end + 1), 
		    all_models[final_model].score[NODE_CNT-1], lcl_utt_id);
	}
#endif

	bp = all_models[final_model].pbp[NODE_CNT-1];
	/* create a "dummy" BP to record the score at the last state since there has been
	 * no transition out of it done in the evaluation above. */
	bp = new_phone_bp(final_model,
			  bp, all_models[final_model].score[NODE_CNT-1]);
#if SHOW&SHOW_BEST_PHONE_PATH
	/*
	printf("%20s %4s %4s %s\n",
	       "Phone", "Beg", "End", "Acoustic Score");
	*/
	traverse_back_trace(phone_bp_table,
			    bp,
			    NULL,
			    build_phone_segment, lcl_utt_id);
	if (time_align_phone) {
	    int32 i;
	    /*
	    for (i = 0; i < n_phone_segments; i++) {
		printf ("%20s %4d %4d %12d\n",
			phseg[i].name, phseg[i].start, phseg[i].end, phseg[i].score);
	    }
	    */
	    printf ("\n\t%5s %5s %10s %11s %s (Phone-falign) (%s)\n",
		    "SFrm", "EFrm", "AScr/Frm", "AScr", "Phone",
		    lcl_utt_id);
	    printf ("\t------------------------------------------------------------\n");
	    for (i = 0; i < n_phone_segments; i++) {
	      printf ("\t%5d %5d %10d %11d %s\n",
		      phseg[i].start, phseg[i].end, 
		      phseg[i].score/(phseg[i].end -phseg[i].start + 1), 
		      phseg[i].score, phseg[i].name);
	    }
	    printf ("\t------------------------------------------------------------\n");
	    printf ("\t%5d %5d %10d %11d %s(TOTAL)\n",
		    phseg[0].start, phseg[n_phone_segments - 1].end, 
		    all_models[final_model].score[NODE_CNT-1]/(phseg[n_phone_segments - 1].end + 1), 
		    all_models[final_model].score[NODE_CNT-1], lcl_utt_id);
	}

	if (phonelabdirname)
	{
	    save_labs(phseg,n_phone_segments,
		      phonelabdirname,
		      Utt,
		      phonelabextname,
		      phonelabtype);
	}
#endif
	
#if (1||(SHOW&SHOW_BEST_STATE_PATH))
	if (time_align_state) {
	    bp = all_models[final_model].sbp[NODE_CNT-1];
	    /*
	    printf("%20s %4s %4s %s\n",
		   "State", "Beg", "End", "Acoustic Score");
	    */
	    traverse_back_trace(state_bp_table,
				bp,
				NULL,
				print_state_segment, lcl_utt_id);
	}
#endif
    }
    else {
	E_ERROR("Last state not reached at end of utterance\n");
	return -1;
    }

    E_INFO("acscr> %d\n", all_models[final_model].score[NODE_CNT-1]);

    if (kb_get_no_lm_flag() == FALSE)
	E_INFO("lmscr> %d\n", lm_score(left_word, word_seq, right_word));

    free(active_models[0]);
    free(active_models[1]);

    free(word_id_map);
    free(boundary);

    return 0;
}

void
next_state_segment(int state_id,
		   int begin,
		   int end,
		   int score,
		   va_list ap)
{
    int mid;
    int ci_id;
    int state;
    unsigned short frame_seg;
    unsigned short *seg_arr = va_arg(ap, unsigned short *);
    int   *seg_idx = va_arg(ap, int *);

    mid   = state_id / NODE_CNT;

    ci_id = phone_id_to_base_id(saved_phone_id_map[mid]);
    state = state_id % NODE_CNT;

    /* encode state segmentation as sort of mixed radix number
     * where the least significant digit is the state and the next
     * most significant digit is the context independent model id. */
    frame_seg = ci_id * (NODE_CNT-1) + state;

    /* the high-bit of the short indicates the start frame of a phone
     * not sure where exactly this is used, but the old seg format
     * included it */
    if ((*seg_idx == 0) ||
	((state == 0) && ((seg_arr[*seg_idx-1] & 0x7fff) % (NODE_CNT-1) != 0))) {
	frame_seg |= 0x8000;
    }

    seg_arr[*seg_idx] = frame_seg;
    (*seg_idx)++;
}

int
time_align_seg_output(unsigned short **seg, int *seg_cnt)
{
    int last_bp;
    static unsigned short *seg_arr = NULL;
    static int seg_idx;

    if (seg_arr)
	free(seg_arr);

    last_bp = all_models[saved_final_model].sbp[NODE_CNT-1];

    if (last_bp == NO_BP) {
	return NO_SEGMENTATION;
    }

    if (n_state_segments == 0) {
	traverse_back_trace(state_bp_table,
			    last_bp,
			    NULL,
			    cnt_state_segments);
    }

    seg_arr = (unsigned short *)calloc(n_state_segments, sizeof(unsigned short));

    if (seg_arr == NULL) {
	return NO_MEMORY;
    }

    seg_idx = 0;

    traverse_back_trace(state_bp_table,
			last_bp,
			NULL,
			next_state_segment,
			seg_arr, &seg_idx);

    *seg = seg_arr;
    *seg_cnt = n_state_segments;

    return 0;
}

char *time_align_best_word_string(void)
{
    int last_bp;

    if (best_word_string) {
	free(best_word_string);
	best_word_string = NULL;
    }

    last_bp = all_models[saved_final_model].wbp[NODE_CNT-1];
    if (last_bp == NO_BP)
	return NULL;

    if (n_word_segments == 0){
	traverse_back_trace(word_bp_table,
			    last_bp,
			    NULL,
			    cnt_word_segments);
    }

    best_word_string = calloc(best_word_string_len+1, sizeof(char));
    if (best_word_string == NULL)
	return NULL;
    
    traverse_back_trace(word_bp_table,
			last_bp,
			NULL,
			append_word, best_word_string);

    best_word_string[strlen(best_word_string)-1] = '\0';

    return best_word_string;
}

void
append_segment(int id,
	       int begin,
	       int end,
	       int score,
	       va_list ap)
{
    seg_kind_t kind = va_arg(ap, seg_kind_t);
    SEGMENT_T *seg = va_arg(ap, SEGMENT_T *);
    int *seg_idx   = va_arg(ap, int *);
    SEGMENT_T *this_seg = &seg[*seg_idx];
    int mid, ci_id, state;

    this_seg->start = begin;
    this_seg->end = end;
    this_seg->score = score;

    switch (kind) {
	case STATE_SEGMENTATION:

	mid   = id / NODE_CNT;
	ci_id = phone_id_to_base_id(saved_phone_id_map[mid]);
	state = id % NODE_CNT;

	this_seg->id = ci_id * NODE_CNT + state;
	this_seg->name = phone_from_id(ci_id);

	break;

	case PHONE_SEGMENTATION:
	this_seg->id = saved_phone_id_map[id];
	this_seg->name = phone_from_id(saved_phone_id_map[id]);
	break;

	case WORD_SEGMENTATION:
	this_seg->id = id;
	this_seg->name = WordDict->dict_list[id]->word;
	break;

	default:
	  E_FATAL("unhandled segment kind %d\n", kind);
    }

    (*seg_idx)++;
}

SEGMENT_T *
time_align_get_segmentation(seg_kind_t kind, int *seg_cnt)
{
    switch (kind) {
    case WORD_SEGMENTATION:
	*seg_cnt = n_word_segments;
	return wdseg;
	break;

    case PHONE_SEGMENTATION:
	*seg_cnt = n_phone_segments;
	return phseg;
	break;

    default:
      E_ERROR("Invalid 'kind' argument: %d\n", kind);
      return NULL;
    }

#if 0
    int last_bp;
    int i;
    SEGMENT_T *out_seg;
    int out_seg_idx;

    last_bp = all_models[saved_final_model].sbp[NODE_CNT-1];
    if (last_bp == NO_BP)
	return NULL;

    if (n_state_segments == 0){
	traverse_back_trace(state_bp_table,
			    last_bp,
			    NULL,
			    cnt_state_segments);
    }

    out_seg = (SEGMENT_T *)CM_calloc(n_state_segments, sizeof(SEGMENT_T));
    out_seg_idx = 0;

    traverse_back_trace(word_bp_table,
			last_bp,
			NULL,
			append_segment, kind, out_seg, &out_seg_idx);

    *seg_cnt = out_seg_idx;
    return out_seg;
#endif
}
