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
 * HISTORY
 * 
 * $Log: dict.c,v $
 * Revision 1.1.1.1  2006/05/23 18:44:59  dhuggins
 * re-importation
 *
 * Revision 1.14  2004/12/10 16:48:56  rkm
 * Added continuous density acoustic model handling
 *
 * 
 * 05-Nov-98  M K Ravishankar (rkm@cs.cmu.edu) at Carnegie-Mellon University
 * 		dict_load now terminates program if input dictionary 
 *              contains errors.
 * 
 * 21-Nov-97  M K Ravishankar (rkm@cs.cmu.edu) at Carnegie-Mellon University
 * 		Bugfix: Noise dictionary was not being considered in figuring
 *              dictionary size.
 * 
 * 18-Nov-97  M K Ravishankar (rkm@cs.cmu.edu) at Carnegie-Mellon University
 * 		Added ability to modify pronunciation of an existing word in
 *              dictionary (in dict_add_word()).
 * 
 * 10-Aug-97  M K Ravishankar (rkm@cs.cmu.edu) at Carnegie-Mellon University
 *		Added check for word already existing in dictionary in 
 *              dict_add_word().
 * 
 * 27-May-97  M K Ravishankar (rkm@cs.cmu.edu) at Carnegie-Mellon University
 * 		Included Bob Brennan's personaldic handling (similar to 
 *              oovdic).
 * 
 * 11-Apr-97  M K Ravishankar (rkm@cs.cmu.edu) at Carnegie-Mellon University
 *		Made changes to replace_dict_entry to handle the addition of
 * 		alternative pronunciations (linking in alt, wid, fwid fields).
 * 
 * 02-Apr-97  M K Ravishankar (rkm@cs.cmu.edu) at Carnegie-Mellon University
 * 		Caused a fatal error if max size exceeded, instead of realloc.
 * 
 * 08-Dec-95  M K Ravishankar (rkm@cs.cmu.edu) at Carnegie-Mellon University
 * 	Added function dict_write_oovdict().
 * 
 * 06-Dec-95  M K Ravishankar (rkm@cs.cmu.edu) at Carnegie-Mellon University
 * 	Added functions dict_next_alt() and dict_pron().
 * 
 * Revision 8.5  94/10/11  12:32:03  rkm
 * Minor changes.
 * 
 * Revision 8.4  94/07/29  11:49:59  rkm
 * Changed handling of OOV subdictionary (no longer alternatives to <UNK>).
 * Added placeholders for dynamic addition of words to dictionary.
 * Added dict_add_word () for adding new words to dictionary.
 * 
 * Revision 8.3  94/04/14  15:08:31  rkm
 * Added function dictid_to_str().
 * 
 * Revision 8.2  94/04/14  14:34:11  rkm
 * Added OOV words sub-dictionary.
 * 
 * Revision 8.1  94/02/15  15:06:26  rkm
 * Basically the same as in v7; includes multiple start symbols for
 * the LISTEN project.
 * 
 * 11-Feb-94  M K Ravishankar (rkm) at Carnegie-Mellon University
 * 	Added multiple start symbols for the LISTEN project.
 * 
 *  9-Sep-92  Fil Alleva (faa) at Carnegie-Mellon University
 *	Added special silences for start_sym and end_sym.
 *	These special silences
 *	(SILb and SILe) are CI models and as such they create a new context,
 *	however since no triphones model these contexts explicity they are
 *	backed off to silence, which is the desired context. Therefore no
 *	special measures are take inside the decoder to handle these
 *	special silences.
 * 14-Oct-92  Eric Thayer (eht) at Carnegie Mellon University
 *	added Ravi's formal declarations for dict_to_id() so int32 -> pointer
 *	problem doesn't happen on DEC Alpha
 * 14-Oct-92  Eric Thayer (eht) at Carnegie Mellon University
 *	added Ravi's changes to make calls into hash.c work properly on Alpha
 *	
 */

#ifdef WIN32
#include <fcntl.h>
#else
#include <sys/file.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "s2types.h"
#include "CM_macros.h"
#include "basic_types.h"
#include "c.h"
#include "list.h"
#include "hash.h"
#include "phone.h"
#include "dict.h"
#include "err.h"
#include "assert.h"
#include "search_const.h"
#include "lmclass.h"
#include "lm_3g.h"
#include "msd.h"
#include "hmm_tied_r.h"
#include "kb.h"

#ifdef DEBUG
#define DFPRINTF(x)		fprintf x
#else
#define DFPRINTF(x)
#endif

#define QUIT(x)		{fprintf x; exit(-1);}

/* FIXME: put these in a header file */
extern char *salloc(char const *);
extern char *nxtarg(char const **, char const *);

extern int32 use_noise_words;

static void buildEntryTable (list_t *list, int32 ***table_p);
static void buildExitTable (list_t *list, int32 ***table_p,
			    int32 ***permuTab_p, int32 **sizeTab_p);
static int32 addToLeftContextTable (char *diphone);
static int32 addToRightContextTable (char *diphone);
static void recordMissingTriphone (char *triphoneStr);
static dict_entry_t * _new_dict_entry (char const *word_str,
				       char const *pronoun_str, int32 use_context);
static void _dict_list_add (dictT *dict, dict_entry_t *entry);
static void dict_load (dictT *dict, char *filename, int32 *word_id,
		       int32 use_context, int32 isa_phrase_dict);

static hash_t mtpHT;		/* Missing triphone hash table */
static list_t *mtpList;

static hash_t lcHT;		/* Left context hash table */
static list_t lcList;
static int32 **lcFwdTable;
static int32 **lcBwdTable;
static int32 **lcBwdPermTable;
static int32 *lcBwdSizeTable;

static hash_t rcHT;		/* Right context hash table */
static list_t rcList;
static int32 **rcFwdTable;
static int32 **rcFwdPermTable;
static int32 **rcBwdTable;
static int32 *rcFwdSizeTable;

/* First and last OOVs loaded DURING INITIALIZATION */
static int32 first_initial_oov, last_initial_oov;

/* Placeholders for dynamically added new words, OOVs */
static int32 initial_dummy;	/* 1st placeholder for dynamic OOVs after initialization */
static int32 first_dummy;	/* 1st dummy available for dynamic OOVs at any time */
static int32 last_dummy;	/* last dummy available for dynamic OOVs */

#define MAX_PRONOUN_LEN 	100

static int32 get_dict_size (char *file)
{
    FILE *fp;
    char line[1024];
    int32 n;
    
    fp = CM_fopen (file, "r");
    for (n = 0;; n++)
	if (fgets (line, sizeof(line), fp) == NULL)
	    break;
    fclose (fp);
    
    return n;
}

int32
dict_read(dictT *dict,
	  char *filename,	/* Main dict file */
	  char *p_filename,	/* Phrase dict file */
	  char *n_filename,	/* Noise dict file */
	  int32 use_context)
/*------------------------------------------------------------*
 * read in the dict file filename
 *------------------------------------------------------------*/
{
    int32               retval = 0;
    int32		word_id = 0, i, j;
    dict_entry_t       *entry;
    int32 		max_new_oov;
    char *oovdic;
    char *personalDic;
    char *startsym_file;
    struct stat statbuf;
    
    /*
     * Find size of dictionary and set hash and list table size hints.
     * (Otherwise, the simple-minded PC malloc library goes berserk.)
     */
    j = get_dict_size (filename);
    if (p_filename)
	j += get_dict_size (p_filename);
    if (n_filename)
	j += get_dict_size (n_filename);
    if ((oovdic = kb_get_oovdic()) != NULL)
	j += get_dict_size (oovdic);
    if ((personalDic = kb_get_personaldic()) != NULL) {
	if (stat (personalDic, &statbuf) == 0)	/* personalDic exists */
	    j += get_dict_size (personalDic);
    }
    
    if ((max_new_oov = kb_get_max_new_oov ()) > 0)
	j += max_new_oov;
    if ((startsym_file = kb_get_startsym_file()) != NULL)
	j += get_dict_size (startsym_file);
    /* FIXME: <unk> is no longer used, is this still correct? */
    j += 4;	/* </s>, <s>, <unk> and <sil> */
    dict->dict.size_hint = j;
    
    if (use_context) {
	/* Context table size hint: (#CI*#CI)/2 */
	j = phoneCiCount();
	j = ((j*j)>>1)+1;

	lcHT.size_hint = j;
	rcHT.size_hint = j;
	lcList.size_hint = j;
	rcList.size_hint = j;
    }
    
    /* Load dictionaries */
    dict_load (dict, filename, &word_id,
	       use_context,
	       FALSE /* is a phrase dictionary */ );
    if (p_filename)
	dict_load (dict, p_filename, &word_id,
		   TRUE, /* use_context */
		   TRUE /* is a phrase dictionary */);
    
    /* Add words with known pronunciations but which are OOVs wrt LM */
    first_initial_oov = word_id;

    if ((oovdic = kb_get_oovdic()) != NULL)
	dict_load (dict, oovdic, &word_id, use_context, FALSE);
    if ((personalDic = kb_get_personaldic()) != NULL) {
	if (stat (personalDic, &statbuf) == 0)
	    dict_load (dict, personalDic, &word_id, use_context, FALSE);
    }
    
    last_initial_oov = word_id-1;
    
    /* Placeholders (dummy pronunciations) for new words that can be added at runtime */
    initial_dummy = first_dummy = word_id;
    if ((max_new_oov = kb_get_max_new_oov ()) > 0)
	E_INFO ("Allocating %d placeholders for new OOVs\n",
		max_new_oov);
    for (i = 0; i < max_new_oov; i++) {
	char tmpstr[100], pronstr[100];
	
	/* Pick a temporary name that doesn't occur in the LM */
	sprintf (tmpstr, "=PLCHLDR%d=", i);

	/* new_dict_entry clobbers pronstr! so need this strcpy in the loop */
	strcpy (pronstr, "SIL");
	entry = _new_dict_entry (tmpstr, pronstr, TRUE);
	if (! entry)
	    E_FATAL("Failed to add DUMMY(SIL) entry to dictionary\n");
	
	_dict_list_add (dict, entry);
	hash_add (&dict->dict, entry->word, (caddr_t)word_id);
	entry->wid = word_id;
	entry->fwid = word_id;
	word_id++;
    }
    last_dummy = word_id-1;
    
    /*
     * Special case the start symbol and end symbol phrase markers.
     * Special case the silence word 'SIL'.
     */
    {
	caddr_t val;
	
	if (hash_lookup (&dict->dict, kb_get_lm_end_sym(), &val)) {
	    /*
 	     * Check if there is a special end silence phone.
	     */
	    if (NO_PHONE == phone_to_id ("SILe", FALSE)) {
		entry = _new_dict_entry (kb_get_lm_end_sym(), "SIL", FALSE);
		if (! entry)
		    E_FATAL("Failed to add </s>(SIL) to dictionary\n");
	    } else {
		E_INFO ("Using special end silence for %s\n",
			kb_get_lm_end_sym());
		entry = _new_dict_entry (kb_get_lm_end_sym(), "SILe", FALSE);
	    }
	    _dict_list_add (dict, entry);
	    hash_add (&dict->dict, entry->word, (caddr_t)word_id);
	    entry->wid = word_id;
	    entry->fwid = word_id;
	    word_id++;
	}
	
	/* Mark the start of filler words */
	dict->filler_start = word_id;
	
	/* Add [multiple] start symbols to dictionary (LISTEN project) */
	if ((startsym_file = kb_get_startsym_file()) != NULL) {
	    FILE *ssfp;
	    char line[1000], startsym[1000];
	    char const *startsym_phone;
	    
	    E_INFO ("Reading start-syms file %s\n", startsym_file);
	    
	    startsym_phone = (phone_to_id ("SILb", FALSE) == NO_PHONE) ? "SIL" : "SILb";
	    ssfp = CM_fopen (startsym_file, "r");
	    while (fgets (line, sizeof(line), ssfp) != NULL) {
		if (sscanf (line, "%s", startsym) != 1)
		  E_FATAL("File format error\n");
		
		entry = _new_dict_entry (startsym, startsym_phone, FALSE);
		if (! entry)
		    E_FATAL("Failed to add %s to dictionary\n", startsym);
		_dict_list_add (dict, entry);
		hash_add (&dict->dict, entry->word, (caddr_t)word_id);
		entry->wid = word_id;
		entry->fwid = word_id;
		word_id++;
	    }
	}
	/* Add the standard start symbol (<s>) if not already in dict */
	if (hash_lookup (&dict->dict, kb_get_lm_start_sym(), &val)) {
	    /*
 	     * Check if there is a special begin silence phone.
	     */
	    if (NO_PHONE == phone_to_id ("SILb", FALSE)) {
		entry = _new_dict_entry (kb_get_lm_start_sym(), "SIL", FALSE);
		if (! entry)
		    E_FATAL("Failed to add <s>(SIL) to dictionary\n");
	    } else {
		E_INFO ("Using special begin silence for %s\n",
			kb_get_lm_start_sym());
		entry = _new_dict_entry (kb_get_lm_start_sym(), "SILb", FALSE);
		if (! entry)
		    E_FATAL("Failed to add <s>(SILb) to dictionary\n");
	    }
	    _dict_list_add (dict, entry);
    	    hash_add (&dict->dict, entry->word, (caddr_t)word_id);
	    entry->wid = word_id;
	    entry->fwid = word_id;
	    word_id++;
	}

	if (hash_lookup (&dict->dict, "SIL", &val)) {
	    entry = _new_dict_entry ("SIL", "SIL", FALSE);
	    if (! entry)
		E_FATAL("Failed to add <sil>(SIL) to dictionary\n");
	    _dict_list_add (dict, entry);
	    hash_add (&dict->dict, entry->word, (caddr_t)word_id);
	    entry->wid = word_id;
	    entry->fwid = word_id;
	    word_id++;
	}
    }
    
    if (n_filename)
	dict_load (dict, n_filename, &word_id,
		   FALSE, /* use_context */
		   FALSE  /* is a phrase dict */ );
    
    E_INFO ("LEFT CONTEXT TABLES\n");
    buildEntryTable(&lcList, &lcFwdTable);
    buildExitTable(&lcList, &lcBwdTable, &lcBwdPermTable, &lcBwdSizeTable);

    E_INFO ("RIGHT CONTEXT TABLES\n");
    buildEntryTable(&rcList, &rcBwdTable);
    buildExitTable(&rcList, &rcFwdTable, &rcFwdPermTable, &rcFwdSizeTable);

    E_INFO("%5d unique triphones were mapped to ci phones\n",
	     mtpHT.inuse);

    mtpList = hash_to_list (&mtpHT);
    hash_free (&mtpHT);
    
    return (retval);
}

#if 0
/*
 * Replace _ (underscores) in a compound word by spaces
 */
void
chk_compound_word (char *str)
{
    for (; *str; str++)
	if (*str == '_')
	    *str = ' ';
}
#endif

void dict_free (dictT *dict)
{
  int32 i;
  int32 entry_count;
  dict_entry_t * entry;

  entry_count = dict->dict_entry_count;

  for (i = 0; i < entry_count; i++) {
    entry = dict_get_entry(dict, i);
    free(entry->word);
    free(entry->phone_ids);
    free(entry->ci_phone_ids);
    free(entry);
  }

  free(dict->ci_index);
  hash_free(&dict->dict);
  free(dict);
}

static void
dict_load (dictT *dict, char *filename, int32 *word_id,
	   int32 use_context, int32 isa_phrase_dict)
{
    static char const *rname = "dict_load";
    char         dict_str[1024];
    char         pronoun_str[1024];
    dict_entry_t *entry;
    FILE 	*fs;
    int32	start_wid = *word_id;
    int32 err = 0;
    
    fs = CM_fopen (filename, "r");

    fscanf (fs, "%s\n", dict_str);
    if (strcmp(dict_str, "!") != 0) {
	E_INFO("%s: first line of %s was %s, expecting '!'\n",
		 rname, filename, dict_str);
	E_INFO("%s: will assume first line contains a word\n",
		 rname);
	rewind (fs);
    }

    pronoun_str[0] = '\0';
    while (EOF != fscanf (fs, "%s%[^\n]\n", dict_str, pronoun_str)) {
#if 0
	chk_compound_word (dict_str);
#endif
	entry = _new_dict_entry (dict_str, pronoun_str, use_context);
	if (! entry) {
	    E_ERROR("Failed to add %s to dictionary\n", dict_str);
	    err = 1;
	    continue;
	}
	
	_dict_list_add (dict, entry);
	hash_add (&dict->dict, entry->word, (caddr_t)*word_id);
        entry->wid = *word_id;
	entry->fwid = *word_id;
	entry->lm_pprob = 0;
	pronoun_str[0] = '\0';
	/*
	 * Look for words of the form ".*(#)". These words are
	 * alternate pronunciations. Also look for phrases
	 * concatenated with '_'.
	 */
	{
	    char *p = strrchr (dict_str, '(');
	    char *q = strchr (dict_str, '_');
	    char *r = strrchr (dict_str, '_');

	    /*
 	     * If this isn't a phrase dictionary then an '_' is an underscore
	     * and the phrase actually appears in the LM.
	     */
	    if (! isa_phrase_dict) {
		q = 0;
		r = 0;
	    }

	    /*
	     * For alternate pron. the last car of the word must be ')'
	     * This will handle the case where the word is something like
	     * "(LEFT_PAREN"
	     */
 	    if (dict_str[strlen(dict_str)-1] != ')')
		p = 0;

	    if ((p != 0) || (q != 0)) {
		caddr_t	wid;

		if (p) *p = '\0';
		if (q) *q = '\0';

		if (hash_lookup (&dict->dict, dict_str, &wid)) {
		    E_FATAL("%s: Missing first pronunciation for [%s]\nThis means that e.g. [%s(2)] was found with no [%s]\nPlease correct the dictionary and re-run.\n",
			      rname, dict_str, dict_str, dict_str);
		    exit(1);
		}
	 	DFPRINTF((stdout, "Alternate transcription for [%s](wid = %d)\n",
			  entry->word, (int32)wid));
		entry->wid = (int32)wid;
		entry->fwid = (int32)wid;
		{
		    while (dict->dict_list[(int32)wid]->alt >= 0)
			wid = (caddr_t)dict->dict_list[(int32)wid]->alt;
	  	    dict->dict_list[(int32)wid]->alt = *word_id;
		}
	    }
	    /*
 	     * Get the word id of the final word in the phrase
	     */
	    if ((r != 0) && isa_phrase_dict) {
		caddr_t wid;

		r += 1;
		
		if (hash_lookup (&dict->dict, r, &wid)) {
		    E_INFO("%s: Missing first pronunciation for [%s]\n",
			     rname, r);
		}
	 	E_INFO("phrase transcription for [%s](wid = %d)\n",
			   entry->word, (int32)wid);
		entry->fwid = (int32)wid;
	    }
	}

	*word_id = *word_id + 1;

#ifdef PHRASE_DEP_MODELS
	/*
 	 * Build a dict entry  for the with in word model
	 * cooresponding to this 'phrase'
  	 * 
	 * DOES THIS LOOK LIKE a HACK to YOU...??? It does to me too.
	 * Here is the story.
	 *	 These within word models are int32er than standard models.
	 * They are multiples of five states int32, + 1. (for the dummy state).
	 * The problem is that the decoder only deals with one flavor of model.
  	 * So what we do else where is break up these big models into little 5 state
	 * models. For instance if phrase ARE_THE we have the trabnscription.
	 *	ARE_THE 	AA R & DH AX
 	 *			AA ARE_THE AX
 	 *		 	AA ARE_THE ARE_THE(1) AX
 	 * Get the idea?
	 * This code is NOT!!!! DEBUGED !!!!
	 */
	if (isa_phrase_dict) {
	    char		tmpstr[256];
	    int32 		pid, i;
	    dict_entry_t 	*entry_copy =
				(dict_entry_t *) calloc ((size_t)1, sizeof (dict_entry_t));

	    /*
	     * Copy the entry and make the required edits
	     */	
	    memcpy (entry_copy, entry, sizeof (dict_entry_t));
	    /*
	     * Update the alt pronunciation index pointers
	     */
	    entry->alt = *word_id;
	    entry_copy->alt = -1;
	    /*
	     * Make new list of CD phones
	     */
	    entry_copy->phone_ids = (int32 *) calloc ((size_t)entry->len, sizeof(int32));
	    memcpy (entry_copy->phone_ids, entry->phone_ids, sizeof(int32) * entry->len);
	    /*
	     * Rename the word by appending "(0)" to indicate phrase
	     */
	    sprintf (tmpstr, "%s(0)", entry_copy->word);
	    entry_copy->word = (char *) salloc(tmpstr);

	    /*
 	     * Look up the first model
	     */
	    pid = phone_to_id (entry->word, TRUE);

	    if (phone_type(pid) != PT_WWPHONE) {
		E_FATAL ("%s: No with in word for for %s\n", rname, entry->word);
	    }

	    entry_copy->phone_ids[1] = hmm_pid2sid(pid);
	    /*
 	     * Look up remaining models
	     */
	    for (i = 2; i < entry_copy->len-2; i++) {
	    	sprintf (tmpstr, "%s(%d)", entry->word, i-1);
		pid = phone_to_id (tmpstr, TRUE);
		if (pid == NO_PHONE)
		    exit (-1);
	        entry_copy->phone_ids[i] = hmm_pid2sid(pid);
	    }

	    _dict_list_add (dict, entry_copy);
	    hash_add (&dict->dict, entry_copy->word, (caddr_t)*word_id);

	    *word_id = *word_id + 1;
	}
#endif
    }
  
    E_INFO("%6d = words in file [%s]\n",
	     *word_id - start_wid, filename);
    
    if (fs)
	fclose (fs);

    if (err) {
	E_FATAL("Dictionary errors; cannot continue\n");
    }
}

caddr_t
dictStrToWordId (dictT *dict, char const *dict_str, int verbose)
/*------------------------------------------------------------*
 * return the dict id for dict_str
 *------------------------------------------------------------*/
{
    static char const *rname = "dict_to_id";
    caddr_t dict_id;

    if (hash_lookup (&dict->dict, dict_str, &dict_id)) {
	if (verbose)
	    fprintf (stderr, "%s: did not find %s\n", rname, dict_str);
	return ((caddr_t) NO_WORD);
    }
	
    return (dict_id);
}

int32 dict_to_id (dictT *dict, char const *dict_str)
{
    return ((int32) dictStrToWordId (dict, dict_str, FALSE));
}

char const *dictid_to_str (dictT *dict, int32 id)
{
    return (dict->dict_list[id]->word);
}

#define MAX_PRONOUN_LEN 100

static dict_entry_t *
_new_dict_entry (char const *word_str, char const *pronoun_str, int32 use_context)
{
    dict_entry_t       *entry;
    char               *phone[MAX_PRONOUN_LEN];
    int32	        ciPhoneId[MAX_PRONOUN_LEN];
    int32	        triphone_ids[MAX_PRONOUN_LEN];
    int32                 pronoun_len = 0;
    int32		i;
    int32		lcTabId;
    int32		rcTabId;
    char 		triphoneStr[80];
    char		position[256];		/* phone position */
    
    memset (position, 0, sizeof(position));	/* zero out the position matrix */

    position[0] = 'b';		/* First phone is at begginging */

    while (1) {
	phone[pronoun_len] = (char *) nxtarg (&pronoun_str, " \t");
	if (*phone[pronoun_len] == 0)
	    break;
	/*
	 * An '&' in the phone string indicates that this is a word break and
 	 * and that the previous phone is in the end of word position and the
	 * next phone is the begining of word position
	 */
	if (phone[pronoun_len][0] == '&') {
	    position[pronoun_len-1] = 'e';
	    position[pronoun_len] = 'b';
	    continue;
    	}
	ciPhoneId[pronoun_len] = phone_to_id (phone[pronoun_len], TRUE);
	if (ciPhoneId[pronoun_len] == NO_PHONE) {
	    E_ERROR("'%s': Unknown phone '%s'\n", word_str, phone[pronoun_len]);
	    return NULL;
	}
	pronoun_len++;
    }

    position[pronoun_len-1] = 'e';	/* Last phone is at the end */

    /*
     * If the position marker sequence 'ee' appears or 'se' appears
     * the sequence should be '*s'.
     */

    if (position[0] == 'e')		/* Also handle single phone word case */
	position[0] = 's';

    for (i = 0; i < pronoun_len-1; i++) {
	if (((position[i] == 'e') || (position[i] == 's')) &&
	   (position[i+1] == 'e'))
	    position[i+1] = 's';
    }

    if (pronoun_len >= 2) {
	i = 0;

	if (use_context) {
	    sprintf (triphoneStr, "%s(%%s,%s)b", phone[i], phone[i+1]);
	    lcTabId = addToLeftContextTable (triphoneStr);
	    triphone_ids[i] = lcTabId;
	}
	else {
	    sprintf (triphoneStr, "%s(%s,%s)b", phone[i], "%s", phone[i+1]);
	    triphone_ids[i] = phone_to_id (triphoneStr, FALSE);
	    if (triphone_ids[i] < 0) {
		triphone_ids[i] = phone_to_id (phone[i], TRUE);
		recordMissingTriphone (triphoneStr);
	    }
	    triphone_ids[i] = hmm_pid2sid(phone_map(triphone_ids[i]));
	}

	for (i = 1; i < pronoun_len-1; i++) {
	    sprintf (triphoneStr, "%s(%s,%s)%c",
		     phone[i], phone[i-1], phone[i+1], position[i]);
	    triphone_ids[i] = phone_to_id (triphoneStr, FALSE);
	    if (triphone_ids[i] < 0) {
		triphone_ids[i] = phone_to_id (phone[i], TRUE);
	        recordMissingTriphone (triphoneStr);
	    }
	    triphone_ids[i] = hmm_pid2sid(triphone_ids[i]);
	}

	if (use_context) {
	    sprintf (triphoneStr, "%s(%s,%%s)e", phone[i], phone[i-1]);
	    rcTabId = addToRightContextTable (triphoneStr);
	    triphone_ids[i] = rcTabId;
	}
	else {
	    sprintf (triphoneStr, "%s(%s,%s)e", phone[i], phone[i-1], "%s");
	    triphone_ids[i] = phone_to_id (triphoneStr, FALSE);
            if (triphone_ids[i] < 0) {
		triphone_ids[i] = phone_to_id (phone[i], TRUE);
 		recordMissingTriphone (triphoneStr);
	    }
	    triphone_ids[i] = hmm_pid2sid(phone_map(triphone_ids[i]));
	}
    }

    /*
     * It's too hard to model both contexts so I choose to model only
     * the left context.
     */
    if (pronoun_len == 1) {
	if (use_context) {
	    sprintf (triphoneStr, "%s(%%s,SIL)s", phone[0]);
	    lcTabId = addToLeftContextTable (triphoneStr);
	    triphone_ids[0] = lcTabId;
	    /*
	     * Put the right context table in the 2 entry
	     */
	    sprintf (triphoneStr, "%s(SIL,%%s)s", phone[0]);
	    rcTabId = addToRightContextTable (triphoneStr);
	    triphone_ids[1] = rcTabId;
	}
	else {
	    sprintf (triphoneStr, "%s(%s,%s)s", phone[0], "%s", "%s");
	    triphone_ids[0] = phone_to_id (triphoneStr, FALSE);
	    if (triphone_ids[0] < 0) {
		triphone_ids[0] = phone_to_id (phone[0], TRUE);
	    }
	    triphone_ids[i] = hmm_pid2sid(triphone_ids[i]);
	}
    }
    
    entry = (dict_entry_t *) calloc ((size_t)1, sizeof (dict_entry_t));
    entry->word = (char *) salloc (word_str);
    entry->len = pronoun_len;
    entry->mpx = use_context;
    entry->alt = -1;
    if (pronoun_len != 0) {
	entry->ci_phone_ids = (int32 *) calloc ((size_t)pronoun_len, sizeof (int32));
	memcpy (entry->ci_phone_ids, ciPhoneId, pronoun_len * sizeof (int32));
	/*
	 * This is a HACK to handle the left right conflict on
	 * single phone words
	 */		
	if (use_context && (pronoun_len == 1))
	    pronoun_len += 1;
	entry->phone_ids = (int32 *) calloc ((size_t)pronoun_len, sizeof (int32));
	memcpy (entry->phone_ids, triphone_ids, pronoun_len * sizeof (int32));
    } else {
    	E_WARN("%s has no pronounciation, will treat as dummy word\n",
		 word_str);
    }

    return (entry);
}

/*
 * Replace an existing dictionary entry with the given one.  The existing entry
 * might be a dummy placeholder for new OOV words, in which case the new_entry argument
 * would be TRUE.  (Some restrictions at this time on words that can be added; the code
 * should be self-explanatory.)
 * Return 1 if successful, 0 if not.
 */
static int32 replace_dict_entry (dictT *dict,
				 dict_entry_t *entry, char const *word_str,
				 char const *pronoun_str,
				 int32 use_context, int32 new_entry)
{
    char *phone[MAX_PRONOUN_LEN];
    int32 ciPhoneId[MAX_PRONOUN_LEN];
    int32 triphone_ids[MAX_PRONOUN_LEN];
    int32 pronoun_len = 0;
    int32 i;
    char triphoneStr[80];
    caddr_t idx;
    int32 basewid;
    
    /* For the moment assume left/right context words... */
    assert (use_context);

    /* For the moment, no phrase dictionary stuff... */
    while (1) {
	phone[pronoun_len] = (char *) nxtarg (&pronoun_str, " \t");
	if (*phone[pronoun_len] == 0)
	    break;
	ciPhoneId[pronoun_len] = phone_to_id (phone[pronoun_len], TRUE);
	if (ciPhoneId[pronoun_len] == NO_PHONE) {
	    E_ERROR("'%s': Unknown phone '%s'\n", word_str, phone[pronoun_len]);
	    return 0;
	}
	
	pronoun_len++;
    }

    /* For the moment, no single phone new word... */
    if (pronoun_len < 2) {
	E_ERROR("Pronunciation string too short\n");
	return (0);
    }

    /* Check if it's an alternative pronunciation; if so base word must exist */
    {
	char *p = strrchr (word_str, '(');
	if (p && (word_str[strlen(word_str)-1] == ')')) {
	    *p = '\0';
	    if (hash_lookup (&dict->dict, word_str, &idx)) {
		*p = '(';
		E_ERROR("Base word missing for %s\n", word_str);
		return 0;
	    }
	    *p = '(';
	    basewid = (int32)idx;
	} else
	    basewid = -1;
    }
    
    /* Parse pron; for the moment, the boundary diphones must be already known... */
    i = 0;
    sprintf (triphoneStr, "%s(%%s,%s)b", phone[i], phone[i+1]);
    if (hash_lookup (&lcHT, triphoneStr, &idx) < 0) {
	E_ERROR("Unknown left diphone '%s'\n", triphoneStr);
	return (0);
    }
    triphone_ids[i] = (int32) idx;

    for (i = 1; i < pronoun_len-1; i++) {
	sprintf (triphoneStr, "%s(%s,%s)", phone[i], phone[i-1], phone[i+1]);
	triphone_ids[i] = phone_to_id (triphoneStr, FALSE);
	if (triphone_ids[i] < 0)
	    triphone_ids[i] = phone_to_id (phone[i], TRUE);
	triphone_ids[i] = hmm_pid2sid(triphone_ids[i]);
    }

    sprintf (triphoneStr, "%s(%s,%%s)e", phone[i], phone[i-1]);
    if (hash_lookup (&rcHT, triphoneStr, &idx) < 0) {
	E_ERROR("Unknown right diphone '%s'\n", triphoneStr);
	return (0);
    }
    triphone_ids[i] = (int32) idx;
    
    /*
     * Set up dictionary entry.  Free the existing attributes (where applicable) and
     * replace with new ones.
     */
    entry->len = pronoun_len;
    entry->mpx = use_context;
    free (entry->word);
    free (entry->ci_phone_ids);
    free (entry->phone_ids);
    entry->word = (char *) salloc (word_str);
    entry->ci_phone_ids = (int32 *) CM_calloc ((size_t)pronoun_len, sizeof (int32));
    entry->phone_ids = (int32 *) CM_calloc ((size_t)pronoun_len, sizeof (int32));
    memcpy (entry->ci_phone_ids, ciPhoneId, pronoun_len * sizeof (int32));
    memcpy (entry->phone_ids, triphone_ids, pronoun_len * sizeof (int32));
    
    /* Update alternatives linking if adding a new entry (not updating existing one) */
    if (new_entry) {
	entry->alt = -1;
	if (basewid >= 0) {
	    entry->alt = dict->dict_list[(int32)basewid]->alt;
	    dict->dict_list[(int32)basewid]->alt = entry->wid;
	    entry->wid = entry->fwid = (int32)basewid;
	}
    }
    
    return (1);
}

/*
 * Add a new word to the dictionary, replacing a dummy placeholder.  Or replace an
 * existing non-dummy word in the dictionary.
 * Return the word id of the entry updated if successful.  If any error, return -1.
 */
int32 dict_add_word (dictT *dict, char const *word, char const *pron)
{
    dict_entry_t *entry;
    int32 wid, new_entry;
    
    /* Word already exists */
    new_entry = 0;
    if ((wid = kb_get_word_id(word)) < 0) {
	if (first_dummy > last_dummy) {
	    E_ERROR ("Dictionary full; cannot add word\n");
	    return -1;
	}
	wid = first_dummy++;
	new_entry = 1;
    }
    
    entry = dict->dict_list[wid];
    if (! replace_dict_entry (dict, entry, word, pron, TRUE, new_entry))
	return -1;

    hash_add (&dict->dict, entry->word, (caddr_t) wid);
    
    return (wid);
}

static void
_dict_list_add (dictT *dict, dict_entry_t *entry)
/*------------------------------------------------------------*/
{
    if (! dict->dict_list)
	dict->dict_list = (dict_entry_t **)
	    CM_calloc (dict->dict.size_hint, sizeof (dict_entry_t *));

    if (dict->dict_entry_count >= dict->dict.size_hint) {
	E_FATAL("dict size (%d) exceeded\n", dict->dict.size_hint);
#if 0
	dict->dict.size_hint = dict->dict_entry_count + 16;
	dict->dict_list = (dict_entry_t **)
	    CM_recalloc (dict->dict_list, dict->dict.size_hint, sizeof (dict_entry_t *));
#endif
    }

    dict->dict_list[dict->dict_entry_count++] = entry;
}

dict_entry_t *
dict_get_entry (dictT *dict, int i)
{
    return ((i < dict->dict_entry_count) ?
		dict->dict_list[i] : (dict_entry_t *) 0);
}

/* FIXME: could be extern inline */
int32
dict_count (dictT *dict)
{
    return dict->dict_entry_count;
}

dictT *dict_new (void)
{
    return (dictT *) CM_calloc (sizeof(dictT), 1);
}

static void
recordMissingTriphone (char *triphoneStr)
{
    caddr_t idx;
    char *cp;

    if (-1 == hash_lookup (&mtpHT, triphoneStr, &idx)) {
	cp = (char *) salloc(triphoneStr);
        E_INFO("Missing triphone: %s\n", triphoneStr);
	hash_add (&mtpHT, cp, cp);
    }
}

list_t *dict_mtpList (void)
{
    return mtpList;
}

static int32
addToContextTable (char *diphone, hash_t *table, list_t *list)
{
    caddr_t idx;
    char *cp;

    if (-1 == hash_lookup (table, diphone, &idx)) {
	cp = (char *) salloc(diphone);
	idx = (caddr_t) table->inuse;
	list_insert (list, cp);
	hash_add (table, cp, idx);
    }
    return ((int32) idx);
}

static int32
addToLeftContextTable (char *diphone)
{
    return addToContextTable (diphone, &lcHT, &lcList);
}

static int32
addToRightContextTable (char *diphone)
{
    return addToContextTable (diphone, &rcHT, &rcList);
}

static int
cmp (void const  *a, void const *b)
{
    return (*(int32 const *)a - *(int32 const *)b);
}

int32 *linkTable;

static int
cmpPT (void const *a, void const *b)
{
    return (linkTable[*(int32 const *)a] - linkTable[*(int32 const *)b]);
}

static void
buildEntryTable (list_t *list, int32 ***table_p)
{
    int32 ciCount = phoneCiCount();
    int32 i, j;
    char triphoneStr[128];
    int32 silContext = 0;
    int32 triphoneContext = 0;
    int32 noContext = 0;
    int32 **table;

    *table_p = (int32 **) CM_calloc (list->in_use, sizeof (int32 *));
    table = *table_p;
    E_INFO("Entry Context table contains\n\t%6d entries\n", list->in_use);
    E_INFO("\t%6d possible cross word triphones.\n", list->in_use * ciCount);

    for (i = 0; i < list->in_use; i++) {
	table[i] = (int32 *) CM_calloc (ciCount, sizeof(int32));
	for (j = 0; j < ciCount; j++) {
	    /*
	     * Look for the triphone
	     */
	    sprintf (triphoneStr, list->list[i], phone_from_id (j));
	    table[i][j] = phone_to_id (triphoneStr, FALSE);
	    if (table[i][j] >= 0)
		triphoneContext++;
	    /*
	     * If we can't find the desired right context use "SIL"
	     */
	    if (table[i][j] < 0) {
		sprintf (triphoneStr, list->list[i], "SIL");
		table[i][j] = phone_to_id (triphoneStr, FALSE);
		if (table[i][j] >= 0)
		    silContext++;
	    }
	    /*
	     * If we can't find "SIL" use context indepedent
	     */
	    if (table[i][j] < 0) {
		char stmp[32];
		char *p;
		strcpy (stmp, list->list[i]);
		p = strchr (stmp, '(');
		*p = '\0';
		table[i][j] = phone_to_id (stmp, TRUE);
		noContext++;
	    }
	    table[i][j] = hmm_pid2sid(phone_map(table[i][j]));
	}
    }
    E_INFO("\t%6d triphones\n\t%6d pseudo diphones\n\t%6d uniphones\n",
	     triphoneContext, silContext, noContext);
}

static void
buildExitTable (list_t *list, int32 ***table_p, int32 ***permuTab_p, int32 **sizeTab_p)
{
    int32 ciCount = phoneCiCount();
    int32 i, j, k;
    char triphoneStr[128];
    int32 silContext = 0;
    int32 triphoneContext = 0;
    int32 noContext = 0;
    int32 entries = 0;
    int32 **table;
    int32 **permuTab;
    int32 *sizeTab;
    int32 ptab[128];

    *table_p = (int32 **) CM_2dcalloc (list->in_use, ciCount+1, sizeof (int32 *));
    table = *table_p;
    *permuTab_p = (int32 **) CM_2dcalloc (list->in_use, ciCount+1, sizeof (int32 *));
    permuTab = *permuTab_p;
    *sizeTab_p = (int32 *) CM_calloc (list->in_use, sizeof (int32 *));
    sizeTab = *sizeTab_p;

    E_INFO("Exit Context table contains\n\t%6d entries\n", list->in_use);
    E_INFO("\t%6d possible cross word triphones.\n", list->in_use * ciCount);

    for (i = 0; i < list->in_use; i++) {
	for (j = 0; j < ciCount; j++) {
	    /*
	     * Look for the triphone
	     */
	    sprintf (triphoneStr, list->list[i], phone_from_id (j));
	    table[i][j] = phone_to_id (triphoneStr, FALSE);
	    if (table[i][j] >= 0)
		triphoneContext++;
	    /*
	     * If we can't find the desired context use "SIL"
	     */
	    if (table[i][j] < 0) {
		sprintf (triphoneStr, list->list[i], "SIL");
		table[i][j] = phone_to_id (triphoneStr, FALSE);
		if (table[i][j] >= 0)
		    silContext++;
	    }
	    /*
	     * If we can't find "SIL" use context indepedent
	     */
	    if (table[i][j] < 0) {
		char stmp[32];
		char *p;
		strcpy (stmp, list->list[i]);
		p = strchr (stmp, '(');
		*p = '\0';
		table[i][j] = phone_to_id (stmp, TRUE);
		noContext++;
	    }
	    table[i][j] = hmm_pid2sid(phone_map(table[i][j]));
	}
    }
    /*
     * Now compress the table to eliminate duplicate entries.
     */
    for (i = 0; i < list->in_use; i++) {
	/*
 	 * Set up the permutation table
	 */
	for (k = 0; k < ciCount; k++) {
	    ptab[k] = k;
	}
	linkTable = table[i];
	qsort (ptab, ciCount, sizeof(int32), cmpPT);

	qsort (table[i], ciCount, sizeof(int32), cmp);
	for (k = 0, j = 0; j < ciCount; j++) {
	    if (table[i][k] != table[i][j]) {
		k = k + 1;
		table[i][k] = table[i][j];
	    }
	    /*
 	     * Mirror the compression in the permutation table
	     */
	    permuTab[i][ptab[j]] = k;
	}
	table[i][k+1] = -1;	/* End of table Marker */
	sizeTab[i] = k+1;
	entries += k+1;
    }
    E_INFO("\t%6d triphones\n\t%6d pseudo diphones\n\t%6d uniphones\n",
	     triphoneContext, silContext, noContext);
    E_INFO("\t%6d right context entries\n", entries);
    E_INFO("\t%6d ave entries per exit context\n",
	    ((list->in_use == 0) ? 0 : entries/list->in_use));
}

int32 **dict_right_context_fwd (void)
{
    return rcFwdTable;
}

int32 **dict_right_context_fwd_perm (void)
{
    return rcFwdPermTable;
}

int32 *dict_right_context_fwd_size (void)
{
    return rcFwdSizeTable;
}

int32 **dict_left_context_fwd (void)
{
    return lcFwdTable;
}

int32 **dict_right_context_bwd (void)
{
    return rcBwdTable;
}

int32 **dict_left_context_bwd (void)
{
    return lcBwdTable;
}

int32 **dict_left_context_bwd_perm (void)
{
    return lcBwdPermTable;
}

int32 *dict_left_context_bwd_size (void)
{
    return lcBwdSizeTable;
}

int32 dict_get_num_main_words (dictT *dict)
{
    return ((int32) dictStrToWordId (dict, kb_get_lm_end_sym(), FALSE));
}

int32 dictid_to_baseid (dictT *dict, int32 wid)
{
    return (dict->dict_list[wid]->wid);
}

int32 dict_get_first_initial_oov ( void )
{
    return (first_initial_oov);
}

int32 dict_get_last_initial_oov ( void )
{
    return (last_initial_oov);
}

/*
 * Return TRUE iff wid is new word dynamically added at run time.
 */
int32 dict_is_new_word (int32 wid)
{
    return ((wid >= initial_dummy) && (wid <= last_dummy));
}

int32 dict_pron (dictT *dict, int32 w, int32 **pron)
{
    *pron = dict->dict_list[w]->ci_phone_ids;
    return (dict->dict_list[w]->len);
}

int32 dict_next_alt (dictT *dict, int32 w)
{
    return (dict->dict_list[w]->alt);
}

/* Write OOV words added at run time to the given file and return #words written */
int32 dict_write_oovdict (dictT *dict, char const *file)
{
    int32 w, p;
    FILE *fp;

    /* If no new words added at run time, no need to write a new file */
    if (initial_dummy == first_dummy) {
	E_ERROR("No new word added; no OOV file written\n");
	return 0;
    }

    if ((fp = fopen(file, "w")) == NULL) {
	E_ERROR("fopen(%s,w) failed\n", file);
	return -1;
    }

    /* Write OOV words added at run time */
    for (w = initial_dummy; w < first_dummy; w++) {
	fprintf (fp, "%s\t", dict->dict_list[w]->word);
	for (p = 0; p < dict->dict_list[w]->len; p++)
	    fprintf (fp, " %s", phone_from_id (dict->dict_list[w]->ci_phone_ids[p]));
	fprintf (fp, "\n");
    }

    fclose (fp);

    return (first_dummy - initial_dummy);
}


void dict_dump (dictT *dict, FILE *out)
{
  int32 w;
  dict_entry_t *de;
  int32 i;
  
  fprintf (out, "<dict>");
  for (w = 0; w < dict->dict_entry_count; w++) {
    de = dict->dict_list[w];
    fprintf (out, " <word index=\"%d\">\n", w);
    fprintf (out, "  <string>%s</string>\n", de->word);
    fprintf (out, "  <len>%d</len>\n", de->len);
    fprintf (out, "  <ci>");
    for (i = 0; i < de->len; i++)
      fprintf (out, " %d", de->ci_phone_ids[i]);
    fprintf (out, " </ci>\n");
    fprintf (out, "  <pid>");
    for (i = 0; i < de->len; i++)
      fprintf (out, " %d", de->phone_ids[i]);
    fprintf (out, " </pid>\n");
    fprintf (out, "  <wid>%d</wid>\n", de->wid);
    fprintf (out, "  <fwid>%d</fwid>\n", de->fwid);
    fprintf (out, "  <alt>%d</alt>\n", de->alt);
    fprintf (out, " </word>\n\n");
    fflush (out);
  }
  fprintf (out, "</dict>");
}


int32 dict_is_filler_word (dictT *dict, int32 wid)
{
  return (wid >= dict->filler_start);
}
