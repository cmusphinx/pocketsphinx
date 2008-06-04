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

/* System headers */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <math.h>
#if defined(__ADSPBLACKFIN__)
#elif !defined(_WIN32_WCE)
#include <sys/types.h>
#endif

#ifndef M_PI 
#define M_PI 3.14159265358979323846 
#endif

/* SphinxBase headers */
#include <sphinx_config.h>
#include <cmd_ln.h>
#include <fixpoint.h>
#include <ckd_alloc.h>
#include <bio.h>
#include <err.h>
#include <prim_type.h>

/* Local headers */
#include "sdc_mgau.h"
#include "vector.h"

#define MGAU_MIXW_VERSION	"1.0"   /* Sphinx-3 file format version for mixw */
#define MGAU_PARAM_VERSION	"1.0"   /* Sphinx-3 file format version for mean/var */
#define MGAU_SDMAP_VERSION	"0.1"   /* file format version for sdmap */
#define NONE		-1
#define WORST_DIST	(int32)(0x80000000)

/** Subtract GMM component b (assumed to be positive) and saturate */
#ifdef FIXED_POINT
#define GMMSUB(a,b) \
	(((a)-(b) > a) ? (INT_MIN) : ((a)-(b)))
/** Add GMM component b (assumed to be positive) and saturate */
#define GMMADD(a,b) \
	(((a)+(b) < a) ? (INT_MAX) : ((a)+(b)))
#else
#define GMMSUB(a,b) ((a)-(b))
#define GMMADD(a,b) ((a)+(b))
#endif

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif


static void
eval_cb(sdc_mgau_t *s, int feat, mfcc_t *z)
{
    mean_t *mean;
    var_t *var, *det;
    int32 i, ceplen;

    /* Thes are organized in 2-d arrays for some reason. */
    mean = s->means[feat];
    var = s->vars[feat];
    det = s->dets[feat];
    ceplen = s->veclen[feat];

    for (i = 0; i < s->n_density; ++i) {
        var_t d;
        int j;

        d = det[i];
        for (j = 0; j < ceplen; ++j) {
            mean_t diff, sqdiff, compl; /* diff, diff^2, component likelihood */
            diff = z[j] - *mean++;
            /* FIXME: Use standard deviations, to avoid squaring, as per Bhiksha. */
            sqdiff = MFCCMUL(diff, diff);
            compl = MFCCMUL(sqdiff, *var);
            d = GMMSUB(d, compl);
            ++var;
        }
        /* FIXME: Do computations themselves shifted. */
        s->cb_scores[feat][i] = (int32)d >> SENSCR_SHIFT;
    }
}

static int32
compute_scores(sdc_mgau_t * s, int16 *senone_scores,
               int32 *senone_active, int32 n_senone_active,
               int32 *out_bestidx)
{
    int32 best = (int32)0x7fffffff;
    int i;

    memset(senone_scores, 0, s->n_sen * sizeof(*senone_scores));
    for (i = 0; i < n_senone_active; ++i) {
        /* Get the senone ID. */
        int sen = senone_active[i];
        int c;
        /* Sum up mixture densities. */
        for (c = 0; c < s->n_mixw_den; ++c) {
            int32 mden = -(int32)s->mixw[sen][c];
            int f;
            /* For each subvector. */
            for (f = 0; f < s->n_sv; ++f) {
                /* Get the codeword for this subvector. */
                int cw = s->sdmap[sen][f][c];
                /* And its score. */
                mden += s->cb_scores[f][cw];
            }
            /* Add to senone score. */
            if (c == 0)
                senone_scores[sen] = mden;
            else
                senone_scores[sen] =
                    logmath_add(s->lmath_8b, senone_scores[sen], mden);
        }
        /* Negate it since that is what everything else expects at the moment. */
        senone_scores[sen] = -senone_scores[sen];
        if (senone_scores[sen] < best) {
            best = senone_scores[sen];
            *out_bestidx = sen;
        }
    }
    return best;
}

static int32
compute_scores_all(sdc_mgau_t * s, int16 *senone_scores,
                   int32 *out_bestidx)
{
    int32 best = (int32)0x7fffffff;
    int sen;

    for (sen = 0; sen < s->n_sen; ++sen) {
        int c;
        /* Sum up mixture densities. */
        for (c = 0; c < s->n_mixw_den; ++c) {
            int32 mden = -(int32)s->mixw[sen][c];
            int f;
            /* For each subvector. */
            for (f = 0; f < s->n_sv; ++f) {
                /* Get the codeword for this subvector. */
                int cw = s->sdmap[sen][f][c];
                /* And its score. */
                mden += s->cb_scores[f][cw];
            }
            /* Add to senone score. */
            if (c == 0)
                senone_scores[sen] = mden;
            else
                senone_scores[sen] =
                    logmath_add(s->lmath_8b, senone_scores[sen], mden);
        }
        /* Negate it since that is what everything else expects at the moment. */
        senone_scores[sen] = -senone_scores[sen];
        if (senone_scores[sen] < best) {
            best = senone_scores[sen];
            *out_bestidx = sen;
        }
    }
    return best;
}

/*
 * Compute senone scores for the active senones.
 */
int32
sdc_mgau_frame_eval(sdc_mgau_t * s,
                    int16 *senone_scores,
                    int32 *senone_active,
                    int32 n_senone_active,
                    mfcc_t ** featbuf, int32 frame,
                    int32 compallsen,
                    int32 *out_bestidx)
{
    int32 bscore;
    int i;

    /* Compute codebook scores, unless this frame is skipped. */
    if (frame % s->ds_ratio == 0) {
        int sv;

        for (sv = 0; sv < s->n_sv; ++sv)
            eval_cb(s, sv, featbuf[sv]);
    }

    /* Compute senone scores. */
    if (compallsen)
        bscore = compute_scores_all(s, senone_scores, out_bestidx);
    else
        bscore = compute_scores(s, senone_scores,
                                senone_active, n_senone_active,
                                out_bestidx);
    /* Normalize them. */
    for (i = 0; i < s->n_sen; ++i)
        senone_scores[i] -= bscore;

    return bscore;
}

static int32
read_mixw(sdc_mgau_t * s, char const *file_name, double SmoothMin)
{
    char **argname, **argval;
    char eofchk;
    FILE *fp;
    int32 byteswap, chksum_present;
    uint32 chksum;
    float32 *pdf;
    int32 i, c, n;
    int32 n_sen;
    int32 n_feat;
    int32 n_comp;
    int32 n_err;

    E_INFO("Reading mixture weights file '%s'\n", file_name);

    if ((fp = fopen(file_name, "rb")) == NULL)
        E_FATAL("fopen(%s,rb) failed\n", file_name);

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
        E_FATAL("#Features streams(%d) != 1\n", n_feat);
    if (n != n_sen * n_feat * n_comp) {
        E_FATAL
            ("%s: #float32s(%d) doesn't match header dimensions: %d x %d x %d\n",
             file_name, i, n_sen, n_feat, n_comp);
    }

    /* n_sen = number of mixture weights per codeword, which is
     * fixed at the number of senones since we have only one codebook.
     */
    s->n_sen = n_sen;

    /* Quantized mixture weight arrays. */
    s->mixw = ckd_calloc_2d(n_sen, n_comp, sizeof(**s->mixw));
    s->n_mixw_den = n_comp;

    /* Temporary structure to read in floats before conversion to (int32) logs3 */
    pdf = (float32 *) ckd_calloc(n_comp, sizeof(float32));

    /* Read senone probs data, normalize, floor, convert to logs3, truncate to 8 bits */
    n_err = 0;
    for (i = 0; i < n_sen; i++) {
        if (bio_fread((void *) pdf, sizeof(float32),
                      n_comp, fp, byteswap, &chksum) != n_comp) {
            E_FATAL("bio_fread(%s) (arraydata) failed\n", file_name);
        }

        /* Normalize and floor */
        if (vector_sum_norm(pdf, n_comp) <= 0.0)
            n_err++;
        vector_floor(pdf, n_comp, SmoothMin);
        vector_sum_norm(pdf, n_comp);

        /* Convert to LOG, quantize, and transpose */
        for (c = 0; c < n_comp; c++) {
            int32 qscr;

            qscr = -logmath_log(s->lmath_8b, pdf[c]);
            if ((qscr > 255) || (qscr < 0))
                qscr = 255;
            s->mixw[i][c] = qscr;
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

static int
read_sdmap(sdc_mgau_t *s, const char *file_name)
{
    char **argname, **argval;
    int32 byteswap, chksum_present;
    unsigned char eofchk;
    uint32 chksum;
    FILE *fp;
    int32 n_sen, n_comp, n_sv, n_map;
    int i;

    E_INFO("Reading subspace mapping file '%s'\n", file_name);

    if ((fp = fopen(file_name, "rb")) == NULL)
        E_FATAL("fopen(%s,rb) failed\n", file_name);

    /* Read header, including argument-value info and 32-bit byteorder magic */
    if (bio_readhdr(fp, &argname, &argval, &byteswap) < 0)
        E_FATAL("bio_readhdr(%s) failed\n", file_name);

    /* Parse argument-value list */
    chksum_present = 0;
    n_sen = n_comp = n_sv = 0;
    for (i = 0; argname[i]; i++) {
        if (strcmp(argname[i], "version") == 0) {
            if (strcmp(argval[i], MGAU_SDMAP_VERSION) != 0)
                E_WARN("Version mismatch(%s): %s, expecting %s\n",
                       file_name, argval[i], MGAU_SDMAP_VERSION);
        }
        else if (strcmp(argname[i], "chksum0") == 0) {
            chksum_present = 1; /* Ignore the associated value */
        }
        else if (strcmp(argname[i], "n_density") == 0) {
            n_comp = atoi(argval[i]);
        }
        else if (strcmp(argname[i], "n_mgau") == 0) {
            n_sen = atoi(argval[i]);
        }
        else if (strncmp(argname[i], "subvec", 6) == 0) {
            ++n_sv;
        }
    }
    bio_hdrarg_free(argname, argval);
    argname = argval = NULL;

    if (n_sen == 0 || n_comp == 0 || n_sv == 0) {
        E_ERROR("Dimensions seem to be missing from file header!\n");
        return -1;
    }
    if (n_sv != s->n_sv) {
        E_ERROR("Number of subvectors mismatch between kmeans and sdmap: %d != %d\n",
                n_sv, s->n_sv);
        return -1;
    }

    chksum = 0;

    n_map = n_sen * n_sv * n_comp;
    s->sdmap = ckd_calloc_3d(n_sen, n_sv, n_comp, sizeof(***s->sdmap));
    if (bio_fread(s->sdmap[0][0], sizeof(***s->sdmap),
                  n_map, fp, byteswap, &chksum) != n_map) {
        E_FATAL("bio_fread(%s) (arraydata) failed\n", file_name);
    }
    if (chksum_present)
        bio_verify_chksum(fp, byteswap, chksum);

    if (fread(&eofchk, 1, 1, fp) == 1)
        E_FATAL("More data than expected in %s\n", file_name);

    fclose(fp);

    E_INFO("Read %d x %d x %d subspace cluster mappings\n", n_sen, n_sv, n_comp);
    return n_sen;
}

/* Read a Sphinx3 mean or variance file. */
static int32
s3_read_mgau(sdc_mgau_t *s, const char *file_name, float32 ***out_cb)
{
    char tmp;
    FILE *fp;
    int32 i, blk, n;
    int32 n_mgau;
    int32 n_sv;
    int32 n_density;
    int32 *veclen;
    int32 byteswap, chksum_present;
    char **argname, **argval;
    uint32 chksum;

    E_INFO("Reading S3 mixture gaussian file '%s'\n", file_name);

    if ((fp = fopen(file_name, "rb")) == NULL)
        E_FATAL("fopen(%s,rb) failed\n", file_name);

    /* Read header, including argument-value info and 32-bit byteorder magic */
    if (bio_readhdr(fp, &argname, &argval, &byteswap) < 0)
        E_FATAL("bio_readhdr(%s) failed\n", file_name);

    /* Parse argument-value list */
    chksum_present = 0;
    for (i = 0; argname[i]; i++) {
        if (strcmp(argname[i], "version") == 0) {
            if (strcmp(argval[i], MGAU_PARAM_VERSION) != 0)
                E_WARN("Version mismatch(%s): %s, expecting %s\n",
                       file_name, argval[i], MGAU_PARAM_VERSION);
        }
        else if (strcmp(argname[i], "chksum0") == 0) {
            chksum_present = 1; /* Ignore the associated value */
        }
    }
    bio_hdrarg_free(argname, argval);
    argname = argval = NULL;

    chksum = 0;

    /* #Codebooks */
    if (bio_fread(&n_mgau, sizeof(int32), 1, fp, byteswap, &chksum) != 1)
        E_FATAL("fread(%s) (#codebooks) failed\n", file_name);
    if (n_mgau != 1) {
        E_ERROR("%s: #codebooks (%d) != 1\n", file_name, n_mgau);
        fclose(fp);
        return -1;
    }

    /* #Features/codebook */
    if (bio_fread(&n_sv, sizeof(int32), 1, fp, byteswap, &chksum) != 1)
        E_FATAL("fread(%s) (#features) failed\n", file_name);
    if (s->n_sv == 0)
        s->n_sv = n_sv;
    else if (n_sv != s->n_sv)
        E_FATAL("#Features streams(%d) != %d\n", n_sv, s->n_sv);

    /* #Gaussian densities/feature in each codebook */
    if (bio_fread(&n_density, sizeof(int32), 1, fp,
                  byteswap, &chksum) != 1)
        E_FATAL("fread(%s) (#density/codebook) failed\n", file_name);
    if (s->n_density == 0)
        s->n_density = n_density;
    else if (n_density != s->n_density)
        E_FATAL("%s: Number of densities per feature(%d) != %d\n",
                file_name, n_mgau, s->n_density);

    /* Vector length of feature stream */
    if (s->veclen == NULL)
        s->veclen = ckd_calloc(s->n_sv, sizeof(int32));
    veclen = ckd_calloc(s->n_sv, sizeof(int32));
    if (bio_fread(veclen, sizeof(int32), s->n_sv,
                  fp, byteswap, &chksum) != s->n_sv)
        E_FATAL("fread(%s) (feature vector-length) failed\n", file_name);
    for (i = 0, blk = 0; i < s->n_sv; ++i) {
        if (s->veclen[i] == 0)
            s->veclen[i] = veclen[i];
        else if (veclen[i] != s->veclen[i])
            E_FATAL("feature stream length %d is inconsistent (%d != %d)\n",
                    i, veclen[i], s->veclen[i]);
        blk += veclen[i];
    }

    /* #Floats to follow; for the ENTIRE SET of CODEBOOKS */
    if (bio_fread(&n, sizeof(int32), 1, fp, byteswap, &chksum) != 1)
        E_FATAL("fread(%s) (total #floats) failed\n", file_name);
    if (n != n_mgau * n_density * blk)
        E_FATAL
            ("%s: #float32s(%d) doesn't match dimensions: %d x %d x %d\n",
             file_name, n, n_mgau, n_density, blk);

    *out_cb = ckd_calloc(s->n_sv, sizeof(float32 *));
    for (i = 0; i < s->n_sv; ++i) {
        (*out_cb)[i] =
            (float32 *) ckd_calloc(n_density * veclen[i],
                                   sizeof(float32));
        if (bio_fread
            ((*out_cb)[i], sizeof(float32),
             n_density * veclen[i], fp,
             byteswap, &chksum) != n_density * veclen[i])
            E_FATAL("fread(%s, %d) of feat %d failed\n", file_name,
                    n_density * veclen[i], i);
    }
    ckd_free(veclen);

    if (chksum_present)
        bio_verify_chksum(fp, byteswap, chksum);

    if (fread(&tmp, 1, 1, fp) == 1)
        E_FATAL("%s: More data than expected\n", file_name);

    fclose(fp);

    E_INFO("%d mixture Gaussians, %d components, %d subvectors, veclen %d\n", n_mgau,
           n_density, n_sv, blk);

    return n;
}

static int32
s3_precomp(sdc_mgau_t *s, logmath_t *lmath, float32 vFloor)
{
    int feat;

    for (feat = 0; feat < s->n_sv; ++feat) {
        float32 *fmp;
        mean_t *mp;
        var_t *vp, *dp;
        int32 vecLen, i;

        vecLen = s->veclen[feat];
        fmp = (float32 *) s->means[feat];
        mp = s->means[feat];
        vp = s->vars[feat];
        dp = s->dets[feat];

        for (i = 0; i < s->n_density; ++i) {
            var_t d;
            int32 j;

            d = 0;
            for (j = 0; j < vecLen; ++j, ++vp, ++mp, ++fmp) {
                float64 fvar;

#ifdef FIXED_POINT
                *mp = FLOAT2FIX(*fmp);
#endif
                /* Always do these pre-calculations in floating point */
                fvar = *(float32 *) vp;
                if (fvar < vFloor)
                    fvar = vFloor;
                d += (var_t)logmath_log(lmath, 1 / sqrt(fvar * 2.0 * M_PI));
                *vp = (var_t)logmath_ln_to_log(lmath, 1.0 / (2.0 * fvar));
            }
            *dp++ = d;
        }
    }
    return 0;
}

sdc_mgau_t *
sdc_mgau_init(cmd_ln_t *config, logmath_t *lmath, bin_mdef_t *mdef)
{
    sdc_mgau_t *s;
    float32 **fgau;

    s = ckd_calloc(1, sizeof(*s));
    s->config = config;

    /* Log-add table. */
    s->lmath_8b = logmath_init(logmath_get_base(lmath), SENSCR_SHIFT, TRUE);
    if (s->lmath_8b == NULL) {
        sdc_mgau_free(s);
        return NULL;
    }
    /* Ensure that it is only 8 bits wide so that fast_logmath_add() works. */
    if (logmath_get_width(s->lmath_8b) != 1) {
        E_ERROR("Log base %f is too small to represent add table in 8 bits\n",
                logmath_get_base(s->lmath_8b));
        sdc_mgau_free(s);
        return NULL;
    }

    /* Read means and variances. */
    if (s3_read_mgau(s, cmd_ln_str_r(config, "-mean"), &fgau) < 0) {
        sdc_mgau_free(s);
        return NULL;
    }
    s->means = (mean_t **)fgau;
    if (s3_read_mgau(s, cmd_ln_str_r(config, "-var"), &fgau) < 0) {
        sdc_mgau_free(s);
        return NULL;
    }
    s->vars = (var_t **)fgau;

    /* Precompute (and fixed-point-ize) means, variances, and determinants. */
    s->dets = (var_t **)ckd_calloc_2d(s->n_sv, s->n_density, sizeof(**s->dets));
    s3_precomp(s, lmath, cmd_ln_float32_r(config, "-varfloor"));

    /* Read mixture weights */
    if (read_mixw(s, cmd_ln_str_r(config, "-mixw"),
                  cmd_ln_float32_r(config, "-mixwfloor")) < 0) {
        sdc_mgau_free(s);
        return NULL;
    }
    s->ds_ratio = cmd_ln_int32_r(config, "-ds");

    /* Read subspace cluster mapping */
    if (read_sdmap(s, cmd_ln_str_r(config, "-sdmap")) < 0) {
        sdc_mgau_free(s);
        return NULL;
    }

    /* Codebook scores. */
    s->cb_scores = ckd_calloc_2d(s->n_sv, s->n_density,
                                 sizeof(**s->cb_scores));

    return s;
}

void
sdc_mgau_free(sdc_mgau_t * s)
{
    uint32 i;

    logmath_free(s->lmath_8b);
    ckd_free_2d(s->mixw);
    for (i = 0; i < s->n_sv; ++i) {
        ckd_free(s->means[i]);
        ckd_free(s->vars[i]);
    }
    ckd_free(s->veclen);
    ckd_free(s->means);
    ckd_free(s->vars);
    ckd_free_3d(s->sdmap);
    ckd_free_2d(s->cb_scores);
    ckd_free(s->score_tmp);
    ckd_free(s);
}
