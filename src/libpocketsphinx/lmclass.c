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
 * lmclass.c -- Class-of-words objects in language models.
 * 
 * HISTORY
 * 
 * $Log: lmclass.c,v $
 * Revision 1.1.1.1  2006/05/23 18:45:00  dhuggins
 * re-importation
 *
 * Revision 1.7  2004/12/10 16:48:56  rkm
 * Added continuous density acoustic model handling
 *
 * 
 * 24-Mar-1998	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Started.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "s2types.h"
#include "CM_macros.h"
#include "str2words.h"
#include "strfuncs.h"
#include "err.h"
#include "log.h"

#include "lmclass.h"

#define LMCLASS_UNDEFINED_PROB		32001	/* Some large +ve non-logprob value */

void lmclass_dump (lmclass_t cl, FILE *fp)
{
    lmclass_word_t w;
    
    assert (cl != NULL);
    
    fprintf (fp, "LMCLASS %s\n", cl->name);
    for (w = cl->wordlist; w; w = w->next)
	fprintf (fp, "    %s\t%d\n", w->word, w->LOGprob);
    fprintf (fp, "END %s\n", cl->name);
    
    fflush (fp);
}

void lmclass_set_dump (lmclass_set_t set, FILE *fp)
{
    lmclass_t cl;
    
    assert (set != NULL);

    for (cl = set->lmclass_list; cl; cl = cl->next)
	lmclass_dump (cl, fp);
}

lmclass_set_t lmclass_newset ( void )
{
    lmclass_set_t set;
    
    set = (lmclass_set_t) CM_calloc (1, sizeof(struct lmclass_set_s));
    set->lmclass_list = NULL;
    return set;
}

static lmclass_set_t lmclass_add (lmclass_set_t set, lmclass_t new)
{
    lmclass_t cl, prev;
    
    assert (set != NULL);
    assert (new != NULL);
    
    prev = NULL;
    for (cl = set->lmclass_list;
	 cl && (strcmp (cl->name, new->name) != 0);
	 cl = cl->next) {
	prev = cl;
    }
    if (cl)
	return NULL;	/* Duplicate */
    
    if (prev)
	prev->next = new;
    else
	set->lmclass_list = new;
    new->next = NULL;
    
    return set;
}

static lmclass_t lmclass_addword (lmclass_t class, lmclass_word_t new)
{
    lmclass_word_t w, prev;
    
    assert (class != NULL);
    assert (new != NULL);
    
    prev = NULL;
    for (w = class->wordlist; w && (strcmp (w->word, new->word) != 0); w = w->next)
	prev = w;
    if (w)
	return NULL;
    
    if (prev)
	prev->next = new;
    else
	class->wordlist = new;
    new->next = NULL;
    
    return class;
}

lmclass_set_t lmclass_loadfile (lmclass_set_t lmclass_set, char *file)
{
    FILE *fp;
    char line[16384], *word[4096], *_eof;
    int32 lineno, nwd;
    lmclass_t lmclass;
    lmclass_word_t lmclass_word;
    float SUMp, p;
    int32 n_implicit_prob, n_word;
    
    assert (lmclass_set != NULL);
    
    E_INFO("Reading LM Class file '%s'\n", file);
    fp = (FILE *) CM_fopen(file, "r");
    
    lineno = 0;
    for (;;) {	/* Read successive LM classes in this file */
	/* Read a non-empty line, presumably the beginning of a new LM class */
	while (((_eof = fgets (line, sizeof(line), fp)) != NULL) &&
	       ((line[0] == '#') || ((nwd = str2words(line, word, 4096)) == 0))) {
	    lineno++;
	}
	if (! _eof)
	    break;	/* EOF */
	lineno++;

	if (nwd < 0)
	    E_FATAL("Line %d: Line too long:\n\t%s\n", lineno, line);
	
	if ((nwd != 2) || (strcmp (word[0], "LMCLASS") != 0))
	    E_FATAL("Line %d: Expecting LMCLASS <classname>\n", lineno);
	
	if ((word[1][0] != '[') || (word[1][strlen(word[1])-1] != ']'))
	    E_WARN("Line %d: LM class name(%s) not enclosed in []\n", lineno, word[1]);
	
	/* Initialize a new LM class object */
	lmclass = (lmclass_t) CM_calloc (1, sizeof(struct lmclass_s));
	lmclass->name = salloc(word[1]);
	lmclass->wordlist = NULL;

	/* Add the new class to the existing set */
	if ((lmclass_set = lmclass_add (lmclass_set, lmclass)) == NULL)
	    E_FATAL("Line %d: lmclass_add(%s) failed (duplicate?)\n", lineno, word[1]);
	
	/* Read in the LMclass body */
	SUMp = 0.0;
	n_implicit_prob = 0;
	n_word = 0;
	for (;;) {
	    int32 LOGp = 0; /* No, GCC, it won't be used uninitialized */

	    while (((_eof = fgets (line, sizeof(line), fp)) != NULL) &&
		   ((line[0] == '#') || ((nwd = str2words(line, word, 4096)) == 0))) {
		lineno++;
	    }
	    if (! _eof)
		E_FATAL("Premature EOF(%s)\n", file);
	    lineno++;
	    
	    if ((nwd != 1) && (nwd != 2))
		E_FATAL("Line %d: Syntax error\n", lineno);

	    if ((nwd == 2) &&
		(strcmp (word[0], "END") == 0) &&
		(strcmp (word[1], lmclass->name) == 0))
		break;
	    
	    if (nwd == 2) {
		if (sscanf (word[1], "%e", &p) == 1) {	/* Not perfect parsing */
		    if ((p <= 0.0) || (p >= 1.0))
			E_FATAL("Line %d: Prob(%s) out of range (0,1)\n", lineno, word[1]);
		    LOGp = LOG(p);
		    SUMp += p;
		} else
		    E_FATAL("Line %d: Syntax error\n", lineno);
	    } else {
		LOGp = LMCLASS_UNDEFINED_PROB;	/* Some large +ve quantity */
		n_implicit_prob++;
	    }

	    /* Create a new word object */
	    lmclass_word = (lmclass_word_t) CM_calloc (1, sizeof(struct lmclass_word_s));
	    lmclass_word->word = salloc(word[0]);
	    lmclass_word->dictwid = -1;		/* To be filled in by application */
	    lmclass_word->LOGprob = LOGp;
	    
	    /* Add the new word to the class */
	    if ((lmclass = lmclass_addword (lmclass, lmclass_word)) == NULL)
		E_FATAL("Line %d: lmclass_addword(%s) failed (duplicate?)\n",
			lineno, word[0]);
	    n_word++;
	}

	if (n_implicit_prob > 0) {
	    int32 LOGp;

	    if (SUMp >= 1.0)
		E_FATAL("Sum(prob)(LMClass %s) = %e\n", lmclass->name, SUMp);
	    
	    p = (1.0 - SUMp) / (float)n_implicit_prob;
	    LOGp = LOG(p);
	    
	    for (lmclass_word = lmclass->wordlist;
		 lmclass_word;
		 lmclass_word = lmclass_word->next) {
		if (lmclass_word->LOGprob == LMCLASS_UNDEFINED_PROB)
		    lmclass_word->LOGprob = LOGp;
	    }
	} else {
	    /* A generous slack of 0.1 for numerical errors */
	    if ((SUMp >= 1.1) || (SUMp <= 0.9))
		E_WARN("Sum(prob)(LMClass %s) = %e\n", lmclass->name, SUMp);
	}
	
	E_INFO("Loaded LM Class '%s'; %d words\n", lmclass->name, n_word);
    }
    
    fclose (fp);

    return lmclass_set;
}

void lmclass_set_dictwid (lmclass_word_t w, int32 dictwid)
{
    assert (w != NULL);
    w->dictwid = dictwid;
}

lmclass_t lmclass_get_lmclass (lmclass_set_t set, char *name)
{
    lmclass_t cl;
    
    for (cl = set->lmclass_list;
	 cl && (strcmp (cl->name, name) != 0);
	 cl = cl->next);
    
    return cl;
}

int32 lmclass_get_nclass (lmclass_set_t set)
{
    lmclass_t cl;
    int32 n;
    
    for (n = 0, cl = set->lmclass_list; cl; cl = cl->next, n++);
    
    return n;
}
