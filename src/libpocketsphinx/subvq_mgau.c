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
 * subvq.c
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
 * Revision 1.12  2006/02/22  17:43:31  arthchan2003
 * Merged from SPHINX3_5_2_RCI_IRII_BRANCH:
 * 1, vector_gautbl_free is not appropiate to be used in this case because it will free a certain piece of memory twice.
 * 2, Fixed dox-doc.
 * 
 * Revision 1.11.4.1  2005/10/17 04:44:45  arthchan2003
 * Free subvq_t correctly.
 *
 * Revision 1.11  2005/06/21 19:01:33  arthchan2003
 * Added $ keyword.
 *
 * Revision 1.3  2005/03/30 01:22:47  archan
 * Fixed mistakes in last updates. Add
 *
 * 
 * 20.Apr.2001  RAH (rhoughton@mediasite.com, ricky.houghton@cs.cmu.edu)
 *              Updated subvq_free () to free allocated memory
 * 
 * 17-Dec-1999	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Added handling of a single sub-vector in subvq_mgau_shortlist().
 * 
 * 15-Dec-1999	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Changes due to moving subvq_t.{frm_sen_eval,frm_gau_eval} to cont_mgau.h.
 * 
 * 14-Dec-1999	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Added subvq_t.{frm_sen_eval,frm_gau_eval}.  Changed subvq_frame_eval to
 * 		return the normalization factor.
 * 
 * 06-Dec-1999	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Added subvq_subvec_eval_logs3().
 * 
 * 14-Oct-1999	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Changed ci_active flags input to sen_active in subvq_frame_eval().
 * 
 * 21-Jul-1999	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Bugfix in subvq_init() that used g for allocating working areas, even
 *		though g wasn't defined.
 * 
 * 20-Jul-1999	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Added subvq_gautbl_eval_logs3() and used it in subvq_frame_eval().
 * 
 * 12-Mar-1999	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Started.
 */

#include <string.h>
#include <assert.h>

#include <sphinx_config.h>
#include <sphinx_types.h>
#include <ckd_alloc.h>
#include <pio.h>
#include <bio.h>
#include <err.h>
#include <fe.h>

#include "senscr.h"
#include "subvq_mgau.h"

#define MGAU_MIXW_VERSION	"1.0"   /* Sphinx-3 file format version for mixw */

/** Subtract GMM component b (assumed to be positive) and saturate */
#define GMMSUB(a,b) \
	(((a)-(b) > a) ? (INT_MIN) : ((a)-(b)))
/** Add GMM component b (assumed to be positive) and saturate */
#define GMMADD(a,b) \
	(((a)+(b) < a) ? (INT_MAX) : ((a)+(b)))

static int32
vector_maha_precomp(var_t * var, int32 len, float64 floor)
{
    int32 i, det;

    det = 0;
    for (i = 0; i < len; i++) {    /* log(1.0/prod(var[i])) */
	float64 fvar;

	fvar = ((float32 *)var)[i];
	if (fvar < floor)
	    fvar = floor;
        det -= logmath_log(lmath, fvar);
        var[i] = (var_t) logmath_ln_to_log(lmath, 1.0 / (fvar * 2.0));
    }
    det -= logmath_log(lmath, 2.0 * PI) * len;

    return det / 2;         /* sqrt */
}


static void
vector_gautbl_alloc(vector_gautbl_t * gautbl, int32 n_gau, int32 veclen)
{
    gautbl->n_gau = n_gau;
    gautbl->veclen = veclen;
    gautbl->mean =
        (mean_t **) ckd_calloc_2d(n_gau, veclen, sizeof(mean_t));
    gautbl->var =
        (var_t **) ckd_calloc_2d(n_gau, veclen, sizeof(var_t));
}


static void
vector_gautbl_free(vector_gautbl_t * gautbl)
{
    if (gautbl->mean != NULL) {
        ckd_free_2d((void **) gautbl->mean);
    }
    if (gautbl->var != NULL) {
        ckd_free_2d((void **) gautbl->var);
    }
    if (gautbl->lrd != NULL) {
        ckd_free((void *) gautbl->lrd);
    }

}

static void
vector_gautbl_maha_precomp(vector_gautbl_t * gautbl, float64 floor)
{
    int32 g;

    for (g = 0; g < gautbl->n_gau; g++)
        gautbl->lrd[g] = vector_maha_precomp(gautbl->var[g], gautbl->veclen, floor);
#ifdef FIXED_POINT
    for (g = 0; g < gautbl->n_gau; g++) {
	int i;

	for (i = 0; i < gautbl->veclen; i++) {
	    gautbl->mean[g][i] = FLOAT2FIX(((float32 *)gautbl->mean[g])[i]);
	}
    }
#endif
}


static void
vector_gautbl_eval_logs3(vector_gautbl_t * gautbl,
                         int32 offset,
                         int32 count, mfcc_t * x,
			 int32 * score)
{
    int32 i, r;
    int32 end, veclen;
    mean_t *m1, *m2, diff1, diff2;
    var_t *v1, *v2, dval1, dval2;

    /* Interleave evaluation of two vectors at a time for speed on pipelined machines */
    end = offset + count;
    veclen = gautbl->veclen;

    for (r = offset; r < end - 1; r += 2) {
        m1 = gautbl->mean[r];
        m2 = gautbl->mean[r + 1];
        v1 = gautbl->var[r];
        v2 = gautbl->var[r + 1];
        dval1 = (var_t) gautbl->lrd[r];
        dval2 = (var_t) gautbl->lrd[r + 1];

        for (i = 0; i < veclen; i++) {
            diff1 = x[i] - m1[i];
            diff1 = MFCCMUL(diff1, diff1);
	    diff1 = MFCCMUL(diff1, v1[i]);
	    dval1 = GMMSUB(dval1, diff1);
            diff2 = x[i] - m2[i];
            diff2 = MFCCMUL(diff2, diff2);
	    diff2 = MFCCMUL(diff2, v2[i]);
	    dval2 = GMMSUB(dval2, diff2);
        }

        score[r] = (int32) dval1;
        score[r + 1] = (int32) dval2;
    }

    if (r < end) {
        m1 = gautbl->mean[r];
        v1 = gautbl->var[r];
        dval1 = (var_t) gautbl->lrd[r];

        for (i = 0; i < veclen; i++) {
            diff1 = x[i] - m1[i];
            diff1 = MFCCMUL(diff1, diff1);
	    diff1 = MFCCMUL(diff1, v1[i]);
	    dval1 = GMMSUB(dval1, diff1);
        }

        score[r] = (int32) dval1;
    }
}

/*
 * Precompute variances/(covariance-matrix-determinants) to simplify Mahalanobis distance
 * calculation.  Also, calculate 1/(det) for the original codebooks, based on the VQ vars.
 * (See comment in libmisc/vector.h.)
 */
static void
subvq_maha_precomp(subvq_mgau_t * svq, float64 floor)
{
    int32 s;
    int32 *lrd;
    vector_gautbl_t *gautbl;

    E_INFO("Precomputing Mahalanobis distance invariants\n");

    lrd = (int32 *) ckd_calloc(svq->n_sv * svq->vqsize, sizeof(int32));

    for (s = 0; s < svq->n_sv; s++) {
        gautbl = &(svq->gautbl[s]);

        gautbl->lrd = lrd;
        lrd += svq->vqsize;
        vector_gautbl_maha_precomp(gautbl, floor);
    }
}


static void
subvq_map_compact(subvq_mgau_t * vq)
{
    int32 r, c, c2, s;

    /*
     * Compress map entries by removing invalid ones.  NOTE: There's not much of a consistency
     * check to ensure that the entries remaining do map correctly on to the valid entries in
     * the original (parent) mixture Gaussian model g.  The only check we have is to verify
     * the NUMBER of valid entries (components) in each mixture.
     */
    for (r = 0; r < vq->n_mgau; r++) {
        for (c = 0, c2 = 0; c < vq->n_density; c++) {
            if (vq->map[r][c][0] < 0) {
                /* All ought to be < 0 */
                for (s = 1; s < vq->n_sv; s++) {
                    if (vq->map[r][c][s] >= 0)
                        E_FATAL("Partially undefined map[%d][%d]\n", r, c);
                }
            }
            else {
                if (c2 != c) {
                    for (s = 0; s < vq->n_sv; s++) {
                        if (vq->map[r][c][s] < 0)
                            E_FATAL("Partially undefined map[%d][%d]\n", r,
                                    c);
                        vq->map[r][c2][s] = vq->map[r][c][s];
                    }
                }
                c2++;
            }
        }

        /* Invalidate the remaining map entries, for good measure */
        for (; c2 < vq->n_density; c2++) {
            for (s = 0; s < vq->n_sv; s++)
                vq->map[r][c2][s] = -1;
        }
    }
}


/*
 * At this point, a map entries is an index within the subvector; i.e., map[r][c][s] points to
 * a subvq codeword within subvector s.  In evaluating a complete density using it subvq
 * component scores, these maps are used, with 2 lookups for each sub-vector.  To reduce the
 * lookups, linearize the map entries by viewing the subvq scores as a linear array rather than
 * a 2-D array.
 */
static void
subvq_map_linearize(subvq_mgau_t * vq)
{
    int32 r, c, s;

    for (r = 0; r < vq->n_mgau; r++) {
        for (c = 0; (c < vq->n_density) && (vq->map[r][c][0] >= 0); c++) {
            for (s = 0; s < vq->n_sv; s++)
                vq->map[r][c][s] = (s * vq->vqsize) + vq->map[r][c][s];
        }
    }
}

/* FIXME: Duplicated code with s2_semi_mgau.c */
static int32
suvbq_mgau_read_mixw(subvq_mgau_t * s, char const *file_name, double mixw_floor)
{
    char **argname, **argval;
    char eofchk;
    FILE *fp;
    int32 byteswap, chksum_present;
    uint32 chksum;
    float32 *pdf;
    int32 i, f, c, n;
    int32 n_sen;
    int32 n_feat;
    int32 n_comp;
    int32 n_err;

    E_INFO("Reading mixture weights file '%s'\n", file_name);
    fp = myfopen(file_name, "rb");

    /* Read header, including argument-value info and 32-bit byteorder magic */
    if (bio_readhdr(fp, &argname, &argval, &byteswap) < 0)
        E_FATAL("bio_readhdr(%s) failed\n", file_name);

    /* Parse argument-value list */
    chksum_present = 0;
    for (i = 0; argname[i]; i++) {
        if (strcmp(argname[i], "version") == 0) {
            if (strcmp(argval[i], MGAU_MIXW_VERSION) != 0)
                E_WARN("Version mismatch(%s): %s, expecting %s\n",
                       file_name, argval[i], MGAU_MIXW_VERSION);
        }
        else if (strcmp(argname[i], "chksum0") == 0) {
            chksum_present = 1; /* Ignore the associated value */
        }
    }
    bio_hdrarg_free(argname, argval);
    argname = argval = NULL;

    chksum = 0;

    /* Read #senones, #features, #codewords, arraysize */
    if ((bio_fread(&n_sen, sizeof(int32), 1, fp, byteswap, &chksum) != 1)
        || (bio_fread(&n_feat, sizeof(int32), 1, fp, byteswap, &chksum) !=
            1)
        || (bio_fread(&n_comp, sizeof(int32), 1, fp, byteswap, &chksum) !=
            1)
        || (bio_fread(&n, sizeof(int32), 1, fp, byteswap, &chksum) != 1)) {
        E_FATAL("bio_fread(%s) (arraysize) failed\n", file_name);
    }
    if (n_feat != 1)
        E_FATAL("#Features streams(%d) != %d\n", n_feat, 1);
    if (n_sen != s->n_mgau)
        E_FATAL("#Features senones(%d) != %d\n", n_sen, s->n_mgau);
    if (n_comp != s->n_density)
        E_FATAL("#Features densities(%d) != %d\n", n_comp, s->n_density);
    if (n != n_sen * n_feat * n_comp) {
        E_FATAL
            ("%s: #float32s(%d) doesn't match header dimensions: %d x %d x %d\n",
             file_name, i, n_sen, n_feat, n_comp);
    }

    /* Mixture weight arrays. */
    s->mixw = (int32 **)
        ckd_calloc_2d(s->n_mgau, s->n_density, sizeof(int32));

    /* Temporary structure to read in floats before conversion to (int32) logs3 */
    pdf = (float32 *) ckd_calloc(n_comp, sizeof(float32));

    /* Read senone probs data, normalize, floor, convert to logs3, truncate to 8 bits */
    n_err = 0;
    for (i = 0; i < n_sen; i++) {
        for (f = 0; f < n_feat; f++) {
            if (bio_fread((void *) pdf, sizeof(float32),
                          n_comp, fp, byteswap, &chksum) != n_comp) {
                E_FATAL("bio_fread(%s) (arraydata) failed\n", file_name);
            }

            /* Normalize and floor */
            if (vector_sum_norm(pdf, n_comp) <= 0.0)
                n_err++;
            vector_floor(pdf, n_comp, mixw_floor);
            vector_sum_norm(pdf, n_comp);

            /* Convert to LOG */
            for (c = 0; c < n_comp; c++) {
		s->mixw[i][c] = logmath_log(lmath, pdf[c]);
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

    E_INFO("Read %d x %d x %d mixture weights\n", n_sen, n_feat, n_comp);

    return n_sen;
}

subvq_mgau_t *
subvq_mgau_init(const char *subvq_file, float64 varfloor,
		const char *mixw_file, float64 mixwfloor, int32 topn)
{
    FILE *fp;
    char line[16384];
    int32 s, k, l, n, r, c;
    char *strp;
    subvq_mgau_t *vq;

    vq = (subvq_mgau_t *) ckd_calloc(1, sizeof(subvq_mgau_t));

    fp = myfopen(subvq_file, "r");
    E_INFO("Loading Mixture Gaussian sub-VQ file '%s'\n", subvq_file);

    /* Read until "Sub-vectors" */
    for (;;) {
        if (fgets(line, sizeof(line), fp) == NULL)
            E_FATAL("Failed to read VQParam header\n");
        if (sscanf(line, "VQParam %d %d -> %d %d",
                   &(vq->n_mgau), &(vq->n_density), &(vq->n_sv),
                   &(vq->vqsize)) == 4)
            break;
    }

    vq->featdim = (int32 **) ckd_calloc(vq->n_sv, sizeof(int32 *));
    vq->gautbl =
        (vector_gautbl_t *) ckd_calloc(vq->n_sv, sizeof(vector_gautbl_t));
    vq->map =
        (int32 ***) ckd_calloc_3d(vq->n_mgau, vq->n_density, vq->n_sv,
                                  sizeof(int32));

    /* Read subvector sizes and feature dimension maps */
    for (s = 0; s < vq->n_sv; s++) {
        if ((fgets(line, sizeof(line), fp) == NULL) ||
            (sscanf(line, "Subvector %d length %d%n", &k, &l, &n) != 2)
            || (k != s))
            E_FATAL("Error reading length(subvector %d)\n", s);

        if (s < vq->n_sv) {
            vq->gautbl[s].veclen = l;
            vq->featdim[s] =
                (int32 *) ckd_calloc(vq->gautbl[s].veclen, sizeof(int32));

            for (strp = line + n, c = 0; c < vq->gautbl[s].veclen; c++) {
                if (sscanf(strp, "%d%n", &(vq->featdim[s][c]), &n) != 1)
                    E_FATAL("Error reading subvector(%d).featdim(%d)\n", s,
                            c);
                strp += n;
            }

            vector_gautbl_alloc(&(vq->gautbl[s]), vq->vqsize,
                                vq->gautbl[s].veclen);
        }
    }

    /* Echo info for sanity check */
    E_INFO("Original #codebooks(states)/codewords: %d x %d\n",
           vq->n_mgau, vq->n_density);
    E_INFO("Subvectors: %d, VQsize: %d\n", vq->n_sv, vq->vqsize);
    for (s = 0; s < vq->n_sv; s++) {
        E_INFO("SV %d feature dims(%d): ", s, vq->gautbl[s].veclen);
        for (c = 0; c < vq->gautbl[s].veclen; c++)
            fprintf(stderr, " %2d", vq->featdim[s][c]);
        fprintf(stderr, "\n");
    }
    fflush(stderr);

    /* Read VQ codebooks and maps for each subvector */
    for (s = 0; s < vq->n_sv; s++) {
        E_INFO("Reading subvq %d%s\n", s,
               (s < vq->n_sv) ? "" : " (not used)");

        E_INFO("Reading codebook\n");
        if ((fgets(line, sizeof(line), fp) == NULL) ||
            (sscanf(line, "Codebook %d", &k) != 1) || (k != s))
            E_FATAL("Error reading codebook header\n", s);

        for (r = 0; r < vq->vqsize; r++) {
            if (fgets(line, sizeof(line), fp) == NULL)
                E_FATAL("Error reading row(%d)\n", r);

            if (s >= vq->n_sv)
                continue;

            for (strp = line, c = 0; c < vq->gautbl[s].veclen; c++) {
                if (sscanf(strp, "%f %f%n",
                           (float32 *)&(vq->gautbl[s].mean[r][c]),
                           (float32 *)&(vq->gautbl[s].var[r][c]), &k) != 2)
                    E_FATAL("Error reading row(%d) col(%d)\n", r, c);
                strp += k;
            }
        }

        E_INFO("Reading map\n");
        if ((fgets(line, sizeof(line), fp) == NULL) ||
            (sscanf(line, "Map %d", &k) != 1) || (k != s))
            E_FATAL("Error reading map header\n", s);

        for (r = 0; r < vq->n_mgau; r++) {
            if (fgets(line, sizeof(line), fp) == NULL)
                E_FATAL("Error reading row(%d)\n", r);

            if (s >= vq->n_sv)
                continue;

            for (strp = line, c = 0; c < vq->n_density; c++) {
                if (sscanf(strp, "%d%n", &(vq->map[r][c][s]), &k) != 1)
                    E_FATAL("Error reading row(%d) col(%d)\n", r, c);
                strp += k;
            }
        }

        fflush(stdout);
    }

    if ((fscanf(fp, "%s", line) != 1) || (strcmp(line, "End") != 0))
        E_FATAL("Error reading 'End' token\n");

    fclose(fp);

    subvq_maha_precomp(vq, varfloor);
    subvq_map_compact(vq);
    subvq_map_linearize(vq);

    n = 0;
    for (s = 0; s < vq->n_sv; s++) {
        if (vq->gautbl[s].veclen > n)
            n = vq->gautbl[s].veclen;
    }
    assert(n > 0);
    vq->subvec = (mfcc_t *) ckd_calloc(n, sizeof(mfcc_t));
    vq->vqdist =
        (int32 **) ckd_calloc_2d(vq->n_sv, vq->vqsize, sizeof(int32));

    /* Now read mixture weights. */
    suvbq_mgau_read_mixw(vq, mixw_file, mixwfloor);

    return vq;
}


static void
subvq_gautbl_eval_logs3(subvq_mgau_t * vq, mfcc_t *feat)
{
    int32 s, i;
    int32 *featdim;

    for (s = 0; s < vq->n_sv; s++) {
        /* Extract subvector from feat */
        featdim = vq->featdim[s];
        for (i = 0; i < vq->gautbl[s].veclen; i++)
            vq->subvec[i] = feat[featdim[i]];

	/* Evaluate the entire codebook. */
	vector_gautbl_eval_logs3(&(vq->gautbl[s]), 0, vq->vqsize,
				 vq->subvec, vq->vqdist[s]);
    }
}

static int32
subvq_mgau_eval(subvq_mgau_t *vq,
		int32 mgau)
{
    int32 *map, *vqdist, score, sv_id, i;

    map = vq->map[mgau][0];
    vqdist = vq->vqdist[0];

    score = MIN_LOG;
    for (i = 0; i < vq->n_density; ++i) {
	int32 v = 0;
	for (sv_id = 0; sv_id < vq->n_sv; ++sv_id) {
	    if (*map == -1)
		goto no_more_densities;
	    v += vqdist[*(map++)];
	}
	score = ADD(score, v + vq->mixw[mgau][i]);
    }
no_more_densities:
    return score;
}

int32
subvq_mgau_frame_eval(subvq_mgau_t * vq,
		      mfcc_t **featbuf,
		      int32 frame,
		      int32 compallsen)
{
    int32 s, n, best;

    /* Evaluate subvq model for given feature vector */
    subvq_gautbl_eval_logs3(vq, *featbuf);

    /* Evaluate senones */
    best = INT_MIN;
    if (compallsen) {
	for (n = 0; n < vq->n_mgau; ++n) {
	    senone_scores[n] = subvq_mgau_eval(vq, n);
	    if (senone_scores[n] > best)
		best = senone_scores[n];
	}
	/* s2_semi_mgau.c normalizes its scores internally (in a
	 * somewhat different way), so we have to do this here too. */
	for (n = 0; n < vq->n_mgau; ++n) {
	    senone_scores[n] -= best;
	}
    }
    else {
	for (s = 0; s < n_senone_active; ++s) {
	    int32 n = senone_active[s];
	    senone_scores[n] = subvq_mgau_eval(vq, n);
	    if (senone_scores[n] > best)
		best = senone_scores[n];
	}
	for (s = 0; s < n_senone_active; ++s) {
	    int32 n = senone_active[s];
	    senone_scores[n] -= best;
	}
    }

    return 0;
}

/* RAH, free memory allocated by subvq_init() */
void
subvq_mgau_free(subvq_mgau_t * s)
{
    int i;

    if (s) {
        for (i = 0; i < s->n_sv; i++) {
            if (i < s->n_sv) {
                /*vector_gautbl_free(&(s->gautbl[i])); */
                if (s->gautbl[i].mean != NULL)
                    ckd_free_2d((void **) (s->gautbl[i].mean));
                if (s->gautbl[i].var != NULL)
                    ckd_free_2d((void **) (s->gautbl[i].var));
                if (s->featdim[i])
                    ckd_free((void *) s->featdim[i]);
            }
        }

        /* This is tricky because this part of memory is actually allocated only once in .
           subvq_maha_precomp.  So multiple free is actually wrong. */
        if (s->gautbl[0].lrd != NULL)
            ckd_free((void *) (s->gautbl[0].lrd));
        if (s->featdim)
            ckd_free((void *) s->featdim);
        /* Free gaussian table */
        if (s->gautbl)
            ckd_free((void *) s->gautbl);
        /* Free map */
        if (s->map)
            ckd_free_3d((void ***) s->map);
        if (s->subvec)
            ckd_free((void *) s->subvec);
        if (s->vqdist)
            ckd_free_2d((void **) s->vqdist);

        ckd_free((void *) s);
    }
}
