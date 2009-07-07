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
 * dict2pid.h -- Triphones for dictionary
 * 
 * **********************************************
 * CMU ARPA Speech Project
 *
 * Copyright (c) 1999 Carnegie Mellon University.
 * ALL RIGHTS RESERVED.
 * **********************************************
 * 
 * HISTORY
 * $Log$
 * Revision 1.1  2006/04/05  20:27:30  dhdfu
 * A Great Reorganzation of header files and executables
 * 
 * Revision 1.9  2006/02/22 21:05:16  arthchan2003
 * Merged from branch SPHINX3_5_2_RCI_IRII_BRANCH:
 *
 * 1, Added logic to handle bothe composite and non composite left
 * triphone.  Composite left triphone's logic (the original one) is
 * tested thoroughly. The non-composite triphone (or full triphone) is
 * found to have bugs.  The latter is fended off from the users in the
 * library level.
 *
 * 2, Fixed dox-doc.
 *
 * Revision 1.8.4.5  2005/11/17 06:13:49  arthchan2003
 * Use compressed right context in expansion in triphones.
 *
 * Revision 1.8.4.4  2005/10/17 04:48:45  arthchan2003
 * Free resource correctly in dict2pid.
 *
 * Revision 1.8.4.3  2005/10/07 19:03:38  arthchan2003
 * Added xwdssid_t structure.  Also added compression routines.
 *
 * Revision 1.8.4.2  2005/09/25 19:13:31  arthchan2003
 * Added optional full triphone expansion support when building context phone mapping.
 *
 * Revision 1.8.4.1  2005/07/17 05:20:30  arthchan2003
 * Fixed dox-doc.
 *
 * Revision 1.8  2005/06/21 21:03:49  arthchan2003
 * 1, Introduced a reporting routine. 2, Fixed doyxgen documentation, 3, Added  keyword.
 *
 * Revision 1.5  2005/06/13 04:02:57  archan
 * Fixed most doxygen-style documentation under libs3decoder.
 *
 * Revision 1.4  2005/04/21 23:50:26  archan
 * Some more refactoring on the how reporting of structures inside kbcore_t is done, it is now 50% nice. Also added class-based LM test case into test-decode.sh.in.  At this moment, everything in search mode 5 is already done.  It is time to test the idea whether the search can really be used.
 *
 * Revision 1.3  2005/03/30 01:22:46  archan
 * Fixed mistakes in last updates. Add
 *
 * 
 * 14-Sep-1999	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Added dict2pid_comsseq2sen_active().
 * 
 * 04-May-1999	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Started.
 */


#ifndef _S3_DICT2PID_H_
#define _S3_DICT2PID_H_


#include <stdio.h>

#include <logmath.h>
#include "s3types.h"
#include "mdef.h"
#include "dict.h"
#include "ctxt_table.h"

/** \file dict2pid.h
 * \brief Building triphones for a dictionary. 
 *
 * This is one of the more complicated parts of a cross-word
 * triphone model decoder.  The first and last phones of each word
 * get their left and right contexts, respectively, from other
 * words.  For single-phone words, both its contexts are from other
 * words, simultaneously.  As these words are not known beforehand,
 * life gets complicated.  In this implementation, when we do not
 * wish to distinguish between distinct contexts, we use a COMPOSITE
 * triphone (a bit like BBN's fast-match implementation), by
 * clubbing together all possible contexts.
 * 
 * There are 3 cases:
 *
 *   1. Internal phones, and boundary phones without any specific
 * context, in each word.  The boundary phones are modelled using
 * composite phones, internal ones using ordinary phones.
 *
 *   2. The first phone of a multi-phone word, for a specific
 *history (i.e., in a 2g/3g/4g...  tree) has known left and right
 *contexts.  The possible left contexts are limited to the possible
 *last phones of the history.  So it can be modelled separately,
 *efficiently, as an ordinary triphone.
 *
 *   3. The one phone in a single-phone word, for a specific history
 * (i.e., in a 2g/3g/4g...  tree) has a known left context, but
 * unknown right context.  It is modelled using a composite
 * triphone.  (Note that right contexts are always composite, left
 * contexts are composite only in the unigram tree.)
 * 
 * A composite triphone is formed as follows.  (NOTE: this assumes
 * that all CIphones/triphones have the same HMM topology,
 * specifically, no. of states.)  A composite triphone represents a
 * situation where either the left or the right context (or both)
 * for a given base phone is unknown.  That is, it represents the
 * set of all possible ordinary triphones derivable from * the
 * unkown context(s).  Let us call this set S.  It is modelled using
 * the same HMM topology * as the ordinary triphones, but with
 * COMPOSITE states.  A composite state (in a given position * in
 * the HMM state topology) is the set of states (senones) at that
 * position derived from S.
 * 
 * Actually, we generally deal with COMPOSITE SENONE-SEQUENCES
 * rather than COMPOSITE PHONES.  The former are compressed forms of
 * the latter, by virtue of state sharing among phones.  (See
 * mdef.h.)
 * 
 * In 3.6, the composite triphone will only be build when -composite
 * 1 (default) is specified.  Other than that, full triphone
 * expansion will be carried out in run-time
 */

#ifdef __cplusplus
extern "C" {
#endif
#if 0
} /* Fool Emacs into not indenting things. */
#endif

/**
   \struct dict2pid_t
   \brief Building composite triphone (as well as word internal triphones) with the dictionary. 
*/

typedef struct {
    s3ssid_t **internal;	/**< For internal phone positions (not first, not last), the
				   ssid; for first and last positions, the composite ssid.
				   ([word][phone-position]) 
				   if -composite is 0, then internal[0] and internal[pronlen-1] will
				   equal to BAD_SSID;
				*/

    /*Notice the order of the arguments */

    s3ssid_t ***ldiph_lc;	/**< For multi-phone words, [base][rc][lc] -> ssid; filled out for
				   word-initial base x rc combinations in current vocabulary */


    s3ssid_t ***rdiph_rc;	/**< For multi-phone words, [base][lc][rc] -> ssid; filled out for
				   word-initial base x lc combinations in current vocabulary */

    xwdssid_t **rssid;          /**< Right context state sequence id table 
                                   First dimension: base phone,
                                   Second dimension: left context. 
                                */


    s3ssid_t ***lrdiph_rc;      /**< For single-phone words, [base][lc][rc] -> ssid; filled out for
                                   word-initial base x lc combinations in current vocabulary */

    xwdssid_t **lrssid;          /**< Left-Right context state sequence id table 
                                    First dimension: base phone,
                                    Second dimension: left context. 

                                 */


    int32 is_composite;         /**< Whether we will build composite triphone. If yes, the 
                                   structure will be in composite triphone mode, single_lc, 
                                   comstate, comsseq and comwt will be initialized. Otherwise, the code
                                   will be in normal triphone mode.  The parameters will be left NULL. 
                                */

    s3ssid_t **single_lc;	/**< For single phone words, [base][lc] -> composite ssid; filled
				   out for single phone words in current vocabulary */
    
    s3senid_t **comstate;	/**< comstate[i] = BAD_S3SENID terminated set of senone IDs in
				   the i-th composite state */
    s3senid_t **comsseq;	/**< comsseq[i] = sequence of composite state IDs in i-th
				   composite phone (composite sseq). */
    int32 *comwt;		/**< Weight associated with each composite state (logs3 value).
				   Final composite state score weighted by this amount */
    int32 n_comstate;		/**< #Composite states */
    int32 n_comsseq;		/**< #Composite senone sequences */
    int32 n_ci;   /**< Number of CI phone in */
    int32 n_dictsize; /**< Dictionary size */

} dict2pid_t;

/** Access macros; not designed for arbitrary use */
#define dict2pid_internal(d,w,p)	((d)->internal[w][p]) /**< return internal dict2pid*/
#define dict2pid_n_comstate(d)		((d)->n_comstate)     /**< return number of composite state*/
#define dict2pid_n_comsseq(d)		((d)->n_comsseq)      /**< return number of composite state sequence*/
#define dict2pid_is_composite(d)	((d)->is_composite)      /**< return whether dict2pid is in composite triphone mode or not*/

#define IS_COMPOSITE 1
#define NOT_COMPOSITE 0

/** Build the dict2pid structure for the given model/dictionary */
dict2pid_t *dict2pid_build (mdef_t *mdef,  /**< A  model definition*/
			    dict_t *dict,   /**< An initialized dictionary */
			    int32 is_composite, /**< Whether composite triphones will be built */
			    logmath_t *logmath
    );

  
/** Free the memory dict2pid structure */
void dict2pid_free(dict2pid_t *d2p /**< In: the d2p */
    );
/**
 * Compute composite senone scores from ordinary senone scores (max of component senones)
 */
void dict2pid_comsenscr (dict2pid_t *d2p,        /**< In: a dict2pid_t structure */
			 int32 *senscr,		/**< In: Ordinary senone scores */
			 int32 *comsenscr	/**< Out: Composite senone scores */
    );

/** 
 * Mark active senones as indicated by the input array of composite senone-sequence active flags.
 * Caller responsible for allocating and clearing sen[] before calling this function.
 */
void dict2pid_comsseq2sen_active (dict2pid_t *d2p,      /**< In: a dict2pid_t structure */
				  mdef_t *mdef,         /**< In: a mdef_t structure */
				  uint8 *comssid,	/**< In: Active flag for each comssid */
				  uint8 *sen		/**< In/Out: Active flags set for senones
							   indicated by the active comssid */
    );
/** For debugging */
void dict2pid_dump (FILE *fp,        /**< In: a file pointer */
		    dict2pid_t *d2p, /**< In: a dict2pid_t structure */
		    mdef_t *mdef,    /**< In: a mdef_t structure*/
		    dict_t *dict     /**< In: a dictionary structure */
    );

/** Report a dict2pid data structure */
void dict2pid_report(dict2pid_t *d2p /**< In: a dict2pid_t structure */
    );

/**
   Get number of rc 
*/
int32 get_rc_nssid(dict2pid_t *d2p,  /**< In: a dict2pid */
		   s3wid_t w,        /**< In: a wid */
		   dict_t *dict      /**< In: a dictionary */
    );

/**
   Get RC map 
*/
s3cipid_t* dict2pid_get_rcmap(dict2pid_t *d2p,  /**< In: a dict2pid */
			      s3wid_t w,        /**< In: a wid */
			      dict_t *dict      /**< In: a dictionary */
    );

#if 0
{ /* Stop indent from complaining */
#endif
#ifdef __cplusplus
}
#endif


#endif
