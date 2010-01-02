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
 * fillpen.c -- Filler penalties (penalties for words that do not show up in
 * the main LM.
 * 
 * **********************************************
 * CMU ARPA Speech Project
 *
 * Copyright (c) 1996 Carnegie Mellon University.
 * ALL RIGHTS RESERVED.
 * **********************************************
 * 
 * HISTORY
 * $Log$
 * Revision 1.6  2006/02/23  04:11:13  arthchan2003
 * Merged from branch SPHINX3_5_2_RCI_IRII_BRANCH: Added silprob and fillprob. Added fillpen_report.
 * 
 * Revision 1.5.4.1  2005/06/28 06:59:04  arthchan2003
 * Add silence probability and filler probability as members of fillpen_t, add reporting functions.
 *
 * Revision 1.5  2005/06/21 21:09:22  arthchan2003
 * 1, Fixed doxygen documentation. 2, Added  keyword.
 *
 * Revision 1.3  2005/03/30 01:22:46  archan
 * Fixed mistakes in last updates. Add
 *
 * 20-Apr-2001  Ricky Houghton (ricky.houghton@cs.cmu.edu or rhoughton@mediasite.com)
 *              Added fillpen_free to free memory allocated by fillpen_init
 * 
 * 30-Dec-2000  Rita Singh (rsingh@cs.cmu.edu) at Carnegie Mellon University
 *		Removed language weight application to wip. To maintain
 *		comparability between s3decode and current decoder. Does
 *		not affect decoding performance.
 *
 * 24-Feb-2000	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Bugfix: Applied language weight to word insertion penalty.
 * 
 * 11-Oct-96	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Created.
 */


#include "fillpen.h"

fillpen_t *
fillpen_init(dict_t * dict, const char *file, float64 silprob, float64 fillprob,
             float64 lw, float64 wip, logmath_t *logmath)
{
    s3wid_t w, bw;
    float64 prob;
    FILE *fp;
    char line[1024], wd[1024];
    int32 k;
    fillpen_t *_fillpen;

    _fillpen = (fillpen_t *) ckd_calloc(1, sizeof(fillpen_t));

    _fillpen->dict = dict;
    _fillpen->lw = lw;
    _fillpen->wip = wip;
    _fillpen->silprob = silprob;
    _fillpen->fillerprob = fillprob;
    if (dict->filler_end >= dict->filler_start)
        _fillpen->prob =
            (int32 *) ckd_calloc(dict->filler_end - dict->filler_start + 1,
                                 sizeof(int32));
    else
        _fillpen->prob = NULL;

    /* Initialize all words with filler penalty (HACK!! backward compatibility) */
    prob = fillprob;
    for (w = dict->filler_start; w <= dict->filler_end; w++)
    _fillpen->prob[w - dict->filler_start] =
        (int32) ((logmath_log(logmath, prob) * lw + logmath_log(logmath, wip)));

    /* Overwrite silence penalty (HACK!! backward compatibility) */
    w = dict_wordid(dict, S3_SILENCE_WORD);
    if (NOT_S3WID(w) || (w < dict->filler_start) || (w > dict->filler_end))
        E_FATAL("%s not a filler word in the given dictionary\n",
                S3_SILENCE_WORD);
    prob = silprob;
    _fillpen->prob[w - dict->filler_start] =
        (int32) ((logmath_log(logmath, prob) * lw + logmath_log(logmath, wip)));

    /* Overwrite with filler prob input file, if specified */
    if (!file)
        return _fillpen;

    E_INFO("Reading filler penalty file: %s\n", file);
    if ((fp = fopen(file, "r")) == NULL)
        E_FATAL("fopen(%s,r) failed\n", file);
    while (fgets(line, sizeof(line), fp) != NULL) {
        if (line[0] == '#')     /* Skip comment lines */
            continue;

        k = sscanf(line, "%s %lf", wd, &prob);
        if ((k != 0) && (k != 2))
            E_FATAL("Bad input line: %s\n", line);
        w = dict_wordid(dict, wd);
        if (NOT_S3WID(w) || (w < dict->filler_start)
            || (w > dict->filler_end))
            E_FATAL("%s not a filler word in the given dictionary\n",
                    S3_SILENCE_WORD);

        _fillpen->prob[w - dict->filler_start] =
            (int32) ((logmath_log(logmath, prob) * lw + logmath_log(logmath, wip)));
    }
    fclose(fp);

    /* Replicate fillpen values for alternative pronunciations */
    for (w = dict->filler_start; w <= dict->filler_end; w++) {
        bw = dict_basewid(dict, w);
        if (bw != w)
            _fillpen->prob[w - dict->filler_start] =
                _fillpen->prob[bw - dict->filler_start];
    }

    return _fillpen;
}

void
fillpen_report(fillpen_t * f)
{
    E_INFO_NOFN("Initialization of fillpen_t, report:\n");
    E_INFO_NOFN("Language weight =%f \n", f->lw);
    E_INFO_NOFN("Word Insertion Penalty =%f \n", f->wip);
    E_INFO_NOFN("Silence probability =%f \n", f->silprob);
    E_INFO_NOFN("Filler probability =%f \n", f->fillerprob);
    E_INFO_NOFN("\n");

}

int32
fillpen(fillpen_t * f, s3wid_t w)
{
    assert((w >= f->dict->filler_start) && (w <= f->dict->filler_end));
    return (f->prob[w - f->dict->filler_start]);
}


/* RAH, free memory allocated above */
void
fillpen_free(fillpen_t * f)
{
    if (f) {
        if (f->prob)
            ckd_free((void *) f->prob);
        ckd_free((void *) f);
    }
}
