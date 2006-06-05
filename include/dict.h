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
 * $Log: dict.h,v $
 * Revision 1.1.1.1  2006/05/23 18:45:02  dhuggins
 * re-importation
 *
 * Revision 1.8  2004/12/10 16:48:55  rkm
 * Added continuous density acoustic model handling
 *
 * 
 * Spring 89, Fil Alleva (faa) at Carnegie Mellon
 * 	Created.
 */


#ifndef _DICT_H_
#define _DICT_H_

#include <sys/types.h>
#include "hash.h"

#define NO_WORD		-1

/* DICT_ENTRY
 *------------------------------------------------------------*
 * DESCRIPTION
 * 	Each dictionary entry consists of the word pronunciation
 * and list of next words in this grammar for this word.
 */ 
typedef struct dict_entry {
    char	       *word;		/* Ascii rep of the word */
    int32	       *phone_ids;	/* gt List of the of the Phones */
    int32	       *ci_phone_ids;	/* context independent phone ids */
    int16               len;		/* Number of Phones in the word */
    int16		mpx;		/* Is this a multiplexed entry? */
    int32		wid;		/* Actual word id */
    int32		alt;		/* Alt word idx */
    int32		fwid;		/* final word id in a phrase */
    int32		lm_pprob;	/* Lang model phrase probability */
}                   dict_entry_t;

typedef struct _dict {
    hash_t 		dict;
    int32 		dict_entry_count;
    dict_entry_t	**dict_list;
    int32		ci_index_len;	 	/* number of indecies */
    int32		*ci_index;		/* Index to each group */
    int32		filler_start;		/* Start of filler words */
} dictT;

int32 dict_read (dictT *dict,
		 char *filename,	/* Main dict file */
		 char *p_filename,	/* Phrase dict file */
		 char *n_filename,	/* Noise dict file */
		 int32 use_context);

void dict_free (dictT *dict);

#define DICT_SILENCE_WORDSTR	"SIL"

dict_entry_t *dict_get_entry (dictT *dict, int i);
int32 dict_count(dictT *dict);
dictT *dict_new(void);
list_t *dict_mtpList(void);
int32 **dict_left_context_fwd(void);
int32 **dict_right_context_fwd(void);
int32 **dict_left_context_bwd(void);
int32 **dict_right_context_bwd(void);
int32 **dict_right_context_fwd_perm (void);
int32 *dict_right_context_fwd_size (void);
int32 **dict_left_context_bwd_perm (void);
int32 *dict_left_context_bwd_size (void);
int32 dict_to_id(dictT *dict, char const *dict_str);
char const *dictid_to_str(dictT *dict, int32 id);
caddr_t dictStrToWordId (dictT *dict, char const *dict_str, int verbose);
int32 dict_add_word (dictT *dict, char const *word, char const *pron);
int32 dict_pron (dictT *dict, int32 w, int32 **pron);
int32 dict_next_alt (dictT *dict, int32 w);
int32 dict_write_oovdict (dictT *dict, char const *file);
int32 dictid_to_baseid (dictT *dict, int32 wid);
int32 dict_get_num_main_words (dictT *dict);
int32 dict_get_first_initial_oov(void);
int32 dict_get_last_initial_oov(void);
int32 dict_is_new_word (int32 wid);

/* Return TRUE if the given wid is a filler word, FALSE otherwise */
int32 dict_is_filler_word (dictT *dict, int32 wid);


#define WordIdToStr(d,x)	((x == NO_WORD) ? "" : d->dict_list[x]->word)

#define WordIdToBaseStr(d,x)	((x == NO_WORD) ? "" :	\
				   d->dict_list[d->dict_list[x]->wid]->word)

#define dict_pronlen(dict,wid)	((dict)->dict_list[wid]->len)
#define dict_ciphone(d,w,p)	((d)->dict_list[w]->ci_phone_ids[p])
#define dict_phone(d,w,p)	((d)->dict_list[w]->phone_ids[p])
#define dict_mpx(d,w)		((d)->dict_list[w]->mpx)


#endif
