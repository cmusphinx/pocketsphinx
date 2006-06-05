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
 * tmat.c
 *
 * **********************************************
 * CMU ARPA Speech Project
 *
 * Copyright (c) 1997 Carnegie Mellon University.
 * ALL RIGHTS RESERVED.
 * **********************************************
 * 
 * HISTORY
 * $Log: tmat.c,v $
 * Revision 1.1.1.1  2006/05/23 18:45:01  dhuggins
 * re-importation
 *
 * Revision 1.4  2005/11/14 16:14:34  dhuggins
 * Use LOG() instead of logs3() for loading tmats, makes startup
 * ***much*** faster.
 *
 * Revision 1.3  2005/10/10 14:50:35  dhuggins
 * Deal properly with empty transition matrices.
 *
 * Revision 1.2  2005/09/30 15:01:23  dhuggins
 * More robust tmat reading - read the tmat in accordance with the fixed s2 topology
 *
 * Revision 1.1  2005/09/29 21:51:19  dhuggins
 * Add support for Sphinx3 tmat files.  Amazingly enough, it Just Works
 * (but it isn't terribly robust)
 *
 * Revision 1.6  2005/07/05 13:12:39  dhdfu
 * Add new arguments to logs3_init() in some tests, main_ep
 *
 * Revision 1.5  2005/06/21 19:23:35  arthchan2003
 * 1, Fixed doxygen documentation. 2, Added $ keyword.
 *
 * Revision 1.5  2005/05/03 04:09:09  archan
 * Implemented the heart of word copy search. For every ci-phone, every word end, a tree will be allocated to preserve its pathscore.  This is different from 3.5 or below, only the best score for a particular ci-phone, regardless of the word-ends will be preserved at every frame.  The graph propagation will not collect unused word tree at this point. srch_WST_propagate_wd_lv2 is also as the most stupid in the century.  But well, after all, everything needs a start.  I will then really get the results from the search and see how it looks.
 *
 * Revision 1.4  2005/04/21 23:50:26  archan
 * Some more refactoring on the how reporting of structures inside kbcore_t is done, it is now 50% nice. Also added class-based LM test case into test-decode.sh.in.  At this moment, everything in search mode 5 is already done.  It is time to test the idea whether the search can really be used.
 *
 * Revision 1.3  2005/03/30 01:22:47  archan
 * Fixed mistakes in last updates. Add
 *
 * 
 * 20.Apr.2001  RAH (rhoughton@mediasite.com, ricky.houghton@cs.cmu.edu)
 *              Added tmat_free to free allocated memory 
 *
 * 29-Feb-2000	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University.
 * 		Added tmat_chk_1skip(), and made tmat_chk_uppertri() public.
 * 
 * 10-Dec-1999	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University.
 * 		Made tmat_dump() public.
 * 
 * 11-Mar-97	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University.
 * 		Started based on original S3 implementation.
 */

#include <string.h>

#include "tmat.h"
#include "bio.h"
#include "vector.h"
#include "log.h"
#include "err.h"
#include "ckd_alloc.h"

#define TMAT_PARAM_VERSION		"1.0"

int
tmat_init (char *file_name, SMD *smds, float64 tpfloor, int32 breport)
{
    char tmp;
    int32 n_src, n_dst;
    FILE *fp;
    int32 byteswap, chksum_present;
    uint32 chksum;
    float32 **tp;
    int32 i, j, k, n_tmat, n_state, tp_per_tmat;
    char **argname, **argval;
    

    if(breport){
      E_INFO("Reading HMM transition probability matrices: %s\n", file_name);
    }

    if ((fp = fopen(file_name, "rb")) == NULL)
	E_FATAL_SYSTEM("fopen(%s,rb) failed\n", file_name);
    
    /* Read header, including argument-value info and 32-bit byteorder magic */
    if (bio_readhdr (fp, &argname, &argval, &byteswap) < 0)
	E_FATAL("bio_readhdr(%s) failed\n", file_name);
    
    /* Parse argument-value list */
    chksum_present = 0;
    for (i = 0; argname[i]; i++) {
	if (strcmp (argname[i], "version") == 0) {
	    if (strcmp(argval[i], TMAT_PARAM_VERSION) != 0)
		E_WARN("Version mismatch(%s): %s, expecting %s\n",
			file_name, argval[i], TMAT_PARAM_VERSION);
	} else if (strcmp (argname[i], "chksum0") == 0) {
	    chksum_present = 1;	/* Ignore the associated value */
	}
    }
    bio_hdrarg_free (argname, argval);
    argname = argval = NULL;
    
    chksum = 0;
    
    /* Read #tmat, #from-states, #to-states, arraysize */
    if ((bio_fread (&n_tmat, sizeof(int32), 1, fp, byteswap, &chksum) != 1) ||
	(bio_fread (&n_src, sizeof(int32), 1, fp, byteswap, &chksum) != 1) ||
	(bio_fread (&n_dst, sizeof(int32), 1, fp, byteswap, &chksum) != 1) ||
	(bio_fread (&i, sizeof(int32), 1, fp, byteswap, &chksum) != 1)) {
	E_FATAL("bio_fread(%s) (arraysize) failed\n", file_name);
    }
    if (n_tmat >= MAX_S3TMATID)
	E_FATAL("%s: #tmat (%d) exceeds limit (%d)\n", file_name, n_tmat, MAX_S3TMATID);
    if (n_dst != n_src+1)
	E_FATAL("%s: #from-states(%d) != #to-states(%d)-1\n", file_name, n_src, n_dst);
    n_state = n_src;
    
    if (i != n_tmat * n_src * n_dst) {
	E_FATAL("%s: #float32s(%d) doesn't match dimensions: %d x %d x %d\n",
		file_name, i, n_tmat, n_src, n_dst);
    }

    if (n_src != HMM_LAST_STATE)
	E_FATAL("%s: #from-states(%d) is not %d\n", n_src, HMM_LAST_STATE);

    /* Temporary structure to read in the float data */
    tp = (float32 **) ckd_calloc_2d (n_src, n_dst, sizeof(float32));

    /* Read transition matrices, normalize and floor them, and convert to logs3 domain */
    tp_per_tmat = n_src * n_dst;
    for (i = 0; i < n_tmat; i++) {
	int arc;

	if (bio_fread (tp[0], sizeof(float32), tp_per_tmat, fp,
		       byteswap, &chksum) != tp_per_tmat) {
	    E_FATAL("fread(%s) (arraydata) failed\n", file_name);
	}
	
	/* Normalize and floor */
	arc = 0;
	for (j = 0; j < n_src; j++) {
	    if (vector_sum_norm (tp[j], n_dst) == 0.0)
		E_WARN("Normalization failed for tmat %d from state %d\n", i, j);
	    vector_nz_floor (tp[j], n_dst, tpfloor);
	    vector_sum_norm (tp[j], n_dst);

	    /* Convert to logs3.  We simply ignore the transitions
	     * that aren't allowed by the Sphinx2 topology. */
	    for (k = j; k < j + 3 && k < n_dst; k++) {
		if (arc >= TRANS_CNT)
		    E_FATAL("Number of arcs is greater than TRANS_CNT\n");
		smds[i].dist[arc] = j; /* Actually unused. */
		/* For these ones, we floor them even if they are
		 * zero, otherwise HMM evaluation goes nuts. */
		if (tp[j][k] == 0.0f)
			tp[j][k] = tpfloor;
		smds[i].tp[arc] = LOG(tp[j][k]);
		++arc;
	    }
	}
    }

    ckd_free_2d ((void **) tp);

    if (chksum_present)
	bio_verify_chksum (fp, byteswap, chksum);

    if (fread (&tmp, 1, 1, fp) == 1)
	E_ERROR("Non-empty file beyond end of data\n");

    fclose(fp);
    
    return 0;
}
