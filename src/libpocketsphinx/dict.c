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
#include "dict.h"
#include "phone.h"

#ifdef DEBUG
#define DFPRINTF(x)		fprintf x
#else
#define DFPRINTF(x)
#endif

#define QUIT(x)		{fprintf x; exit(-1);}

static void buildEntryTable(glist_t list, int16 *** table_p,
                            bin_mdef_t *mdef);
static void buildExitTable(glist_t list, int16 *** table_p,
                           int16 *** permuTab_p, int16 ** sizeTab_p,
                           bin_mdef_t *mdef);
static int32 addToLeftContextTable(dict_t *dict, char *diphone);
static int32 addToRightContextTable(dict_t *dict, char *diphone);
static dict_entry_t *_new_dict_entry(dict_t *dict,
                                     char *word_str,
                                     char *pronoun_str,
                                     int32 use_context);
static void _dict_list_add(dict_t * dict, dict_entry_t * entry);
static void dict_load(dict_t * dict, bin_mdef_t *mdef,
                      char const *filename, int32 * word_id,
                      int32 use_context);

#define MAX_PRONOUN_LEN 	150

static int32
get_dict_size(char const *file)
{
    FILE *fp;
    __BIGSTACKVARIABLE__ char line[1024];
    int32 n;

    fp = myfopen(file, "r");
    for (n = 0;; n++)
        if (fgets(line, sizeof(line), fp) == NULL)
            break;
    fclose(fp);

    return n;
}

dict_t *
dict_init(cmd_ln_t *config, bin_mdef_t *mdef)
{
    dict_t *dict = ckd_calloc(1, sizeof(*dict));
    int32 word_id = 0, i, j;
    dict_entry_t *entry;
    int32 max_new_oov;
    void *val;
    char const *filename, *n_filename;
    int use_context;

    dict->config = config;
    dict->mdef = mdef;
    filename = cmd_ln_str_r(config, "-dict");
    n_filename = cmd_ln_str_r(config, "-fdict");
    use_context = !cmd_ln_boolean_r(config, "-usewdphones");

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
    j = bin_mdef_n_ciphone(mdef);
    j = ((j * j) >> 1) + 1;
    if (use_context) {
        dict->lcHT = hash_table_new(j, HASH_CASE_YES);
        dict->rcHT = hash_table_new(j, HASH_CASE_YES);
    }

    /* Placeholders (dummy pronunciations) for new words that can be
     * added at runtime.  We can expand this region of the dictionary
     * later if need be. */
    dict->initial_dummy = dict->first_dummy = word_id;
    if ((max_new_oov = cmd_ln_int32_r(dict->config, "-maxnewoov")) > 0)
        E_INFO("Allocating %d placeholders for new OOVs\n", max_new_oov);
    for (i = 0; i < max_new_oov; i++) {
        char tmpstr[100], pronstr[100];

        /* Pick a temporary name that doesn't occur in the LM */
        sprintf(tmpstr, "=PLCHLDR%d=", i);

        /* new_dict_entry clobbers pronstr! so need this strcpy in the loop */
        strcpy(pronstr, "SIL");
        entry = _new_dict_entry(dict, tmpstr, pronstr, use_context);
        if (!entry)
            E_FATAL("Failed to add DUMMY(SIL) entry to dictionary\n");

        _dict_list_add(dict, entry);
        (void)hash_table_enter_int32(dict->dict, entry->word, word_id);
        entry->wid = word_id;
        word_id++;
    }
    dict->last_dummy = word_id - 1;

    /* Load dictionaries */
    dict_load(dict, mdef, filename, &word_id, use_context);

    /* Make sure that <s>, </s>, and <sil> are always in the dictionary. */
    if (hash_table_lookup(dict->dict, "</s>", &val) != 0) {
        char pronstr[5];
        /*
         * Check if there is a special end silence phone.
         */
        if (-1 == bin_mdef_ciphone_id(mdef, "SILe")) {
            strcpy(pronstr, "SIL");
            entry = _new_dict_entry(dict, "</s>", pronstr, FALSE);
            if (!entry)
                E_FATAL("Failed to add </s>(SIL) to dictionary\n");
        }
        else {
            E_INFO("Using special end silence for </s>\n");
            strcpy(pronstr, "SILe");
            entry =
                _new_dict_entry(dict, "</s>", pronstr, FALSE);
        }
        _dict_list_add(dict, entry);
        hash_table_enter(dict->dict, entry->word, (void *)(long)word_id);
        entry->wid = word_id;
        word_id++;
    }

    dict->config = config;
    /* Mark the start of filler words */
    dict->filler_start = word_id;

    /* Add the standard start symbol (<s>) if not already in dict */
    if (hash_table_lookup(dict->dict, "<s>", &val) != 0) {
        char pronstr[5];
        /*
         * Check if there is a special begin silence phone.
         */
        if (-1 == bin_mdef_ciphone_id(mdef, "SILb")) {
            strcpy(pronstr, "SIL");
            entry =
                _new_dict_entry(dict, "<s>", pronstr, FALSE);
            if (!entry)
                E_FATAL("Failed to add <s>(SIL) to dictionary\n");
        }
        else {
            E_INFO("Using special begin silence for <s>\n");
            strcpy(pronstr, "SILb");
            entry =
                _new_dict_entry(dict, "<s>", pronstr, FALSE);
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
        entry = _new_dict_entry(dict, "<sil>", pronstr, FALSE);
        if (!entry)
            E_FATAL("Failed to add <sil>(SIL) to dictionary\n");
        _dict_list_add(dict, entry);
        hash_table_enter(dict->dict, entry->word, (void *)(long)word_id);
        entry->wid = word_id;
        word_id++;
    }

    if (n_filename)
        dict_load(dict, mdef, n_filename, &word_id, FALSE /* use_context */);

    E_INFO("LEFT CONTEXT TABLES\n");
    dict->lcList = glist_reverse(dict->lcList);
    buildEntryTable(dict->lcList, &dict->lcFwdTable, mdef);
    buildExitTable(dict->lcList, &dict->lcBwdTable, &dict->lcBwdPermTable,
                   &dict->lcBwdSizeTable, mdef);

    E_INFO("RIGHT CONTEXT TABLES\n");
    dict->rcList = glist_reverse(dict->rcList);
    buildEntryTable(dict->rcList, &dict->rcBwdTable, mdef);
    buildExitTable(dict->rcList, &dict->rcFwdTable, &dict->rcFwdPermTable,
                   &dict->rcFwdSizeTable, mdef);

    return dict;
}

void
dict_free(dict_t * dict)
{
    int32 i;
    int32 entry_count;
    dict_entry_t *entry;
    gnode_t *gn;

    for (i = 0, gn = dict->lcList; gn; gn = gnode_next(gn), ++i) {
        ckd_free(dict->lcFwdTable[i]);
        ckd_free(gnode_ptr(gn));
    }
    ckd_free(dict->lcFwdTable);
    ckd_free_2d(dict->lcBwdTable);
    ckd_free_2d(dict->lcBwdPermTable);
    ckd_free(dict->lcBwdSizeTable);
    if (dict->lcHT)
        hash_table_free(dict->lcHT);
    glist_free(dict->lcList);

    for (i = 0, gn = dict->rcList; gn; gn = gnode_next(gn), ++i) {
        ckd_free(dict->rcBwdTable[i]);
        ckd_free(gnode_ptr(gn));
    }
    ckd_free(dict->rcBwdTable);
    ckd_free_2d(dict->rcFwdTable);
    ckd_free_2d(dict->rcFwdPermTable);
    ckd_free(dict->rcFwdSizeTable);
    if (dict->rcHT)
        hash_table_free(dict->rcHT);
    glist_free(dict->rcList);

    entry_count = dict->dict_entry_count;

    for (i = 0; i < entry_count; i++) {
        entry = dict->dict_list[i];
        ckd_free(entry->word);
        ckd_free(entry->phone_ids);
        ckd_free(entry->ci_phone_ids);
        ckd_free(entry);
    }

    ckd_free(dict->dict_list);
    ckd_free(dict->ci_index);
    if (dict->dict)
        hash_table_free(dict->dict);
    ckd_free(dict);
}

static void
dict_load(dict_t * dict, bin_mdef_t *mdef,
          char const *filename, int32 *word_id, int32 use_context)
{
    static char const *rname = "dict_load";
    __BIGSTACKVARIABLE__ char dict_str[1024];
    __BIGSTACKVARIABLE__ char pronoun_str[1024];
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
        int32 wid;
        /* Check for duplicate before we do anything. */
        if (hash_table_lookup_int32(dict->dict, dict_str, &wid) == 0) {
            E_WARN("Skipping duplicate definition of %s\n", dict_str);
            continue;
        }
        entry = _new_dict_entry(dict, dict_str, pronoun_str, use_context);
        if (!entry) {
            E_ERROR("Failed to add %s to dictionary\n", dict_str);
            err = 1;
            continue;
        }

        if (hash_table_enter_int32(dict->dict, entry->word, *word_id) != *word_id) {
            E_ERROR("Failed to add %s to dictionary hash!\n", entry->word);
            err = 1;
            continue;
        }
        _dict_list_add(dict, entry);
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
dict_to_id(dict_t * dict, char const *dict_str)
{
    int32 dictid;

    if (hash_table_lookup_int32(dict->dict, dict_str, &dictid) < 0)
        return NO_WORD;
    return dictid;
}

static dict_entry_t *
_new_dict_entry(dict_t *dict, char *word_str, char *pronoun_str, int32 use_context)
{
    dict_entry_t *entry;
    __BIGSTACKVARIABLE__ char *phone[MAX_PRONOUN_LEN];
    __BIGSTACKVARIABLE__ int32 ciPhoneId[MAX_PRONOUN_LEN];
    __BIGSTACKVARIABLE__ int32 triphone_ids[MAX_PRONOUN_LEN];
    int32 pronoun_len = 0;
    int32 i;
    int32 lcTabId;
    int32 rcTabId;
    __BIGSTACKVARIABLE__ char triphoneStr[80];
    __BIGSTACKVARIABLE__ char position[256];         /* phone position */
    bin_mdef_t *mdef = dict->mdef;

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
            position[pronoun_len - 1] = WORD_POSN_END;
            position[pronoun_len] = WORD_POSN_BEGIN;
            continue;
        }
        ciPhoneId[pronoun_len] = bin_mdef_ciphone_id(mdef, phone[pronoun_len]);
        if (ciPhoneId[pronoun_len] == -1) {
            E_ERROR("'%s': Unknown phone '%s'\n", word_str,
                    phone[pronoun_len]);
            return NULL;
        }
        pronoun_len++;
        if (delim == '\0')
            break;
    }

    position[pronoun_len - 1] = WORD_POSN_END;    /* Last phone is at the end */

    /*
     * If the position marker sequence 'ee' appears or 'se' appears
     * the sequence should be '*s'.
     */

    if (position[0] == WORD_POSN_END)     /* Also handle single phone word case */
        position[0] = WORD_POSN_SINGLE;

    for (i = 0; i < pronoun_len - 1; i++) {
        if (((position[i] == WORD_POSN_END)
             || (position[i] == WORD_POSN_SINGLE)) &&
            (position[i + 1] == WORD_POSN_END))
            position[i + 1] = WORD_POSN_SINGLE;
    }

    if (pronoun_len >= 2) {
        i = 0;

        if (use_context) {
            sprintf(triphoneStr, "%s(%%s,%s)b", phone[i], phone[i + 1]);
            lcTabId = addToLeftContextTable(dict, triphoneStr);
            triphone_ids[i] = lcTabId;
        }
        else {
            triphone_ids[i] = bin_mdef_phone_id(mdef,
                                                bin_mdef_ciphone_id(mdef, phone[i]),
                                                -1,
                                                bin_mdef_ciphone_id(mdef, phone[i+1]),
                                                WORD_POSN_BEGIN);
            if (triphone_ids[i] < 0)
                triphone_ids[i] = bin_mdef_ciphone_id(mdef, phone[i]);
            triphone_ids[i] = bin_mdef_pid2ssid(mdef, triphone_ids[i]);
        }

        for (i = 1; i < pronoun_len - 1; i++) {
            triphone_ids[i] = bin_mdef_phone_id(mdef,
                                                bin_mdef_ciphone_id(mdef, phone[i]),
                                                bin_mdef_ciphone_id(mdef, phone[i-1]),
                                                bin_mdef_ciphone_id(mdef, phone[i+1]),
                                                position[i]);
            if (triphone_ids[i] < 0)
                triphone_ids[i] = bin_mdef_ciphone_id(mdef, phone[i]);
            triphone_ids[i] = bin_mdef_pid2ssid(mdef, triphone_ids[i]);
        }

        if (use_context) {
            sprintf(triphoneStr, "%s(%s,%%s)e", phone[i], phone[i - 1]);
            rcTabId = addToRightContextTable(dict, triphoneStr);
            triphone_ids[i] = rcTabId;
        }
        else {
            triphone_ids[i] = bin_mdef_phone_id(mdef,
                                                bin_mdef_ciphone_id(mdef, phone[i]),
                                                bin_mdef_ciphone_id(mdef, phone[i-1]),
                                                -1,
                                                position[i]);
            if (triphone_ids[i] < 0)
                triphone_ids[i] = bin_mdef_ciphone_id(mdef, phone[i]);
            triphone_ids[i] = bin_mdef_pid2ssid(mdef, triphone_ids[i]);
        }
    }

    /*
     * It's too hard to model both contexts so I choose to model only
     * the left context.
     */
    if (pronoun_len == 1) {
        if (use_context) {
            sprintf(triphoneStr, "%s(%%s,SIL)s", phone[0]);
            lcTabId = addToLeftContextTable(dict, triphoneStr);
            triphone_ids[0] = lcTabId;
            /*
             * Put the right context table in the 2 entry
             */
            sprintf(triphoneStr, "%s(SIL,%%s)s", phone[0]);
            rcTabId = addToRightContextTable(dict, triphoneStr);
            triphone_ids[1] = rcTabId;
        }
        else {
            triphone_ids[0] = bin_mdef_ciphone_id(mdef,phone[0]);
            triphone_ids[0] = bin_mdef_pid2ssid(mdef,triphone_ids[0]);
        }
    }

    entry = (dict_entry_t *) ckd_calloc((size_t) 1, sizeof(dict_entry_t));
    entry->word = ckd_salloc(word_str);
    entry->len = pronoun_len;
    entry->mpx = use_context;
    entry->alt = -1;
    if (pronoun_len != 0) {
        entry->ci_phone_ids =
            (int32 *) ckd_calloc((size_t) pronoun_len, sizeof(int32));
        memcpy(entry->ci_phone_ids, ciPhoneId,
               pronoun_len * sizeof(int32));
        /*
         * This is a HACK to handle the left right conflict on
         * single phone words
         */
        if (use_context && (pronoun_len == 1))
            pronoun_len += 1;
        entry->phone_ids =
            (int32 *) ckd_calloc((size_t) pronoun_len, sizeof(int32));
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
                   int32 use_context,
                   int32 new_entry)
{
    char *phone[MAX_PRONOUN_LEN];
    int32 ciPhoneId[MAX_PRONOUN_LEN];
    int32 triphone_ids[MAX_PRONOUN_LEN];
    int32 pronoun_len = 0;
    int32 i;
    char triphoneStr[80];
    int32 idx;
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

        ciPhoneId[pronoun_len] = bin_mdef_ciphone_id(dict->mdef, phone[pronoun_len]);
        if (ciPhoneId[pronoun_len] == -1) {
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
            if (hash_table_lookup_int32(dict->dict, word_str, &idx)) {
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
    if (hash_table_lookup_int32(dict->lcHT, triphoneStr, &idx) < 0) {
        E_ERROR("Unknown left diphone '%s'\n", triphoneStr);
        return (0);
    }
    triphone_ids[i] = idx;

    for (i = 1; i < pronoun_len - 1; i++) {
        triphone_ids[i] = bin_mdef_phone_id(dict->mdef,
                                            bin_mdef_ciphone_id(dict->mdef, phone[i]),
                                            bin_mdef_ciphone_id(dict->mdef, phone[i-1]),
                                            bin_mdef_ciphone_id(dict->mdef, phone[i+1]),
                                            WORD_POSN_INTERNAL);
        triphone_ids[i] = bin_mdef_pid2ssid(dict->mdef, triphone_ids[i]);
    }

    sprintf(triphoneStr, "%s(%s,%%s)e", phone[i], phone[i - 1]);
    if (hash_table_lookup_int32(dict->rcHT, triphoneStr, &idx) < 0) {
        E_ERROR("Unknown right diphone '%s'\n", triphoneStr);
        return (0);
    }
    triphone_ids[i] = idx;

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
    if ((wid = dict_to_id(dict, word)) < 0) {
        /* FIXME: Do some pointer juggling to make this work? */
        /* Or better yet, use a better way to determine what words are
         * filler words... */
        if (dict->first_dummy > dict->last_dummy) {
            E_ERROR("Dictionary full; cannot add word\n");
            return -1;
        }
        wid = dict->first_dummy++;
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

static int32
addToContextTable(char *diphone, hash_table_t * table, glist_t *list)
{
    int32 idx;
    char *cp;

    if (-1 == hash_table_lookup_int32(table, diphone, &idx)) {
        cp = ckd_salloc(diphone);
        idx = table->inuse;
        *list = glist_add_ptr(*list, cp);
        (void)hash_table_enter_int32(table, cp, idx);
    }
    return idx;
}

static int32
addToLeftContextTable(dict_t *dict, char *diphone)
{
    return addToContextTable(diphone, dict->lcHT, &dict->lcList);
}

static int32
addToRightContextTable(dict_t *dict, char *diphone)
{
    return addToContextTable(diphone, dict->rcHT, &dict->rcList);
}

static void
buildEntryTable(glist_t list, int16 *** table_p, bin_mdef_t *mdef)
{
    int32 i, j;
    char triphoneStr[128];
    int32 ciCount = bin_mdef_n_ciphone(mdef);
    int32 silContext = 0;
    int32 triphoneContext = 0;
    int32 noContext = 0;
    int16 **table;
    gnode_t *gn;
    int n;

    *table_p = ckd_calloc(glist_count(list), sizeof(**table_p));
    table = *table_p;
    n = glist_count(list);
    E_INFO("Entry Context table contains\n\t%6d entries\n", n);
    E_INFO("\t%6d possible cross word triphones.\n", n * ciCount);

    for (i = 0, gn = list; gn; gn = gnode_next(gn), ++i) {
        table[i] = ckd_calloc(ciCount, sizeof(**table));
        for (j = 0; j < ciCount; j++) {
            int32 phoneid;
            /*
             * Look for the triphone
             */
            sprintf(triphoneStr, (char *)gnode_ptr(gn),
                    bin_mdef_ciphone_str(mdef, j));
            phoneid = phone_to_id(mdef, triphoneStr);
            if (phoneid >= 0)
                triphoneContext++;
            /*
             * If we can't find the desired right context use "SIL"
             */
            if (phoneid < 0) {
                sprintf(triphoneStr, (char *)gnode_ptr(gn), "SIL");
                phoneid = phone_to_id(mdef, triphoneStr);
                if (phoneid >= 0)
                    silContext++;
            }
            /*
             * If we can't find "SIL" use context indepedent
             */
            if (phoneid < 0) {
                char stmp[32];
                char *p;
                strcpy(stmp, (char *)gnode_ptr(gn));
                p = strchr(stmp, '(');
                *p = '\0';
                phoneid = phone_to_id(mdef, stmp);
                noContext++;
            }
            table[i][j] = bin_mdef_pid2ssid(mdef, phoneid);
        }
    }
    E_INFO("\t%6d triphones\n\t%6d pseudo diphones\n\t%6d uniphones\n",
           triphoneContext, silContext, noContext);
}

static int
cmp(void const *a, void const *b)
{
    return (*(int16 const *) a - *(int16 const *) b);
}

/* FIXME: Not re-entrant. */
static int16 *linkTable;

static int
cmpPT(void const *a, void const *b)
{
    return (linkTable[*(int16 const *) a] - linkTable[*(int16 const *) b]);
}

static void
buildExitTable(glist_t list, int16 *** table_p, int16 *** permuTab_p,
               int16 ** sizeTab_p, bin_mdef_t *mdef)
{
    int32 i, j, k;
    char triphoneStr[128];
    int32 ciCount = bin_mdef_n_ciphone(mdef);
    int32 silContext = 0;
    int32 triphoneContext = 0;
    int32 noContext = 0;
    int32 entries = 0;
    int16 **table;
    int16 **permuTab;
    int16 *sizeTab;
    int16 ptab[128];
    gnode_t *gn;
    int32 n;

    n = glist_count(list);
    *table_p = ckd_calloc_2d(n, ciCount + 1, sizeof(***table_p));
    table = *table_p;
    *permuTab_p = ckd_calloc_2d(n, ciCount + 1, sizeof(***permuTab_p));
    permuTab = *permuTab_p;
    *sizeTab_p = ckd_calloc(n, sizeof(**sizeTab_p));
    sizeTab = *sizeTab_p;

    E_INFO("Exit Context table contains\n\t%6d entries\n", n);
    E_INFO("\t%6d possible cross word triphones.\n", n * ciCount);

    for (i = 0, gn = list; gn; gn = gnode_next(gn), ++i) {
        for (j = 0; j < ciCount; j++) {
            int32 phoneid;

            /*
             * Look for the triphone
             */
            sprintf(triphoneStr, (char *)gnode_ptr(gn),
                    bin_mdef_ciphone_str(mdef, j));
            phoneid = phone_to_id(mdef, triphoneStr);
            if (phoneid >= 0)
                triphoneContext++;
            /*
             * If we can't find the desired context use "SIL"
             */
            if (phoneid < 0) {
                sprintf(triphoneStr, (char *)gnode_ptr(gn), "SIL");
                phoneid = phone_to_id(mdef, triphoneStr);
                if (phoneid >= 0)
                    silContext++;
            }
            /*
             * If we can't find "SIL" use context indepedent
             */
            if (phoneid < 0) {
                char stmp[32];
                char *p;
                strcpy(stmp, (char *)gnode_ptr(gn));
                p = strchr(stmp, '(');
                *p = '\0';
                phoneid = phone_to_id(mdef, stmp);
                noContext++;
            }
            table[i][j] = bin_mdef_pid2ssid(mdef, phoneid);
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
        qsort(ptab, ciCount, sizeof(*ptab), cmpPT);

        qsort(table[i], ciCount, sizeof(**table), cmp);
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

int32
dict_get_num_main_words(dict_t * dict)
{
    /* FIXME FIXME: Relies on a particular ordering of the dictionary. */
    return dict_to_id(dict, "</s>");
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

int32
dict_is_filler_word(dict_t * dict, int32 wid)
{
    return (wid >= dict->filler_start);
}
