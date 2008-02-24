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
 * gauden.c -- gaussian density module.
 *
 ***********************************************
 * CMU ARPA Speech Project
 *
 * Copyright (c) 1996 Carnegie Mellon University.
 * ALL RIGHTS RESERVED.
 ***********************************************
 *
 * HISTORY
 * $Log$
 * Revision 1.7  2006/02/22  17:09:55  arthchan2003
 * Merged from SPHINX3_5_2_RCI_IRII_BRANCH: 1, Followed Dave's change, keep active to be uint8 instead int8 in gauden_dist_norm.\n 2, Introdued gauden_dump and gauden_dump_ind.  This allows debugging of ms_gauden routine. \n 3, Introduced gauden_free, this fixed some minor memory leaks. \n 4, gauden_init accept an argument precompute to specify whether the distance is pre-computed or not.\n 5, Added license. \n 6, Fixed dox-doc.
 * 
 *
 * Revision 1.5.4.7  2006/01/16 19:45:59  arthchan2003
 * Change the gaussian density dumping routine to a function.
 *
 * Revision 1.5.4.6  2005/10/09 19:51:05  arthchan2003
 * Followed Dave's changed in the trunk.
 *
 * Revision 1.5.4.5  2005/09/25 18:54:20  arthchan2003
 * Added a flag to turn on and off precomputation.
 *
 * Revision 1.6  2005/10/05 00:31:14  dhdfu
 * Make int8 be explicitly signed (signedness of 'char' is
 * architecture-dependent).  Then make a bunch of things use uint8 where
 * signedness is unimportant, because on the architecture where 'char' is
 * unsigned, it is that way for a reason (signed chars are slower).
 *
 * Revision 1.5.4.4  2005/09/07 23:29:07  arthchan2003
 * Added FIXME warning.
 *
 * Revision 1.5.4.3  2005/09/07 23:25:10  arthchan2003
 * 1, Behavior changes of cont_mgau, instead of remove Gaussian with zero variance vector before flooring, now remove Gaussian with zero mean and variance before flooring. Notice that this is not yet synchronize with ms_mgau. 2, Added warning message in multi-stream gaussian distribution.
 *
 * Revision 1.5.4.2  2005/08/03 18:53:44  dhdfu
 * Add memory deallocation functions.  Also move all the initialization
 * of ms_mgau_model_t into ms_mgau_init (duh!), which entails removing it
 * from decode_anytopo and friends.
 *
 * Revision 1.5.4.1  2005/07/20 19:39:01  arthchan2003
 * Added licences in ms_* series of code.
 *
 * Revision 1.5  2005/06/21 18:55:09  arthchan2003
 * 1, Add comments to describe this modules, 2, Fixed doxygen documentation. 3, Added $ keyword.
 *
 * Revision 1.3  2005/03/30 01:22:47  archan
 * Fixed mistakes in last updates. Add
 *
 * 
 * 20-Dec-96	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University.
 * 		Changed gauden_param_read to use the new libio/bio_fread functions.
 * 
 * 26-Sep-96	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University.
 * 		Added gauden_mean_reload() for application of MLLR; and correspondingly
 * 		made gauden_param_read allocate memory for parameter only if not
 * 		already allocated.
 * 
 * 09-Sep-96	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University.
 * 		Interleaved two density computations for speed improvement.
 * 
 * 19-Aug-96	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University.
 * 		Added compute_dist_all special case for improving speed.
 * 
 * 26-Jan-96	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University.
 * 		Added check for underflow and floor insertion in gauden_dist.
 * 
 * 20-Jan-96	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University.
 * 		Added active argument to gauden_dist_norm and gauden_dist_norm_global,
 * 		and made the latter a static function.
 * 
 * 07-Nov-95	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University.
 * 		Initial version created.
 * 		Very liberally borrowed/adapted from Eric's S3 trainer implementation.
 */

/* System headers. */
#include <assert.h>
#include <string.h>
#include <math.h>
#include <float.h>

/* SphinxBase headers. */
#include <bio.h>
#include <err.h>
#include <ckd_alloc.h>

/* Local headesr. */
#include "ms_gauden.h"
#include "search_const.h"
#include "log.h"


#define GAUDEN_PARAM_VERSION	"1.0"

#undef M_PI
#define M_PI	3.1415926535897932385e0

void
gauden_dump(const gauden_t * g)
{
    int32 c;

    for (c = 0; c < g->n_mgau; c++)
        gauden_dump_ind(g, c);
}


void
gauden_dump_ind(const gauden_t * g, int senidx)
{
    int32 f, d, i;

    for (f = 0; f < g->n_feat; f++) {
        E_INFO("Codebook %d, Feature %d (%dx%d):\n",
               senidx, f, g->n_density, g->featlen[f]);

        for (d = 0; d < g->n_density; d++) {
            printf("m[%3d]", d);
            for (i = 0; i < g->featlen[f]; i++)
                printf(" %7.4f", g->mean[senidx][f][d][i]);
            printf("\n");
        }
        printf("\n");

        for (d = 0; d < g->n_density; d++) {
            printf("v[%3d]", d);
            for (i = 0; i < g->featlen[f]; i++)
                printf(" %7.4f", g->var[senidx][f][d][i]);
            printf("\n");
        }
        printf("\n");

        for (d = 0; d < g->n_density; d++)
            printf("d[%3d] %7.4f\n", d, g->det[senidx][f][d]);
    }
    fflush(stderr);
}



static int32
gauden_param_read(mfcc_t***** out_param,      /* Alloc space iff *out_param == NULL */
                  int32 * out_n_mgau,
                  int32 * out_n_feat,
                  int32 * out_n_density,
                  int32 ** out_veclen, const char *file_name)
{
    char tmp;
    FILE *fp;
    int32 i, j, k, l, n, blk;
    int32 n_mgau;
    int32 n_feat;
    int32 n_density;
    int32 *veclen;
    int32 byteswap, chksum_present;
    mfcc_t****out;
    mfcc_t *buf;
    char **argname, **argval;
    uint32 chksum;

    E_INFO("Reading mixture gaussian parameter: %s\n", file_name);

    if ((fp = fopen(file_name, "rb")) == NULL)
        E_FATAL_SYSTEM("fopen(%s,rb) failed\n", file_name);

    /* Read header, including argument-value info and 32-bit byteorder magic */
    if (bio_readhdr(fp, &argname, &argval, &byteswap) < 0)
        E_FATAL("bio_readhdr(%s) failed\n", file_name);

    /* Parse argument-value list */
    chksum_present = 0;
    for (i = 0; argname[i]; i++) {
        if (strcmp(argname[i], "version") == 0) {
            if (strcmp(argval[i], GAUDEN_PARAM_VERSION) != 0)
                E_WARN("Version mismatch(%s): %s, expecting %s\n",
                       file_name, argval[i], GAUDEN_PARAM_VERSION);
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
    *out_n_mgau = n_mgau;

    /* #Features/codebook */
    if (bio_fread(&n_feat, sizeof(int32), 1, fp, byteswap, &chksum) != 1)
        E_FATAL("fread(%s) (#features) failed\n", file_name);
    *out_n_feat = n_feat;

    /* #Gaussian densities/feature in each codebook */
    if (bio_fread(&n_density, sizeof(int32), 1, fp, byteswap, &chksum) !=
        1)
        E_FATAL("fread(%s) (#density/codebook) failed\n", file_name);
    *out_n_density = n_density;

    /* #Dimensions in each feature stream */
    veclen = ckd_calloc(n_feat, sizeof(uint32));
    *out_veclen = veclen;
    if (bio_fread(veclen, sizeof(int32), n_feat, fp, byteswap, &chksum) !=
        n_feat)
        E_FATAL("fread(%s) (feature-lengths) failed\n", file_name);

    /* blk = total vector length of all feature streams */
    for (i = 0, blk = 0; i < n_feat; i++)
        blk += veclen[i];

    /* #Floats to follow; for the ENTIRE SET of CODEBOOKS */
    if (bio_fread(&n, sizeof(int32), 1, fp, byteswap, &chksum) != 1)
        E_FATAL("fread(%s) (total #floats) failed\n", file_name);
    if (n != n_mgau * n_density * blk) {
        E_FATAL
            ("%s: #mfcc_ts(%d) doesn't match dimensions: %d x %d x %d\n",
             file_name, n, n_mgau, n_density, blk);
    }

    /* Allocate memory for mixture gaussian densities if not already allocated */
    if (!(*out_param)) {
        out = (mfcc_t****) ckd_calloc_3d(n_mgau, n_feat, n_density,
                                           sizeof(mfcc_t*));
        buf = (mfcc_t *) ckd_calloc(n, sizeof(float));
        for (i = 0, l = 0; i < n_mgau; i++) {
            for (j = 0; j < n_feat; j++) {
                for (k = 0; k < n_density; k++) {
                    out[i][j][k] = &buf[l];

                    l += veclen[j];
                }
            }
        }
    }
    else {
        out = *out_param;
        buf = out[0][0][0];
    }

    /* Read mixture gaussian densities data */
    if (bio_fread(buf, sizeof(mfcc_t), n, fp, byteswap, &chksum) != n)
        E_FATAL("fread(%s) (densitydata) failed\n", file_name);

    if (chksum_present)
        bio_verify_chksum(fp, byteswap, chksum);

    if (fread(&tmp, 1, 1, fp) == 1)
        E_FATAL("More data than expected in %s\n", file_name);

    fclose(fp);

    *out_param = out;

    E_INFO("%d codebook, %d feature, size\n", n_mgau, n_feat);
    for (i = 0; i < n_feat; i++)
        printf(" %dx%d", n_density, veclen[i]);
    printf("\n");
    fflush(stdout);

    return 0;
}

static void
gauden_param_free(mfcc_t**** p)
{
    ckd_free(p[0][0][0]);
    ckd_free_3d((void ***) p);
}


/*
 * Some of the gaussian density computation can be carried out in advance:
 * 	log(determinant) calculation,
 * 	1/(2*var) in the exponent,
 * NOTE; The density computation is performed in log domain.
 */
static int32
gauden_dist_precompute(gauden_t * g, mfcc_t varfloor)
{
    int32 i, m, f, d, flen;
    mfcc_t *varp, *detp;
    int32 n;

    n = 0;
    /* Allocate space for determinants */
    g->det =
        (mfcc_t ***) ckd_calloc_3d(g->n_mgau, g->n_feat, g->n_density,
                                    sizeof(mfcc_t));

    /** FIX ME!, There is no removal of Gaussian in ms_mgau. This is
	not yet synchronized with cont_mgau's behavior. */

    for (m = 0; m < g->n_mgau; m++) {
        for (f = 0; f < g->n_feat; f++) {
            flen = g->featlen[f];

            /* Determinants for all variance vectors in g->[m][f] */
            for (d = 0, detp = g->det[m][f]; d < g->n_density; d++, detp++) {
                *detp = (mfcc_t) 0.0;// TODO: MFCCize me!

                for (i = 0, varp = g->var[m][f][d]; i < flen; i++, varp++) {
                    if (*varp < varfloor) {
#if 0
                        E_INFO
                            ("varp %f , floor %f n=%d, m %d, f %d c %d, i %d\n",
                             *varp, varfloor, n, m, f, d, i);
#endif
                        *varp = varfloor;
                        n++;
                    }

                    *detp += (mfcc_t) (log(*varp)); // TODO: MFCCize me!

                    /* Precompute this part of the exponential */
                    *varp = (mfcc_t) (1.0 / (*varp * 2.0));// TODO: MFCCize me!
                }

                /* 2pi */
                *detp += (mfcc_t) (flen * log(2.0 * M_PI));// TODO: MFCCize me!

                /* Sqrt */
                *detp *= (mfcc_t) 0.5;// TODO: MFCCize me!
            }
        }
    }

    if (1)
        E_INFO("%d variance values floored\n", n);

    return 0;
}


gauden_t *
gauden_init(char *meanfile, char *varfile, mfcc_t varfloor,
            int32 precompute, logmath_t *lmath)
{
    int32 i, m, f, d, *flen;
    gauden_t *g;

    assert(meanfile != NULL);
    assert(varfile != NULL);
    assert(varfloor > 0.0);

    g = (gauden_t *) ckd_calloc(1, sizeof(gauden_t));
    g->lmath = lmath;
    g->mean = g->var = NULL;    /* To force them to be allocated */

    /* Read means and (diagonal) variances for all mixture gaussians */
    gauden_param_read(&(g->mean), &g->n_mgau, &g->n_feat, &g->n_density,
                      &g->featlen, meanfile);
    gauden_param_read(&(g->var), &m, &f, &d, &flen, varfile);

    /* Verify mean and variance parameter dimensions */
    if ((m != g->n_mgau) || (f != g->n_feat) || (d != g->n_density))
        E_FATAL
            ("Mixture-gaussians dimensions for means and variances differ\n");
    for (i = 0; i < g->n_feat; i++)
        if (g->featlen[i] != flen[i])
            E_FATAL("Feature lengths for means and variances differ\n");
    ckd_free(flen);

    /* Floor variances and precompute variance determinants */
    if (precompute)
        gauden_dist_precompute(g, varfloor);

    return g;
}

void
gauden_free(gauden_t * g)
{
    if (g == NULL)
        return;
    if (g->mean)
        gauden_param_free(g->mean);
    if (g->var)
        gauden_param_free(g->var);
    if (g->det)
        ckd_free_3d((void *) g->det);
    if (g->featlen)
        ckd_free(g->featlen);
    ckd_free(g);
}

int32
gauden_mean_reload(gauden_t * g, char *meanfile)
{
    int32 i, m, f, d, *flen;

    assert(g->mean != NULL);

    gauden_param_read(&(g->mean), &m, &f, &d, &flen, meanfile);

    /* Verify original and new mean parameter dimensions */
    if ((m != g->n_mgau) || (f != g->n_feat) || (d != g->n_density))
        E_FATAL
            ("Mixture-gaussians dimensions for original and new means differ\n");
    for (i = 0; i < g->n_feat; i++)
        if (g->featlen[i] != flen[i])
            E_FATAL("Feature lengths for original and new means differ\n");
    ckd_free(flen);

    return 0;
}

/*
 * Temporary structure for computing density values.  The only difference between
 * this and gauden_dist_t is the use of float64 for dist.
 */


typedef struct {
    int32 id;
    float64 dist;               /* Can probably use mfcc_t */
} dist_t;

static dist_t *dist;
static int32 n_dist = 0;


/* See compute_dist below */
static int32
compute_dist_all(dist_t * out_dist, mfcc_t* obs, int32 featlen,
                 mfcc_t** mean, mfcc_t** var, mfcc_t * det,
                 int32 n_density)
{
    int32 i, d;
    mfcc_t *m1,*m2, *v1, *v2;
    float64 dval1, dval2, diff1, diff2;// TODO: MFCCize me!

    for (d = 0; d < n_density - 1; d += 2) {
        m1 = mean[d];
        v1 = var[d];
        dval1 = det[d];
        m2 = mean[d + 1];
        v2 = var[d + 1];
        dval2 = det[d + 1];

        for (i = 0; i < featlen; i++) {
            diff1 = obs[i] - m1[i];
            dval1 += diff1 * diff1 * v1[i];// TODO: MFCCize me!
            diff2 = obs[i] - m2[i];
            dval2 += diff2 * diff2 * v2[i];// TODO: MFCCize me!
        }

        out_dist[d].dist = dval1;
        out_dist[d].id = d;
        out_dist[d + 1].dist = dval2;
        out_dist[d + 1].id = d + 1;
    }

    if (d < n_density) {
        m1 = mean[d];
        v1 = var[d];
        dval1 = det[d];

        for (i = 0; i < featlen; i++) {
            diff1 = obs[i] - m1[i];
            dval1 += diff1 * diff1 * v1[i];// TODO: MFCCize me!
        }

        out_dist[d].dist = dval1;
        out_dist[d].id = d;
    }

    return 0;
}


/*
 * Compute the top-N closest gaussians from the chosen set (mgau,feat)
 * for the given input observation vector.
 * NOTE: The density values computed are in log-domain, and while they are being
 * computed they're really the DENOMINATOR of the distance expression.
 */
static int32
compute_dist(dist_t * out_dist, int32 n_top,
             mfcc_t* obs, int32 featlen,
             mfcc_t** mean, mfcc_t** var, mfcc_t * det,
             int32 n_density)
{
    int32 i, j, d;
    dist_t *worst;
    mfcc_t *m, *v;
    float64 dval, diff;// TODO: MFCCize me!

    /* Special case optimization when n_density <= n_top */
    if (n_top >= n_density)
        return (compute_dist_all
                (out_dist, obs, featlen, mean, var, det, n_density));

    /*
     * We are really computing denominators in the gaussian density expression:
     *   sqrt(2pi * det), and (x-mean)^2*var.
     * To maximize the density value, we want to minimize the denominators.
     */

    for (i = 0; i < n_top; i++)
        out_dist[i].dist = DBL_MAX;
    worst = &(out_dist[n_top - 1]);

    for (d = 0; d < n_density; d++) {
        m = mean[d];
        v = var[d];
        dval = det[d];

        for (i = 0; (i < featlen) && (dval <= worst->dist); i++) {
            diff = obs[i] - m[i];
            dval += diff * diff * v[i];// TODO: MFCCize me!
        }

        if ((i < featlen) || (dval >= worst->dist))     /* Codeword d worse than worst */
            continue;

        /* Codeword d at least as good as worst so far; insert in the ordered list */
        for (i = 0; (i < n_top) && (dval >= out_dist[i].dist); i++);
        assert(i < n_top);
        for (j = n_top - 1; j > i; --j)
            out_dist[j] = out_dist[j - 1];
        out_dist[i].dist = dval;
        out_dist[i].id = d;
    }

    return 0;
}


#if 1
/*
 * Compute distances of the input observation from the top N codewords in the given
 * codebook (g->{mean,var}[mgau]).  The input observation, obs, includes vectors for
 * all features in the codebook.
 */
int32
gauden_dist(gauden_t * g,
            s3mgauid_t mgau,
            int32 n_top, mfcc_t** obs, gauden_dist_t ** out_dist)
{
    int32 f, t;
/* Density values, once converted to (int32)logs3 domain, can
   underflow (or overflow?), causing headaches all around.  To avoid
   underflow, use this floor value */
    float64 min_density = logmath_log_to_ln(g->lmath, WORST_SCORE);

    assert((n_top > 0) && (n_top <= g->n_density));

    /* Allocate temporary space for distance computation, if necessary */
    if (n_dist < n_top) {
        if (n_dist > 0)
            ckd_free(dist);
        n_dist = n_top;
        dist = (dist_t *) ckd_calloc(n_dist, sizeof(dist_t));
    }

    for (f = 0; f < g->n_feat; f++) {
        compute_dist(dist, n_top,
                     obs[f], g->featlen[f],
                     g->mean[mgau][f], g->var[mgau][f], g->det[mgau][f],
                     g->n_density);

        /*
         * Convert distances to logs3 domain and return result.  Remember that until now,
         * we've been computing log(DENOMINATOR) of the normal density function.
         * Check for underflow before converting to (int32)logs3 value.
         */
        for (t = 0; t < n_top; t++) {
            out_dist[f][t].id = dist[t].id;

            dist[t].dist = -dist[t].dist;       /* log(numerator) = -log(denom.) */
            if (dist[t].dist < min_density) {
                dist[t].dist = min_density;
            }
            out_dist[f][t].dist = logmath_ln_to_log(g->lmath, dist[t].dist);
        }
    }

    return 0;
}
#endif

/*
 * Normalize density values, but globally.
 */
static int32
gauden_dist_norm_global(gauden_t * g,
                        int32 n_top, gauden_dist_t *** dist,
                        uint8 * active)
{
    int32 gid, f, t;
    int32 best;

    best = WORST_SCORE;

    for (gid = 0; gid < g->n_mgau; gid++) {
        if ((!active) || active[gid]) {
            for (f = 0; f < g->n_feat; f++) {
                for (t = 0; t < n_top; t++)
                    if (best < dist[gid][f][t].dist)
                        best = dist[gid][f][t].dist;
            }
        }
    }

    for (gid = 0; gid < g->n_mgau; gid++) {
        if ((!active) || active[gid]) {
            for (f = 0; f < g->n_feat; f++) {
                for (t = 0; t < n_top; t++)
                    dist[gid][f][t].dist -= best;
            }
        }
    }

    return (best * g->n_feat);  /* Scale factor applied to EVERY senone score */
}


/*
 * Normalize density values.
 */
int32
gauden_dist_norm(gauden_t * g, int32 n_top, gauden_dist_t *** dist,
                 uint8 * active)
{
    int32 gid, f, t;
    int32 sum, scale;

    if (g->n_mgau > 1) {
        /* Normalize by subtracting max(density values) from each density */
        return (gauden_dist_norm_global(g, n_top, dist, active));
    }

    /* Normalize by subtracting log(sum of density values) from each density */
    gid = 0;
    scale = 0;
    for (f = 0; f < g->n_feat; f++) {
        sum = dist[gid][f][0].dist;
        for (t = 1; t < n_top; t++)
	    sum = logmath_add(g->lmath, sum, dist[gid][f][t].dist);

        for (t = 0; t < n_top; t++)
            dist[gid][f][t].dist -= sum;

        scale += sum;
    }

    return scale;               /* Scale factor applied to EVERY senone score */
}
