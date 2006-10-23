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
 *------------------------------------------------------------*
 * DESCRIPTION
 *	Read tied distribution hmms on behalf of fbs.
 *------------------------------------------------------------*
 * HISTORY
 *
 * 14-Apr-94  M K Ravishankar (rkm@cs) at Carnegie-Mellon University
 * 	Added map-file dump and load facility for faster startup.
 * 
 *  2-Nov-91  Fil Alleva (faa) at Carnegie-Mellon University
 *	Massive changes for fbs.
 *
 * 16-Mar-87 Hsiao-wuen Hon (Hon)  at Carnegie Mellon University.
 *	Initial version.
 */

#include <stdio.h>
#include <assert.h>
#include "s2types.h"
#include "kb.h"
#include "basic_types.h"
#include "msd.h"
#include "hmm_tied_r.h"
#include "bin_mdef.h"

void
remap_mdef(SMD * smdV, bin_mdef_t * mdef)
{
    int32 i, j;

    for (i = 0; i < bin_mdef_n_sseq(mdef); i++) {
        /* This looks like a hack, but it really isn't, because the
         * distribution IDs in the mdef are global rather than
         * per-CIphone, so they will always be sequential within a
         * senone sequence.  */
        for (j = 0; j < TRANS_CNT; j++)
            smdV[i].senone[j] = bin_mdef_sseq2sen(mdef, i, j / 3);
    }
}

/* Call this only after entire kb loaded */
void
hmm_tied_r_dumpssidlist()
{
    FILE *dumpfp;
    int32 i, j, numSSeq;

    numSSeq = bin_mdef_n_sseq(mdef);
    if ((dumpfp = fopen("ssid_list.txt", "w")) != NULL) {
        for (i = 0; i < numSSeq; i++) {
            fprintf(dumpfp, "%6d\t", i);
            for (j = 0; j < 5; j++)
                fprintf(dumpfp, " %5d", smds[i].senone[j * 3]);
            fprintf(dumpfp, "\n");
        }
    }
    fclose(dumpfp);
}

int32
hmm_pid2sid(int32 pid)
/*------------------------------------------------------------*
 * Convert a phone id to a senone sequence id\
 */
{
    return bin_mdef_pid2ssid(mdef, pid);
}
