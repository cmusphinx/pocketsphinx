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

#ifndef _S2_SEARCH_H_
#define _S2_SEARCH_H_	1

#include <fe.h>

#include "basic_types.h"
#include "fbs.h"
#include "dict.h"
#include "hmm.h"

#define NODE_CNT	(HMM_LAST_STATE+1)	/* No. of nodes in Sphinx HMM */

/*
 * Lexical tree node data type.  Not the first HMM for words, which multiplex HMMs
 * based on different left contexts.  This structure is used both in the dynamic
 * HMM tree structure and in the per-word last-phone right context fanout.
 */
typedef struct chan_s
{
    hmm_t hmm;                  /* Basic HMM structure.  This *must*
                                 * be first in the structure because
                                 * chan_t and root_chan_t are
                                 * sometimes used interchangeably */
    struct chan_s *next;	/* first descendant of this channel; or, in the
				   case of the last phone of a word, the next
				   alternative right context channel */
    struct chan_s *alt;		/* sibling; i.e., next descendant of parent HMM */

    int32    ciphone;		/* ciphone for this node */
    union {
	WORD_ID penult_phn_wid;	/* list of words whose last phone follows this one;
				   this field indicates the first of the list; the
				   rest must be built up in a separate array.  Used
				   only within HMM tree.  -1 if none */
	int32 rc_id;		/* right-context id for last phone of words */
    } info;
} chan_t;

/*
 * Lexical tree node data type for the first phone (root) of each dynamic HMM tree
 * structure.  Each state may have a different parent static HMM.  Most fields are
 * similar to those in chan_t.
 */
typedef struct root_chan_s
{
    hmm_t hmm;                  /* Basic HMM structure.  This *must*
                                 * be first in the structure because
                                 * chan_t and root_chan_t are
                                 * sometimes used interchangeably */
    chan_t *next;		/* first descendant of this channel */

    WORD_ID  penult_phn_wid;
    WORD_ID  this_phn_wid;	/* list of words consisting of this single phone;
				   actually the first of the list, like penult_phn_wid;
				   -1 if none */
    int32    diphone;		/* first diphone of this node; all words rooted at this
				   node begin with this diphone */
    int32    ciphone;		/* first ciphone of this node; all words rooted at this
				   node begin with this ciphone */
} root_chan_t;

/*
 * Back pointer table (forward pass lattice; actually a tree)
 */
typedef struct bptbl_s {
    FRAME_ID frame;		/* start or end frame */
    WORD_ID  wid;		/* Word index */
    LAT_ID   bp;		/* Back Pointer */
    int32    score;		/* Score (best among all right contexts) */
    int32    s_idx;		/* Start of BScoreStack for various right contexts*/
    WORD_ID  real_wid;		/* wid of this or latest predecessor real word */
    WORD_ID  prev_real_wid;	/* real word predecessor of real_wid */
    int32    r_diph;		/* rightmost diphone of this word */
    int32    ascr;
    int32    lscr;
    int32    valid;		/* For absolute pruning */
} BPTBL_T;

#define NO_BP		-1

/* #hyp entries */
#define HYP_SZ		1024

/* -------------- Lattice (DAG) search related -------------- */

/*
 * Links between DAG nodes (see latnode_t below).
 * Also used to keep scores in a bestpath search.
 */
typedef struct latlink_s {
    struct latnode_s *from;	/* From node */
    struct latnode_s *to;	/* To node */
    struct latlink_s *next;	/* Next link from the same "from" node */
    struct latlink_s *best_prev;
    struct latlink_s *q_next;
    int32 link_scr;		/* Score for from->wid (from->sf to this->ef) */
    int32 path_scr;		/* Best path score from root of DAG */
    int32 ef;			/* end-frame for the "from" node */
} latlink_t;

typedef struct rev_latlink_s {
    latlink_t *link;
    struct rev_latlink_s *next;
} rev_latlink_t;

/*
 * DAG nodes.
 */
typedef struct latnode_s {
    int32 wid;			/* Dictionary word id */
    int32 fef;			/* First end frame */
    int32 lef;			/* Last end frame */
    int16 sf;			/* Start frame */
    int16 reachable;		/* From </s> or <s> */
    union {
	int32 fanin;		/* #nodes with links to this node */
	int32 rem_score;	/* Estimated best score from node.sf to end */
    } info;
    latlink_t *links;		/* Links out of this node */
    rev_latlink_t *revlinks;	/* Reverse links (for housekeeping purposes only) */
    struct latnode_s *next;	/* Next node (for housekeeping purposes only) */
} latnode_t;

/* Interface */
void search_initialize (void);
void search_free (void);
void search_set_beam_width (double beam);
void search_set_new_word_beam_width (double beam);
void search_set_lastphone_alone_beam_width (double beam);
void search_set_new_phone_beam_width (double beam);
void search_set_last_phone_beam_width (double beam);

/* Get logs2 beam, phone-exit beam, word-exit beam */
void search_get_logbeams (int32 *b, int32 *pb, int32 *wb);

void search_set_channels_per_frame_target (int32 cpf);
int32 searchFrame (void);
void searchSetFrame (int32 frame);
int32 searchCurrentFrame (void);
void search_set_newword_penalty (double nw_pen);
void search_set_silence_word_penalty (float pen, float pip);
void search_set_filler_word_penalty (float pen, float pip);
void search_set_lw (double p1lw, double p2lw, double p3lw);
void search_set_ip (float ip);
void search_set_hyp_alternates (int32 arg);
void search_set_skip_alt_frm (int32 flag);
void search_set_hyp_total_score (int32 score);
void search_set_hyp_total_lscr (int32 lscr);
void search_set_context (int32 w1, int32 w2);
void search_set_startword (char const *str);
int32 search_get_current_startwid ( void );

int32 search_result(int32 *fr, char **res);	/* Decoded result as a single string */
int32 search_partial_result (int32 *fr, char **res);

/* Total score for the utterance */
int32 search_get_score(void);
/* Total LM score for the utterance (is this reliable in N-gram mode?) */
int32 search_get_lscr(void);

int32 *search_get_bestpscr(void);
/*
 * Get the HYP_SZ array of filtered hyp words (no <s>, </s>, fillers, or
 * null transitions) in this array.
 */
search_hyp_t *search_get_hyp (void);

char *search_get_wordlist (int *len, char sep_char);
int32 search_get_bptable_size (void);
int32 *search_get_lattice_density ( void );
latnode_t *search_get_lattice( void );
double *search_get_phone_perplexity ( void );
int32 search_get_sil_penalty (void);
int32 search_get_filler_penalty ( void );
BPTBL_T *search_get_bptable ( void );
void search_postprocess_bptable (lw_t lwf, char const *pass);
int32 *search_get_bscorestack ( void );
lw_t search_get_lw ( void );

void search_remove_context (search_hyp_t *hyp);
void search_hyp_to_str ( void );
void search_hyp_free (search_hyp_t *h);

void sort_lattice(void);
void search_dump_lattice (char const *file);
void search_dump_lattice_ascii (char const *file);
void dump_traceword_chan (void);

void init_search_tree (dictT *dict);
void create_search_tree (dictT *dict, int32 use_lm);
void delete_search_tree (void);
void delete_search_subtree (chan_t *hmm);
void free_search_tree(void);

int32 eval_root_chan (void);
int32 eval_nonroot_chan (void);
int32 eval_word_chan (void);
void save_bwd_ptr (WORD_ID w, int32 score, int32 path, int32 rc);
void prune_root_chan (void);
void prune_nonroot_chan (void);
void last_phone_transition (void);
void prune_word_chan (void);
void alloc_all_rc (int32 w);
void free_all_rc (int32 w);
void word_transition (void);

void search_set_current_lm (void); /* Need to call lm_set_current() first */

/* Return sum of top senone scores for the given frame range (inclusive) */
int32 seg_topsen_score (int32 sf, int32 ef);

void compute_seg_scores (lw_t lwf);
void compute_sen_active (void);
void evaluateChannels (void);
void pruneChannels (void);

void search_fwd (mfcc_t **feat);
void search_start_fwd (void);
void search_finish_fwd (void);
void search_one_ply_fwd (void);

void bestpath_search ( void );

void search_fwdflat_init ( void );
void search_fwdflat_finish ( void );
void search_fwdflat_free ( void );
void build_fwdflat_chan ( void );
void destroy_fwdflat_chan ( void );
void search_set_fwdflat_bw (double bw, double nwbw);
void search_fwdflat_start ( void );
void search_fwdflat_frame (mfcc_t **feat);
void compute_fwdflat_senone_active ( void );
void fwdflat_eval_chan ( void );
void fwdflat_prune_chan ( void );
void fwdflat_word_transition ( void );

void destroy_frm_wordlist ( void );
void get_expand_wordlist (int32 frm, int32 win);

int32 search_bptbl_wordlist (int32 wid, int32 frm);
int32 search_bptbl_pred (int32 b);

/* Warning: the following block of functions are not actually implemented. */
void search_finish_document (void);
void searchBestN (void);
void search_hyp_write (void);
void parse_ref_str (void);
void search_filtered_endpts (void);

/* Functions from searchlat.c */
void searchlat_init ( void );
int32 bptbl2latdensity (int32 bptbl_sz, int32 *density);
int32 lattice_rescore ( lw_t lwf );

/* Make the given channel inactive and set state scores to WORST_SCORE */
void search_chan_deactivate(chan_t *);

/* Note the top senone score for the given frame */
void search_set_topsen_score (int32 frm, int32 score);

/*
 * Set the hyp_wid (and n_hyp_wid) global variables in search.c to the given
 * hypothesis (linked list of search_hyp_t entries).
 */
void search_set_hyp_wid (search_hyp_t *hyp);


#endif
