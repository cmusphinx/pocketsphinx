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
 *------------------------------------------------------------*
 * DESCRIPTION
 *	Interface to Sphinx-II global knowledge base
 *-------------------------------------------------------------*
 */


#ifndef _KB_EXPORTS_H_
#define _KB_EXPORTS_H_

/* SphinxBase headers. */
#include <ngram_model.h>
#include <logmath.h>

/* Local headers. */
#include "log.h"
#include "dict.h"
#include "tmat.h"
#include "bin_mdef.h"
#include "s2_semi_mgau.h"
#include "ms_mgau.h"

/* Initialize the acoustic and language models. */
void kb_init(void);
/* Free up (or try, at least) the acoustic and language models. */
void kb_close(void);

/* Look up a word in the dictionary. */
int32 kb_get_word_id(char const *word);
char *kb_get_word_str(int32 wid);

/* Update the dictionary to LM word ID mapping. */
void kb_update_widmap(void);

/* FIXME: silly function to get the sendump file. */
char *kb_get_senprob_dump_file(void);

/*
 * Return the maximum size of the dictionary.
 */
int32 kb_dict_maxsize(void);

/* Global model definition. */
extern bin_mdef_t *mdef;
extern tmat_t *tmat;

/* Global model set. */
extern s2_semi_mgau_t *semi_mgau;
extern ms_mgau_model_t *ms_mgau;

/* Global dictionary. */
extern dictT *word_dict;

/* Global language model set. */
extern ngram_model_t *lmset;

/* Global phone transition matrix (to be replaced by a LM) */
extern int32 **phonetp;

#endif
