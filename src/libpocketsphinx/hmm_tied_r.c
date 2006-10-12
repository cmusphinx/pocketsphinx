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
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef WIN32
#include <fcntl.h>
#else
#include <sys/file.h>
#endif

#include "s2types.h"
#include "basic_types.h"
#include "list.h"
#include "phone.h"
#include "search_const.h"
#include "msd.h"
#include "magic.h"
#include "log.h"
#include "cviterbi4.h"
#include "smmap4f.h"
#include "dict.h"
#include "lmclass.h"
#include "lm_3g.h"
#include "kb.h"
#include "hmm_tied_r.h"
#include "err.h"
#include "s2io.h"
#include "byteorder.h"
#include "bio.h"
#include "ckd_alloc.h"
#include "vector.h"

#define QUIT(x)		{fprintf x; exit(-1);}

#define MGAU_MIXW_VERSION	"1.0"   /* Sphinx-3 file format version for mixw */
#define MAX_ALPHABET		256

#define HAS_ARC(fs,ts,S)	S->topo[((fs * S->stateCnt) + ts) / 32] & \
				(1 << ((fs * S->stateCnt) + ts) % 32)

int32 totalDists;               /* Total number of distributions */
int32 *numDists;                /* Number of senones for each of the CI
                                 * and WD base phone units.
                                 */
int32 *numDPDists;              /* Number of DiPhone senones. These are
                                 * generated from the appropriate cd units.
                                 */
static int32 **distMap;         /* Distribution map
                                 */
static int32 *ssIdMap;          /* Senone sequence Id Map.
                                 * ssIdMap[phone_id] == the senone sequence id.
                                 */
static int32 numSSeq = 0;       /* Number of unique senone sequences
                                 */

void
remap(SMD * smdV)
{                               /* smd pointer vector */

/*------------------------------------------------------------*
 * DESCRIPTION
 *	Remap the distributions in the smd's.
 */
    int32 i, j;

    for (i = 0; i < numSSeq; i++) {
        for (j = 0; j < TRANS_CNT; j++) {
            smdV[i].dist[j] = distMap[i][smdV[i].dist[j]];
        }
    }
    free(distMap);
}

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
            smdV[i].dist[j] = bin_mdef_sseq2sen(mdef, i, j / 3);
    }
    free(distMap);
}

typedef struct {
    int32 name;
    int32 idx;
} ARC;



/* Call this only after entire kb loaded */
void
hmm_tied_r_dumpssidlist()
{
    SMD *models;
    FILE *dumpfp;
    int32 i, j;

    models = kb_get_models();
    numSSeq = hmm_num_sseq();
    if ((dumpfp = fopen("ssid_list.txt", "w")) != NULL) {
        for (i = 0; i < numSSeq; i++) {
            fprintf(dumpfp, "%6d\t", i);
            for (j = 0; j < 5; j++)
                fprintf(dumpfp, " %5d", models[i].dist[j * 3]);
            fprintf(dumpfp, "\n");
        }
    }
    fclose(dumpfp);
}


#define MAX_MEMBERS 256

int32 sets[NUMDISTRTYPES][MAX_MEMBERS];
int32 set_size[NUMDISTRTYPES];

int32
hmm_num_sseq(void)
/*------------------------------------------------------------*
 * Return number of unique senone sequences.
 * If the number is 0 we call this a fatal error.
 */
{
    bin_mdef_t *mdef;

    if ((mdef = kb_mdef()) != NULL)
        return bin_mdef_n_sseq(mdef);

    if (numSSeq == 0) {
        E_FATAL("numSSeq (number of senone sequences is 0\n");
    }
    return numSSeq;
}

int32
hmm_pid2sid(int32 pid)
/*------------------------------------------------------------*
 * Convert a phone id to a senone sequence id\
 */
{
    bin_mdef_t *mdef;

    if ((mdef = kb_mdef()) != NULL)
        return bin_mdef_pid2ssid(mdef, pid);

    return ssIdMap[pid];
}


int32 *
hmm_get_psen(void)
{
    return numDists;
}
