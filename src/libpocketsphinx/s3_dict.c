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
 * dict.c -- Pronunciation dictionary.
 *
 * **********************************************
 * CMU ARPA Speech Project
 *
 * Copyright (c) 1997 Carnegie Mellon University.
 * ALL RIGHTS RESERVED.
 * **********************************************
 * 
 * HISTORY
 * $Log: dict.c,v $
 * Revision 1.7  2006/02/28  02:06:46  egouvea
 * Updated MS Visual C++ 6.0 support files. Fixed things that didn't
 * compile in Visual C++ (declarations didn't match, etc). There are
 * still some warnings, so this is not final. Also, sorted files in
 * several Makefile.am.
 * 
 * Revision 1.6  2006/02/22 20:55:06  arthchan2003
 * Merged from branch SPHINX3_5_2_RCI_IRII_BRANCH:
 *
 * 1, Added Letter-to-sound LTS rule, dict_init will only specify
 * d->lts_rules to be true if the useLTS is specified.  Only if
 * d->lts_rules is specified, the LTS logic will be used. The code safe
 * guarded the case when a phone in mdef doesn't appear in LTS, in that
 * case, the code will force exit.
 *
 * 2, The LTS logic is only used as a reserved measure.  By default, it
 * is not turned on.  See also the comment in kbcore.c and the default
 * parameters in revision 1.3 cmdln_macro.h . We added it because we have
 * this functionality in SphinxTrain.
 *
 * Revision 1.5.4.2  2006/01/16 19:53:17  arthchan2003
 * Changed the option name from -ltsoov to -lts_mismatch
 *
 * Revision 1.5.4.1  2005/09/25 19:12:09  arthchan2003
 * Added optional LTS support for the dictionary.
 *
 * Revision 1.5  2005/06/21 21:04:36  arthchan2003
 * 1, Introduced a reporting routine. 2, Fixed doyxgen documentation, 3, Added  keyword.
 *
 * Revision 1.5  2005/06/19 03:58:16  archan
 * 1, Move checking of Silence wid, start wid, finish wid to dict_init. This unify the checking and remove several segments of redundant code. 2, Remove all startwid, silwid and finishwid.  They are artefacts of 3.0/3.x merging. This is already implemented in dict.  (In align, startwid, endwid, finishwid occured in several places.  Checking is also done multiple times.) 3, Making corresponding changes to all files which has variable startwid, silwid and finishwid.  Should make use of the marco more.
 *
 * Revision 1.4  2005/04/21 23:50:26  archan
 * Some more refactoring on the how reporting of structures inside kbcore_t is done, it is now 50% nice. Also added class-based LM test case into test-decode.sh.in.  At this moment, everything in search mode 5 is already done.  It is time to test the idea whether the search can really be used.
 *
 * Revision 1.3  2005/03/30 01:22:46  archan
 * Fixed mistakes in last updates. Add
 *
 * 19-Apr-01    Ricky Houghton, added code for freeing memory that is allocated internally.
 * 
 * 23-Apr-98	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University.
 * 		Made usage of mdef optional.  If no mdef is specified while loading
 *		a dictionary, it maintains the needed CI phone information internally.
 * 		Added dict_ciphone_str().
 * 
 * 02-Jul-97	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University.
 * 		Added startwid, finishwid, silwid to dict_t.  Modified dict_filler_word
 * 		to check for start and finishwid.
 * 
 * 07-Feb-97	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University.
 * 		Created from previous Sphinx-3 version.
 */


#include <string.h>

#include "strfuncs.h"
#include "s3_dict.h"


#define DELIM	" \t\n"         /* Set of field separator characters */
#define DEFAULT_NUM_PHONE	(MAX_S3CIPID+1)

#if WIN32
#define snprintf sprintf_s
#endif 

extern const char *const cmu6_lts_phone_table[];

static s3cipid_t
s3dict_ciphone_id(s3dict_t * d, const char *str)
{
    return bin_mdef_ciphone_id(d->mdef, str);
}


const char *
s3dict_ciphone_str(s3dict_t * d, s3wid_t wid, int32 pos)
{
    assert(d != NULL);
    assert((wid >= 0) && (wid < d->n_word));
    assert((pos >= 0) && (pos < d->word[wid].pronlen));

    return bin_mdef_ciphone_str(d->mdef, d->word[wid].ciphone[pos]);
}


s3wid_t
s3dict_add_word(s3dict_t * d, char *word, s3cipid_t * p, int32 np)
{
    int32 len;
    dictword_t *wordp;
    s3wid_t newwid;

    if (d->n_word >= d->max_words) {
        E_INFO
            ("Dictionary max size (%d) exceeded; reallocate another entries %d \n",
             d->max_words, S3DICT_INC_SZ);
        d->word =
            (dictword_t *) ckd_realloc(d->word,
                                       (d->max_words +
                                        S3DICT_INC_SZ) * sizeof(dictword_t));
        d->max_words = d->max_words + S3DICT_INC_SZ;

        return (BAD_S3WID);
    }

    wordp = d->word + d->n_word;
    wordp->word = (char *) ckd_salloc(word);    /* Freed in s3dict_free */

    /* Associate word string with d->n_word in hash table */
    if (hash_table_enter_int32(d->ht, wordp->word, d->n_word) != d->n_word) {
        ckd_free(wordp->word);
        return (BAD_S3WID);
    }

    /* Fill in word entry, and set defaults */
    if (p && (np > 0)) {
        wordp->ciphone = (s3cipid_t *) ckd_malloc(np * sizeof(s3cipid_t));      /* Freed in s3dict_free */
        memcpy(wordp->ciphone, p, np * sizeof(s3cipid_t));
        wordp->pronlen = np;
    }
    else {
        wordp->ciphone = NULL;
        wordp->pronlen = 0;
    }
    wordp->alt = BAD_S3WID;
    wordp->basewid = d->n_word;

    /* Determine base/alt wids */
    if ((len = s3dict_word2basestr(word)) > 0) {
	int32 w;

        /* Truncated to a baseword string; find its ID */
        if (hash_table_lookup_int32(d->ht, word, &w) < 0) {
            word[len] = '(';    /* Get back the original word */
            E_FATAL("Missing base word for: %s\n", word);
        }
        else
            word[len] = '(';    /* Get back the original word */

        /* Link into alt list */
        wordp->basewid = w;
        wordp->alt = d->word[w].alt;
        d->word[w].alt = d->n_word;
    }

    newwid = d->n_word++;

    return newwid;
}


static int32
s3dict_read(FILE * fp, s3dict_t * d)
{
    char line[16384], **wptr;
    s3cipid_t p[4096];
    int32 lineno, nwd;
    s3wid_t w;
    int32 i, maxwd;
    s3cipid_t ci;
    int32 ph;

    maxwd = 4092;
    wptr = (char **) ckd_calloc(maxwd, sizeof(char *)); /* Freed below */

    lineno = 0;
    while (fgets(line, sizeof(line), fp) != NULL) {
        lineno++;
        if (line[0] == '#')     /* Comment line */
            continue;

        if ((nwd = str2words(line, wptr, maxwd)) < 0)
            E_FATAL("str2words(%s) failed; Increase maxwd from %d\n", line,
                    maxwd);

        if (nwd == 0)           /* Empty line */
            continue;
        /* wptr[0] is the word-string and wptr[1..nwd-1] the pronunciation sequence */
        if (nwd == 1) {
            E_ERROR("Line %d: No pronunciation for word %s; ignored\n",
                    lineno, wptr[0]);
            continue;
        }

        /* Convert pronunciation string to CI-phone-ids */
        for (i = 1; i < nwd; i++) {
            p[i - 1] = s3dict_ciphone_id(d, wptr[i]);
            if (NOT_S3CIPID(p[i - 1])) {
                E_ERROR("Line %d: Bad ciphone: %s; word %s ignored\n",
                        lineno, wptr[i], wptr[0]);
                break;
            }
        }

        if (i == nwd) {         /* All CI-phones successfully converted to IDs */
            w = s3dict_add_word(d, wptr[0], p, nwd - 1);
            if (NOT_S3WID(w))
                E_ERROR
                    ("Line %d: s3dict_add_word (%s) failed (duplicate?); ignored\n",
                     lineno, wptr[0]);
        }
    }


    if (d->lts_rules) {

#if 1                           /* Until we allow user to put in a mapping of the phoneset from LTS to the phoneset from mdef, 
                                   The checking will intrusively stop the recognizer.  */

        for (ci = 0; ci < bin_mdef_n_ciphone(d->mdef); ci++) {

            if (!bin_mdef_is_fillerphone(d->mdef, ci)) {
                for (ph = 0; cmu6_lts_phone_table[ph] != NULL; ph++) {

                    /*        E_INFO("%s %s\n",cmu6_lts_phone_table[ph],mdef_ciphone_str(d->mdef,ci)); */
                    if (!strcmp
                        (cmu6_lts_phone_table[ph],
                         bin_mdef_ciphone_str(d->mdef, ci)))
                        break;
                }
                if (cmu6_lts_phone_table[ph] == NULL) {
                    E_FATAL
                        ("A phone in the model definition doesn't appear in the letter to sound ",
                         "rules. \n This is case we don't recommend user to ",
                         "use the built-in LTS. \n Please kindly turn off ",
                         "-lts_mismatch\n");
                }
            }
        }
#endif
    }
    ckd_free(wptr);

    return 0;
}

s3dict_t *
s3dict_init(bin_mdef_t * mdef, const char *dictfile, const char *fillerfile,
            int useLTS, int breport)
{
    FILE *fp, *fp2;
    int32 n;
    char line[1024];
    s3dict_t *d;
    s3cipid_t i, sil;

    if (!dictfile)
        E_FATAL("No dictionary file\n");

    /*
     * First obtain #words in dictionary (for hash table allocation).
     * Reason: The PC NT system doesn't like to grow memory gradually.  Better to allocate
     * all the required memory in one go.
     */
    if ((fp = fopen(dictfile, "r")) == NULL)
        E_FATAL_SYSTEM("fopen(%s,r) failed\n", dictfile);
    n = 0;
    while (fgets(line, sizeof(line), fp) != NULL) {
        if (line[0] != '#')
            n++;
    }
    rewind(fp);

    fp2 = NULL;
    if (fillerfile) {
        if ((fp2 = fopen(fillerfile, "r")) == NULL)
            E_FATAL_SYSTEM("fopen(%s,r) failed\n", fillerfile);

        while (fgets(line, sizeof(line), fp2) != NULL) {
            if (line[0] != '#')
                n++;
        }
        rewind(fp2);
    }

    /*
     * Allocate dict entries.  HACK!!  Allow some extra entries for words not in file.
     * Also check for type size restrictions.
     */
    d = (s3dict_t *) ckd_calloc(1, sizeof(s3dict_t));       /* freed in s3dict_free() */
    d->max_words =
        (n + S3DICT_INC_SZ < MAX_S3WID) ? n + S3DICT_INC_SZ : MAX_S3WID;
    if (n >= MAX_S3WID)
        E_FATAL("#Words in dictionaries (%d) exceeds limit (%d)\n", n,
                MAX_S3WID);

    d->word = (dictword_t *) ckd_calloc(d->max_words, sizeof(dictword_t));      /* freed in s3dict_free() */
    d->n_word = 0;
    d->mdef = mdef;

    /* Create new hash table for word strings; case-insensitive word strings */
    d->ht = hash_table_new(d->max_words, 1 /* no-case */ );

    d->lts_rules = NULL;
    if (useLTS)
        d->lts_rules = (lts_t *) & (cmu6_lts_rules);


    /* Digest main dictionary file */
    E_INFO("Reading main dictionary: %s\n", dictfile);
    s3dict_read(fp, d);
    fclose(fp);
    E_INFO("%d words read\n", d->n_word);

    /* Now the filler dictionary file, if it exists */
    d->filler_start = d->n_word;
    if (fillerfile) {
        E_INFO("Reading filler dictionary: %s\n", fillerfile);
        s3dict_read(fp2, d);
        fclose(fp2);
        E_INFO("%d words read\n", d->n_word - d->filler_start);
    }
    sil = bin_mdef_silphone(mdef);
    for (n = i = 0; i < mdef->n_ciphone; ++i) {
        /*
         * SIL is disguised as <sil>
         */
        if (i != sil && bin_mdef_is_fillerphone(mdef, i)) {
            /*
             * Add as a filler word, like ++NOISE++
             */
            snprintf(line, sizeof(line), "+%s+", bin_mdef_ciphone_str(mdef, i));
            if (s3dict_wordid(d, line) == BAD_S3WID) {
                E_INFO("Adding filler word: %s\n", line);
                s3dict_add_word(d, line, &i, 1);
                ++n;
            }
        }
    }
    E_INFO("Added %d fillers from mdef file\n", n);
    if (s3dict_wordid(d, S3_START_WORD) == BAD_S3WID) {
        s3dict_add_word(d, S3_START_WORD, &sil, 1);
    }
    if (s3dict_wordid(d, S3_FINISH_WORD) == BAD_S3WID) {
        s3dict_add_word(d, S3_FINISH_WORD, &sil, 1);
    }
    if (s3dict_wordid(d, S3_SILENCE_WORD) == BAD_S3WID) {
        s3dict_add_word(d, S3_SILENCE_WORD, &sil, 1);
    }

    d->filler_end = d->n_word - 1;

    /* Initialize distinguished word-ids */
    d->startwid = s3dict_wordid(d, S3_START_WORD);
    d->finishwid = s3dict_wordid(d, S3_FINISH_WORD);
    d->silwid = s3dict_wordid(d, S3_SILENCE_WORD);

    if ((d->filler_start > d->filler_end)
        || (!s3dict_filler_word(d, d->silwid)))
        E_FATAL("%s must occur (only) in filler dictionary\n",
                S3_SILENCE_WORD);

    /* No check that alternative pronunciations for filler words are in filler range!! */

    return d;
}


s3wid_t
s3dict_wordid(s3dict_t * d, const char *word)
{
    int32 w;

    assert(d);
    assert(word);

    if (hash_table_lookup_int32(d->ht, word, &w) < 0)
        return (BAD_S3WID);
    return w;
}


s3wid_t
_s3dict_basewid(s3dict_t * d, s3wid_t w)
{
    assert(d);
    assert((w >= 0) && (w < d->n_word));

    return (d->word[w].basewid);
}


char *
_s3dict_wordstr(s3dict_t * d, s3wid_t wid)
{
    assert(d);
    assert(IS_S3WID(wid) && (wid < d->n_word));

    return (d->word[wid].word);
}


s3wid_t
_s3dict_nextalt(s3dict_t * d, s3wid_t wid)
{
    assert(d);
    assert(IS_S3WID(wid) && (wid < d->n_word));

    return (d->word[wid].alt);
}


int32
s3dict_filler_word(s3dict_t * d, s3wid_t w)
{
    assert(d);
    assert((w >= 0) && (w < d->n_word));

    w = s3dict_basewid(d, w);
    if ((w == d->startwid) || (w == d->finishwid))
        return 0;
    if ((w >= d->filler_start) && (w <= d->filler_end))
        return 1;
    return 0;
}


int32
s3dict_word2basestr(char *word)
{
    int32 i, len;

    len = strlen(word);
    if (word[len - 1] == ')') {
        for (i = len - 2; (i > 0) && (word[i] != '('); --i);

        if (i > 0) {
            /* The word is of the form <baseword>(...); strip from left-paren */
            word[i] = '\0';
            return i;
        }
    }

    return -1;
}

/* RAH 4.19.01, try to free memory allocated by the calls above.
   All testing I've done shows that this gets all the memory, however I've 
   likely not tested all cases. 
*/
void
s3dict_free(s3dict_t * d)
{
    int i;
    dictword_t *word;

    if (d) {                    /* Clean up the dictionary stuff */
        /* First Step, free all memory allocated for each word */
        for (i = 0; i < d->n_word; i++) {
            word = (dictword_t *) & (d->word[i]);
            if (word->word)
                ckd_free((void *) word->word);
            if (word->ciphone)
                ckd_free((void *) word->ciphone);
        }

        if (d->word)
            ckd_free((void *) d->word);
        if (d->ht)
            hash_table_free(d->ht);
        ckd_free((void *) d);
    }
}

void
s3dict_report(s3dict_t * d)
{
    E_INFO_NOFN("Initialization of s3dict_t, report:\n");
    E_INFO_NOFN("Max word: %d\n", d->max_words);
    E_INFO_NOFN("No of word: %d\n", d->n_word);
    E_INFO_NOFN("\n");
}
