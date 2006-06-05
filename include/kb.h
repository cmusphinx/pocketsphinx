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
 * HISTORY
 * 
 * $Log: kb.h,v $
 * Revision 1.1.1.1  2006/05/23 18:45:02  dhuggins
 * re-importation
 *
 * Revision 1.10  2004/12/10 16:48:58  rkm
 * Added continuous density acoustic model handling
 *
 * 
 * 02-Dec-2004	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Added acoustic score weight (applied only to S3 continuous
 * 		acoustic models).
 * 
 * 27-May-97	M K Ravishankar (rkm@cs) at Carnegie Mellon University
 * 		Added Bob Brennan's declarations kbAddGrammar() and kb_get_personaldic().
 * 
 * 02-Apr-97	M K Ravishankar (rkm@cs) at Carnegie Mellon University
 * 		Added dict_maxsize().
 * 
 * Spring 89, Fil Alleva (faa) at Carnegie Mellon
 *	Created.
 */


#ifndef _KB_EXPORTS_H_
#define _KB_EXPORTS_H_

#include "lm_3g.h"
#include "msd.h"
#include "dict.h"
#include "cont_mgau.h"
#include "bin_mdef.h"

int32 kb_get_total_dists(void);
int32 kb_get_aw_tprob(void);
int32 kb_get_num_models(void);
int32 kb_get_num_dist(void);
int32 kb_get_num_model_instances(void);
int32 kb_get_num_words(void);
int32 kb_get_num_codebooks(void);
SMD  *kb_get_models(void);
int32 *kb_get_codebook_0_dist(void);
int32 *kb_get_codebook_1_dist(void);
int32 *kb_get_codebook_2_dist(void);
int32 *kb_get_codebook_3_dist(void);
int32 kb_get_dist_prob_bytes(void);
int32 kb_get_start_word_id(void);
int32 kb_get_finish_word_id(void);
int32 kb_get_silence_word_id(void);
int32 kb_get_silence_ciphone_id(void);
int32 **kb_get_word_transitions(void);
dictT *kb_get_word_dict(void);
LM     kb_get_lang_model(void);
int   kb_get_darpa_lm_flag(void);
int   kb_get_no_lm_flag(void);
char const *kb_get_lm_start_sym(void);
char const *kb_get_lm_end_sym(void);
char  *kb_get_word_str(int32 wid);
int32  kb_get_word_id(char const *word);
int32  dict_maxsize(void);
void   kbAddGrammar(char const *fileName, char const *grammarName);
char  *kb_get_dump_dir(void);
char  *kb_get_senprob_dump_file(void);
char  *kb_get_startsym_file(void);
int32  kb_get_senprob_size(void);
char  *kb_get_oovdic(void);
char  *kb_get_personaldic(void);
double kb_get_oov_ugprob(void);
int32  kb_get_max_new_oov(void);
int32 kb_get_mmap_flag(void);

/* Language-weight */
float32 kb_get_lw(void);

/*
 * Acoustic-weight; log2 scaling factor.  Used with continuous density
 * acoustic models only; scores SCALED DOWN by so many bits.
 */
int32 kb_get_ascr_scale(void);

/* Silence word penalty (prob) */
float32 kb_get_silpen(void);

/* Filler word penalty (prob) */
float32 kb_get_fillpen(void);

/* Phone insertion penalty (prob) */
float32 kb_get_pip(void);

/* Word insertion penalty (prob) */
float32 kb_get_wip(void);

void kb (int argc, char *argv[], float ip, float lw, float pip);

/* FSG grammar file name */
char *kb_get_fsg_file_name ( void );
char *kb_get_fsg_ctlfile_name ( void );

int32 query_fsg_use_altpron ( void );

int32 query_fsg_use_filler ( void );

/*
 * Phone transition LOGprobs table; langwt and pip already applied.
 */
int32 **kb_get_phonetp ( void );

float32 kb_get_filler_pfpen ( void );

/*
 * Return the S3 (continuous) acoustic model object, if one has been read
 * in, otherwise NULL.
 */
mgau_model_t *kb_s3model( void );

/*
 * Return the S3 model definition object, if one has been read in,
 * otherwise NULL.
 */
bin_mdef_t *kb_mdef(void);

/* Return ptr to array for storing s3 model senone scores */
int32 *kb_s3senscr ( void );

/* Return ptr to array for storing s3 model input feature vector */
float32 *kb_s3feat ( void );

/*
 * Return ptr to S3->S2 or S2->S3 senone reordering map table.  Senone
 * ordering in S3 is different from that in S2.  s2map[s3id] = s2id, and
 * s3map[s2id] = s3id.
 */
int32 *kb_s3_s2_senmap ( void );	/* s2map */
int32 *kb_s2_s3_senmap ( void );	/* s3map */


#endif
