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
 * senone.c -- Mixture density weights associated with each tied state.
 *
 * **********************************************
 * CMU ARPA Speech Project
 *
 * Copyright (c) 1996 Carnegie Mellon University.
 * ALL RIGHTS RESERVED.
 * **********************************************
 * 
 * HISTORY
 * 
 * $Log$
 * Revision 1.6  2006/02/22  17:27:39  arthchan2003
 * Merged from SPHINX3_5_2_RCI_IRII_BRANCH: 1, NOT doing truncation in the multi-stream GMM computation \n. 2, Added .s3cont. to be the alias of the old multi-stream GMM computation routine \n. 3, Added license \n.  4, Fixed dox-doc. \n
 * 
 * Revision 1.5.4.5  2006/01/17 22:57:06  arthchan2003
 * Add directive TRUNCATE_LOGPDF in ms_senone.c
 *
 * Revision 1.5.4.4  2006/01/16 19:47:05  arthchan2003
 * Removed the truncation of senone probability code.
 *
 * Revision 1.5.4.3  2005/08/03 18:53:43  dhdfu
 * Add memory deallocation functions.  Also move all the initialization
 * of ms_mgau_model_t into ms_mgau_init (duh!), which entails removing it
 * from decode_anytopo and friends.
 *
 * Revision 1.5.4.2  2005/08/02 21:06:33  arthchan2003
 * Change options such that .s3cont. works as well.
 *
 * Revision 1.5.4.1  2005/07/20 19:39:01  arthchan2003
 * Added licences in ms_* series of code.
 *
 * Revision 1.5  2005/06/21 18:57:31  arthchan2003
 * 1, Fixed doxygen documentation. 2, Added $ keyword.
 *
 * Revision 1.1.1.1  2005/03/24 15:24:00  archan
 * I found Evandro's suggestion is quite right after yelling at him 2 days later. So I decide to check this in again without any binaries. (I have done make distcheck. ) . Again, this is a candidate for s3.6 and I believe I need to work out 4-5 intermediate steps before I can complete the first prototype.  That's why I keep local copies. 
 *
 * Revision 1.4  2004/12/05 12:01:31  arthchan2003
 * 1, move libutil/libutil.h to s3types.h, seems to me not very nice to have it in every files. 2, Remove warning messages of main_align.c 3, Remove warning messages in chgCase.c
 *
 * Revision 1.3  2004/11/13 21:25:19  arthchan2003
 * commit of 1, absolute CI-GMMS , 2, fast CI senone computation using svq, 3, Decrease the number of static variables, 4, fixing the random generator problem of vector_vqgen, 5, move all unused files to NOTUSED
 *
 * Revision 1.2  2004/08/09 01:02:33  arthchan2003
 * check in the windows setup for align
 *
 * Revision 1.1  2004/08/09 00:17:11  arthchan2003
 * Incorporating s3.0 align, at this point, there are still some small problems in align but they don't hurt. For example, the score doesn't match with s3.0 and the output will have problem if files are piped to /dev/null/. I think we can go for it.
 *
 * Revision 1.1  2003/02/14 14:40:34  cbq
 * Compiles.  Analysis is probably hosed.
 *
 * Revision 1.1  2000/04/24 09:39:41  lenzo
 * s3 import.
 *
 * 
 * 06-Mar-97	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University.
 * 		Added handling of .semi. and .cont. special cases for senone-mgau
 * 		mapping.
 * 
 * 20-Dec-96	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University.
 * 		Changed senone_mixw_read and senone_mgau_map_read to use the new
 *		libio/bio_fread functions.
 * 
 * 20-Jan-96	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University.
 * 		Modified senone_eval to accommodate both normal and transposed
 *		senone organization.
 * 
 * 13-Dec-95	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University.
 * 		Added implementation of senone_mgau_map_read.
 * 		Added senone_eval_all() optimized for the semicontinuous case.
 * 
 * 12-Nov-95	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University.
 * 		Created.
 */

#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <bio.h>

#include "ms_senone.h"
#include "log.h"


#define MIXW_PARAM_VERSION	"1.0"
#define SPDEF_PARAM_VERSION	"1.2"


static int32
senone_mgau_map_read(senone_t * s, char *file_name)
{
    FILE *fp;
    int32 byteswap, chksum_present, n_gauden_present;
    uint32 chksum;
    int32 i;
    char eofchk;
    char **argname, **argval;
    float32 v;

    E_INFO("Reading senone gauden-codebook map file: %s\n", file_name);

    if ((fp = fopen(file_name, "rb")) == NULL)
        E_FATAL_SYSTEM("fopen(%s,rb) failed\n", file_name);

    /* Read header, including argument-value info and 32-bit byteorder magic */
    if (bio_readhdr(fp, &argname, &argval, &byteswap) < 0)
        E_FATAL("bio_readhdr(%s) failed\n", file_name);

    /* Parse argument-value list */
    chksum_present = 0;
    n_gauden_present = 0;
    for (i = 0; argname[i]; i++) {
        if (strcmp(argname[i], "version") == 0) {
            if (strcmp(argval[i], SPDEF_PARAM_VERSION) != 0) {
                E_WARN("Version mismatch(%s): %s, expecting %s\n",
                       file_name, argval[i], SPDEF_PARAM_VERSION);
            }

            /* HACK!! Convert version# to float32 and take appropriate action */
            if (sscanf(argval[i], "%f", &v) != 1)
                E_FATAL("%s: Bad version no. string: %s\n", file_name,
                        argval[i]);

            n_gauden_present = (v > 1.1) ? 1 : 0;
        }
        else if (strcmp(argname[i], "chksum0") == 0) {
            chksum_present = 1; /* Ignore the associated value */
        }
    }
    bio_hdrarg_free(argname, argval);
    argname = argval = NULL;

    chksum = 0;

    /* Read #gauden (if version matches) */
    if (n_gauden_present) {
        if (bio_fread
            (&(s->n_gauden), sizeof(int32), 1, fp, byteswap, &chksum) != 1)
            E_FATAL("fread(%s) (#gauden) failed\n", file_name);
    }

    /* Read 1d array data */
    if (bio_fread_1d
        ((void **) (&s->mgau), sizeof(s3mgauid_t), &(s->n_sen), fp,
         byteswap, &chksum) < 0) {
        E_FATAL("bio_fread_1d(%s) failed\n", file_name);
    }

    /* Infer n_gauden if not present in this version */
    if (!n_gauden_present) {
        s->n_gauden = 1;
        for (i = 0; i < s->n_sen; i++)
            if (s->mgau[i] >= s->n_gauden)
                s->n_gauden = s->mgau[i] + 1;
    }

    if (chksum_present)
        bio_verify_chksum(fp, byteswap, chksum);

    if (fread(&eofchk, 1, 1, fp) == 1)
        E_FATAL("More data than expected in %s\n", file_name);

    fclose(fp);

    E_INFO("Read %d->%d senone-codebook mappings\n", s->n_sen,
           s->n_gauden);

    return 1;
}


static int32
senone_mixw_read(senone_t * s, char *file_name)
{
    char eofchk;
    FILE *fp;
    int32 byteswap, chksum_present;
    uint32 chksum;
    float32 *pdf;
    int32 i, f, c, p, n_err;
    char **argname, **argval;

    E_INFO("Reading senone mixture weights: %s\n", file_name);

    if ((fp = fopen(file_name, "rb")) == NULL)
        E_FATAL_SYSTEM("fopen(%s,rb) failed\n", file_name);

    /* Read header, including argument-value info and 32-bit byteorder magic */
    if (bio_readhdr(fp, &argname, &argval, &byteswap) < 0)
        E_FATAL("bio_readhdr(%s) failed\n", file_name);

    /* Parse argument-value list */
    chksum_present = 0;
    for (i = 0; argname[i]; i++) {
        if (strcmp(argname[i], "version") == 0) {
            if (strcmp(argval[i], MIXW_PARAM_VERSION) != 0)
                E_WARN("Version mismatch(%s): %s, expecting %s\n",
                       file_name, argval[i], MIXW_PARAM_VERSION);
        }
        else if (strcmp(argname[i], "chksum0") == 0) {
            chksum_present = 1; /* Ignore the associated value */
        }
    }
    bio_hdrarg_free(argname, argval);
    argname = argval = NULL;

    chksum = 0;

    /* Read #senones, #features, #codewords, arraysize */
    if ((bio_fread(&(s->n_sen), sizeof(int32), 1, fp, byteswap, &chksum) !=
         1)
        ||
        (bio_fread(&(s->n_feat), sizeof(int32), 1, fp, byteswap, &chksum)
         != 1)
        || (bio_fread(&(s->n_cw), sizeof(int32), 1, fp, byteswap, &chksum)
            != 1)
        || (bio_fread(&i, sizeof(int32), 1, fp, byteswap, &chksum) != 1)) {
        E_FATAL("bio_fread(%s) (arraysize) failed\n", file_name);
    }
    if (i != s->n_sen * s->n_feat * s->n_cw) {
        E_FATAL
            ("%s: #float32s(%d) doesn't match dimensions: %d x %d x %d\n",
             file_name, i, s->n_sen, s->n_feat, s->n_cw);
    }

    /*
     * Compute #LSB bits to be dropped to represent mixwfloor with 8 bits.
     * All PDF values will be truncated (in the LSB positions) by these many bits.
     */
    if ((s->mixwfloor <= 0.0) || (s->mixwfloor >= 1.0))
        E_FATAL("mixwfloor (%e) not in range (0, 1)\n", s->mixwfloor);

    p = logmath_log(lmath, s->mixwfloor);

#if TRUNCATE_LOGPDF
    for (s->shift = 0, p = -p; p >= 256; s->shift++, p >>= 1);
    E_INFO("Truncating senone logs3(pdf) values by %d bits, to 8 bits\n",
           s->shift);
#endif

    /*
     * Allocate memory for senone PDF data.  Organize normally or transposed depending on
     * s->n_gauden.
     */
    if (s->n_gauden > 1) {
        s->pdf =
            (senprob_t ***) ckd_calloc_3d(s->n_sen, s->n_feat, s->n_cw,
                                          sizeof(senprob_t));
    }
    else {
        s->pdf =
            (senprob_t ***) ckd_calloc_3d(s->n_feat, s->n_cw, s->n_sen,
                                          sizeof(senprob_t));
    }

    /* Temporary structure to read in floats */
    pdf = (float32 *) ckd_calloc(s->n_cw, sizeof(float32));

    /* Read senone probs data, normalize, floor, convert to logs3, truncate to 8 bits */
    n_err = 0;
    for (i = 0; i < s->n_sen; i++) {
        for (f = 0; f < s->n_feat; f++) {
            if (bio_fread
                ((void *) pdf, sizeof(float32), s->n_cw, fp, byteswap,
                 &chksum)
                != s->n_cw) {
                E_FATAL("bio_fread(%s) (arraydata) failed\n", file_name);
            }

            /* Normalize and floor */
            if (vector_sum_norm(pdf, s->n_cw) <= 0.0)
                n_err++;
            vector_floor(pdf, s->n_cw, s->mixwfloor);
            vector_sum_norm(pdf, s->n_cw);

            /* Convert to logs3, truncate to 8 bits, and store in s->pdf */
            for (c = 0; c < s->n_cw; c++) {
                p = -(logmath_log(lmath, pdf[c]));

#if TRUNCATE_LOGPDF
                p += (1 << (s->shift - 1)) - 1; /* Rounding before truncation */

                if (s->n_gauden > 1)
                    s->pdf[i][f][c] =
                        (p < (255 << s->shift)) ? (p >> s->shift) : 255;
                else
                    s->pdf[f][c][i] =
                        (p < (255 << s->shift)) ? (p >> s->shift) : 255;

#else
                if (s->n_gauden > 1)
                    s->pdf[i][f][c] = p;
                else
                    s->pdf[f][c][i] = p;
#endif
            }
        }
    }
    if (n_err > 0)
        E_ERROR("Weight normalization failed for %d senones\n", n_err);

    ckd_free(pdf);

    if (chksum_present)
        bio_verify_chksum(fp, byteswap, chksum);

    if (fread(&eofchk, 1, 1, fp) == 1)
        E_FATAL("More data than expected in %s\n", file_name);

    fclose(fp);

    E_INFO
        ("Read mixture weights for %d senones: %d features x %d codewords\n",
         s->n_sen, s->n_feat, s->n_cw);

    return 1;
}


senone_t *
senone_init(gauden_t *g, char *mixwfile, char *sen2mgau_map_file, float32 mixwfloor)
{
    senone_t *s;
    int32 n = 0, i;

    s = (senone_t *) ckd_calloc(1, sizeof(senone_t));
    s->mixwfloor = mixwfloor;

    s->n_gauden = g->n_mgau;
    if (sen2mgau_map_file) {
	if (!(strcmp(sen2mgau_map_file, ".semi.") == 0
	      || strcmp(sen2mgau_map_file, ".cont.") == 0)) {
	    senone_mgau_map_read(s, sen2mgau_map_file);
	    n = s->n_sen;
	}
    }
    else {
	if (s->n_gauden == 1)
	    sen2mgau_map_file = ".semi.";
	else
	    sen2mgau_map_file = ".cont.";
    }

    senone_mixw_read(s, mixwfile);

    if (strcmp(sen2mgau_map_file, ".semi.") == 0) {
        /* All-to-1 senones-codebook mapping */
        s->mgau = (s3mgauid_t *) ckd_calloc(s->n_sen, sizeof(s3mgauid_t));
    }
    else if (strcmp(sen2mgau_map_file, ".cont.") == 0
             || strcmp(sen2mgau_map_file, ".s3cont.") == 0) {
        /* 1-to-1 senone-codebook mapping */
        if (s->n_sen <= 1)
            E_FATAL("#senone=%d; must be >1\n", s->n_sen);

        s->mgau = (s3mgauid_t *) ckd_calloc(s->n_sen, sizeof(s3mgauid_t));
        for (i = 0; i < s->n_sen; i++)
            s->mgau[i] = i;

        s->n_gauden = s->n_sen;
    }
    else {
        if (s->n_sen != n)
            E_FATAL("#senones inconsistent: %d in %s; %d in %s\n",
                    n, sen2mgau_map_file, s->n_sen, mixwfile);
    }

    s->featscr = NULL;
    return s;
}

void
senone_free(senone_t * s)
{
    if (s == NULL)
        return;
    if (s->pdf)
        ckd_free_3d((void *) s->pdf);
    if (s->mgau)
        ckd_free(s->mgau);
    if (s->featscr)
        ckd_free(s->featscr);
    ckd_free(s);
}


/*
 * Compute senone score for one senone.
 * NOTE:  Remember that senone PDF tables contain SCALED logs3 values.
 * NOTE:  Remember also that PDF data may be transposed or not depending on s->n_gauden.
 */
int32
senone_eval(senone_t * s, s3senid_t id, gauden_dist_t ** dist, int32 n_top)
{
    int32 scr;                  /* total senone score */
    int32 fscr;                 /* senone score for one feature */
    int32 fwscr;                /* senone score for one feature, one codeword */
    int32 f, t;
    gauden_dist_t *fdist;

    assert((id >= 0) && (id < s->n_sen));
    assert((n_top > 0) && (n_top <= s->n_cw));

    scr = 0;

    for (f = 0; f < s->n_feat; f++) {
        fdist = dist[f];

        /* Top codeword for feature f */
#if TRUNCATE_LOGPDF
        fscr = (s->n_gauden > 1) ? fdist[0].dist - (s->pdf[id][f][fdist[0].id] << s->shift) :   /* untransposed */
            fdist[0].dist - (s->pdf[f][fdist[0].id][id] << s->shift);   /* transposed */
#else

        fscr = (s->n_gauden > 1) ? fdist[0].dist - (s->pdf[id][f][fdist[0].id]) :       /* untransposed */
            fdist[0].dist - (s->pdf[f][fdist[0].id][id]);       /* transposed */
#endif

        /* Remaining of n_top codewords for feature f */
        for (t = 1; t < n_top; t++) {
#if TRUNCATE_LOGPDF
            fwscr = (s->n_gauden > 1) ?
                fdist[t].dist - (s->pdf[id][f][fdist[t].id] << s->shift) :
                fdist[t].dist - (s->pdf[f][fdist[t].id][id] << s->shift);
#else

            fwscr = (s->n_gauden > 1) ?
                fdist[t].dist - (s->pdf[id][f][fdist[t].id]) :
                fdist[t].dist - (s->pdf[f][fdist[t].id][id]);
#endif

            fscr = logmath_add(lmath, fscr, fwscr);

        }

        scr += fscr;

    }

    return scr;
}


/*
 * Optimized for special case of all senones sharing one codebook (perhaps many features).
 * In particular, the PDF tables are transposed in memory.
 */
void
senone_eval_all(senone_t * s, gauden_dist_t ** dist, int32 n_top,
                int32 * senscr)
{
    int32 i, f, k, cwdist, scr;

    senprob_t *pdf;
    int32 *featscr = NULL;
    featscr = s->featscr;

    assert(s->n_gauden == 1);
    assert((n_top > 0) && (n_top <= s->n_cw));

    if ((s->n_feat > 1) && (!featscr))
        featscr = (int32 *) ckd_calloc(s->n_sen, sizeof(int32));

    /* Feature 0 */
    /* Top-N codeword 0 */
    cwdist = dist[0][0].dist;
    pdf = s->pdf[0][dist[0][0].id];

#if TRUNCATE_LOGPDF
    for (i = 0; i < s->n_sen; i++)
        senscr[i] = cwdist - (pdf[i] << s->shift);
#else
    for (i = 0; i < s->n_sen; i++)
        senscr[i] = cwdist - (pdf[i]);

#endif

    /* Remaining top-N codewords */
    for (k = 1; k < n_top; k++) {
        cwdist = dist[0][k].dist;
        pdf = s->pdf[0][dist[0][k].id];

        for (i = 0; i < s->n_sen; i++) {
#if TRUNCATE_LOGPDF
            scr = cwdist - (pdf[i] << s->shift);
#else
            scr = cwdist - (pdf[i]);
#endif
            senscr[i] = logmath_add(lmath, senscr[i], scr);
        }
    }

    /* Remaining features */
    for (f = 1; f < s->n_feat; f++) {
        /* Top-N codeword 0 */
        cwdist = dist[f][0].dist;
        pdf = s->pdf[f][dist[f][0].id];

#if TRUNCATE_LOGPDF
        for (i = 0; i < s->n_sen; i++)
            featscr[i] = cwdist - (pdf[i] << s->shift);
#else
        for (i = 0; i < s->n_sen; i++)
            featscr[i] = cwdist - (pdf[i]);
#endif

        /* Remaining top-N codewords */
        for (k = 1; k < n_top; k++) {
            cwdist = dist[f][k].dist;
            pdf = s->pdf[f][dist[f][k].id];

            for (i = 0; i < s->n_sen; i++) {
#if TRUNCATE_LOGPDF
                scr = cwdist - (pdf[i] << s->shift);
#else
                scr = cwdist - (pdf[i]);
#endif
                featscr[i] = logmath_add(lmath, featscr[i], scr);
            }
        }

        for (i = 0; i < s->n_sen; i++)
            senscr[i] += featscr[i];
    }
}
