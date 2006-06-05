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
 * cache_lm.c -- Dynamic cache language model based on Roni Rosenfeld's work.
 *
 * HISTORY
 * 
 * $Log: cache_lm.c,v $
 * Revision 1.1.1.1  2006/05/23 18:44:59  dhuggins
 * re-importation
 *
 * Revision 1.8  2004/12/10 16:48:56  rkm
 * Added continuous density acoustic model handling
 *
 * 
 * 01-Apr-97	M K Ravishankar (rkm@cs) at Carnegie Mellon University
 * 		Started, based on earlier FBS6 version.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "s2types.h"
#include "CM_macros.h"
#include "basic_types.h"
#include "search_const.h"
#include "msd.h"
#include "log.h"
#include "cache_lm.h"
#include "linklist.h"
#include "list.h"
#include "hash.h"
#include "dict.h"
#include "lmclass.h"
#include "lm_3g.h"
#include "kb.h"
#include "err.h"

static int32 log0;
static int32 *log_count_tbl = NULL;
#define LOG_COUNT_TBLSIZE	4096
#define LOG_COUNT(x) (((x) >= LOG_COUNT_TBLSIZE) ? LOG((double)(x)) : log_count_tbl[(x)])

/*
 * Cache LM initialize.
 */
cache_lm_t *cache_lm_init (double ug_thresh,
			   double min_uw, double max_uw, int32 nwd_uwrange,
			   double bw)
{
    cache_lm_t *lm;
    int32 i;
    
    lm = (cache_lm_t *) CM_calloc (1, sizeof (cache_lm_t));

    lm->ugprob_thresh = LOG(ug_thresh);

    lm->min_uw = min_uw;
    lm->max_uw = max_uw;
    lm->per_word_uw = (max_uw - min_uw) / (double)nwd_uwrange;
    lm->uw_ugcount_limit = nwd_uwrange;

    lm->uw = min_uw;
    lm->log_uw = LOG(min_uw);
    lm->bw = bw;
    lm->log_bw = LOG(bw);

    lm->log_remwt = LOG(1.0 - min_uw - bw);
    
    lm->n_word = dict_maxsize ();
    lm->clm_ug = (clm_ug_t *) CM_calloc (lm->n_word, sizeof(clm_ug_t));
    lm->sum_ugcount = 0;

    log0 = LOG(0.0);
    if (! log_count_tbl) {
	log_count_tbl = (int32 *) CM_calloc (LOG_COUNT_TBLSIZE, sizeof(int32));
	for (i = 0; i < LOG_COUNT_TBLSIZE; i++)
	    log_count_tbl[i] = LOG((double) i);
    }

    return lm;
}

void cache_lm_reset (cache_lm_t *lm)
{
    clm_bg_t *bg, *nextbg;
    int32 i;
    
    for (i = 0; i < lm->n_word; i++) {
	for (bg = lm->clm_ug[i].w2list; bg; bg = nextbg) {
	    nextbg = bg->next;
	    listelem_free (bg, sizeof(clm_bg_t));
	}
	lm->clm_ug[i].w2list = NULL;
	lm->clm_ug[i].count = 0;
	lm->clm_ug[i].sum_w2count = 0;
    }
    lm->sum_ugcount = 0;
    
    lm->log_uw = LOG(lm->min_uw);
    lm->log_remwt = LOG(1.0 - lm->min_uw - lm->bw);
}

void cache_lm_add_ug (cache_lm_t *lm, int32 w)
{
    lm->clm_ug[w].count++;
    lm->sum_ugcount++;

    if (lm->sum_ugcount > lm->uw_ugcount_limit)
	return;
    
    /* Still within linear uw region; update unigram cache weight */
    lm->uw += lm->per_word_uw;
    lm->log_uw = LOG(lm->uw);
    lm->log_remwt = LOG(1.0 - lm->uw - lm->bw);
}

void cache_lm_add_bg (cache_lm_t *lm, int32 w1, int32 w2)
{
    clm_bg_t *bg;

    /* Find cache LM bigram entry for w1,w2 */
    for (bg = lm->clm_ug[w1].w2list; bg && (bg->w2 != w2); bg = bg->next);

    if (! bg) {
	/* First encounter of w1,w2 */
	bg = (clm_bg_t *) listelem_alloc (sizeof(clm_bg_t));
	bg->w2 = w2;
	bg->count = 1;
	bg->next = lm->clm_ug[w1].w2list;
	lm->clm_ug[w1].w2list = bg;
    } else
	bg->count++;
    
    lm->clm_ug[w1].sum_w2count++;
}

/* NOTE: Some approximations in the way the relative language weights are applied */
int32 cache_lm_score (cache_lm_t *lm, int32 w1, int32 w2, int32 *remwt)
{
    int32 bgscr, ugscr, clmscr;
    clm_bg_t *bg;

    /* Unigram cache component */
    if (lm->clm_ug[w2].count > 0)
	ugscr = LOG_COUNT(lm->clm_ug[w2].count) - LOG_COUNT(lm->sum_ugcount);
    else
	ugscr = log0;
    ugscr += lm->log_uw;
    
    /* Bigram cache component */
    for (bg = lm->clm_ug[w1].w2list; bg && (bg->w2 != w2); bg = bg->next);
    if (bg)
	bgscr = LOG_COUNT(bg->count) - LOG_COUNT(lm->clm_ug[w1].sum_w2count);
    else
	bgscr = log0;
    bgscr += lm->log_bw;
    
    /* Combine unigram and bigram cache component scores */
    if ((ugscr > log0) || (bgscr > log0)) {
	FAST_ADD (clmscr, ugscr, bgscr, Addition_Table, Table_Size);
    } else
	clmscr = log0;

    *remwt = lm->log_remwt;
    
    return (clmscr);
}

void cache_lm_dump (cache_lm_t *lm, char *file)
{
    FILE *fp;
    int32 i;
    clm_bg_t *bg;

    if ((fp = fopen(file, "w")) == NULL) {
	E_ERROR("fopen(%s,w) failed\n", file);
	return;
    }
    
    fprintf (fp, "#CacheLMDump\n");

    fprintf (fp, "#Unigrams\n");
    for (i = 0; i < lm->n_word; i++) {
	if (lm->clm_ug[i].count > 0)
	    fprintf (fp, "%d %s\n", lm->clm_ug[i].count, kb_get_word_str (i));
    }
    
    fprintf (fp, "#Bigrams\n");
    for (i = 0; i < lm->n_word; i++) {
	for (bg = lm->clm_ug[i].w2list; bg; bg = bg->next) {
	    fprintf (fp, "%d %s %s\n", bg->count,
		     kb_get_word_str (i), kb_get_word_str (bg->w2));
	}
    }
    
    fprintf (fp, "#End\n");

    fclose (fp);
}

void cache_lm_load (cache_lm_t *lm, char *file)
{
    FILE *fp;
    int32 i, n, w, w2;
    char line[16384], wd[4096], wd2[4096];
    
    if ((fp = fopen(file, "r")) == NULL) {
	E_ERROR("fopen(%s,r) failed\n", file);
	return;
    }
    
    if (fgets (line, sizeof(line), fp) == NULL) {
	E_ERROR("%s: No header\n", file);
	fclose (fp);
	return;
    }
    if (strcmp (line, "#CacheLMDump\n") != 0) {
	E_ERROR("%s: Bad header line: %s\n", file, line);
	fclose (fp);
	return;
    }

    if ((fgets (line, sizeof(line), fp) == NULL) ||
	(strcmp (line, "#Unigrams\n") != 0)) {
	E_ERROR("%s: Missing #Unigrams keyword\n", file);
	fclose (fp);
	return;
    }

    while (fgets (line, sizeof(line), fp) != NULL) {
	if (sscanf (line, "%d %s", &n, wd) != 2)
	    break;
	
	w = kb_get_word_id (wd);
	if ((w < 0) || (w >= lm->n_word)) {
	    E_ERROR("%s: Unknown word(%s); ignored\n", file, wd);
	    continue;
	}
	
	for (i = 0; i < n; i++)
	    cache_lm_add_ug (lm, w);
    }

    if (strcmp (line, "#Bigrams\n") != 0) {
	E_ERROR("%s: Missing #Bigrams keyword: %s\n", file, line);
	fclose (fp);
	return;
    }

    while (fgets (line, sizeof(line), fp) != NULL) {
	if (sscanf (line, "%d %s %s", &n, wd, wd2) != 3)
	    break;
	
	w = kb_get_word_id (wd);
	w2 = kb_get_word_id (wd2);
	if ((w < 0) || (w >= lm->n_word)) {
	    E_ERROR("%s: Unknown word(%s); ignored\n", file, wd);
	    continue;
	}
	if ((w2 < 0) || (w2 >= lm->n_word)) {
	    E_ERROR("%s: Unknown word(%s); ignored\n", file, wd2);
	    continue;
	}
	
	for (i = 0; i < n; i++)
	    cache_lm_add_bg (lm, w, w2);
    }
    
    if (strcmp (line, "#End\n") != 0)
	E_ERROR("%s: Missing #End keyword: %s\n", file, line);

    fclose (fp);
}
