/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                        Copyright (c) 1999                             */
/*                        All Rights Reserved.                           */
/*                                                                       */
/*  Permission is hereby granted, free of charge, to use and distribute  */
/*  this software and its documentation without restriction, including   */
/*  without limitation the rights to use, copy, modify, merge, publish,  */
/*  distribute, sublicense, and/or sell copies of this work, and to      */
/*  permit persons to whom this work is furnished to do so, subject to   */
/*  the following conditions:                                            */
/*   1. The code must retain the above copyright notice, this list of    */
/*      conditions and the following disclaimer.                         */
/*   2. Any modifications must be clearly marked as such.                */
/*   3. Original authors' names are not deleted.                         */
/*   4. The authors' names are not used to endorse or promote products   */
/*      derived from this software without specific prior written        */
/*      permission.                                                      */
/*                                                                       */
/*  CARNEGIE MELLON UNIVERSITY AND THE CONTRIBUTORS TO THIS WORK         */
/*  DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING      */
/*  ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT   */
/*  SHALL CARNEGIE MELLON UNIVERSITY NOR THE CONTRIBUTORS BE LIABLE      */
/*  FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES    */
/*  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN   */
/*  AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,          */
/*  ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF       */
/*  THIS SOFTWARE.                                                       */
/*                                                                       */
/*************************************************************************/
/*             Author:  Alan W Black (awb@cs.cmu.edu)                    */
/*               Date:  December 1999                                    */
/*************************************************************************/
/*                                                                       */
/*  Letter to sound rules                                                */
/*                                                                       */
/*************************************************************************/

/*
 * lts.h -- Letter to sound rule support 
 * 
 * $Log$
 * Revision 1.1  2006/04/05  20:27:30  dhdfu
 * A Great Reorganzation of header files and executables
 * 
 * Revision 1.2  2006/02/22 20:44:17  arthchan2003
 * Merged from branch SPHINX3_5_2_RCI_IRII_BRANCH: As we have done in SphinxTrain and with the permission of flite developer. check in the routines for using LTS. In dict.c, one will see detail comment on how it was used and how we avoid problematic conditions.
 *
 * Revision 1.1.2.1  2005/09/25 19:07:52  arthchan2003
 * Added LTS rules and the module to use it.
 *
 *
 */

#ifndef _CST_LTS_H__
#define _CST_LTS_H__

#include <s3types.h>


#ifdef __cplusplus
extern "C" {
#endif
#if 0
/* Fool Emacs. */
}
#endif

typedef unsigned short cst_lts_addr;
typedef int cst_lts_phone;
typedef unsigned char  cst_lts_feat;
typedef unsigned char  cst_lts_letter;
typedef unsigned char  cst_lts_model;

/* end of rule value */
#define CST_LTS_EOR 255

/*
 */
typedef uint32 acmod_id_t;
typedef uint32 word_id_t;

typedef struct lex_entry_str {
    char *ortho;
    word_id_t word_id;
    char **phone;
    acmod_id_t *ci_acmod_id;
    uint32  phone_cnt;

    struct lex_entry_str *next;
} lex_entry_t;

typedef struct cst_lts_rules_struct {
    char *name; /**< The name of the rule */
    const cst_lts_addr *letter_index;  /**< index into model first state */
    const cst_lts_model *models;
    const char * const * phone_table; 
    int context_window_size;
    int context_extra_feats;
    const char * const * letter_table;
} cst_lts_rules;
/* For Sphinx naming conventions. */
typedef struct cst_lts_rules_struct lts_t;

/* \struct cst_lts_rule
   
 */
typedef struct cst_lts_rule_struct {
    cst_lts_feat   feat;
    cst_lts_letter val;
    cst_lts_addr   qtrue;
    cst_lts_addr   qfalse;
} cst_lts_rule;
typedef struct cst_lts_rule_struct lts_rule_t;

struct lex_entry_str; /* This is actually lex_entry_t */
cst_lts_rules *new_lts_rules(void);
int lts_apply(const char *word,const char *feats,
	      const cst_lts_rules *r, struct lex_entry_str *out_phones);

/**
   Print lexical entry 
*/
void lex_print(lex_entry_t *ent /**< A lexical entry */
    );

extern const cst_lts_rules cmu6_lts_rules;

#ifdef __cplusplus
}
#endif


#endif

