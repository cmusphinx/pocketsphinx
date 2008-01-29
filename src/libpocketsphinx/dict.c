/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
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

/* System headers. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* SphinxBase headers. */
#include <prim_type.h>
#include <cmd_ln.h>
#include <ckd_alloc.h>
#include <pio.h>
#include <hash_table.h>
#include <err.h>
#include <strfuncs.h>
#include <glist.h>

/* Local headers. */
#include "phone.h"
#include "dict.h"
#include "search_const.h"
#include "kb.h"

#ifdef DEBUG
#define DFPRINTF(x)		fprintf x
#else
#define DFPRINTF(x)
#endif

#define QUIT(x)		{fprintf x; exit(-1);}

extern int32 use_noise_words;

static void buildEntryTable(glist_t list, int32 *** table_p);
static void buildExitTable(glist_t list, int32 *** table_p,
                           int32 *** permuTab_p, int32 ** sizeTab_p);
static int32 addToLeftContextTable(char *diphone);
static int32 addToRightContextTable(char *diphone);
static void recordMissingTriphone(char *triphoneStr);
static dict_entry_t *_new_dict_entry(char *word_str,
                                     char *pronoun_str,
                                     int32 use_context);
static void _dict_list_add(dict_t * dict, dict_entry_t * entry);
static void dict_load(dict_t * dict, char *filename, int32 * word_id,
                      int32 use_context);

static hash_table_t *mtpHT;     /* Missing triphone hash table */
static glist_t mtpList;

static hash_table_t *lcHT;      /* Left context hash table */
static glist_t lcList;
static int32 **lcFwdTable;
static int32 **lcBwdTable;
static int32 **lcBwdPermTable;
static int32 *lcBwdSizeTable;

static hash_table_t *rcHT;      /* Right context hash table */
static glist_t rcList;
static int32 **rcFwdTable;
static int32 **rcFwdPermTable;
static int32 **rcBwdTable;
static int32 *rcFwdSizeTable;

/* Placeholders for dynamically added new words, OOVs */
static int32 initial_dummy;     /* 1st placeholder for dynamic OOVs after initialization */
static int32 first_dummy;       /* 1st dummy available for dynamic OOVs at any time */
static int32 last_dummy;        /* last dummy available for dynamic OOVs */

#define MAX_PRONOUN_LEN 	150

static int32
get_dict_size(char *file)
{
    FILE *fp;
    char line[1024];
    int32 n;

    fp = myfopen(file, "r");
    for (n = 0;; n++)
        if (fgets(line, sizeof(line), fp) == NULL)
            break;
    fclose(fp);

    return n;
}

int32
dict_read(dict_t * dict, char *filename, /* Main dict file */
          char *n_filename,     /* Noise dict file */
          int32 use_context)
/*------------------------------------------------------------*
 * read in the dict file filename
 *------------------------------------------------------------*/
{
    int32 retval = 0;
    int32 word_id = 0, i, j;
    dict_entry_t *entry;
    int32 max_new_oov;
    void *val;

    /*
     * Find size of dictionary and set hash and list table size hints.
     * (Otherwise, the simple-minded PC malloc library goes berserk.)
     */
    j = get_dict_size(filename);
    if (n_filename)
        j += get_dict_size(n_filename);
    /* FIXME: <unk> is no longer used, is this still correct? */
    j += 4;                     /* </s>, <s>, <unk> and <sil> */
    if (dict->dict)
        hash_table_free(dict->dict);
    dict->dict = hash_table_new(j, HASH_CASE_NO);

    /* Context table size hint: (#CI*#CI)/2 */
    j = phoneCiCount();
    j = ((j * j) >> 1) + 1;
    mtpHT = hash_table_new(j, HASH_CASE_YES);
    if (use_context) {
        if (lcHT)
            hash_table_free(lcHT);
        lcHT = hash_table_new(j, HASH_CASE_YES);
        if (rcHT)
            hash_table_free(rcHT);
        rcHT = hash_table_new(j, HASH_CASE_YES);
        if (lcList)
            glist_free(lcList);
        lcList = NULL;
        if (rcList)
            glist_free(rcList);
        rcList = NULL;
    }

    /* Placeholders (dummy pronunciations) for new words that can be
     * added at runtime.  We can expand this region of the dictionary
     * later if need be. */
    initial_dummy = first_dummy = word_id;
    if ((max_new_oov = cmd_ln_int32("-maxnewoov")) > 0)
        E_INFO("Allocating %d placeholders for new OOVs\n", max_new_oov);
    for (i = 0; i < max_new_oov; i++) {
        char tmpstr[100], pronstr[100];

        /* Pick a temporary name that doesn't occur in the LM */
        sprintf(tmpstr, "=PLCHLDR%d=", i);

        /* new_dict_entry clobbers pronstr! so need this strcpy in the loop */
        strcpy(pronstr, "SIL");
        entry = _new_dict_entry(tmpstr, pronstr, TRUE);
        if (!entry)
            E_FATAL("Failed to add DUMMY(SIL) entry to dictionary\n");

        _dict_list_add(dict, entry);
        (void)hash_table_enter_int32(dict->dict, entry->word, word_id);
        entry->wid = word_id;
        word_id++;
    }
    last_dummy = word_id - 1;

    /* Load dictionaries */
    dict_load(dict, filename, &word_id, use_context);

    /* Make sure that <s>, </s>, and <sil> are always in the dictionary. */
    if (hash_table_lookup(dict->dict, "</s>", &val) != 0) {
        char pronstr[5];
        /*
         * Check if there is a special end silence phone.
         */
        if (NO_PHONE == phone_to_id("SILe", FALSE)) {
            strcpy(pronstr, "SIL");
            entry = _new_dict_entry("</s>", pronstr, FALSE);
            if (!entry)
                E_FATAL("Failed to add </s>(SIL) to dictionary\n");
        }
        else {
            E_INFO("Using special end silence for </s>\n");
            strcpy(pronstr, "SILe");
            entry =
                _new_dict_entry("</s>", pronstr, FALSE);
        }
        _dict_list_add(dict, entry);
        hash_table_enter(dict->dict, entry->word, (void *)(long)word_id);
        entry->wid = word_id;
        word_id++;
    }

    /* Mark the start of filler words */
    dict->filler_start = word_id;

    /* Add the standard start symbol (<s>) if not already in dict */
    if (hash_table_lookup(dict->dict, "<s>", &val) != 0) {
        char pronstr[5];
        /*
         * Check if there is a special begin silence phone.
         */
        if (NO_PHONE == phone_to_id("SILb", FALSE)) {
            strcpy(pronstr, "SIL");
            entry =
                _new_dict_entry("<s>", pronstr, FALSE);
            if (!entry)
                E_FATAL("Failed to add <s>(SIL) to dictionary\n");
        }
        else {
            E_INFO("Using special begin silence for <s>\n");
            strcpy(pronstr, "SILb");
            entry =
                _new_dict_entry("<s>", pronstr, FALSE);
            if (!entry)
                E_FATAL("Failed to add <s>(SILb) to dictionary\n");
        }
        _dict_list_add(dict, entry);
        hash_table_enter(dict->dict, entry->word, (void *)(long)word_id);
        entry->wid = word_id;
        word_id++;
    }

    /* Finally create a silence word if it isn't there already. */
    if (hash_table_lookup(dict->dict, "<sil>", &val) != 0) {
        char pronstr[4];

        strcpy(pronstr, "SIL");
        entry = _new_dict_entry("<sil>", pronstr, FALSE);
        if (!entry)
            E_FATAL("Failed to add <sil>(SIL) to dictionary\n");
        _dict_list_add(dict, entry);
        hash_table_enter(dict->dict, entry->word, (void *)(long)word_id);
        entry->wid = word_id;
        word_id++;
    }

    if (n_filename)
        dict_load(dict, n_filename, &word_id, FALSE /* use_context */);

    E_INFO("LEFT CONTEXT TABLES\n");
    lcList = glist_reverse(lcList);
    buildEntryTable(lcList, &lcFwdTable);
    buildExitTable(lcList, &lcBwdTable, &lcBwdPermTable, &lcBwdSizeTable);

    E_INFO("RIGHT CONTEXT TABLES\n");
    rcList = glist_reverse(rcList);
    buildEntryTable(rcList, &rcBwdTable);
    buildExitTable(rcList, &rcFwdTable, &rcFwdPermTable, &rcFwdSizeTable);

    mtpList = hash_table_tolist(mtpHT, &i);
    E_INFO("%5d unique triphones were mapped to ci phones\n", i);
    /* Free all the strings in mtpHT.  FIXME: There should be a
     * function for this in libutil. */
    for (i = 0; i < mtpHT->size; ++i) {
        hash_entry_t *e;

        ckd_free(mtpHT->table[i].val);
        for (e = mtpHT->table[i].next; e; e = e->next) {
            ckd_free(e->val);
        }
    }
    hash_table_free(mtpHT);
    mtpHT = NULL;

    return (retval);
}

void
dict_cleanup(void)
{
    int32 i;
    gnode_t *gn;

    for (i = 0, gn = lcList; gn; gn = gnode_next(gn), ++i) {
        ckd_free(lcFwdTable[i]);
        ckd_free(gnode_ptr(gn));
    }
    ckd_free(lcFwdTable); lcFwdTable = NULL;
    ckd_free_2d((void **)lcBwdTable); lcBwdTable = NULL;
    ckd_free_2d((void **)lcBwdPermTable); lcBwdPermTable = NULL;
    ckd_free(lcBwdSizeTable); lcBwdSizeTable = NULL;
    if (lcHT)
        hash_table_free(lcHT);
    lcHT = NULL;
    glist_free(lcList); lcList = NULL;

    for (i = 0, gn = rcList; gn; gn = gnode_next(gn), ++i) {
        ckd_free(rcBwdTable[i]);
        ckd_free(gnode_ptr(gn));
    }
    ckd_free(rcBwdTable); rcBwdTable = NULL;
    ckd_free_2d((void **)rcFwdTable); rcFwdTable = NULL;
    ckd_free_2d((void **)rcFwdPermTable); rcFwdPermTable = NULL;
    ckd_free(rcFwdSizeTable); rcFwdSizeTable = NULL;
    if (rcHT)
        hash_table_free(rcHT);
    rcHT = NULL;
    glist_free(rcList); rcList = NULL;
    glist_free(mtpList); mtpList = NULL;
}


void
dict_free(dict_t * dict)
{
    int32 i;
    int32 entry_count;
    dict_entry_t *entry;

    entry_count = dict->dict_entry_count;

    for (i = 0; i < entry_count; i++) {
        entry = dict_get_entry(dict, i);
        free(entry->word);
        free(entry->phone_ids);
        free(entry->ci_phone_ids);
        free(entry);
    }

    free(dict->dict_list);
    free(dict->ci_index);
    hash_table_free(dict->dict);
    free(dict);
}

static void
dict_load(dict_t * dict, char *filename, int32 * word_id, int32 use_context)
{
    static char const *rname = "dict_load";
    char dict_str[1024];
    char pronoun_str[1024];
    dict_entry_t *entry;
    FILE *fs;
    int32 start_wid = *word_id;
    int32 err = 0;

    fs = myfopen(filename, "r");

    fscanf(fs, "%s\n", dict_str);
    if (strcmp(dict_str, "!") != 0) {
        E_INFO("%s: first line of %s was %s, expecting '!'\n",
               rname, filename, dict_str);
        E_INFO("%s: will assume first line contains a word\n", rname);
        fseek(fs, 0, SEEK_SET);
    }

    pronoun_str[0] = '\0';
    while (EOF != fscanf(fs, "%s%[^\n]\n", dict_str, pronoun_str)) {
        entry = _new_dict_entry(dict_str, pronoun_str, use_context);
        if (!entry) {
            E_ERROR("Failed to add %s to dictionary\n", dict_str);
            err = 1;
            continue;
        }

        _dict_list_add(dict, entry);
        hash_table_enter(dict->dict, entry->word, (void *)(long)*word_id);
        entry->wid = *word_id;
        pronoun_str[0] = '\0';
        /*
         * Look for words of the form ".*(#)". These words are
         * alternate pronunciations. Also look for phrases
         * concatenated with '_'.
         */
        {
            char *p = strrchr(dict_str, '(');

            /*
             * For alternate pron. the last car of the word must be ')'
             * This will handle the case where the word is something like
             * "(LEFT_PAREN"
             */
            if (dict_str[strlen(dict_str) - 1] != ')')
                p = NULL;

            if (p != NULL) {
                void *wid;

                *p = '\0';
                if (hash_table_lookup(dict->dict, dict_str, &wid) != 0) {
                    E_FATAL
                        ("%s: Missing first pronunciation for [%s]\nThis means that e.g. [%s(2)] was found with no [%s]\nPlease correct the dictionary and re-run.\n",
                         rname, dict_str, dict_str, dict_str);
                    exit(1);
                }
                DFPRINTF((stdout,
                          "Alternate transcription for [%s](wid = %d)\n",
                          entry->word, (long)wid));
                entry->wid = (long)wid;
                {
                    while (dict->dict_list[(long)wid]->alt >= 0)
                        wid = (void *)(long)dict->dict_list[(long)wid]->alt;
                    dict->dict_list[(long)wid]->alt = *word_id;
                }
            }
        }

        *word_id = *word_id + 1;
    }

    E_INFO("%6d = words in file [%s]\n", *word_id - start_wid, filename);

    if (fs)
        fclose(fs);

    if (err) {
        E_FATAL("Dictionary errors; cannot continue\n");
    }
}

int32
dictStrToWordId(dict_t * dict, char const *dict_str, int verbose)
/*------------------------------------------------------------*
 * return the dict id for dict_str
 *------------------------------------------------------------*/
{
    static char const *rname = "dict_to_id";
    void * dict_id;

    if (hash_table_lookup(dict->dict, dict_str, &dict_id)) {
        if (verbose)
            fprintf(stderr, "%s: did not find %s\n", rname, dict_str);
        return NO_WORD;
    }

    return (int32)(long)dict_id;
}

int32
dict_to_id(dict_t * dict, char const *dict_str)
{
    return ((int32) dictStrToWordId(dict, dict_str, FALSE));
}

char const *
dictid_to_str(dict_t * dict, int32 id)
{
    return (dict->dict_list[id]->word);
}

static dict_entry_t *
_new_dict_entry(char *word_str, char *pronoun_str, int32 use_context)
{
    dict_entry_t *entry;
    char *phone[MAX_PRONOUN_LEN];
    int32 ciPhoneId[MAX_PRONOUN_LEN];
    int32 triphone_ids[MAX_PRONOUN_LEN];
    int32 pronoun_len = 0;
    int32 i;
    int32 lcTabId;
    int32 rcTabId;
    char triphoneStr[80];
    char position[256];         /* phone position */

    memset(position, 0, sizeof(position));      /* zero out the position matrix */

    position[0] = 'b';          /* First phone is at begginging */

    while (1) {
        int n;
        char delim;

	if (pronoun_len >= MAX_PRONOUN_LEN) {
	    E_ERROR("'%s': Too many phones for bogus hard-coded limit (%d), skipping\n",
		    word_str, MAX_PRONOUN_LEN);
	    return NULL;
	}
        n = nextword(pronoun_str, " \t", &phone[pronoun_len], &delim);
        if (n < 0)
            break;
        pronoun_str = phone[pronoun_len] + n + 1;
        /*
         * An '&' in the phone string indicates that this is a word break and
         * and that the previous phone is in the end of word position and the
         * next phone is the begining of word position
         */
        if (phone[pronoun_len][0] == '&') {
            position[pronoun_len - 1] = 'e';
            position[pronoun_len] = 'b';
            continue;
        }
        ciPhoneId[pronoun_len] = phone_to_id(phone[pronoun_len], TRUE);
        if (ciPhoneId[pronoun_len] == NO_PHONE) {
            E_ERROR("'%s': Unknown phone '%s'\n", word_str,
                    phone[pronoun_len]);
            return NULL;
        }
        pronoun_len++;
        if (delim == '\0')
            break;
    }

    position[pronoun_len - 1] = 'e';    /* Last phone is at the end */

    /*
     * If the position marker sequence 'ee' appears or 'se' appears
     * the sequence should be '*s'.
     */

    if (position[0] == 'e')     /* Also handle single phone word case */
        position[0] = 's';

    for (i = 0; i < pronoun_len - 1; i++) {
        if (((position[i] == 'e') || (position[i] == 's')) &&
            (position[i + 1] == 'e'))
            position[i + 1] = 's';
    }

    if (pronoun_len >= 2) {
        i = 0;

        if (use_context) {
            sprintf(triphoneStr, "%s(%%s,%s)b", phone[i], phone[i + 1]);
            lcTabId = addToLeftContextTable(triphoneStr);
            triphone_ids[i] = lcTabId;
        }
        else {
            sprintf(triphoneStr, "%s(%s,%s)b", phone[i], "%s",
                    phone[i + 1]);
            triphone_ids[i] = phone_to_id(triphoneStr, FALSE);
            if (triphone_ids[i] < 0) {
                triphone_ids[i] = phone_to_id(phone[i], TRUE);
                recordMissingTriphone(triphoneStr);
            }
            triphone_ids[i] = bin_mdef_pid2ssid(mdef, phone_map(triphone_ids[i]));
        }

        for (i = 1; i < pronoun_len - 1; i++) {
            sprintf(triphoneStr, "%s(%s,%s)%c",
                    phone[i], phone[i - 1], phone[i + 1], position[i]);
            triphone_ids[i] = phone_to_id(triphoneStr, FALSE);
            if (triphone_ids[i] < 0) {
                triphone_ids[i] = phone_to_id(phone[i], TRUE);
                recordMissingTriphone(triphoneStr);
            }
            triphone_ids[i] = bin_mdef_pid2ssid(mdef,triphone_ids[i]);
        }

        if (use_context) {
            sprintf(triphoneStr, "%s(%s,%%s)e", phone[i], phone[i - 1]);
            rcTabId = addToRightContextTable(triphoneStr);
            triphone_ids[i] = rcTabId;
        }
        else {
            sprintf(triphoneStr, "%s(%s,%s)e", phone[i], phone[i - 1],
                    "%s");
            triphone_ids[i] = phone_to_id(triphoneStr, FALSE);
            if (triphone_ids[i] < 0) {
                triphone_ids[i] = phone_to_id(phone[i], TRUE);
                recordMissingTriphone(triphoneStr);
            }
            triphone_ids[i] = bin_mdef_pid2ssid(mdef,phone_map(triphone_ids[i]));
        }
    }

    /*
     * It's too hard to model both contexts so I choose to model only
     * the left context.
     */
    if (pronoun_len == 1) {
        if (use_context) {
            sprintf(triphoneStr, "%s(%%s,SIL)s", phone[0]);
            lcTabId = addToLeftContextTable(triphoneStr);
            triphone_ids[0] = lcTabId;
            /*
             * Put the right context table in the 2 entry
             */
            sprintf(triphoneStr, "%s(SIL,%%s)s", phone[0]);
            rcTabId = addToRightContextTable(triphoneStr);
            triphone_ids[1] = rcTabId;
        }
        else {
            sprintf(triphoneStr, "%s(%s,%s)s", phone[0], "%s", "%s");
            triphone_ids[0] = phone_to_id(triphoneStr, FALSE);
            if (triphone_ids[0] < 0) {
                triphone_ids[0] = phone_to_id(phone[0], TRUE);
            }
            triphone_ids[i] = bin_mdef_pid2ssid(mdef,triphone_ids[i]);
        }
    }

    entry = (dict_entry_t *) calloc((size_t) 1, sizeof(dict_entry_t));
    entry->word = ckd_salloc(word_str);
    entry->len = pronoun_len;
    entry->mpx = use_context;
    entry->alt = -1;
    if (pronoun_len != 0) {
        entry->ci_phone_ids =
            (int32 *) calloc((size_t) pronoun_len, sizeof(int32));
        memcpy(entry->ci_phone_ids, ciPhoneId,
               pronoun_len * sizeof(int32));
        /*
         * This is a HACK to handle the left right conflict on
         * single phone words
         */
        if (use_context && (pronoun_len == 1))
            pronoun_len += 1;
        entry->phone_ids =
            (int32 *) calloc((size_t) pronoun_len, sizeof(int32));
        memcpy(entry->phone_ids, triphone_ids,
               pronoun_len * sizeof(int32));
    }
    else {
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
static int32
replace_dict_entry(dict_t * dict,
                   dict_entry_t * entry,
                   char const *word_str,
                   char *pronoun_str,
                   int32 use_context, int32 new_entry)
{
    char *phone[MAX_PRONOUN_LEN];
    int32 ciPhoneId[MAX_PRONOUN_LEN];
    int32 triphone_ids[MAX_PRONOUN_LEN];
    int32 pronoun_len = 0;
    int32 i;
    char triphoneStr[80];
    void * idx;
    int32 basewid;

    /* For the moment assume left/right context words... */
    assert(use_context);

    /* For the moment, no phrase dictionary stuff... */
    while (1) {
        int n;
        char delim;

	if (pronoun_len >= MAX_PRONOUN_LEN) {
	    E_ERROR("'%s': Too many phones for bogus hard-coded limit (%d), skipping\n",
		    word_str, MAX_PRONOUN_LEN);
	    return 0;
	}
        n = nextword(pronoun_str, " \t", &phone[pronoun_len], &delim);
        if (n < 0)
            break;
        pronoun_str = phone[pronoun_len] + n + 1;

        ciPhoneId[pronoun_len] = phone_to_id(phone[pronoun_len], TRUE);
        if (ciPhoneId[pronoun_len] == NO_PHONE) {
            E_ERROR("'%s': Unknown phone '%s'\n", word_str,
                    phone[pronoun_len]);
            return 0;
        }
        pronoun_len++;
        if (delim == '\0')
            break;
    }

    /* For the moment, no single phone new word... */
    if (pronoun_len < 2) {
        E_ERROR("Pronunciation string too short\n");
        return (0);
    }

    /* Check if it's an alternative pronunciation; if so base word must exist */
    {
        char *p = strrchr(word_str, '(');
        if (p && (word_str[strlen(word_str) - 1] == ')')) {
            *p = '\0';
            if (hash_table_lookup(dict->dict, word_str, &idx)) {
                *p = '(';
                E_ERROR("Base word missing for %s\n", word_str);
                return 0;
            }
            *p = '(';
            basewid = (long)idx;
        }
        else
            basewid = -1;
    }

    /* Parse pron; for the moment, the boundary diphones must be already known... */
    i = 0;
    sprintf(triphoneStr, "%s(%%s,%s)b", phone[i], phone[i + 1]);
    if (hash_table_lookup(lcHT, triphoneStr, &idx) < 0) {
        E_ERROR("Unknown left diphone '%s'\n", triphoneStr);
        return (0);
    }
    triphone_ids[i] = (long) idx;

    for (i = 1; i < pronoun_len - 1; i++) {
        sprintf(triphoneStr, "%s(%s,%s)", phone[i], phone[i - 1],
                phone[i + 1]);
        triphone_ids[i] = phone_to_id(triphoneStr, FALSE);
        if (triphone_ids[i] < 0)
            triphone_ids[i] = phone_to_id(phone[i], TRUE);
        triphone_ids[i] = bin_mdef_pid2ssid(mdef,triphone_ids[i]);
    }

    sprintf(triphoneStr, "%s(%s,%%s)e", phone[i], phone[i - 1]);
    if (hash_table_lookup(rcHT, triphoneStr, &idx) < 0) {
        E_ERROR("Unknown right diphone '%s'\n", triphoneStr);
        return (0);
    }
    triphone_ids[i] = (long) idx;

    /*
     * Set up dictionary entry.  Free the existing attributes (where applicable) and
     * replace with new ones.
     */
    entry->len = pronoun_len;
    entry->mpx = use_context;
    free(entry->word);
    free(entry->ci_phone_ids);
    free(entry->phone_ids);
    entry->word = ckd_salloc(word_str);
    entry->ci_phone_ids =
        ckd_calloc((size_t) pronoun_len, sizeof(int32));
    entry->phone_ids =
        ckd_calloc((size_t) pronoun_len, sizeof(int32));
    memcpy(entry->ci_phone_ids, ciPhoneId, pronoun_len * sizeof(int32));
    memcpy(entry->phone_ids, triphone_ids, pronoun_len * sizeof(int32));

    /* Update alternatives linking if adding a new entry (not updating existing one) */
    if (new_entry) {
        entry->alt = -1;
        if (basewid >= 0) {
            entry->alt = dict->dict_list[(int32) basewid]->alt;
            dict->dict_list[(int32) basewid]->alt = entry->wid;
            entry->wid = (int32) basewid;
        }
    }

    return (1);
}

/*
 * Add a new word to the dictionary, replacing a dummy placeholder.  Or replace an
 * existing non-dummy word in the dictionary.
 * Return the word id of the entry updated if successful.  If any error, return -1.
 */
int32
dict_add_word(dict_t * dict, char const *word, char *pron)
{
    dict_entry_t *entry;
    int32 wid, new_entry;

    /* Word already exists */
    new_entry = 0;
    if ((wid = kb_get_word_id(word)) < 0) {
        /* FIXME: Do some pointer juggling to make this work? */
        /* Or better yet, use a better way to determine what words are
         * filler words... */
        if (first_dummy > last_dummy) {
            E_ERROR("Dictionary full; cannot add word\n");
            return -1;
        }
        wid = first_dummy++;
        new_entry = 1;
    }

    entry = dict->dict_list[wid];
    if (!replace_dict_entry(dict, entry, word, pron, TRUE, new_entry))
        return -1;

    (void)hash_table_enter_int32(dict->dict, entry->word, wid);

    return (wid);
}

static void
_dict_list_add(dict_t * dict, dict_entry_t * entry)
/*------------------------------------------------------------*/
{
    if (!dict->dict_list)
        dict->dict_list = (dict_entry_t **)
            ckd_calloc(hash_table_size(dict->dict), sizeof(dict_entry_t *));

    if (dict->dict_entry_count >= hash_table_size(dict->dict)) {
        E_FATAL("dict size (%d) exceeded\n", hash_table_size(dict->dict));
        dict->dict_list = (dict_entry_t **)
            ckd_realloc(dict->dict_list,
                        (hash_table_size(dict->dict) + 16) * sizeof(dict_entry_t *));
    }

    dict->dict_list[dict->dict_entry_count++] = entry;
}

dict_entry_t *
dict_get_entry(dict_t * dict, int i)
{
    return ((i < dict->dict_entry_count) ?
            dict->dict_list[i] : (dict_entry_t *) 0);
}

/* FIXME: could be extern inline */
int32
dict_count(dict_t * dict)
{
    return dict->dict_entry_count;
}

dict_t *
dict_new(void)
{
    return ckd_calloc(sizeof(dict_t), 1);
}

static void
recordMissingTriphone(char *triphoneStr)
{
    void * idx;
    char *cp;

    if (-1 == hash_table_lookup(mtpHT, triphoneStr, &idx)) {
        cp = ckd_salloc(triphoneStr);
        /* E_INFO("Missing triphone: %s\n", triphoneStr); */
        hash_table_enter(mtpHT, cp, cp);
    }
}

glist_t
dict_mtpList(void)
{
    return mtpList;
}

static int32
addToContextTable(char *diphone, hash_table_t * table, glist_t *list)
{
    void * idx;
    char *cp;

    if (-1 == hash_table_lookup(table, diphone, &idx)) {
        cp = ckd_salloc(diphone);
        idx = (void *)(long)table->inuse;
        *list = glist_add_ptr(*list, cp);
        hash_table_enter(table, cp, idx);
    }
    return ((int32)(long)idx);
}

static int32
addToLeftContextTable(char *diphone)
{
    return addToContextTable(diphone, lcHT, &lcList);
}

static int32
addToRightContextTable(char *diphone)
{
    return addToContextTable(diphone, rcHT, &rcList);
}

static int
cmp(void const *a, void const *b)
{
    return (*(int32 const *) a - *(int32 const *) b);
}

int32 *linkTable;

static int
cmpPT(void const *a, void const *b)
{
    return (linkTable[*(int32 const *) a] - linkTable[*(int32 const *) b]);
}

static void
buildEntryTable(glist_t list, int32 *** table_p)
{
    int32 ciCount = phoneCiCount();
    int32 i, j;
    char triphoneStr[128];
    int32 silContext = 0;
    int32 triphoneContext = 0;
    int32 noContext = 0;
    int32 **table;
    gnode_t *gn;
    int n;

    *table_p = ckd_calloc(glist_count(list), sizeof(int32 *));
    table = *table_p;
    n = glist_count(list);
    E_INFO("Entry Context table contains\n\t%6d entries\n", n);
    E_INFO("\t%6d possible cross word triphones.\n", n * ciCount);

    for (i = 0, gn = list; gn; gn = gnode_next(gn), ++i) {
        table[i] = ckd_calloc(ciCount, sizeof(int32));
        for (j = 0; j < ciCount; j++) {
            /*
             * Look for the triphone
             */
            sprintf(triphoneStr, (char *)gnode_ptr(gn), phone_from_id(j));
            table[i][j] = phone_to_id(triphoneStr, FALSE);
            if (table[i][j] >= 0)
                triphoneContext++;
            /*
             * If we can't find the desired right context use "SIL"
             */
            if (table[i][j] < 0) {
                sprintf(triphoneStr, (char *)gnode_ptr(gn), "SIL");
                table[i][j] = phone_to_id(triphoneStr, FALSE);
                if (table[i][j] >= 0)
                    silContext++;
            }
            /*
             * If we can't find "SIL" use context indepedent
             */
            if (table[i][j] < 0) {
                char stmp[32];
                char *p;
                strcpy(stmp, (char *)gnode_ptr(gn));
                p = strchr(stmp, '(');
                *p = '\0';
                table[i][j] = phone_to_id(stmp, TRUE);
                noContext++;
            }
            table[i][j] = bin_mdef_pid2ssid(mdef,phone_map(table[i][j]));
        }
    }
    E_INFO("\t%6d triphones\n\t%6d pseudo diphones\n\t%6d uniphones\n",
           triphoneContext, silContext, noContext);
}

static void
buildExitTable(glist_t list, int32 *** table_p, int32 *** permuTab_p,
               int32 ** sizeTab_p)
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
    gnode_t *gn;
    int32 n;

    n = glist_count(list);
    *table_p =
        (int32 **) ckd_calloc_2d(n, ciCount + 1, sizeof(int32 *));
    table = *table_p;
    *permuTab_p =
        (int32 **) ckd_calloc_2d(n, ciCount + 1, sizeof(int32 *));
    permuTab = *permuTab_p;
    *sizeTab_p = ckd_calloc(n, sizeof(int32 *));
    sizeTab = *sizeTab_p;

    E_INFO("Exit Context table contains\n\t%6d entries\n", n);
    E_INFO("\t%6d possible cross word triphones.\n", n * ciCount);

    for (i = 0, gn = list; gn; gn = gnode_next(gn), ++i) {
        for (j = 0; j < ciCount; j++) {
            /*
             * Look for the triphone
             */
            sprintf(triphoneStr, (char *)gnode_ptr(gn), phone_from_id(j));
            table[i][j] = phone_to_id(triphoneStr, FALSE);
            if (table[i][j] >= 0)
                triphoneContext++;
            /*
             * If we can't find the desired context use "SIL"
             */
            if (table[i][j] < 0) {
                sprintf(triphoneStr, (char *)gnode_ptr(gn), "SIL");
                table[i][j] = phone_to_id(triphoneStr, FALSE);
                if (table[i][j] >= 0)
                    silContext++;
            }
            /*
             * If we can't find "SIL" use context indepedent
             */
            if (table[i][j] < 0) {
                char stmp[32];
                char *p;
                strcpy(stmp, (char *)gnode_ptr(gn));
                p = strchr(stmp, '(');
                *p = '\0';
                table[i][j] = phone_to_id(stmp, TRUE);
                noContext++;
            }
            table[i][j] = bin_mdef_pid2ssid(mdef,phone_map(table[i][j]));
        }
    }
    /*
     * Now compress the table to eliminate duplicate entries.
     */
    for (i = 0; i < n; ++i) {
        /*
         * Set up the permutation table
         */
        for (k = 0; k < ciCount; k++) {
            ptab[k] = k;
        }
        linkTable = table[i];
        qsort(ptab, ciCount, sizeof(int32), cmpPT);

        qsort(table[i], ciCount, sizeof(int32), cmp);
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
        table[i][k + 1] = -1;   /* End of table Marker */
        sizeTab[i] = k + 1;
        entries += k + 1;
    }
    E_INFO("\t%6d triphones\n\t%6d pseudo diphones\n\t%6d uniphones\n",
           triphoneContext, silContext, noContext);
    E_INFO("\t%6d right context entries\n", entries);
    E_INFO("\t%6d ave entries per exit context\n",
           ((n == 0) ? 0 : entries / n));
}

int32 **
dict_right_context_fwd(void)
{
    return rcFwdTable;
}

int32 **
dict_right_context_fwd_perm(void)
{
    return rcFwdPermTable;
}

int32 *
dict_right_context_fwd_size(void)
{
    return rcFwdSizeTable;
}

int32 **
dict_left_context_fwd(void)
{
    return lcFwdTable;
}

int32 **
dict_right_context_bwd(void)
{
    return rcBwdTable;
}

int32 **
dict_left_context_bwd(void)
{
    return lcBwdTable;
}

int32 **
dict_left_context_bwd_perm(void)
{
    return lcBwdPermTable;
}

int32 *
dict_left_context_bwd_size(void)
{
    return lcBwdSizeTable;
}

int32
dict_get_num_main_words(dict_t * dict)
{
    return ((int32) dictStrToWordId(dict, "</s>", FALSE));
}

int32
dictid_to_baseid(dict_t * dict, int32 wid)
{
    return (dict->dict_list[wid]->wid);
}

/*
 * Return TRUE iff wid is new word dynamically added at run time.
 */
int32
dict_is_new_word(int32 wid)
{
    return ((wid >= initial_dummy) && (wid <= last_dummy));
}

int32
dict_pron(dict_t * dict, int32 w, int32 ** pron)
{
    *pron = dict->dict_list[w]->ci_phone_ids;
    return (dict->dict_list[w]->len);
}

int32
dict_next_alt(dict_t * dict, int32 w)
{
    return (dict->dict_list[w]->alt);
}

/* Write OOV words added at run time to the given file and return #words written */
int32
dict_write_oovdict(dict_t * dict, char const *file)
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
        fprintf(fp, "%s\t", dict->dict_list[w]->word);
        for (p = 0; p < dict->dict_list[w]->len; p++)
            fprintf(fp, " %s",
                    phone_from_id(dict->dict_list[w]->ci_phone_ids[p]));
        fprintf(fp, "\n");
    }

    fclose(fp);

    return (first_dummy - initial_dummy);
}

int32
dict_is_filler_word(dict_t * dict, int32 wid)
{
    return (wid >= dict->filler_start);
}
