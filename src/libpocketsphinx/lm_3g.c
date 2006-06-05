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
 *
 * lm_3g.c -- Darpa Trigram LM module.
 *
 * HISTORY
 * 
 * $Log: lm_3g.c,v $
 * Revision 1.1.1.1  2006/05/23 18:45:00  dhuggins
 * re-importation
 *
 * Revision 1.14  2005/09/01 21:09:54  dhdfu
 * Really, actually, truly consolidate byteswapping operations into
 * byteorder.h.  Where unconditional byteswapping is needed, SWAP_INT32()
 * and SWAP_INT16() are to be used.  The WORDS_BIGENDIAN macro from
 * autoconf controls the functioning of the conditional swap macros
 * (SWAP_?[LW]) whose names and semantics have been regularized.
 * Private, adhoc macros have been removed.
 *
 * Revision 1.13  2004/12/10 16:48:56  rkm
 * Added continuous density acoustic model handling
 *
 * Revision 1.12  2004/07/16 00:57:11  egouvea
 * Added Ravi's implementation of FSG support.
 *
 * Revision 1.1.1.1  2004/03/01 14:30:29  rkm
 *
 *
 * Revision 1.2  2004/02/27 16:15:13  rkm
 * Added FSG switching
 *
 * Revision 1.1.1.1  2003/12/03 20:05:04  rkm
 * Initial CVS repository
 *
 * Revision 1.11  2001/12/11 00:24:48  lenzo
 * Acknowledgement in License.
 *
 * Revision 1.10  2001/12/07 17:30:02  lenzo
 * Clean up and remove extra lines.
 *
 * Revision 1.9  2001/12/07 13:11:30  lenzo
 * Consolidate byte order code into byteorder.h.  Note that there are still
 * two "senses" of byte swapping that are confusing, and all this should
 * be cleaned up and verified.
 *
 * Revision 1.8  2001/12/07 05:09:30  lenzo
 * License.xsxc
 *
 * Revision 1.7  2001/12/07 04:27:35  lenzo
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
 * Revision 1.6  2001/10/23 22:20:30  lenzo
 * Change error logging and reporting to the E_* macros that call common
 * functions.  This will obsolete logmsg.[ch] and they will be removed
 * or changed in future versions.
 *
 * Revision 1.5  2001/02/13 19:51:38  lenzo
 * *** empty log message ***
 *
 * Revision 1.4  2001/02/13 18:50:35  lenzo
 * Adding some more comments for a Solaris port.
 *
 * Revision 1.3  2000/12/12 23:01:42  lenzo
 * Rationalizing libs and names some more.  Split a/d and fe libs out.
 *
 * Revision 1.2  2000/12/05 01:45:12  lenzo
 * Restructuring, hear rationalization, warning removal, ANSIfy
 *
 * Revision 1.1.1.1  2000/01/28 22:08:51  lenzo
 * Initial import of sphinx2
 *
 * 
 * 28-Oct-98	M K Ravishankar (rkm@cs) at Carnegie Mellon University
 * 		Added lm3g_access_type() and necessary support.
 * 
 * 15-Oct-98	M K Ravishankar (rkm@cs) at Carnegie Mellon University
 * 		Bugfix: inclass_ugscore[lw3] changed to inclass_ugscore[w3] in
 * 		lm3g_tg_score().  (Thanks to dbansal@cs.)
 * 
 * 15-Jul-98	M K Ravishankar (rkm@cs) at Carnegie Mellon University
 * 		Corrected references to unigram_t.wid to unigram_t.mapid.
 * 
 * 14-Apr-98	M K Ravishankar (rkm@cs) at Carnegie Mellon University
 * 		Added lm3g_n_lm() and lm3g_index2name().
 * 
 * 03-Apr-97	M K Ravishankar (rkm@cs) at Carnegie Mellon University
 * 		Added lm3g_raw_score() and lm_t.invlw.
 * 		Changed a number of function names from lm_... to lm3g_...
 * 
 * 09-Jan-97	M K Ravishankar (rkm@cs) at Carnegie Mellon University
 * 		BUGFIX: Added check for lmp->unigrams[i].wid in lm_set_current().
 * 
 * 06-Dec-95	M K Ravishankar (rkm@cs) at Carnegie Mellon University
 * 		Changed function name lmname_to_lm() to lm_name2lm().
 * 
 * 06-Dec-95	M K Ravishankar (rkm@cs) at Carnegie Mellon University
 * 		Changed function name get_current_lm to lm_get_current.
 * 		Changed check for already existing word in lm_add_word, and added
 * 		condition to updating dictwid_map.
 * 
 * 01-Jul-95	M K Ravishankar (rkm@cs) at Carnegie Mellon University
 * 		Removed LM cache and replaced with find_bg and find_tg within the main
 * 		bigrams and trigram structures.  No loss of speed and uses less memory.
 * 
 * 24-Jun-95	M K Ravishankar (rkm@cs) at Carnegie Mellon University
 * 		Fixed a number of memory leaks while deleting an LM.  Added the global
 * 		dictwid_map, and allocated it once and never freed.  Made sure lm_cache
 * 		is created only once.
 * 
 * 14-Jun-95	M K Ravishankar (rkm@cs) at Carnegie Mellon University
 * 		Modified lm_read to return 0 on success, and to delete any existing LM
 * 		with the new LM name (instead of reporting error and exiting).
 * 		Added backslash option in building filenames (for PC compatibility).
 *
 * Revision 8.9  94/10/11  12:36:28  rkm
 * Changed lm_tg_score to call lm_bg_score if no trigrams present or
 * the first word is invalid.
 * 
 * Revision 8.8  94/07/29  11:54:23  rkm
 * Renamed lmSetParameters to lm_set_param and moved it into lm_add().
 * Added functions lm_init_oov() to create initial list of OOVs,
 * and lm_add_word() to add new OOV at run time.
 * 
 * Revision 8.7  94/05/19  14:19:59  rkm
 * Rewrote LM cache code for greater efficiency.
 * 
 * Revision 8.6  94/05/10  10:47:58  rkm
 * Added lm_add() and lm_set_param() functions, for dynamically adding a new
 * in-memory LM to the set of available LMs.
 * 
 * Revision 8.5  94/04/22  13:53:27  rkm
 * Added query_lm_cache_lines() to allow run-time spec of #cache lines.
 * 
 * Revision 8.4  94/04/14  15:08:46  rkm
 * Added function lm_delete() to delete a named LM and reclaim space.
 * 
 * Revision 8.3  94/04/14  14:40:27  rkm
 * Minor changes.
 * 
 * Revision 8.1  94/02/15  15:09:22  rkm
 * Derived from v7.  Includes multiple LMs for grammar switching.
 * 
 * Revision 6.13  94/02/11  13:14:45  rkm
 * Added bigram and trigram multi-line caches, and functions, for v7.
 * Replaced sequential search in wstr2wid() with hash_lookup().
 * 
 * Revision 6.12  94/01/07  10:56:16  rkm
 * Corrected bug relating to input file format.
 * 
 * Revision 6.11  93/12/17  13:14:52  rkm
 * *** empty log message ***
 * 
 * Revision 6.10  93/12/03  17:09:59  rkm
 * Added ability to handle bigram-only dump files.
 * Added <s> </s> bigram -> MIN_PROB.
 * Added timestamp to dump files.
 * 
 * Revision 6.9  93/12/01  12:29:55  rkm
 * Added ability to handle LM files containing only bigrams.
 * Excluded start_sym from interpolation of unigram prob with uniform prob.
 * 
 * 
 * 93/10/21 rkm@cs.cmu.edu
 * Added <c.h>
 * 
 * Revision 6.6  93/10/19  18:58:10  rkm
 * Added code to change bigram-prob(<s>,<s>) to very low value.  The
 * Darpa LM file contains a spurious value to be ignored.
 * Fixed bug that dumps one trigram entry too many.
 * 
 * Revision 6.5  93/10/15  15:00:14  rkm
 * 
 * Revision 6.4  93/10/13  16:56:04  rkm
 * Added LM cache line stats.
 * Added bg_only option for lm_read parameter list
 * (but not yet implemented).
 * Changed proc name ilm_LOG_prob_of to lm3g_prob, to avoid conflict
 * with Roni's ILM function of the same name.
 * 
 * Revision 6.3  93/10/09  17:01:55  rkm
 * M K Ravishankar (rkm@cs) at Carnegie Mellon
 * Cleaned up handling precompiled binary 3g LM file,
 * Added SWAP option for HP platforms.
 * 
 * Revision 6.2  93/10/06  11:08:15  rkm
 * M K Ravishankar (rkm@cs) at Carnegie Mellon University
 * Darpa Trigram LM module.  Created.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "s2types.h"
#include "s2io.h"
#include "CM_macros.h"
#include "basic_types.h"
#include "assert.h"
#include "strfuncs.h"
#include "linklist.h"
#include "list.h"
#include "hash.h"
#include "err.h"
#include "lmclass.h"
#include "lm_3g.h"
#include "log.h"
#include "search_const.h"
#include "msd.h"
#include "dict.h"
#include "kb.h"
#include "fbs.h"
#include "byteorder.h"

#ifdef NO_DICT
#define NO_WORD	-1
#else
#include "c.h"
#include "dict.h"
static dictT *WordDict;
#endif

#define UG_MAPID(m,u)		((m)->unigrams[u].mapid)
#define UG_PROB_F(m,u)		((m)->unigrams[u].prob1.f)
#define UG_BO_WT_F(m,u)		((m)->unigrams[u].bo_wt1.f)
#define UG_PROB_L(m,u)		((m)->unigrams[u].prob1.l)
#define UG_BO_WT_L(m,u)		((m)->unigrams[u].bo_wt1.l)
#define FIRST_BG(m,u)		((m)->unigrams[u].bigrams)
#define LAST_BG(m,u)		(FIRST_BG((m),(u)+1)-1)

#define BG_WID(m,b)		((m)->bigrams[b].wid)
#define BG_PROB_F(m,b)		((m)->prob2[(m)->bigrams[b].prob2].f)
#define BG_BO_WT_F(m,b)		((m)->bo_wt2[(m)->bigrams[b].bo_wt2].f)
#define BG_PROB_L(m,b)		((m)->prob2[(m)->bigrams[b].prob2].l)
#define BG_BO_WT_L(m,b)		((m)->bo_wt2[(m)->bigrams[b].bo_wt2].l)
#define TSEG_BASE(m,b)		((m)->tseg_base[(b)>>LOG_BG_SEG_SZ])
#define FIRST_TG(m,b)		(TSEG_BASE((m),(b))+((m)->bigrams[b].trigrams))
#define LAST_TG(m,b)		(FIRST_TG((m),(b)+1)-1)

#define TG_WID(m,t)		((m)->trigrams[t].wid)
#define TG_PROB_F(m,t)		((m)->prob3[(m)->trigrams[t].prob3].f)
#define TG_PROB_L(m,t)		((m)->prob3[(m)->trigrams[t].prob3].l)

static double  oov_ugprob = -5.0;	/* Actually, logprob */

static char const *start_sym = "<s>";
static char const *end_sym = "</s>";
static char const *darpa_hdr = "Darpa Trigram LM";

/* FIXME: Why does lm3g2dmp.c have its own versions of these functions? */
static int32	lmname_to_id (char const * name);
static int32	lm3g_load (char const *file, lm_t *model,
			   char const *lmfile, int32 mtime);
static int32	lm3g_dump (char const *file, lm_t *model,
			   char const *lmfile, int32 mtime);
static void	lm_set_param (lm_t *model, double lw, double uw,
			      double wip, int32 word_pair);

/* Structure for maintaining multiple, named LMs */
static struct lmset_s {
    char *name;
    lm_t *lm;
} *lmset;
static int32 n_lm = 0;		/* Total #LMs (actual) */
static int32 n_lm_alloc = 0;	/* Total #LMs (for which space has been allocated) */

/* The currently active LM */
static lm_t *lmp;

/* Words in LM; used only for building internal LM from LM file */
static char **word_str;

static int32 lm_last_access_type;	/* Hack!! See ACCESS definitions in .h */

#define MIN_PROB_F		-99.0

#define MAX_SORTED_ENTRIES	65534

/* Base values for ranges of unigram_t.mapid */
#define LM_DICTWID_BASE		0		/* Do not change this */
#define LM_CLASSID_BASE		0x01000000	/* Interpreted as LMclass ID */
#define LM_DICTWID_BADMAP	-16000		/* An illegal mapping */
#define LM_CLASSID_TO_CLASS(m,i)	((model)->lmclass[(i)-LM_CLASSID_BASE])

/*
 * Bigram probs and bo-wts, and trigram probs are kept in separate tables
 * rather than within the bigram_t and trigram_t structures.  These tables
 * hold unique prob and bo-wt values, and can be < 64K long (see lm_3g.h).
 * The following tree structure is used to construct these tables of unique
 * values.  Whenever a new value is read from the LM file, the sorted tree
 * structure is searched to see if the value already exists, and inserted
 * if not found.
 */
typedef struct sorted_entry_s {
    log_t val;		/* value being kept in this node */
    uint16 lower;	/* index of another entry.  All descendants down
			   this path have their val < this node's val.
			   0 => no son exists (0 is root index) */
    uint16 higher;	/* index of another entry.  All descendants down
			   this path have their val > this node's val
			   0 => no son exists (0 is root index) */
} sorted_entry_t;

/*
 * The sorted list.  list is a (64K long) array.  The first entry is the
 * root of the tree and is created during initialization.
 */
typedef struct {
    sorted_entry_t *list;
    int32 free;		/* first free element in list */
} sorted_list_t;

/* Arrays of unique bigram probs and bo-wts, and trigram probs */
static sorted_list_t sorted_prob2;
static sorted_list_t sorted_bo_wt2;
static sorted_list_t sorted_prob3;

/*
 * Initialize sorted list with the 0-th entry = MIN_PROB_F, which may be needed
 * to replace spurious values in the Darpa LM file.
 */
static void init_sorted_list (sorted_list_t *l)
{
    l->list =
	(sorted_entry_t *) CM_calloc (MAX_SORTED_ENTRIES, sizeof (sorted_entry_t));
    l->list[0].val.f = MIN_PROB_F;
    l->list[0].lower = 0;
    l->list[0].higher = 0;
    l->free = 1;
}

static void free_sorted_list (sorted_list_t *l)
{
    free (l->list);
}

static log_t *vals_in_sorted_list (sorted_list_t *l)
{
    log_t *vals;
    int32 i;
    
    vals = (log_t *) CM_calloc (l->free, sizeof (log_t));
    for (i = 0; i < l->free; i++)
	vals[i].f = l->list[i].val.f;
    return (vals);
}

static int32 sorted_id (sorted_list_t *l, float *val)
{
    int32 i = 0;
    
    for (;;) {
	if (*val == l->list[i].val.f)
	    return (i);
	if (*val < l->list[i].val.f) {
	    if (l->list[i].lower == 0) {
		if (l->free >= MAX_SORTED_ENTRIES)
		    E_FATAL("sorted list overflow\n");
		
		l->list[i].lower = l->free;
		(l->free)++;
		i = l->list[i].lower;
		l->list[i].val.f = *val;
		return (i);
	    } else
		i = l->list[i].lower;
	} else {
	    if (l->list[i].higher == 0) {
		if (l->free >= MAX_SORTED_ENTRIES)
		    E_FATAL("sorted list overflow\n");
		
		l->list[i].higher = l->free;
		(l->free)++;
		i = l->list[i].higher;
		l->list[i].val.f = *val;
		return (i);
	    } else
		i = l->list[i].higher;
	}
    }
}

/*
 * allocate, initialize and return pointer to an array of unigram entries.
 */
static unigram_t *NewUnigramTable (int32 n_ug)
{
    unigram_t *table;
    int32 i;

    table = (unigram_t *) CM_calloc (n_ug, sizeof (unigram_t));
    for (i = 0; i < n_ug; i++) {
	table[i].mapid = NO_WORD;
	table[i].prob1.f = -99.0;
	table[i].bo_wt1.f = -99.0;
    }
    return table;
}

/*
 * returns a pointer to a new language model record.  The size is passed in
 * as a parameter.
 */
lm_t *
NewModel (int32 n_ug, int32 n_bg, int32 n_tg, int32 n_dict)
{
    lm_t *model;

    model = (lm_t *) CM_calloc (1, sizeof (lm_t));
    /* Only allocate the stuff that isn't done elsewhere */
    model->unigrams = NewUnigramTable (n_ug+1);
    model->dictwid_map = (int32 *) CM_calloc (n_dict, sizeof (int32));

    model->max_ucount = model->ucount = n_ug;
    model->bcount = n_bg;
    model->tcount = n_tg;
    model->dict_size = n_dict;
    
    model->HT.size = 0;
    model->HT.inuse = 0;
    model->HT.tab = NULL;

    return model;
}

#ifdef NO_DICT
#define GET_WORD_IDX(w)		wstr2wid (lmp, w)
#else
#define GET_WORD_IDX(w)		dictStrToWordId (WordDict, w, FALSE)
#endif

static int32 wstr2wid (lm_t *model, char *w)
{
    caddr_t val;
    
    if (hash_lookup (&(model->HT), w, &val) != 0)
	return NO_WORD;
    return ((int32) val);
}

/*
 * Read and return #unigrams, #bigrams, #trigrams as stated in input file.
 */
static void ReadNgramCounts (FILE *fp,
			     int32 *n_ug, int32 *n_bg, int32 *n_tg)
{
    char string[256];
    int32 ngram, ngram_cnt;
    
    /* skip file until past the '\data\' marker */
    do
	fgets (string, sizeof (string), fp);
    while ( (strcmp (string, "\\data\\\n") != 0) && (! feof (fp)) );

    if (strcmp (string, "\\data\\\n") != 0)
	E_FATAL("No \\data\\ mark in LM file\n");

    *n_ug = *n_bg = *n_tg = 0;
    while (fgets (string, sizeof (string), fp) != NULL) {
	if (sscanf (string, "ngram %d=%d", &ngram, &ngram_cnt) != 2)
	    break;
	switch (ngram) {
	case 1: *n_ug = ngram_cnt;
	    break;
	case 2: *n_bg = ngram_cnt;
	    break;
	case 3: *n_tg = ngram_cnt;
	    break;
	default:
	    E_FATAL("Unknown ngram (%d)\n", ngram);
	    break;
	}
    }
    
    /* Position file to just after the unigrams header '\1-grams:\' */
    while ( (strcmp (string, "\\1-grams:\n") != 0) && (! feof (fp)) )
	fgets (string, sizeof (string), fp);
    
    /* Check counts;  NOTE: #trigrams *CAN* be 0 */
    if ((*n_ug <= 0) || (*n_bg <= 0) || (*n_tg < 0))
	E_FATAL("Bad or missing ngram count\n");
}

/*
 * Read in the unigrams from given file into the LM structure model.  On
 * entry to this procedure, the file pointer is positioned just after the
 * header line '\1-grams:'.
 */
static void ReadUnigrams (FILE *fp, lm_t *model)
{
    char string[256];
    char name[128];
    int32 wcnt;
    float p1, bo_wt;
    
    E_INFO ("Reading unigrams\n");

    wcnt = 0;
    while ((fgets (string, sizeof(string), fp) != NULL) &&
	   (strcmp (string, "\\2-grams:\n") != 0))
    {
	if (sscanf (string, "%f %s %f", &p1, name, &bo_wt) != 3) {
	    if (string[0] != '\n')
		E_WARN ("Format error; unigram ignored:%s", string);
	    continue;
	}
	
	if (wcnt >= model->ucount)
	    E_FATAL("Too many unigrams\n");

	/* Associate name with word id */
	word_str[wcnt] = (char *) salloc (name);
	hash_add (&(model->HT), word_str[wcnt], (caddr_t) wcnt);
	model->unigrams[wcnt].prob1.f = p1;
	model->unigrams[wcnt].bo_wt1.f = bo_wt;

	model->unigrams[wcnt].mapid = wcnt;
	wcnt++;
    }

    if (model->ucount != wcnt) {
	E_WARN ("lm_t.ucount(%d) != #unigrams read(%d)\n",
		model->ucount, wcnt);
	model->ucount = wcnt;
    }
}

/*
 * Read bigrams from given file into given model structure.  File may be arpabo
 * or arpabo-id format, depending on idfmt = 0 or 1.
 */
static void ReadBigrams (FILE *fp, lm_t *model, int32 idfmt)
{
    char string[1024], word1[256], word2[256];
    int32 w1, w2, prev_w1, bgcount, p;
    bigram_t *bgptr;
    float p2, bo_wt;
    int32 n_fld, n;
    
    E_INFO ("Reading bigrams\n");
    
    bgcount = 0;
    bgptr = model->bigrams;
    prev_w1 = -1;
    n_fld = (model->tcount > 0) ? 4 : 3;

    bo_wt = 0.0;
    while (fgets (string, sizeof(string), fp) != NULL) {
	if (! idfmt)
	    n = sscanf (string, "%f %s %s %f", &p2, word1, word2, &bo_wt);
	else
	    n = sscanf (string, "%f %d %d %f", &p2, &w1, &w2, &bo_wt);
	if (n < n_fld) {
	    if (string[0] != '\n')
		break;
	    continue;
	}

	if (! idfmt) {
	    if ((w1 = wstr2wid (model, word1)) == NO_WORD)
		E_FATAL("Unknown word: %s\n", word1);
	    if ((w2 = wstr2wid (model, word2)) == NO_WORD)
		E_FATAL("Unknown word: %s\n", word2);
	} else {
	    if ((w1 >= model->ucount) || (w2 >= model->ucount) || (w1 < 0) || (w2 < 0))
		E_FATAL("Bad bigram: %s", string);
	}
	
	/* HACK!! to quantize probs to 4 decimal digits */
	p = p2*10000;
	p2 = p*0.0001;
	p = bo_wt*10000;
	bo_wt = p*0.0001;

	if (bgcount >= model->bcount)
	    E_FATAL("Too many bigrams\n");
	
	bgptr->wid = w2;
	bgptr->prob2 = sorted_id (&sorted_prob2, &p2);
	if (model->tcount > 0)
	    bgptr->bo_wt2 = sorted_id (&sorted_bo_wt2, &bo_wt);

	if (w1 != prev_w1) {
	    if (w1 < prev_w1)
	      E_FATAL("Bigrams not in unigram order\n");
	    
	    for (prev_w1++; prev_w1 <= w1; prev_w1++)
		model->unigrams[prev_w1].bigrams = bgcount;
	    prev_w1 = w1;
	}
	
	bgcount++;
	bgptr++;

	if ((bgcount & 0x0000ffff) == 0) {
	    E_INFOCONT (".");
	}
    }
    if ((strcmp (string, "\\end\\\n") != 0) && (strcmp (string, "\\3-grams:\n") != 0))
	E_FATAL("Bad bigram: %s\n", string);
    
    for (prev_w1++; prev_w1 <= model->ucount; prev_w1++)
	model->unigrams[prev_w1].bigrams = bgcount;
}

/*
 * Very similar to ReadBigrams.
 */
static void ReadTrigrams (FILE *fp, lm_t *model, int32 idfmt)
{
    char string[1024], word1[256], word2[256], word3[256];
    int32 i, n, w1, w2, w3, prev_w1, prev_w2, tgcount, prev_bg, bg, endbg, p;
    int32 seg, prev_seg, prev_seg_lastbg;
    trigram_t *tgptr;
    bigram_t *bgptr;
    float p3;
    
    E_INFO ("Reading trigrams\n");
    
    tgcount = 0;
    tgptr = model->trigrams;
    prev_w1 = -1;
    prev_w2 = -1;
    prev_bg = -1;
    prev_seg = -1;

    while (fgets (string, sizeof(string), fp) != NULL) {
	if (! idfmt)
	    n = sscanf (string, "%f %s %s %s", &p3, word1, word2, word3);
	else
	    n = sscanf (string, "%f %d %d %d", &p3, &w1, &w2, &w3);
	if (n != 4) {
	    if (string[0] != '\n')
		break;
	    continue;
	}

	if (! idfmt) {
	    if ((w1 = wstr2wid (model, word1)) == NO_WORD)
		E_FATAL("Unknown word: %s\n", word1);
	    if ((w2 = wstr2wid (model, word2)) == NO_WORD)
		E_FATAL("Unknown word: %s\n", word2);
	    if ((w3 = wstr2wid (model, word3)) == NO_WORD)
		E_FATAL("Unknown word: %s\n", word3);
	} else {
	    if ((w1 >= model->ucount) || (w2 >= model->ucount) || (w3 >= model->ucount) ||
		    (w1 < 0) || (w2 < 0) || (w3 < 0))
		E_FATAL("Bad trigram: %s", string);
	}
	
	/* HACK!! to quantize probs to 4 decimal digits */
	p = p3*10000;
	p3 = p*0.0001;

	if (tgcount >= model->tcount)
	  E_FATAL("Too many trigrams\n");
	
	tgptr->wid = w3;
	tgptr->prob3 = sorted_id (&sorted_prob3, &p3);

	if ((w1 != prev_w1) || (w2 != prev_w2)) {
	    /* Trigram for a new bigram; update tg info for all previous bigrams */
	    if ((w1 < prev_w1) || ((w1 == prev_w1) && (w2 < prev_w2)))
	      E_FATAL("Trigrams not in bigram order\n");
	    
	    bg = (w1 != prev_w1) ? model->unigrams[w1].bigrams : prev_bg+1;
	    endbg = model->unigrams[w1+1].bigrams;
	    bgptr = model->bigrams + bg;
	    for (; (bg < endbg) && (bgptr->wid != w2); bg++, bgptr++);
	    if (bg >= endbg)
		E_FATAL("Missing bigram for trigram: %s", string);

	    /* bg = bigram entry index for <w1,w2>.  Update tseg_base */
	    seg = bg >> LOG_BG_SEG_SZ;
	    for (i = prev_seg+1; i <= seg; i++)
		model->tseg_base[i] = tgcount;

	    /* Update trigrams pointers for all bigrams until bg */
	    if (prev_seg < seg) {
		int32 tgoff = 0;

		if (prev_seg >= 0) {
		    tgoff = tgcount - model->tseg_base[prev_seg];
		    if (tgoff > 65535)
		      E_FATAL("Offset from tseg_base > 65535\n");
		}
		
		prev_seg_lastbg = ((prev_seg+1) << LOG_BG_SEG_SZ) - 1;
		bgptr = model->bigrams + prev_bg;
		for (++prev_bg, ++bgptr; prev_bg <= prev_seg_lastbg; prev_bg++, bgptr++)
		    bgptr->trigrams = tgoff;
		
		for (; prev_bg <= bg; prev_bg++, bgptr++)
		    bgptr->trigrams = 0;
	    } else {
		int32 tgoff;

		tgoff = tgcount - model->tseg_base[prev_seg];
		if (tgoff > 65535)
		  E_FATAL("Offset from tseg_base > 65535\n");
		
		bgptr = model->bigrams + prev_bg;
		for (++prev_bg, ++bgptr; prev_bg <= bg; prev_bg++, bgptr++)
		    bgptr->trigrams = tgoff;
	    }
	    
	    prev_w1 = w1;
	    prev_w2 = w2;
	    prev_bg = bg;
	    prev_seg = seg;
	}
	
	tgcount++;
	tgptr++;

	if ((tgcount & 0x0000ffff) == 0) {
	    E_INFOCONT (".");
	}
    }
    if (strcmp (string, "\\end\\\n") != 0)
	E_FATAL("Bad trigram: %s\n", string);
    
    for (prev_bg++; prev_bg <= model->bcount; prev_bg++) {
	if ((prev_bg & (BG_SEG_SZ-1)) == 0)
	    model->tseg_base[prev_bg >> LOG_BG_SEG_SZ] = tgcount;
	if ((tgcount - model->tseg_base[prev_bg >> LOG_BG_SEG_SZ]) > 65535)
	    E_FATAL("Offset from tseg_base > 65535\n");
	model->bigrams[prev_bg].trigrams =
	    tgcount - model->tseg_base[prev_bg >> LOG_BG_SEG_SZ];
    }
}

static FILE *lm_file_open (char const *filename, int32 usepipe)
{
    char command[1024];
    FILE *fp;
    
    if (usepipe) {
#ifdef GNUWINCE
	    E_FATAL("No popen() on WinCE\n");
#else
#ifdef WIN32
	sprintf (command, "D:\\compress\\gzip.exe -d -c %s", filename);
	if ((fp = _popen (command, "r")) == NULL)
	    E_FATAL("Cannot popen %s\n", command);
#else
	sprintf (command, "zcat %s", filename);
	if ((fp = popen (command, "r")) == NULL)
	    E_FATAL("Cannot popen %s\n", command);
#endif /* !WIN32 */
#endif /* !GNUWINCE */
    }
    else {
        fp = CM_fopen (filename, "r");
    }
    return (fp);
}

static int32 lm_get_classid (lm_t *model, char *name)
{
    int32 i;
    
    if (! model->lmclass)
	return -1;
    
    for (i = 0; i < model->n_lmclass; i++) {
	if (strcmp (lmclass_getname(model->lmclass[i]), name) == 0)
	    return (i + LM_CLASSID_BASE);
    }
    return -1;
}

/*
 * Read in a trigram language model from the given file.  The LM tokens can be word
 * classes.  However a given actual word can belong to AT MOST ONE of the LM classes
 * used by this LM.
 */
int32 lm_read_clm (char const *filename,
		   char const *lmname,
		   double lw,
		   double uw,
		   double wip,
		   lmclass_t *lmclass,
		   int32 n_lmclass)
{
    lm_t *model;
    FILE *fp = NULL;
    int32 usingPipe = FALSE;
    int32 n_unigram;
    int32 n_bigram;
    int32 n_trigram;
    int32 dict_size;
    int32 i, j, k, last_bg, last_tg;
    char *kbdumpdir, dumpfile[1024];
    struct stat statbuf;
    int32 first_oov = 0, last_oov = -1;
    int32 max_new_oov = 0;
    int32 idfmt;
    int32 dictid, classid, notindict, maperr;
    lmclass_word_t lmclass_word;
    int do_mmap;
    
    E_INFO ("Reading LM file %s (name \"%s\")\n", filename, lmname);

    do_mmap = kb_get_mmap_flag();
    
    /* Make sure no LM with same lmname already exists; if so, delete it */
    if (lmname_to_id (lmname) >= 0)
	lm_delete (lmname);
    
    /* Check if a compressed file */
    k = strlen(filename);
#ifdef WIN32
    usingPipe = (k > 3) &&
	((strcmp (filename+k-3, ".gz") == 0) || (strcmp (filename+k-3, ".GZ") == 0));
#else
    usingPipe = (k > 2) &&
	((strcmp (filename+k-2, ".Z") == 0) || (strcmp (filename+k-2, ".z") == 0));
#endif
    /* Check if an .arpabo-id format file; More HACK!! Hardwired check for -id */
    if (usingPipe)
	k -= 2;
    idfmt = ((k > 3) && (strncmp (filename+k-3, "-id", 3) == 0));

    fp = lm_file_open (filename, usingPipe);
    if (stat(filename, &statbuf) < 0)
      E_FATAL("stat(%s) failed\n", filename);

#ifndef NO_DICT
    WordDict = kb_get_word_dict ();
#endif

    /* Read #unigrams, #bigrams, #trigrams from file */
    ReadNgramCounts (fp, &n_unigram, &n_bigram, &n_trigram);
    E_INFO ("ngrams 1=%d, 2=%d, 3=%d\n", n_unigram, n_bigram, n_trigram);
    
    /* Determine dictionary size (for dict-wid -> LM-wid map) */
#ifdef NO_DICT    
    dict_size = n_unigram;
#else
    dict_size = kb_get_num_words ();
    E_INFO("%d words in dictionary\n", dict_size);

    /*
     * If this is the "BASE" LM also count space for OOVs and words added at run time.
     * UGLY!!  Assumes that OOVs will only be added to LM with no name.
     */
    if (lmname[0] == '\0') {
	first_oov = dict_get_first_initial_oov ();
	last_oov = dict_get_last_initial_oov ();
	n_unigram += (last_oov - first_oov + 1);
    }
    /* Add space for words added in at run time */
    max_new_oov = kb_get_max_new_oov ();
    n_unigram += max_new_oov;
#endif

    if (dict_size >= 65535)
	E_FATAL("#dict-words(%d) > 65534\n", dict_size);

    /* Allocate space for LM, including initial OOVs and placeholders; initialize it */
    model = lmp = NewModel (n_unigram, n_bigram, n_trigram, dict_size);
    word_str = (char **) CM_calloc (n_unigram, sizeof (char *));
    
    /* Create name for binary dump form of Darpa LM file */
    {
#ifdef WIN32
	for (i = strlen(filename)-1;
	     (i >= 0) && (filename[i] != '\\') && (filename[i] != '/'); --i);
#else
	for (i = strlen(filename)-1; (i >= 0) && (filename[i] != '/'); --i);
#endif
	i++;
	kbdumpdir = kb_get_dump_dir();
	/* form dumpfilename */
	if (kbdumpdir)
	    sprintf (dumpfile, "%s/%s.DMP", kbdumpdir, filename+i);
    }
    
    /* Load the precompiled binary dump form of the Darpa LM file if it exists */
    if ((! kbdumpdir) ||
	    (! lm3g_load (dumpfile, model, filename, (int32)statbuf.st_mtime))) {
	do_mmap = 0;
	/*
	 * Allocate one extra unigram and bigram entry: sentinels to terminate
	 * followers (bigrams and trigrams, respectively) of previous entry.
	 */
	model->bigrams	= (bigram_t *) CM_calloc (n_bigram+1, sizeof (bigram_t));
	if (n_trigram > 0)
	    model->trigrams	= (trigram_t *) CM_calloc (n_trigram, sizeof (trigram_t));

	if (n_trigram > 0) {
	    model->tseg_base = (int32 *) CM_calloc ((n_bigram+1)/BG_SEG_SZ+1, sizeof (int32));
#if 0
	    E_INFO("%8d = tseg_base entries allocated\n",
		   (n_bigram+1)/BG_SEG_SZ+1);
#endif
	}
	ReadUnigrams (fp, model);
	E_INFO("%8d = #unigrams created\n", model->ucount);
	
	init_sorted_list (&sorted_prob2);
	if (model->tcount > 0)
	    init_sorted_list (&sorted_bo_wt2);
	
	ReadBigrams (fp, model, idfmt);
	
	model->bcount = FIRST_BG(model,model->ucount);
	model->n_prob2 = sorted_prob2.free;
	model->prob2  = vals_in_sorted_list (&sorted_prob2);
	free_sorted_list (&sorted_prob2);
	E_INFO("\n%8d = #bigrams created\n", model->bcount);
	E_INFO("%8d = #prob2 entries\n", model->n_prob2);
	
	if (model->tcount > 0) {
	    /* Create trigram bo-wts array */
	    model->n_bo_wt2 = sorted_bo_wt2.free;
	    model->bo_wt2 = vals_in_sorted_list (&sorted_bo_wt2);
	    free_sorted_list (&sorted_bo_wt2);
	    E_INFO("%8d = #bo_wt2 entries\n", model->n_bo_wt2);
	    
	    init_sorted_list (&sorted_prob3);
	    
	    ReadTrigrams (fp, model, idfmt);
	    
	    model->tcount = FIRST_TG(model,model->bcount);
	    model->n_prob3 = sorted_prob3.free;
	    model->prob3  = vals_in_sorted_list (&sorted_prob3);
	    E_INFO("\n%8d = #trigrams created\n", model->tcount);
	    E_INFO("%8d = #prob3 entries\n", model->n_prob3);
	    
	    free_sorted_list (&sorted_prob3);
	}
	
	/* HACK!! to avoid unnecessarily creating dump files for small LMs */
	if (kbdumpdir && (model->bcount + model->tcount > 200000))
	    lm3g_dump (dumpfile, model, filename, (int32)statbuf.st_mtime);
    }

    if (usingPipe) {
#ifdef GNUWINCE
	    E_FATAL("No popen() on WinCE!\n");
#else /* !GNUWINCE */
#ifdef WIN32
	_pclose (fp);
#else
	pclose (fp);
#endif /* !WIN32 */
#endif /* !GNUWINCE */
    } else
	fclose (fp);

    /*
     * Make a local copy of the LM Classes used by this LM.  The unigrams_t.mapid
     * field can refer to the index of the local copy array.
     */
    if (n_lmclass > 0) {
	model->lmclass = (lmclass_t *) CM_calloc (n_lmclass, sizeof(lmclass_t));
	for (i = 0; i < n_lmclass; i++)
	    model->lmclass[i] = lmclass[i];
    } else
	model->lmclass = NULL;
    model->n_lmclass = n_lmclass;
    model->inclass_ugscore = (int32 *) CM_calloc (dict_size, sizeof(int32));

    /*
     * Create mapping from dictionary ID to unigram index.  And also mapping for
     * LM wids (to dictionary or LMclass ids) in unigram array.
     */
    for (i = 0; i < model->dict_size; i++)
	model->dictwid_map[i] = LM_DICTWID_BADMAP;
    
    notindict = 0;
    
    maperr = 0;
    for (i = 0; i < model->ucount; i++) {
#ifndef NO_DICT
	model->unigrams[i].mapid = kb_get_word_id (word_str[i]);
#else
	E_FATAL("Cannot run standalone, without a dictionary\n");
#endif
	classid = lm_get_classid (model, word_str[i]);
	
	if (model->unigrams[i].mapid >= 0) {	/* unigram[i] is a dictionary word */
	    if (classid >= 0) {
		E_ERROR("'%s' is both a word and an LM class name\n", word_str[i]);
		maperr = 1;
	    } else
		model->dictwid_map[model->unigrams[i].mapid] = i;
	} else {
	    if (classid >= 0) {			/* unigram[i] is an LM class */
		model->unigrams[i].mapid = classid;
		
		/* Create entries in dictwid_map for each word in the class */
		lmclass_word = lmclass_firstword (LM_CLASSID_TO_CLASS(model,classid));
		while (lmclass_isword(lmclass_word)) {
		    dictid = lmclass_getwid(lmclass_word);
		    if (dictid >= 0) {
			if (model->dictwid_map[dictid] >= 0) {
			    E_ERROR("Multiple mappings of '%s' in LM\n",
				    lmclass_getword(lmclass_word));
			    maperr = 1;
			} else {
			    model->dictwid_map[dictid] = i;
			    model->inclass_ugscore[dictid] =
				lmclass_getprob(lmclass_word)*lw;
			}
		    } else
			notindict++;
		    
		    lmclass_word = lmclass_nextword (LM_CLASSID_TO_CLASS(model,classid),
						     lmclass_word);
		}
	    } else
		notindict++;
	}
    }
    if (maperr)
	E_FATAL("Errors in LM; exiting\n");
    
    if (notindict > 0)
	E_WARN ("%d LM words not in dict; ignored\n", notindict);
    
    /*
     * Discourage expansion of end_sym and transition to start_sym.  (The given
     * Darpa LM may contain some spurious values that don't reflect these
     * requirements.)
     */
    /* bo_wt(</s>) = MIN_PROB_F */
    for (i = 0; (i < model->ucount) && (strcmp (word_str[i], end_sym) != 0); i++);
    E_INFO("bo_wt(%s) changed from %.4f to %.4f\n",
	   word_str[i], model->unigrams[i].bo_wt1.f, MIN_PROB_F);
    model->unigrams[i].bo_wt1.f = MIN_PROB_F;

    /* unigram prob(<s>) = MIN_PROB_F */
    for (i = 0; (i < model->ucount) && (strcmp (word_str[i], start_sym) != 0); i++);
    E_INFO("prob(%s) changed from %.4f to %.4f\n",
	   word_str[i], model->unigrams[i].prob1.f, MIN_PROB_F);
    model->unigrams[i].prob1.f = MIN_PROB_F;

    /* bigram prob(<s>,<s>) = MIN_PROB_F (if bigram exists) */
    j = FIRST_BG(model,i);
    last_bg = LAST_BG(model,i);
    for (; (j<=last_bg) && (strcmp(word_str[BG_WID(model,j)],start_sym)!=0); j++);
    if (j <= last_bg) {
	E_INFO("prob(%s,%s) changed from %.4f to %.4f\n",
	       word_str[i], word_str[BG_WID(model,j)],
	       model->prob2[model->bigrams[j].prob2].f,
	       model->prob2[0].f);
	if (!do_mmap)
	    model->bigrams[j].prob2 = 0;

	if (model->tcount > 0) {
	    /* trigram prob(<s>,<s>,<s>) = MIN_PROB_F (if trigram exists) */
	    k = FIRST_TG(model,j);
	    last_tg = LAST_TG(model,j);
	    for (; k <= last_tg; k++) {
		if (strcmp (word_str[TG_WID(model,k)], start_sym) == 0)
		    break;
	    }
	    if (k <= last_tg) {
		E_INFO("prob(%s,%s,%s) changed from %.4f to %.4f\n",
		       word_str[i], word_str[BG_WID(model,j)], word_str[TG_WID(model,k)], 
		       model->prob3[model->trigrams[k].prob3].f,
		       model->prob3[0].f);
		if (!do_mmap)
		    model->trigrams[k].prob3 = 0;
	    }
	}
    }

    /* bigram prob(<s>,</s>) = MIN_PROB_F (if bigram exists) */
    j = FIRST_BG(model,i);
    last_bg = LAST_BG(model,i);
    for (; (j<=last_bg) && (strcmp(word_str[BG_WID(model,j)],end_sym)!=0); j++);
    if (j <= last_bg) {
	E_INFO("prob(%s,%s) changed from %.4f to %.4f\n",
	       word_str[i], word_str[BG_WID(model,j)],
	       model->prob2[model->bigrams[j].prob2].f,
	       model->prob2[0].f);
	if (!do_mmap)
	    model->bigrams[j].prob2 = 0;
    }
    
    lm_add (lmname, model, lw, uw, wip);
    
    hash_free (&(model->HT));
    if (!do_mmap)
	for (i = 0; i < model->ucount; i++)
	    free (word_str[i]);
    free (word_str);
    
    return 0;
}

int32 lm_read (char const *filename, char const *lmname,
	       double lw, double uw, double wip)
{
    return lm_read_clm(filename, lmname, lw, uw, wip, NULL, 0);
}

void lm_init_oov ( void )
{
    int32 i, j, baseid;
    int32 first_oov = 0, last_oov = -1;
    lm_t *model;
    
    model = lm_name2lm ("");
    
    /* Add initial list of OOV words to LM unigrams */
    first_oov = dict_get_first_initial_oov ();
    last_oov = dict_get_last_initial_oov ();
    E_INFO ("Adding %d initial OOV words to LM\n", last_oov-first_oov+1);

    oov_ugprob = kb_get_oov_ugprob ();

    for (i = first_oov; i <= last_oov; i++) {
	/* Add only base pronunciations */
	if ((baseid = dictid_to_baseid (WordDict, i)) == i) {
	    if ((j = lm_add_word (model, i)) >= 0)
		model->dictwid_map[i] = j;
	}
    }
}

/*
 * Add new word with given dictionary wid and unigram prob = oov_ugprob to
 * model->unigrams.
 * Return LM wid of inserted word if successful, otherwise -1.
 * (Currently some problems with adding alternative pronunciations...)
 */
int32 lm_add_word (lm_t *model, int32 dictwid)
{
    /* Make sure new word not already in LM */
    if (model->dictwid_map[dictwid] >= 0) {
	E_WARN("lm_add_word: Word '%s' already in LM, ignored\n",
		dictid_to_str (WordDict, dictwid));
	return model->dictwid_map[dictwid];
    }

    if (model->ucount >= model->max_ucount) {
	E_ERROR ("lm_add_word(%s) failed; LM full\n",
		 dictid_to_str (WordDict, dictwid));
	return -1;
    }
    
    /* Append new word to unigrams */
    model->unigrams[model->ucount].mapid = dictwid;
    model->unigrams[model->ucount].prob1.l =
	    LWMUL(LOG10TOLOG(oov_ugprob), model->lw) + model->log_wip;
    model->unigrams[model->ucount].bo_wt1.l =
	    LWMUL(LOG10TOLOG(0.0), model->lw);

    /* Advance the sentinel unigram */
    model->unigrams[model->ucount+1].bigrams = model->unigrams[model->ucount].bigrams;

    /* Update dictwid_map for this LM */
    model->dictwid_map[dictwid] = model->ucount;
    
    return (model->ucount++);
}

/*
 * Add named model to list of models.  If another with same name exists, delete it first.
 */
void lm_add (char const *lmname, lm_t *model, double lw, double uw, double wip)
{
    if (lmname_to_id (lmname) >= 0)
	lm_delete (lmname);
    
    model->tginfo = (tginfo_t **) CM_calloc (model->max_ucount, sizeof(tginfo_t *));
    
    if (n_lm == n_lm_alloc) {
	lmset = (struct lmset_s *) CM_recalloc (lmset, n_lm+15, sizeof(struct lmset_s));
	n_lm_alloc += 15;
    }
    lmset[n_lm].lm = model;
    lmset[n_lm].name = salloc (lmname);
    
    lm_set_param (model, lw, uw, wip, FALSE);
    
    n_lm++;
    
    E_INFO ("LM(\"%s\") added\n", lmname);
}

/*
 * Delete named LM from list of LMs and reclaim all space.
 */
int32 lm_delete (char const *name)
{
    int32 i, u;
    lm_t *model;
    tginfo_t *tginfo, *next_tginfo;
    
    if ((i = lmname_to_id (name)) < 0)
	return (-1);
    
    model = lmset[i].lm;
    free (model->unigrams);
    free (model->bigrams);
    free (model->prob2);
    if (model->tcount > 0) {
	free (model->trigrams);
	free (model->tseg_base);
	free (model->bo_wt2);
	free (model->prob3);
    }
    if (model->HT.tab != NULL)
	hash_free (&model->HT);
    
    for (u = 0; u < model->max_ucount; u++)
	for (tginfo = model->tginfo[u]; tginfo; tginfo = next_tginfo) {
	    next_tginfo = tginfo->next;
	    listelem_free ((void *)tginfo, sizeof(tginfo_t));
	}
    free (model->tginfo);

    if (model->lmclass)
	free (model->lmclass);
    free (model->inclass_ugscore);
    
    free (model->dictwid_map);
    
    free (model);
    
    free (lmset[i].name);
    
    for (; i < n_lm-1; i++)
	lmset[i] = lmset[i+1];
    --n_lm;
    
    E_INFO ("LM(\"%s\") deleted\n", name);
    
    return (0);
}

/*
 * Set the active LM to the one identified by "name".  Return 0 if successful,
 * -1 otherwise.
 */
int32 lm_set_current (char const *name)
{
    int32 i;
    
    if ((i = lmname_to_id (name)) < 0)
	return (-1);
    
    lmp = lmset[i].lm;
    
#ifdef USE_ILM
    ilm_set_lm (lmp);
#endif
    
    return (0);
}

static int32 lmname_to_id (char const *name)
{
    int32 i;
    
    for (i = 0; (i < n_lm) && (strcmp (lmset[i].name, name) != 0); i++);
    return ((i < n_lm) ? i : -1);
}

lm_t *lm_name2lm (char const *name)
{
    int32 i;
    
    i = lmname_to_id (name);
    return ((i >= 0) ? lmset[i].lm : NULL);
}

char *get_current_lmname ()
{
    int32 i;
    
    for (i = 0; (i < n_lm) && (lmset[i].lm != lmp); i++);
    return ((i < n_lm) ? lmset[i].name : NULL);
}

lm_t *lm_get_current ()
{
    return (lmp);
}

int32 get_n_lm ()
{
    return (n_lm);
}

int32 lm3g_n_lm ( void )
{
    return n_lm;
}

char *lm3g_index2name (int k)
{
    if ((k >= 0) && (k < n_lm))
	return (lmset[k].name);
    else
	return NULL;
}

/*
 * dict base wid; check if present in LM.  
 * return TRUE if present, FALSE otherwise.
 */
int32 dictwd_in_lm (wid)
    int32 wid;
{
    return (lmp->dictwid_map[wid] >= 0);
}

/*
 * Load pre-compiled trigram LM file, if it exists, into model.  If file
 * does not exist return 0.  Otherwise, if successful, return 1.
 */
static int32 lm3g_load (char const *file, lm_t *model,
			char const *lmfile, int32 mtime)
{
    int32 i, j, k, vn, ts, err, do_swap;
    FILE *fp;
    char str[1024];
    unigram_t *ugptr;
    bigram_t *bgptr;
    trigram_t *tgptr;
    char *tmp_word_str;
    int do_mmap, fd = -1;
    char *map_base = NULL;
    size_t offset = 0, filesize;
    
    err = 0;
    E_INFO ("Looking for precompiled LM dump file %s\n", file);
    
    if ((fp = fopen (file, "rb")) == NULL) {
      E_INFO("Precompiled file not found; continue with LM file\n");
      return (0);
    }

    k = fread_int32 (fp, strlen(darpa_hdr)+1, strlen(darpa_hdr)+1, "header size",
		     0, &do_swap);
    if (fread (str, sizeof (char), k, fp) != (size_t) k)
      E_FATAL("Cannot read header\n");
    if (strncmp (str, darpa_hdr, k) != 0)
	E_FATAL("Wrong header %s\n", darpa_hdr);
    E_INFO("%s\n", str);
    
    do_mmap = kb_get_mmap_flag();
    if (do_mmap) {
	if (do_swap) {
	    E_INFO("Byteswapping required, will not use memory-mapped I/O for LM file\n");
	    do_mmap = 0;
	}
	else {
	    E_INFO("Will use memory-mapped I/O for LM file\n");
	    fd = fileno(fp);
	}
    }
    
    k = fread_int32 (fp, 1, 1023, "LM filename size", 1, &do_swap);
    if (fread (str, sizeof (char), k, fp) != (size_t) k)
	E_FATAL("Cannot read LM filename in header\n");
#if 0
    if (strncmp (str, lmfile, k) != 0)
	E_WARN ("LM filename in header = %s\n", str);
#endif

    /* read version#, if present (must be <= 0) */
    vn = fread_int32 (fp, (int32)0x80000000, 0x7fffffff, "version#", 1, &do_swap);
    if (vn <= 0) {
	/* read and compare timestamps */
	ts = fread_int32 (fp, (int32)0x80000000, 0x7fffffff, "timestamp", 1, &do_swap);
	if (ts < mtime) {
	  E_WARN("**WARNING** LM file newer than dump file\n");
	}
	
	/* read and skip format description */
	for (;;) {
	    k = fread_int32 (fp, 0, 1023, "string length", 1, &do_swap);
	    if (k == 0)
		break;
	    if (fread (str, sizeof(char), k, fp) != (size_t) k)
		E_FATAL("fread(word) failed\n");
	}
	/* read model->ucount */
	model->ucount = fread_int32 (fp, 1, model->ucount, "LM.ucount", 1, &do_swap);
    } else {
	/* oldest dump file version has no version# or any of the above */
	if (vn > model->ucount)
	    E_FATAL("LM.ucount(%d) out of range [1..%d]\n", vn, model->ucount);
	model->ucount = vn;
    }
    
    /* read model->bcount, tcount */
    model->bcount = fread_int32 (fp, 1, model->bcount, "LM.bcount", 1, &do_swap);
    model->tcount = fread_int32 (fp, 0, model->tcount, "LM.tcount", 1, &do_swap);
    E_INFO ("ngrams 1=%d, 2=%d, 3=%d\n",
	    model->ucount, model->bcount, model->tcount);

    /* read unigrams (always in memory, as they contain dictionary
     * mappings that can't be precomputed) */
    if (fread (model->unigrams, sizeof(unigram_t), model->ucount+1, fp)
	!= (size_t) model->ucount+1)
	E_FATAL("fread(unigrams) failed\n");
    if (do_swap) {
	for (i = 0, ugptr = model->unigrams; i <= model->ucount; i++, ugptr++) {
	    SWAP_INT32(&ugptr->mapid);
	    SWAP_INT32(&ugptr->prob1.l);
	    SWAP_INT32(&ugptr->bo_wt1.l);
	    SWAP_INT32(&ugptr->bigrams);
	}
    }
    for (i = 0, ugptr = model->unigrams; i < model->ucount; i++, ugptr++) {
	if (ugptr->mapid != i)
	    err = 1;
	ugptr->mapid = i;
    }
    if (err)
	E_WARN ("Corrected corrupted dump file created by buggy fbs8\n");
    E_INFO("%8d = LM.unigrams(+trailer) read\n", model->ucount);

    /* Now mmap() the file and read in the rest of the (read-only) stuff. */
    if (do_mmap) {
	offset = ftell(fp);
	fseek(fp, 0, SEEK_END);
	filesize = ftell(fp);
	fseek(fp, offset, SEEK_SET);

	map_base = s2_mmap(file);
    }

    /* read bigrams */
    if (do_mmap) {
	model->bigrams = (bigram_t *) (map_base + offset);
	offset += (model->bcount+1)*sizeof(bigram_t);
    }
    else {
	model->bigrams	= (bigram_t *) CM_calloc (model->bcount+1, sizeof (bigram_t));
	if (fread (model->bigrams, sizeof(bigram_t), model->bcount+1, fp)
	    != (size_t) model->bcount+1)
	    E_FATAL("fread(bigrams) failed\n");
	if (do_swap) {
	    for (i = 0, bgptr = model->bigrams; i <= model->bcount; i++, bgptr++) {
		SWAP_INT16(&bgptr->wid);
		SWAP_INT16(&bgptr->prob2);
		SWAP_INT16(&bgptr->bo_wt2);
		SWAP_INT16(&bgptr->trigrams);
	    }
	}
    }
    E_INFO("%8d = LM.bigrams(+trailer) read\n", model->bcount);
    
    /* read trigrams */
    if (model->tcount > 0) {
	if (do_mmap) {
	    model->trigrams = (trigram_t *) (map_base + offset);
	    offset += model->tcount*sizeof(trigram_t);
	}
	else {
	    model->trigrams = (trigram_t *) CM_calloc (model->tcount, sizeof (trigram_t));
	    if (fread(model->trigrams, sizeof(trigram_t), model->tcount, fp)
		!= (size_t) model->tcount)
		E_FATAL("fread(trigrams) failed\n");
	    if (do_swap) {
		for (i = 0, tgptr = model->trigrams; i < model->tcount; i++, tgptr++) {
		    SWAP_INT16(&tgptr->wid);
		    SWAP_INT16(&tgptr->prob3);
		}
	    }
	}
	E_INFO("%8d = LM.trigrams read\n", model->tcount);
    }
    
    /* read n_prob2 and prob2 array (in memory, should be pre-scaled on disk) */
    if (do_mmap)
	fseek(fp, offset, SEEK_SET);
    model->n_prob2 = k = fread_int32 (fp, 1, 65535, "LM.n_prob2", 1, &do_swap);
    model->prob2 = (log_t *) CM_calloc (k, sizeof (log_t));
    if (fread (model->prob2, sizeof (log_t), k, fp) != (size_t) k)
	E_FATAL("fread(prob2) failed\n");
    if (do_swap)
	for (i = 0; i < k; i++)
	    SWAP_INT32(&model->prob2[i].l);
    E_INFO("%8d = LM.prob2 entries read\n", k);

    /* read n_bo_wt2 and bo_wt2 array (in memory) */
    if (model->tcount > 0) {
	k = fread_int32 (fp, 1, 65535, "LM.n_bo_wt2", 1, &do_swap);
	model->n_bo_wt2 = k;
	model->bo_wt2 = (log_t *) CM_calloc (k, sizeof (log_t));
	if (fread (model->bo_wt2, sizeof (log_t), k, fp) != (size_t) k)
	    E_FATAL("fread(bo_wt2) failed\n");
	if (do_swap)
	    for (i = 0; i < k; i++)
		SWAP_INT32(&model->bo_wt2[i].l);
	E_INFO("%8d = LM.bo_wt2 entries read\n", k);
    }
	
    /* read n_prob3 and prob3 array (in memory) */
    if (model->tcount > 0) {
	k = fread_int32 (fp, 1, 65535, "LM.n_prob3", 1, &do_swap);
	model->n_prob3 = k;
	model->prob3 = (log_t *) CM_calloc (k, sizeof (log_t));
	if (fread (model->prob3, sizeof (log_t), k, fp) != (size_t) k)
	    E_FATAL("fread(prob3) failed\n");
	if (do_swap)
	    for (i = 0; i < k; i++)
		SWAP_INT32(&model->prob3[i].l);
	E_INFO("%8d = LM.prob3 entries read\n", k);
    }
    
    /* read tseg_base size and tseg_base */
    /* FIXME: There could be alignment issues here. */
    if (do_mmap)
	offset = ftell(fp);
    if (model->tcount > 0) {
	if (do_mmap) {
	    k = *(int32 *)(map_base + offset);
	    offset += sizeof(int32);
	    model->tseg_base = (int32 *) (map_base + offset);
	    offset += k * sizeof(int32);
	}
	else {
	    k = (model->bcount+1)/BG_SEG_SZ + 1;
	    k = fread_int32 (fp, k, k, "tseg_base size", 1, &do_swap);
	    model->tseg_base = (int32 *) CM_calloc (k, sizeof (int32));
	    if (fread (model->tseg_base, sizeof(int32), k, fp) != (size_t) k)
		E_FATAL("fread(tseg_base) failed\n");
	    if (do_swap)
		for (i = 0; i < k; i++)
		    SWAP_INT32(&model->tseg_base[i]);
	}
	E_INFO("%8d = LM.tseg_base entries read\n", k);
    }

    /* read ascii word strings */
    if (do_mmap) {
	k = *(int32 *)(map_base + offset);
	offset += sizeof(int32);
	tmp_word_str = (char *)(map_base + offset);
	offset += k;
    }
    else {
	k = fread_int32 (fp, 1, 0x7fffffff, "words string-length", 1, &do_swap);
	tmp_word_str = (char *) CM_calloc (k, sizeof (char));
	if (fread (tmp_word_str, sizeof(char), k, fp) != (size_t) k)
	    E_FATAL("fread(word-string) failed\n");
    }

    /* First make sure string just read contains ucount words (PARANOIA!!) */
    for (i = 0, j = 0; i < k; i++)
	if (tmp_word_str[i] == '\0')
	    j++;
    if (j != model->ucount)
	E_FATAL("Error reading word strings\n");

    /* Break up string just read into words */
    if (do_mmap) {
	j = 0;
	for (i = 0; i < model->ucount; i++) {
	    word_str[i] = tmp_word_str + j;
	    j += strlen(word_str[i]) + 1;
	}
    }
    else {
	j = 0;
	for (i = 0; i < model->ucount; i++) {
	    word_str[i] = (char *) salloc (tmp_word_str+j);
	    j += strlen(word_str[i]) + 1;
	}
	free (tmp_word_str);
    }
    E_INFO("%8d = ascii word strings read\n", i);

    return (1);
}

static char const *fmtdesc[] = {
    "BEGIN FILE FORMAT DESCRIPTION",
    "Header string length (int32) and string (including trailing 0)",
    "Original LM filename string-length (int32) and filename (including trailing 0)",
    "(int32) version number (present iff value <= 0)",
    "(int32) original LM file modification timestamp (iff version# present)",
    "(int32) string-length and string (including trailing 0) (iff version# present)",
    "... previous entry continued any number of times (iff version# present)",
    "(int32) 0 (terminating sequence of strings) (iff version# present)",
    "(int32) lm_t.ucount (must be > 0)",
    "(int32) lm_t.bcount",
    "(int32) lm_t.tcount",
    "lm_t.ucount+1 unigrams (including sentinel)",
    "lm_t.bcount+1 bigrams (including sentinel)",
    "lm_t.tcount trigrams (present iff lm_t.tcount > 0)",
    "(int32) lm_t.n_prob2",
    "(int32) lm_t.prob2[]",
    "(int32) lm_t.n_bo_wt2 (present iff lm_t.tcount > 0)",
    "(int32) lm_t.bo_wt2[] (present iff lm_t.tcount > 0)",
    "(int32) lm_t.n_prob3 (present iff lm_t.tcount > 0)",
    "(int32) lm_t.prob3[] (present iff lm_t.tcount > 0)",
    "(int32) (lm_t.bcount+1)/BG_SEG_SZ+1 (present iff lm_t.tcount > 0)",
    "(int32) lm_t.tseg_base[] (present iff lm_t.tcount > 0)",
    "(int32) Sum(all word string-lengths, including trailing 0 for each)",
    "All word strings (including trailing 0 for each)",
    "END FILE FORMAT DESCRIPTION",
    NULL,
};

/*
 * Dump internal LM to file.  Format described above
 * We don't swap bytes because it could be mmap()ed
 */
static int32 lm3g_dump (char const *file, lm_t *model,
			char const *lmfile, int32 mtime)
{
    int32 i, k, zero=0;
    FILE *fp;

    E_INFO ("Dumping LM to %s\n", file);
    if ((fp = fopen (file, "wb")) == NULL) {
	E_ERROR ("Cannot create file %s\n", file);
	return 0;
    }

    k = strlen(darpa_hdr)+1;
    fwrite_int32 (fp, k);
    fwrite (darpa_hdr, sizeof(char), k, fp);

    k = strlen(lmfile)+1;
    fwrite_int32 (fp, k);
    fwrite (lmfile, sizeof(char), k, fp);
    
    /* Write version# and LM file modification date */
    fwrite_int32 (fp, -1);	/* version # */
    fwrite_int32 (fp, mtime);
    
    /* Write file format description into header */
    for (i = 0; fmtdesc[i] != NULL; i++) {
	k = strlen(fmtdesc[i])+1;
	fwrite_int32 (fp, k);
	fwrite (fmtdesc[i], sizeof(char), k, fp);
    }
    /* Pad it out for alignment purposes */
    k = ftell(fp) & 3;
    if (k > 0) {
	fwrite_int32(fp, 4-k);
	fwrite(&zero, 1, 4-k, fp);
    }
    fwrite_int32 (fp, 0);
    
    fwrite_int32 (fp, model->ucount);
    fwrite_int32 (fp, model->bcount);
    fwrite_int32 (fp, model->tcount);

    for (i = 0; i <= model->ucount; i++)
	fwrite (&(model->unigrams[i]), sizeof(model->unigrams[i]), 1, fp);
    for (i = 0; i <= model->bcount; i++)
	fwrite (&(model->bigrams[i]), sizeof(model->bigrams[i]), 1, fp);
    for (i = 0; i < model->tcount; i++)
	fwrite (&(model->trigrams[i]), sizeof(model->trigrams[i]), 1, fp);
    
    fwrite_int32 (fp, model->n_prob2);
    for (i = 0; i < model->n_prob2; i++)
	fwrite_int32 (fp, model->prob2[i].l);
    
    if (model->tcount > 0) {
	fwrite_int32 (fp, model->n_bo_wt2);
	for (i = 0; i < model->n_bo_wt2; i++)
	    fwrite_int32 (fp, model->bo_wt2[i].l);
	fwrite_int32 (fp, model->n_prob3);
	for (i = 0; i < model->n_prob3; i++)
	    fwrite_int32 (fp, model->prob3[i].l);
	
	k = (model->bcount+1)/BG_SEG_SZ + 1;
	fwrite_int32 (fp, k);
	for (i = 0; i < k; i++)
	    fwrite_int32 (fp, model->tseg_base[i]);
    }
    
    k = 0;
    for (i = 0; i < model->ucount; i++)
	k += strlen(word_str[i])+1;
    fwrite_int32 (fp, k);
    for (i = 0; i < model->ucount; i++)
	fwrite (word_str[i], sizeof(char), strlen(word_str[i])+1, fp);

    fclose (fp);
    return 0;
}

void lmSetStartSym (char const *sym)
/*----------------------------*
 * Description - reconfigure the start symbol
 */
{
    start_sym = (char *) salloc(sym);
}

void lmSetEndSym (char const *sym)
/*----------------------------*
 * Description - reconfigure the end symbol
 */
{
    end_sym = (char *) salloc(sym);
}

/*
 * Convert probs and backoff weights to LOG quantities, add language weight
 * and insertion penalty.
 */
static void lm_set_param (lm_t *model, double lw, double uw,
			  double wip, int32 word_pair)
{
    int32 i;
    int32 tmp1, tmp2;
    int32 logUW, logOneMinusUW, logUniform;
    int16 *at = Addition_Table;
    int32 ts = Table_Size;

    model->lw = FLOAT2LW(lw);
    model->invlw = FLOAT2LW(1.0/lw);
    model->uw = uw;
    model->log_wip = LOG(wip);
    E_INFO ("%8.2f = Language Weight\n", LW2FLOAT(model->lw));
    E_INFO ("%8.2f = Unigram Weight\n", model->uw);
    E_INFO ("%8d = LOG (Insertion Penalty (%.2f))\n", model->log_wip, wip);
    
    logUW = LOG(model->uw);
    logOneMinusUW = LOG(1.0 - model->uw);
    logUniform = LOG(1.0/(model->ucount-1));	/* -1 for ignoring <s> */
    
    if (word_pair)
	E_FATAL("word-pair LM not implemented\n");

    for (i = 0; i < model->ucount; i++) {
	model->unigrams[i].bo_wt1.l =
		(LOG10TOLOG(UG_BO_WT_F(model,i)) * lw);

	/* Interpolate LM unigram prob with uniform prob (except start_sym) */
	if (strcmp (word_str[i], start_sym) == 0) {
	    model->unigrams[i].prob1.l =
		    (LOG10TOLOG(UG_PROB_F(model,i)) * lw) + model->log_wip;
	} else {
	    tmp1 = (LOG10TOLOG(UG_PROB_F(model,i))) + logUW;
	    tmp2 = logUniform + logOneMinusUW;
	    FAST_ADD (tmp1,tmp1,tmp2,at,ts);
	    model->unigrams[i].prob1.l =
		    (tmp1 * lw) + model->log_wip;
	}
    }

    for (i = 0; i < model->n_prob2; i++) {
	model->prob2[i].l =
		(LOG10TOLOG(model->prob2[i].f) * lw) + model->log_wip;
    }
    if (model->tcount > 0) {
	for (i = 0; i < model->n_bo_wt2; i++) {
	    model->bo_wt2[i].l =
		    (LOG10TOLOG(model->bo_wt2[i].f) * lw);
	}
    }
    
    if (model->tcount > 0) {
	for (i = 0; i < model->n_prob3; i++) {
	    model->prob3[i].l =
		    (LOG10TOLOG(model->prob3[i].f) * lw) + model->log_wip;
	}
    }
}

#ifdef NO_DICT
main (argc, argv)
    int32 argc;
    char *argv[];
{
    lm_t *model;
    int32 i, n, score;
    float lw, uw, wip;
    char wd[3][100], line[1000];
    int32 wid[3];
    int32 unkwid;
    
    if (argc < 4)
	E_FATAL("Usage: %s <lw> <uw> <wip> [LM-file]\n", argv[0]);
    if (sscanf(argv[1], "%f", &lw) != 1)
	E_FATAL("Usage: %s <lw> <uw> <wip> [LM-file]\n", argv[0]);
    if (sscanf(argv[2], "%f", &uw) != 1)
	E_FATAL("Usage: %s <lw> <uw> <wip> [LM-file]\n", argv[0]);
    if (sscanf(argv[3], "%f", &wip) != 1)
	E_FATAL("Usage: %s <lw> <uw> <wip> [LM-file]\n", argv[0]);
    if (argc == 5)
	lm_read(argv[4], lw, uw, wip, 1.0);
    else
	lm_read("/net/alf8/cdrom/wsj1/grammar/tgboc20o.nvp", lw, uw, wip, 1.0);
	
    lmSetParameters (model, 0);
    
    unkwid = wstr2wid (model, "<UNK>");
    for (;;) {
	printf ("Enter 1, 2, or 3 words: ");
	fgets (line, sizeof(line), stdin);
	if (((n = sscanf (line, "%s %s %s", wd[0], wd[1], wd[2])) < 1) || (n > 3))
	    break;
	for (i = 0; i < n; i++)
	    if ((wid[i] = wstr2wid (model, wd[i])) == NO_WORD) {
		printf ("  %s -> <UNK>\n", wd[i]);
		wid[i] = unkwid;
	    }
	
	switch (n) {
	case 1:
	    score = unigram_score (wid[0]);
	    printf ("unigram_score(%d) = %d\n",
		    wid[0], score);
	    break;
	case 2:
	    score = bigram_score (wid[0], wid[1]);
	    printf ("bigram_score(%d, %d) = %d\n",
		    wid[0], wid[1], score);
	    break;
	case 3:
	    score = trigram_score (wid[0], wid[1], wid[2]);
	    printf ("trigram_score(%d, %d, %d) = %d\n",
		    wid[0], wid[1], wid[2], score);
	    break;
	default:
	    break;
	}
    }
}
#endif

#define BINARY_SEARCH_THRESH	16

int32 lm3g_ug_score (int32 wid)
{
    int32 lwid;
    
    if ((lwid = lmp->dictwid_map[wid]) < 0)
	E_FATAL("dictwid[%d] not in LM\n", wid);

    lm_last_access_type = LM3G_ACCESS_UG;
    
    return (lmp->unigrams[lwid].prob1.l + lmp->inclass_ugscore[wid]);
}

/* Locate a specific bigram within a bigram list */
static int32 find_bg (bigram_t *bg, int32 n, int32 w)
{
    int32 i, b, e;
    
    /* Binary search until segment size < threshold */
    b = 0;
    e = n;
    while (e-b > BINARY_SEARCH_THRESH) {
	i = (b+e)>>1;
	if (bg[i].wid < w)
	    b = i+1;
	else if (bg[i].wid > w)
	    e = i;
	else
	    return i;
    }

    /* Linear search within narrowed segment */
    for (i = b; (i < e) && (bg[i].wid != w); i++);
    return ((i < e) ? i : -1);
}

/* w1, w2 are dictionary (base-)word ids */
int32 lm3g_bg_score (int32 w1, int32 w2)
{
    int32 lw1, lw2, i, n, b, score;
    lm_t *lm;
    bigram_t *bg;
    
    lm = lmp;
    
    /* lm->n_bg_score++; */
    
    if ((lw1 = lm->dictwid_map[w1]) < 0)
	E_FATAL("dictwid[%d] not in LM\n", w1);
    if ((lw2 = lm->dictwid_map[w2]) < 0)
	E_FATAL("dictwid[%d] not in LM\n", w2);
    
    b = FIRST_BG(lm, lw1);
    n = FIRST_BG(lm, lw1+1) - b;
    bg = lm->bigrams + b;
    
    if ((i = find_bg (bg, n, lw2)) >= 0) {
	score = lm->prob2[bg[i].prob2].l;
	lm_last_access_type = LM3G_ACCESS_BG;
    } else {
	/* lm->n_bg_bo++; */
	score = lm->unigrams[lw1].bo_wt1.l + lm->unigrams[lw2].prob1.l;
	lm_last_access_type = LM3G_ACCESS_UG;
    }
    score += lm->inclass_ugscore[w2];

#if 0
    printf ("      %5d %5d -> %8d (%16s %16s)\n", w1, w2, score,
	    WordDict->dict_list[UG_MAPID(lm,lw1)]->word,
	    WordDict->dict_list[UG_MAPID(lm,lw2)]->word);
#endif

    return (score);
}

static void load_tginfo (lm_t *lm, int32 lw1, int32 lw2)
{
    int32 i, n, b, t;
    bigram_t *bg;
    tginfo_t *tginfo;
    
    /* First allocate space for tg information for bg lw1,lw2 */
    tginfo = (tginfo_t *) listelem_alloc (sizeof(tginfo_t));
    tginfo->w1 = lw1;
    tginfo->tg = NULL;
    tginfo->next = lm->tginfo[lw2];
    lm->tginfo[lw2] = tginfo;
    
    /* Locate bigram lw1,lw2 */

    b = lm->unigrams[lw1].bigrams;
    n = lm->unigrams[lw1+1].bigrams - b;
    bg = lm->bigrams + b;
    
    if ((n > 0) && ((i = find_bg (bg, n, lw2)) >= 0)) {
	tginfo->bowt = lm->bo_wt2[bg[i].bo_wt2].l;
	
	/* Find t = Absolute first trigram index for bigram lw1,lw2 */
	b += i;			/* b = Absolute index of bigram lw1,lw2 on disk */
	t = FIRST_TG(lm, b);

	tginfo->tg = lm->trigrams + t;
	
	/* Find #tg for bigram w1,w2 */
	tginfo->n_tg = FIRST_TG(lm, b+1) - t;
    } else {			/* No bigram w1,w2 */
	tginfo->bowt = 0;
	tginfo->n_tg = 0;
    }
}

/* Similar to find_bg */
static int32 find_tg (trigram_t *tg, int32 n, int32 w)
{
    int32 i, b, e;
    
    b = 0;
    e = n;
    while (e-b > BINARY_SEARCH_THRESH) {
	i = (b+e)>>1;
	if (tg[i].wid < w)
	    b = i+1;
	else if (tg[i].wid > w)
	    e = i;
	else
	    return i;
    }
    
    for (i = b; (i < e) && (tg[i].wid != w); i++);
    return ((i < e) ? i : -1);
}

/* w1, w2, w3 are dictionary wids */
int32 lm3g_tg_score (int32 w1, int32 w2, int32 w3)
{
    int32 lw1, lw2, lw3, i, n, score;
    lm_t *lm;
    trigram_t *tg;
    tginfo_t *tginfo, *prev_tginfo;
    
    lm = lmp;
    
    if ((lm->tcount <= 0) || (w1 < 0))
	return (lm3g_bg_score (w2, w3));
    
    /* lm->n_tg_score++; */
    
    if ((lw1 = lm->dictwid_map[w1]) < 0)
	E_FATAL("dictwid[%d] not in LM\n", w1);
    if ((lw2 = lm->dictwid_map[w2]) < 0)
	E_FATAL("dictwid[%d] not in LM\n", w2);
    if ((lw3 = lm->dictwid_map[w3]) < 0)
	E_FATAL("dictwid[%d] not in LM\n", w3);
    
    prev_tginfo = NULL;
    for (tginfo = lm->tginfo[lw2]; tginfo; tginfo = tginfo->next) {
	if (tginfo->w1 == lw1)
	    break;
	prev_tginfo = tginfo;
    }
    
    if (! tginfo) {
    	load_tginfo (lm, lw1, lw2);
	tginfo = lm->tginfo[lw2];
    } else if (prev_tginfo) {
	prev_tginfo->next = tginfo->next;
	tginfo->next = lm->tginfo[lw2];
	lm->tginfo[lw2] = tginfo;
    }

    tginfo->used = 1;
    
    /* Trigrams for w1,w2 now pointed to by tginfo */
    n = tginfo->n_tg;
    tg = tginfo->tg;
    if ((i = find_tg (tg, n, lw3)) >= 0) {
	score = lm->prob3[tg[i].prob3].l + lm->inclass_ugscore[w3];
	lm_last_access_type = LM3G_ACCESS_TG;
    } else {
	/* lm->n_tg_bo++; */
	score = tginfo->bowt + lm3g_bg_score(w2, w3);
    }

#if 0
    printf ("%5d %5d %5d -> %8d (%16s %16s %16s)\n", w1, w2, w3, score,
	    WordDict->dict_list[UG_MAPID(lm,lw1)]->word,
	    WordDict->dict_list[UG_MAPID(lm,lw2)]->word,
	    WordDict->dict_list[UG_MAPID(lm,lw3)]->word);
#endif

    return (score);
}

void lm3g_cache_reset ( void )
{
    int32 i;
    lm_t *lm;
    tginfo_t *tginfo, *next_tginfo, *prev_tginfo;
    
    lm = lmp;
    for (i = 0; i < lm->ucount; i++) {
	prev_tginfo = NULL;
	for (tginfo = lm->tginfo[i]; tginfo; tginfo = next_tginfo) {
	    next_tginfo = tginfo->next;
	    
	    if (! tginfo->used) {
		/* lm->n_tg_inmem -= tginfo->n_tg; */
		listelem_free ((void *) tginfo, sizeof(tginfo_t));
		if (prev_tginfo)
		    prev_tginfo->next = next_tginfo;
		else
		    lm->tginfo[i] = next_tginfo;
		
		/* n_tgfree++; */
	    } else {
		tginfo->used = 0;
		prev_tginfo = tginfo;
	    }
	}
    }
}

void lm3g_cache_stats_dump (FILE *file)
{
    /* FIXME: does nothing! */
}

void lm_next_frame ( void )
{
}

int32 lm3g_raw_score (int32 score)
{
    score -= lmp->log_wip;
    score = LWMUL(score,lmp->invlw);
    
    return score;
}

int32 lm3g_access_type ( void )
{
    return lm_last_access_type;
}
