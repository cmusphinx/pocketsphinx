/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 1995-2004 Carnegie Mellon University.  All rights
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
 * ctxt_table.h -- Phone Context Table Structure
 *
 * **********************************************
 * CMU ARPA Speech Project
 *
 * Copyright (c) 1995 Carnegie Mellon University.
 * ALL RIGHTS RESERVED.
 * **********************************************
 * 14-Jul-05    ARCHAN (archan@cs.cmu.edu) at Carnegie Mellon Unversity 
 *              First created it. 
 *
 * $Log$
 * Revision 1.1  2006/04/05  20:27:30  dhdfu
 * A Great Reorganzation of header files and executables
 * 
 * Revision 1.2  2006/02/22 20:46:05  arthchan2003
 * Merged from branch SPHINX3_5_2_RCI_IRII_BRANCH: ctxt_table is a wrapper of the triphone context structure and its maniuplations which were used in flat_fwd.c .  The original flat_fwd.c was very long (3000) lines.  It was broken in 5 parts, ctxt_table is one of the 5.
 *
 * Revision 1.1.2.2  2005/09/27 07:39:17  arthchan2003
 * Added ctxt_table_free.
 *
 * Revision 1.1.2.1  2005/09/25 19:08:25  arthchan2003
 * Move context table from search to here.
 *
 * Revision 1.1.2.3  2005/09/07 23:32:03  arthchan2003
 * 1, Added get_lcpid in parrallel with get_rcpid. 2, Also fixed small mistakes in the macro.
 *
 * Revision 1.1.2.2  2005/07/17 05:42:27  arthchan2003
 * Added super-detailed comments ctxt_table.h. Also added dimension to the arrays that stores all context tables.
 *
 * Revision 1.1.2.1  2005/07/15 07:48:32  arthchan2003
 * split the hmm (whmm_t) and context building process (ctxt_table_t) from the the flat_fwd.c
 *
 *
 */

/* 
 * \file ctxt_table.h
 * \brief data structure for building cross word triphones for Sphinx 3. 
 */

#ifndef _CTX_TAB_
#define _CTX_TAB_

#include <prim_type.h>

#include "s3types.h"
#include "bin_mdef.h"
#include "s3dict.h"


#ifdef __cplusplus
extern "C" {
#endif
#if 0
/* Fool Emacs. */
}
#endif

/**
 * Triphone information in the flat lexicon search in Sphinx 3.0
 * for all word hmm modelling broken up into 4 cases:
 * 	within-word triphones
 * 	left-context cross-word triphones (multi-phone words)
 * 	right-context cross-word triphones (multi-phone words)
 * 	left- and right-cross-word triphones (single-phone words)
 * These 4 cases captured by the following data structures.
 */

/**
 * \struct xwdssid_t
 * \brief cross word triphone model structure 
 */

typedef struct {
    s3ssid_t  *ssid;	/**< Senone Sequence ID list for all context ciphones */
    s3cipid_t *cimap;	/**< Index into ssid[] above for each ci phone */
    int32     n_ssid;	/**< #Unique ssid in above, compressed ssid list */
} xwdssid_t;


#define ctxt_table_left_ctxt_ssid(ct,l,b,r)  ((ct)->lcssid[b][r].ssid[ct->lcssid[b][r].cimap[l]])
#define ctxt_table_word_int_ssid(ct,wid,wpos)  ((ct)->wwssid[wid][wpos])
#define ctxt_table_right_ctxt_ssid(ct,l,b,r)  ((ct)->rcssid[b][l].ssid[ct->rcssid[b][l].cimap[r]])
#define ctxt_table_single_phone_ssid(ct,l,b,r)  ((ct)->lrcssid[b][l].ssid[ct->lrcssid[b][l].cimap[r]])

/**
 * \struct ctxt_table_t 
 *
 * Ravi's Comment
 * First, the within word triphone models.  wwssid[w] = list of
 * senone sequences for word w.  Since left and right extremes
 * require cross-word modelling (see below), wwssid[w][0] and
 * wwssid[w][pronlen-1] contain no information and shouldn't be
 * touched.
 *
 * Left context mapping (for multiphone words): given the 1st base phone, b, of a word
 * and its right context, r, the senone sequence for any left context l =
 *     lcssid[b][r].ssid[lcssid[b][r].cimap[l]].
 * 
 * Similarly, right context mapping (for multiphone words): given b and left context l,
 * the senone sequence for any right context r =
 *     rcssid[b][l].ssid[lcssid[b][l].cimap[r]].
 * 
 * A single phone word is a combination of the above, where both l and r are unknown.
 * Senone sequence, given any l and r context ciphones:
 *     lrcssid[b][l].ssid[lcssid[b][l].cimap[r]].
 * For simplicity, all cimap[] vectors (for all l) must be identical.  For now, this is
 * attained by avoiding any compression and letting cimap be the identity map.
 *
 *
 * Note by ARCHAN at 20050715
 *
 * Ever wonder how cross word triphones in a search actually work? I
 * will guess sphinx 3.x is perhaps the best example of how
 * context-dependent phone is implemented. Either exact or approximation. 
 * 
 * The following is more a mental exercise that help all of you who
 * are not familiar with this part of programming and get you
 * understand what Ravi and I were actually do it.  (I used this note
 * to train myself when I tried to understand Ravi's code.)
 *
 * If you know some stuffs, you will know that the following are all
 * well-explored by some other people (I think), one can also use FSM
 * to implement most of the below so why bother? Well, it is always
 * not a bad idea to work things our from its simplest form.  I always
 * find insights when working things out step by step. Therefore I
 * believe you will find the following discussion be interesting. 
 *
 * First part, If you are into static allocation of a CD graph. 
 *
 * 0, Something very very trivial. In the case of word internal
 * triphone expansion, what will be the graph look like? Hmm. This
 * looks trivial but could already many block many people. I will
 * guess the reason is how CD phone is expressed is already something
 * very non-intuitive. For example, with base phone b, left context l,
 * right context r. Many will represent it as b(l,r), so I found it
 * pretty hard to understand.  So I prefer the following "tied-fighter"
 * representation for triphones
 * l<-b->r 
 *
 * For quinphones:
 * l2<-l1<-b->r1->r2
 *
 * For septaphone
 * l3<-l2<l1<-b->r1->r2->r3
 *
 * So for a ci-phone graph look like this
 *
 * -> ph1 -> ph2 -> ph3 -> 
 * -> ph4 -> ph5 -> ph6 ->
 * 
 * Then the ci phone graph will look like
 * -> (ph1->ph2) -> (ph1<-ph2->ph3) -> (ph2<-ph3) ->
 * -> (ph4->ph5) -> (ph4<-ph5->ph6) -> (ph5<-ph6) ->
 *
 * 1, Something trivial: In the case of full cross word triphone
 * expansion, how do you quickly decide which phone to expand?  Assume
 * you have an arbitrary polyphone with Base phone B, with L phone
 * context at the left and R phone context at the right (or a
 * (L+R+1)-phone).
 * 
 * Then what you need to do is just to look at all R phones right
 * after word begin.  And to look at all L phones before the word end.
 *
 * For example, In this two words case, AND: AE N D , BIT, B IY D. And
 * you consider triphone, i.e. L=1, R=1, then You know that the D in
 * "AE N D" needs to be expanded to the right.  As well as D in "B IY
 * D".  You will also know that AE in "AE N D" and B in "B IY D".  
 *
 * 2, Another something which is trivial, Once we know that a ci-phone
 * in a word need to be expanded.  How to expand then? The first thing
 * to remember is that context-dependent phone (triphone,quinphone,
 * setaphone) is a stupid approach.  It just tries to eliminate
 * things.  For example, when you know that the word-end phone need to
 * be expanded to the right by 2 contexts.  Then what you need to do
 * is to enumerate all possible 2 phones begins. 
 *
 * 3, Here comes a part which is slighly non-trivial.  This is related
 * to state-tying. What is the effect of having tied-state in the HMM
 * (or in CMU terminology, senone)? This is one of the fun-part. You
 * will come up with situations where the definition of state of a HMM
 * (means, variances) would be exactly the same as another state of a
 * HMM. (usually the same location.). So in some situations, one
 * triphone HMM could actually be exactly the same as another one. 
 *
 * Once you enumerate all possible contexts (either left or right),
 * then, you will find that some of them are actually the same because
 * of state-tying. So, here comes a process where we (CMU folks) call
 * it compression.  That is why in Sphinx's code, you will see a term
 * call "senone sequence ID". Which essentially means a unique HMM
 * instance that is shared by multiple expanded triphones.  You can
 * imagine this will be the same for quinphones or septaphones. 
 *
 * 4, What if we have a very short word?  Simple example is a
 * single-phone word in the case of triphone. How do we deal with
 * that? In a single word case for triphone expansion case, we just
 * need to enumerate all possible left **and** right context.  Then,
 * you will get everything you want. HTK's HVite will exactly handle
 * this. When I first read its code, I am very impressed. 
 * 
 * Now in general, how long of a word will be affected by
 * context-dependent phone expansion which has L left phone context
 * and R right phone context? Or we need to think of "a special case"
 * to consider both left and right context at the same time?  First,
 * consider what length of the word, the answer is trivial, if you
 * have a crossword n-phone, then if a word has less (n-2) phones,
 * then you need to consider both left and right context together. I
 * usually think in this way, this is case where the phone has totally
 * "covered" the whole word! 
 * 
 * An even more detail question, how many contexts one needs to expand
 * in position i of a word with m phones but we are using n-phone
 * expansion where m<=n-2? This is slightly tougher. This is how you
 * could think about it.  What you need to do to think you always need
 * to consider L left context and R right context. Let's just say they
 * are all right context for the sake of argument.  Then, for i=1 to
 * m, if m-i <= R, then you don't need to consider right context. else
 * you will need to consider R-(m-i) cross word context.  Similar
 * argument will work for left context. 
 *
 *
 * 5, Now make all together, the following is one way for you to
 * allocate a graph with CD phone or m-phone.  
 *
 * i, look at its length, if it is smaller than m-2, then use the
 * discussion in 4 to handle it. 
 *
 * ii, if it is larger than m-2, then 
 *     iia, identify all position which will have CD phone expansion
 *     iib, expand those which will not have CD phone expansion first
 *     iic then expand those which has CD phone expansion by enumerate
 *     its context. If it is at the right of the word begin, then
 *     consider all L phone combination at the word end.  Vice versa. 
 *
 * Most algorithm essentially just work in this way but this
 * simplistic views forgot the one important problem. After the phone
 * expanded, how do we link expanded CD phone together within a word?
 * There is no such problem at all in triphones because we just need
 * to consider the left-most or right most context.  So if you work on
 * quinphone for the first time, you will be slightly surprised. What
 * you got from a flat lexicon after quinphone expansion will be
 * actually a tree lexicon.  Why?
 *
 * Consider this example
 *
 * ph1 -> ph2 -> ph3 -> ph4 ->ph5
 *     -> ph2_2-> ......
 * ph6 -> ph7 -> ph8 -> ph9 ->ph10
 *     -> ph7_2-> ......
 *
 * If we just consider expansion to right context and we start the the
 * internal phone. Then we have
 *
 * Step 1
 * ph1 -> ph2 -> (ph1<-ph2<-ph3->ph4->ph5)  -> ph4 ->ph5
 *   |--> ph2_2-> ......
 * ph6 -> ph7 -> (ph6<-ph7<-ph8->ph9->ph10) -> ph9 ->ph10
 *   |--> ph7_2-> ......
 * 
 * Step 2
 * ph1 -> ph2 -> (ph1<-ph2<-ph3->ph4->ph5)  -> (ph2<-ph3<-ph4->ph5->ph1) -> ph5
 *   |--> ph2_2-> ......                  |--> (ph2<-ph3<-ph4->ph5->ph6) -> ph5
 * ph6 -> ph7 -> (ph6<-ph7<-ph8->ph9->ph10) -> (ph7<-ph8<-ph9->ph10->ph1)-> ph10
 *   |--> ph7_2-> ......                  |--> (ph7<-ph8<-ph9->ph10->ph1)-> ph10
 *
 * Step 3
 * ph1 -> ph2 -> (ph1<-ph2<-ph3->ph4->ph5)  -> (ph2<-ph3<-ph4->ph5->ph1) -> (ph3<-ph4<-ph5->ph1->ph2)
 *   |                                    |                           |---> (ph3<-ph4<-ph5->ph1->ph2_2)
 *   |--> ph2_2-> ......                  |--> (ph2<-ph3<-ph4->ph5->ph6) -> (ph3<-ph4<-ph5->ph6->ph7)
 *                                                                    |---> (ph3<-ph4<-ph5->6->ph7_2)
 * ph6 -> ph7 -> (ph6<-ph7<-ph8->ph9->ph10) -> (ph7<-ph8<-ph9->ph10->ph1)-> (ph8<-ph9->ph10->ph1->ph2 )
 *   |                                    |                           |---> (ph8<-ph9->ph10->ph1->ph2_2 )
 *   |--> ph7_2-> ......                  |--> (ph7<-ph8<-ph9->ph10->ph1)-> (ph8<-ph9->ph10->ph1->ph7) 
 *                                                                    |---> (ph8<-ph9->ph10->ph1->ph7_2 )
 *
 * See? A tree!
 *
 * So, here comes an important insight.  If there is a techniques that
 * could be used in lexical tree, it can also be used in cross word
 * triphones. Vice versa. Of course, there is small difference, but
 * this is one way you can understand algorithm in the field.  See XW
 * triphone lookahead below. 
 *
 * 6, We are almost done with static allocation. How about silence in
 * between the words? What should we do with them? This is actually a
 * simpler situation. As an approximation, you could treat silence not
 * as a word, then, you will avoid CD phone but a one phone word (sil
 * with sil as prounciation). Then, you could treat it just as another
 * left or right context. This is perhaps nice enough. 
 * 
 * 7, What we have done so far.  What we observe is how consideration
 * of context dependency complicates the search graph if we statically
 * allocate the tree.  No fast search algorithm will even bother to do
 * these things.  So, here we come an important aspects of search in
 * speech recognition.  i.e. For most of the time time, one could not
 * statically allocate the search graph with context-dependency. Most
 * of the context are dynamically allocated.  
 * 
 * Another important aspect is you will seldom see a good fast search
 * implement triphones to be exact because it usually causes too much
 * computation than necessary.  
 * 
 * So, we will spend more time on this in the second part.  This is
 * also more relate to your purpose, tracing the source code of Sphinx
 * 2 and Sphinx 3 to try to understand something out of it. 
 *
 * Second part, cross-word context-dependent phone in search in speech
 * recognition.
 *
 * 8, Types of approximation of triphones. 
 * 
 * 
 * 
 */

typedef struct {
    xwdssid_t **lcssid; /**< Left context phone id table 
                           First dimension: basephone, 
                           Second dimension: right context
                        */
  
    xwdssid_t **rcssid; /**< right context phone id table 
                           First dimension: basephone, 
                           Second dimension: left context

                        */
    xwdssid_t **lrcssid; /**< left-right ntext phone id table 
                            First dimension: basephone
                            Second dimension: left context. 
                         */


    s3ssid_t **wwssid;  /**< Within word triphone models 
                           First dimension: the word id
                           Second dimension: the phone position. 
                        */
    int32 n_backoff_ci; /**< # of backoff CI phone */
    int32 n_ci, n_word;
} ctxt_table_t ;

/**
 * Initialize a context table 
 */

ctxt_table_t *ctxt_table_init(s3dict_t *dict,  /**< A dictionary*/
			      bin_mdef_t *mdef   /**< A model definition*/
    );

/**
 * Uninitialize a context table
 */

void ctxt_table_free(ctxt_table_t *ct); /**< Context Table */

/**
 * Get the array of right context senone sequence ID for the last phone. 
 */
void get_rcssid(ctxt_table_t *ct,   /**< A context table */
                s3wid_t w,          /**< A word for query */
                s3ssid_t **ssid,    /**< Out: An array of right context phone ID */
                int32 *nssid,       /**< Out: Number of SSID */
                s3dict_t *dict        /**< In: a dictionary */
    );

/**
 * Get the array of left context senone sequence ID for the first phone.
 */
void get_lcssid(ctxt_table_t *ct,  /**< A context table */
                s3wid_t w,         /**< A word for query */
                s3ssid_t **ssid,    /**< Out: An array of right context SSID */
                int32 *nssid,      /**< Out: Number of SSID */
                s3dict_t *dict       /**< In: a dictionary */
    );


/**
 * Get the context-independent phone map for the last phone of a
 * parcitular word 
 * @return an array of ciphone ID. 
 */
s3cipid_t *get_rc_cimap(ctxt_table_t *ct, /**< A context table */
                        s3wid_t w, /**< A word for query*/
                        s3dict_t *dict /**< A dictionary */
    );

/**
 * Get the context-independent phone map for the last phone of a
 * parcitular word 
 * @return an array of ciphone ID. 
 */
s3cipid_t *get_lc_cimap(ctxt_table_t *ct, /**< A context table */
                        s3wid_t w, /**< A word for query*/
                        s3dict_t *dict /**< A dictionary */
    );

/**
 * Get number of right context for the last phone of a word. 
 * @return number of right context 
 *
 */
int32 ct_get_rc_nssid(ctxt_table_t *ct,  /**< A context table */
                      s3wid_t w,          /**< Word for query. */
                      s3dict_t *dict        /**< A dictionary */
    );

#ifdef __cplusplus
}
#endif


#endif /*_CTX_TAB_*/
