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
 * mdef.c -- HMM model definition: base (CI) phones and triphones
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
 * $Log: mdef.c,v $
 * Revision 1.1.1.1  2006/05/23 18:45:01  dhuggins
 * re-importation
 *
 * Revision 1.1  2004/12/10 16:48:56  rkm
 * Added continuous density acoustic model handling
 *
 * 
 * 22-Nov-2004	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Imported from s3.2, for supporting s3 format continuous
 * 		acoustic models.
 * 
 * 14-Oct-1999	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon
 * 		Added mdef_sseq2sen_active().
 * 
 * 06-May-1999	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon
 * 		In mdef_phone_id(), added backing off to silence phone context from filler
 * 		context if original triphone not found.
 * 
 * 30-Apr-1999	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon
 * 		Added senone-sequence id (ssid) to phone_t and appropriate functions to
 * 		maintain it.  Instead, moved state sequence info to mdef_t.
 * 
 * 13-Jul-96	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University.
 * 		Added mdef_phone_str().
 * 
 * 01-Jan-96	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University.
 * 		Allowed mdef_phone_id_nearest to return base phone id if either
 * 		left or right context (or both) is undefined.
 * 
 * 01-Jan-96	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University.
 * 		Created.
 */


/*
 * Major assumptions:
 *   All phones have same #states, same topology.
 *   Every phone has exactly one non-emitting, final state--the last one.
 *   CI phones must appear first in model definition file.
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "mdef.h"
#include "ckd_alloc.h"
#include "err.h"


#define MODEL_DEF_VERSION	"0.3"

extern int32 totalDists;

void mdef_dump (FILE *fp, mdef_t *m)
{
    int32 i, j;
    int32 ssid;
    char buf[1024];
    
    fprintf (fp, "%d ciphone\n", m->n_ciphone);
    fprintf (fp, "%d phone\n", m->n_phone);
    fprintf (fp, "%d emitstate\n", m->n_emit_state);
    fprintf (fp, "%d cisen\n", m->n_ci_sen);
    fprintf (fp, "%d sen\n", m->n_sen);
    fprintf (fp, "%d tmat\n", m->n_tmat);
    
    for (i = 0; i < m->n_phone; i++) {
	mdef_phone_str (m, i, buf);
	ssid = m->phone[i].ssid;
	
	fprintf (fp, "%3d %5d", m->phone[i].tmat, ssid);
	for (j = 0; j < m->n_emit_state; j++)
	    fprintf (fp, " %5d", m->sseq[ssid][j]);
	fprintf (fp, "\t");
	for (j = 0; j < m->n_emit_state; j++)
	    fprintf (fp, " %3d", m->cd2cisen[m->sseq[ssid][j]]);
	fprintf (fp, "\t%s\n", buf);
    }
    
    fflush (fp);
}


#if 0
int32 mdef_hmm_cmp (mdef_t *m, s3pid_t p1, s3pid_t p2)
{
    int32 i;
    
    if (m->phone[p1].tmat != m->phone[p2].tmat)
	return -1;
    
    for (i = 0; i < m->n_emit_state; i++)
	if (m->phone[p1].state[i] != m->phone[p2].state[i])
	    return -1;
    
    return 0;
}
#endif


static void ciphone_add (mdef_t *m, char *ci, s3pid_t p)
{
    assert (p < m->n_ciphone);

    m->ciphone[p].name = (char *) ckd_salloc (ci);
    if (s3hash_enter (m->ciphone_ht, m->ciphone[p].name, p) != p)
	E_FATAL("s3hash_enter(%s) failed; duplicate CIphone?\n", m->ciphone[p].name);
}


static ph_lc_t *find_ph_lc (ph_lc_t *lclist, s3cipid_t lc)
{
    ph_lc_t *lcptr;

    for (lcptr = lclist; lcptr && (lcptr->lc != lc); lcptr = lcptr->next);
    return lcptr;
}


static ph_rc_t *find_ph_rc (ph_rc_t *rclist, s3cipid_t rc)
{
    ph_rc_t *rcptr;

    for (rcptr = rclist; rcptr && (rcptr->rc != rc); rcptr = rcptr->next);
    return rcptr;
}


static void triphone_add (mdef_t *m,
			  int32 ci, int32 lc, int32 rc, int32 wpos,
			  int32 p)
{
    ph_lc_t *lcptr;
    ph_rc_t *rcptr;
    
    assert (p < m->n_phone);

    /* Fill in phone[p] information (state and tmat mappings added later) */
    m->phone[p].ci = ci;
    m->phone[p].lc = lc;
    m->phone[p].rc = rc;
    m->phone[p].wpos = wpos;

    /* Create <ci,lc,rc,wpos> -> p mapping if not a CI phone */
    if (p >= m->n_ciphone) {
	if ((lcptr = find_ph_lc (m->wpos_ci_lclist[wpos][ci], lc)) == NULL) {
	    lcptr = (ph_lc_t *) ckd_calloc (1, sizeof(ph_lc_t));
	    lcptr->lc = lc;
	    lcptr->next = m->wpos_ci_lclist[wpos][ci];
	    m->wpos_ci_lclist[wpos][ci] = lcptr;
	}
	if ((rcptr = find_ph_rc (lcptr->rclist, rc)) != NULL) {
	    char buf[4096];
	    
	    mdef_phone_str (m, rcptr->pid, buf);
	    E_FATAL("Duplicate triphone: %s\n", buf);
	}
	
	rcptr = (ph_rc_t *) ckd_calloc (1, sizeof(ph_rc_t));
	rcptr->rc = rc;
	rcptr->pid = p;
	rcptr->next = lcptr->rclist;
	lcptr->rclist = rcptr;
    }
}


s3cipid_t mdef_ciphone_id (mdef_t *m, char *ci)
{
    int32 id;
    
    assert (m);
    assert (ci);
    
    if (s3hash_lookup (m->ciphone_ht, ci, &id) < 0)
	return (BAD_S3CIPID);
    return ((s3cipid_t) id);
}


const char *mdef_ciphone_str (mdef_t *m, int32 id)
{
    assert (m);
    assert ((id >= 0) && (id < m->n_ciphone));
    
    return (m->ciphone[id].name);
}


int32 mdef_phone_str (mdef_t *m, s3pid_t pid, char *buf)
{
    char *wpos_name;
    
    assert (m);
    assert ((pid >= 0) && (pid < m->n_phone));
    wpos_name = WPOS_NAME;
    
    buf[0] = '\0';
    if (pid < m->n_ciphone)
	sprintf (buf, "%s", mdef_ciphone_str (m, (s3cipid_t) pid));
    else {
	sprintf (buf, "%s(%s,%s)%c",
		 mdef_ciphone_str(m, m->phone[pid].ci),
		 mdef_ciphone_str(m, m->phone[pid].lc),
		 mdef_ciphone_str(m, m->phone[pid].rc),
		 wpos_name[m->phone[pid].wpos]);
    }
    return 0;
}


s3pid_t mdef_phone_id (mdef_t *m, 
		       int32 ci, int32 lc, int32 rc, int32 wpos)
{
    ph_lc_t *lcptr;
    ph_rc_t *rcptr;
    s3cipid_t newl, newr;
    
    assert (m);
    assert ((ci >= 0) && (ci < m->n_ciphone));
    assert ((lc >= 0) && (lc < m->n_ciphone));
    assert ((rc >= 0) && (rc < m->n_ciphone));
    assert ((wpos >= 0) && (wpos < N_WORD_POSN));

    if (((lcptr = find_ph_lc (m->wpos_ci_lclist[wpos][ci], lc)) == NULL) ||
	((rcptr = find_ph_rc (lcptr->rclist, rc)) == NULL)) {
	/* Not found; backoff to silence context if non-silence filler context */
	if (NOT_S3CIPID(m->sil))
	    return BAD_S3PID;
	
	newl = m->ciphone[lc].filler ? m->sil : lc;
	newr = m->ciphone[rc].filler ? m->sil : rc;
	if ((newl == lc) && (newr == rc))
	    return BAD_S3PID;
	
	return (mdef_phone_id (m, ci, newl, newr, wpos));
    }
    
    return (rcptr->pid);
}


s3pid_t mdef_phone_id_nearest (mdef_t *m, 
			       int32 b, int32 l, int32 r, int32 pos)
{
    word_posn_t tmppos;
    s3pid_t p;
    s3cipid_t newl, newr;
    char *wpos_name;
    
    assert (m);
    assert ((b >= 0) && (b < m->n_ciphone));
    assert ((pos >= 0) && (pos < N_WORD_POSN));

    if ((NOT_S3CIPID(l)) || (NOT_S3CIPID(r)))
	return ((s3pid_t) b);
    
    assert ((l >= 0) && (l < m->n_ciphone));
    assert ((r >= 0) && (r < m->n_ciphone));
    
    p = mdef_phone_id (m, b, l, r, pos);
    if (IS_S3PID(p))
	return p;
    
    /* Exact triphone not found; backoff to other word positions */
    for (tmppos = 0; tmppos < N_WORD_POSN; tmppos++) {
	if (tmppos != pos) {
	    p = mdef_phone_id (m, b, l, r, tmppos);
	    if (IS_S3PID(p))
		return p;
	}
    }
    
    /* Nothing yet; backoff to silence phone if non-silence filler context */
    if (IS_S3CIPID(m->sil)) {
	newl = m->ciphone[l].filler ? m->sil : l;
	newr = m->ciphone[r].filler ? m->sil : r;
	if ((newl != l) || (newr != r)) {
	    p = mdef_phone_id (m, b, newl, newr, pos);
	    if (IS_S3PID(p))
		return p;
	    
	    for (tmppos = 0; tmppos < N_WORD_POSN; tmppos++) {
		if (tmppos != pos) {
		    p = mdef_phone_id (m, b, newl, newr, tmppos);
		    if (IS_S3PID(p))
			return p;
		}
	    }
	}
    }
    
    /* Nothing yet; backoff to base phone */
    if ((m->n_phone > m->n_ciphone) && (! m->ciphone[b].filler)) {
	wpos_name = WPOS_NAME;
#if 0
	E_WARN("Triphone(%s,%s,%s,%c) not found; backing off to CIphone\n",
	       mdef_ciphone_str(m, b),
	       mdef_ciphone_str(m, l),
	       mdef_ciphone_str(m, r),
	       wpos_name[pos]);
#endif
    }
    return ((s3pid_t) b);
}


int32 mdef_phone_components (mdef_t *m,
			     s3pid_t p,
			     s3cipid_t *b,
			     s3cipid_t *l,
			     s3cipid_t *r,
			     word_posn_t *pos)
{
    assert (m);
    assert ((p >= 0) && (p < m->n_phone));

    *b = m->phone[p].ci;
    *l = m->phone[p].lc;
    *r = m->phone[p].rc;
    *pos = m->phone[p].wpos;

    return 0;
}


int32 mdef_is_ciphone (mdef_t *m, s3pid_t p)
{
    assert (m);
    assert ((p >= 0) && (p < m->n_phone));
    
    return ((p < m->n_ciphone) ? 1 : 0);
}


/* Parse tmat and state->senone mappings for phone p and fill in structure */
static void parse_tmat_senmap (mdef_t *m, char *line, int32 off, s3pid_t p)
{
    int32 wlen, n, s;
    char word[1024], *lp;

    lp = line + off;
    
    /* Read transition matrix id */
    if ((sscanf (lp, "%d%n", &n, &wlen) != 1) || (n < 0))
        E_FATAL("Missing or bad transition matrix id: %s\n", line);
    m->phone[p].tmat = n;
    if (m->n_tmat <= n)
	E_FATAL("tmat-id(%d) > #tmat in header(%d): %s\n", n, m->n_tmat, line);
    lp += wlen;
    
    /* Read senone mappings for each emitting state */
    for (n = 0; n < m->n_emit_state; n++) {
        if ((sscanf (lp, "%d%n", &s, &wlen) != 1) || (s < 0))
	    E_FATAL("Missing or bad state[%d]->senone mapping: %s\n", n, line);
	if ((p < m->n_ciphone) && (m->n_ci_sen <= s))
	    E_FATAL("CI-senone-id(%d) > #CI-senones(%d): %s\n", s, m->n_ci_sen, line);
	if (m->n_sen <= s)
	    E_FATAL("Senone-id(%d) > #senones(%d): %s\n", s, m->n_sen, line);

	m->sseq[p][n] = s;
	lp += wlen;
    }

    /* Check for the last non-emitting state N */
    if ((sscanf (lp, "%s%n", word, &wlen) != 1) || (strcmp (word, "N") != 0))
        E_FATAL("Missing non-emitting state spec: %s\n", line);
    lp += wlen;

    /* Check for end of line */
    if (sscanf (lp, "%s%n", word, &wlen) == 1)
        E_FATAL("Non-empty beyond non-emitting final state: %s\n", line);
}


static void parse_base_line (mdef_t *m, char *line, int32 p)
{
    int32 wlen, n;
    char word[1024], *lp;
    int32 ci;

    lp = line;
    
    /* Read base phone name */
    if (sscanf (lp, "%s%n", word, &wlen) != 1)
	E_FATAL("Missing base phone name: %s\n", line);
    lp += wlen;
    
    /* Make sure it's not a duplicate */
    ci = mdef_ciphone_id (m, word);
    if (IS_S3CIPID(ci))
        E_FATAL("Duplicate base phone: %s\n", line);

    /* Add ciphone to ciphone table with id p */
    ciphone_add (m, word, p);
    ci = (s3cipid_t) p;

    /* Read and skip "-" for lc, rc, wpos */
    for (n = 0; n < 3; n++) {
	if ((sscanf (lp, "%s%n", word, &wlen) != 1) || (strcmp (word, "-") != 0))
	    E_FATAL("Bad context info for base phone: %s\n", line);
	lp += wlen;
    }
    
    /* Read filler attribute, if present */
    if (sscanf (lp, "%s%n", word, &wlen) != 1)
	E_FATAL("Missing filler atribute field: %s\n", line);
    lp += wlen;
    if (strcmp (word, "filler") == 0)
        m->ciphone[ci].filler = 1;
    else if (strcmp (word, "n/a") == 0)
        m->ciphone[ci].filler = 0;
    else
        E_FATAL("Bad filler attribute field: %s\n", line);

    triphone_add (m, ci, BAD_S3CIPID, BAD_S3CIPID, WORD_POSN_UNDEFINED, p);

    /* Parse remainder of line: transition matrix and state->senone mappings */
    parse_tmat_senmap (m, line, lp-line, p);
}


static void parse_tri_line (mdef_t *m, char *line, int32 p)
{
    int32 wlen;
    char word[1024], *lp;
    int32 ci, lc, rc;
    word_posn_t wpos;

    lp = line;
    
    /* Read base phone name */
    if (sscanf (lp, "%s%n", word, &wlen) != 1)
	E_FATAL("Missing base phone name: %s\n", line);
    lp += wlen;

    ci = mdef_ciphone_id (m, word);
    if (NOT_S3CIPID(ci))
        E_FATAL("Unknown base phone: %s\n", line);

    /* Read lc */
    if (sscanf (lp, "%s%n", word, &wlen) != 1)
	E_FATAL("Missing left context: %s\n", line);
    lp += wlen;
    lc = mdef_ciphone_id (m, word);
    if (NOT_S3CIPID(lc))
        E_FATAL("Unknown left context: %s\n", line);

    /* Read rc */
    if (sscanf (lp, "%s%n", word, &wlen) != 1)
	E_FATAL("Missing right context: %s\n", line);
    lp += wlen;
    rc = mdef_ciphone_id (m, word);
    if (NOT_S3CIPID(rc))
        E_FATAL("Unknown right  context: %s\n", line);
    
    /* Read tripone word-position within word */
    if ((sscanf (lp, "%s%n", word, &wlen) != 1) || (word[1] != '\0'))
        E_FATAL("Missing or bad word-position spec: %s\n", line);
    lp += wlen;
    switch (word[0]) {
    case 'b': wpos = WORD_POSN_BEGIN; break;
    case 'e': wpos = WORD_POSN_END; break;
    case 's': wpos = WORD_POSN_SINGLE; break;
    case 'i': wpos = WORD_POSN_INTERNAL; break;
    default: E_FATAL("Bad word-position spec: %s\n", line);
    }

    /* Read filler attribute, if present.  Must match base phone attribute */
    if (sscanf (lp, "%s%n", word, &wlen) != 1)
	E_FATAL("Missing filler attribute field: %s\n", line);
    lp += wlen;
    if (((strcmp (word, "filler") == 0) && (m->ciphone[ci].filler)) ||
	((strcmp (word, "n/a") == 0) && (! m->ciphone[ci].filler))) {
	/* Everything is fine */
    } else
        E_FATAL("Bad filler attribute field: %s\n", line);
    
    triphone_add (m, ci, lc, rc, wpos, p);

    /* Parse remainder of line: transition matrix and state->senone mappings */
    parse_tmat_senmap (m, line, lp-line, p);
}


static void sseq_compress (mdef_t *m)
{
    s3hash_table_t *h;
    s3senid_t **sseq;
    int32 n_sseq;
    int32 p, j, k;
    glist_t g;
    gnode_t *gn;
    s3hash_entry_t *he;
    
    k = m->n_emit_state * sizeof(s3senid_t);
    
    h = s3hash_new (m->n_phone, S3HASH_CASE_YES);
    n_sseq = 0;
    
    /* Identify unique senone-sequence IDs.  BUG: tmat-id not being considered!! */
    for (p = 0; p < m->n_phone; p++) {
	/* Add senone sequence to hash table */
	if ((j = s3hash_enter_bkey (h, (char *)(m->sseq[p]), k, n_sseq)) == n_sseq)
	    n_sseq++;
	
	m->phone[p].ssid = j;
    }
    
    /* Generate compacted sseq table */
    sseq = (s3senid_t **) ckd_calloc_2d (n_sseq, m->n_emit_state, sizeof(s3senid_t));
    g = s3hash_tolist (h, &j);
    assert (j == n_sseq);
    
    for (gn = g; gn; gn = gnode_next(gn)) {
	he = (s3hash_entry_t *) gnode_ptr (gn);
	j = s3hash_entry_val(he);
	memcpy (sseq[j], s3hash_entry_key(he), k);
    }
    glist_free (g);
    
    /* Free the old, temporary senone sequence table, replace with compacted one */
    ckd_free_2d ((void **) m->sseq);
    m->sseq = sseq;
    m->n_sseq = n_sseq;
    
    s3hash_free (h);
}


static int32 noncomment_line(char *line, int32 size, FILE *fp)
{
    while (fgets (line, size, fp) != NULL) {
        if (line[0] != '#')
	    return 0;
    }
    return -1;
}


/*
 * Initialize phones (ci and triphones) and state->senone mappings from .mdef file.
 */
mdef_t *mdef_init (char *mdeffile)
{
    FILE *fp;
    int32 n_ci, n_tri, n_map, n;
    char tag[1024], buf[1024];
    s3senid_t **senmap;
    s3pid_t p;
    int32 s, ci, cd;
    mdef_t *m;
    int32 *cdsen_start, *cdsen_end;

    if (! mdeffile)
	E_FATAL("No mdef-file\n");

    E_INFO("Reading model definition: %s\n", mdeffile);

    m = (mdef_t *) ckd_calloc (1, sizeof(mdef_t));
    
    if ((fp = fopen(mdeffile, "r")) == NULL)
        E_FATAL_SYSTEM("fopen(%s,r) failed\n", mdeffile);

    if (noncomment_line(buf, sizeof(buf), fp) < 0)
        E_FATAL("Empty file: %s\n", mdeffile);

    if (strncmp(buf, "BMDF", 4) == 0
	|| strncmp(buf, "FDMB", 4) == 0) {
	    E_INFO("Found byte-order mark %.4s, assuming this is a binary mdef file\n",
		   buf);
	    fclose(fp);
	    ckd_free(m);
	    return NULL;
    }
    if (strncmp(buf, MODEL_DEF_VERSION, strlen(MODEL_DEF_VERSION)) != 0)
        E_FATAL("Version error: Expecing %s, but read %s\n", MODEL_DEF_VERSION, buf);

    /* Read #base phones, #triphones, #senone mappings defined in header */
    n_ci = -1;
    n_tri = -1;
    n_map = -1;
    m->n_ci_sen = -1;
    m->n_sen = -1;
    m->n_tmat = -1;
    do {
	if (noncomment_line(buf, sizeof(buf), fp) < 0)
	    E_FATAL("Incomplete header\n");

	if ((sscanf(buf, "%d %s", &n, tag) != 2) || (n < 0))
	    E_FATAL("Error in header: %s\n", buf);

	if (strcmp(tag, "n_base") == 0)
	    n_ci = n;
	else if (strcmp(tag, "n_tri") == 0)
	    n_tri = n;
	else if (strcmp(tag, "n_state_map") == 0)
	    n_map = n;
	else if (strcmp(tag, "n_tied_ci_state") == 0)
	    m->n_ci_sen = n;
	else if (strcmp(tag, "n_tied_state") == 0)
	    m->n_sen = n;
	else if (strcmp(tag, "n_tied_tmat") == 0)
	    m->n_tmat = n;
	else
	    E_FATAL("Unknown header line: %s\n", buf);
    } while ((n_ci < 0) || (n_tri < 0) || (n_map < 0) ||
	     (m->n_ci_sen < 0) || (m->n_sen < 0) || (m->n_tmat < 0));

    if ((n_ci == 0) || (m->n_ci_sen == 0) || (m->n_tmat == 0) || (m->n_ci_sen > m->n_sen))
        E_FATAL("%s: Error in header\n", mdeffile);
    
    /* Check typesize limits */
    if (n_ci >= MAX_S3CIPID)
	E_FATAL("%s: #CI phones (%d) exceeds limit (%d)\n", mdeffile, n_ci, MAX_S3CIPID);
    if (n_ci + n_tri >= MAX_S3PID)
	E_FATAL("%s: #Phones (%d) exceeds limit (%d)\n", mdeffile, n_ci+n_tri, MAX_S3PID);
    if (m->n_sen >= MAX_S3SENID)
	E_FATAL("%s: #senones (%d) exceeds limit (%d)\n", mdeffile, m->n_sen, MAX_S3SENID);
    if (m->n_tmat >= MAX_S3TMATID)
	E_FATAL("%s: #tmats (%d) exceeds limit (%d)\n", mdeffile, m->n_tmat, MAX_S3TMATID);
    
    m->n_emit_state = (n_map / (n_ci+n_tri)) - 1;
    if ((m->n_emit_state+1) * (n_ci+n_tri) != n_map)
        E_FATAL("Header error: n_state_map not a multiple of n_ci*n_tri\n");

    /* Initialize ciphone info */
    m->n_ciphone = n_ci;
    m->ciphone_ht = s3hash_new (n_ci, 1);	/* With case-insensitive string names */
    m->ciphone = (ciphone_t *) ckd_calloc (n_ci, sizeof(ciphone_t));

    /* Initialize phones info (ciphones + triphones) */
    m->n_phone = n_ci + n_tri;
    m->phone = (phone_t *) ckd_calloc (m->n_phone, sizeof(phone_t));

    /* Allocate space for state->senone map for each phone */
    senmap = (s3senid_t **) ckd_calloc_2d (m->n_phone, m->n_emit_state, sizeof(s3senid_t));
    m->sseq = senmap;	/* TEMPORARY; until it is compressed into just the unique ones */

    /* Set totalDists like read_map() would have done for S2 models. */
    totalDists = m->n_sen;
    
    /* Allocate initial space for <ci,lc,rc,wpos> -> pid mapping */
    m->wpos_ci_lclist = (ph_lc_t ***) ckd_calloc_2d (N_WORD_POSN, m->n_ciphone,
						     sizeof(ph_lc_t *));

    /*
     * Read base phones and triphones.  They'll simply be assigned a running sequence
     * number as their "phone-id".  If the phone-id < n_ci, it's a ciphone.
     */

    /* Read base phones */
    for (p = 0; p < n_ci; p++) {
        if (noncomment_line(buf, sizeof(buf), fp) < 0)
	    E_FATAL("Premature EOF reading CIphone %d\n", p);
        parse_base_line (m, buf, p);
    }
    m->sil = mdef_ciphone_id (m, S3_SILENCE_CIPHONE);
    
    /* Read triphones, if any */
    for (; p < m->n_phone; p++) {
        if (noncomment_line(buf, sizeof(buf), fp) < 0)
	    E_FATAL("Premature EOF reading phone %d\n", p);
        parse_tri_line (m, buf, p);
    }

    if (noncomment_line(buf, sizeof(buf), fp) >= 0)
	E_ERROR("Non-empty file beyond expected #phones (%d)\n", m->n_phone);

    /* Build CD senones to CI senones map */
    if (m->n_ciphone * m->n_emit_state != m->n_ci_sen)
	E_FATAL("#CI-senones(%d) != #CI-phone(%d) x #emitting-states(%d)\n",
		m->n_ci_sen, m->n_ciphone, m->n_emit_state);
    m->cd2cisen = (s3senid_t *) ckd_calloc (m->n_sen, sizeof(s3senid_t));
    m->sen2cimap = (s3cipid_t *) ckd_calloc (m->n_sen, sizeof(s3cipid_t));
    for (s = 0; s < m->n_sen; s++)
	m->sen2cimap[s] = BAD_S3CIPID;
    for (s = 0; s < m->n_ci_sen; s++) {		/* CI senones */
	m->cd2cisen[s] = (s3senid_t) s;
	m->sen2cimap[s] = s / m->n_emit_state;
    }
    for (p = n_ci; p < m->n_phone; p++) {	/* CD senones */
	for (s = 0; s < m->n_emit_state; s++) {
	    cd = m->sseq[p][s];
	    ci = m->sseq[(int32)m->phone[p].ci][s];
	    m->cd2cisen[cd] = (s3senid_t) ci;
	    m->sen2cimap[cd] = m->phone[p].ci;
	}
    }
    
    /*
     * Count #senones (CI+CD) for each CI phone.
     * HACK!!  For handling holes in senone-CIphone mappings.  Does not work if holes
     * are present at the beginning or end of senones for a given CIphone.
     */
    cdsen_start = (int32 *) ckd_calloc (m->n_ciphone, sizeof(int32));
    cdsen_end = (int32 *) ckd_calloc (m->n_ciphone, sizeof(int32));
    for (s = m->n_ci_sen; s < m->n_sen; s++) {
	if (NOT_S3CIPID(m->sen2cimap[s]))
	    continue;
	
	if (! cdsen_start[(int32)(m->sen2cimap[s])])
	    cdsen_start[(int32)(m->sen2cimap[s])] = s;
	cdsen_end[(int32)(m->sen2cimap[s])] = s;
    }

    /* Fill up holes */
    for (s = m->n_ci_sen; s < m->n_sen; s++) {
	if (IS_S3CIPID(m->sen2cimap[s]))
	    continue;

	/* Check if properly inside the observed ranges above */
	for (p = 0; p < m->n_ciphone; p++) {
	    if ((s > cdsen_start[p]) && (s < cdsen_end[p]))
		break;
	}
	if (p >= m->n_ciphone)
	    E_FATAL("Unreferenced senone %d; cannot determine parent CIphone\n", s);
	m->sen2cimap[s] = p;
    }

    /* Build #CD-senones for each CIphone */
    m->ciphone2n_cd_sen = (int32 *) ckd_calloc (m->n_ciphone, sizeof(int32));
    n = 0;
    for (p = 0; p < m->n_ciphone; p++) {
	if (cdsen_start[p] > 0) {
	    m->ciphone2n_cd_sen[p] = cdsen_end[p] - cdsen_start[p] + 1;
	    n += m->ciphone2n_cd_sen[p];
	}
    }
    n += m->n_ci_sen;
    assert (n == m->n_sen);

    ckd_free (cdsen_start);
    ckd_free (cdsen_end);

    sseq_compress (m);
    
    E_INFO("%d CI-phone, %d CD-phone, %d emitstate/phone, %d CI-sen, %d Sen, %d Sen-Seq\n",
	   m->n_ciphone, m->n_phone - m->n_ciphone, m->n_emit_state,
	   m->n_ci_sen, m->n_sen, m->n_sseq);
    
    fclose (fp);
    
    return m;
}


void mdef_sseq2sen_active (mdef_t *mdef, int32 *sseq, int32 *sen)
{
    int32 ss, i;
    s3senid_t *sp;
    
    for (ss = 0; ss < mdef_n_sseq(mdef); ss++) {
	if (sseq[ss]) {
	    sp = mdef->sseq[ss];
	    for (i = 0; i < mdef_n_emit_state(mdef); i++)
		sen[sp[i]] = 1;
	}
    }
}
