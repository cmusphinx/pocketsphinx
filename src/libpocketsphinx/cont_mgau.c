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
 * cont_mgau.c -- Mixture Gaussians for continuous HMM models.
 *
 * **********************************************
 * CMU ARPA Speech Project
 *
 * Copyright (c) 1997 Carnegie Mellon University.
 * ALL RIGHTS RESERVED.
 * **********************************************
 *
 * HISTORY
 * 
 * $Log: cont_mgau.c,v $
 * Revision 1.1.1.1  2006/05/23 18:44:59  dhuggins
 * re-importation
 *
 * Revision 1.1  2004/12/10 16:48:56  rkm
 * Added continuous density acoustic model handling
 *
 * 
 * 22-Nov-2004	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Imported from s3.2, for supporting s3 format continuous
 * 		acoustic models.
 * 
 * 15-Dec-1999	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Added mgau_var_nzvec_floor().
 * 
 * 28-Mar-1999	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University.
 * 		Started.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "s2types.h"
#include "s3types.h"
#include "err.h"
#include "ckd_alloc.h"
#include "bio.h"
#include "vector.h"
#include "logs3.h"
#include "cont_mgau.h"


#define MGAU_PARAM_VERSION	"1.0"	/* Sphinx-3 file format version for mean/var */
#define MGAU_MIXW_VERSION	"1.0"	/* Sphinx-3 file format version for mixw */
#define MGAU_MEAN		1
#define MGAU_VAR		2

/*
 * Sphinx-3 model mean and var files have the same format.  Use this routine for reading
 * either one.
 */
static int32 mgau_file_read(mgau_model_t *g, char *file_name, int32 type)
{
    char tmp;
    FILE *fp;
    int32 i, k, n;
    int32 n_mgau;
    int32 n_feat;
    int32 n_density;
    int32 veclen;
    int32 byteswap, chksum_present;
    float32 *buf, **pbuf;
    char **argname, **argval;
    uint32 chksum;
    
    E_INFO("Reading mixture gaussian file '%s'\n", file_name);
    
    if ((fp = fopen (file_name, "rb")) == NULL)
      E_FATAL("fopen(%s,rb) failed\n", file_name);
    
    /* Read header, including argument-value info and 32-bit byteorder magic */
    if (bio_readhdr (fp, &argname, &argval, &byteswap) < 0)
	E_FATAL("bio_readhdr(%s) failed\n", file_name);
    
    /* Parse argument-value list */
    chksum_present = 0;
    for (i = 0; argname[i]; i++) {
	if (strcmp (argname[i], "version") == 0) {
	    if (strcmp(argval[i], MGAU_PARAM_VERSION) != 0)
		E_WARN("Version mismatch(%s): %s, expecting %s\n",
		       file_name, argval[i], MGAU_PARAM_VERSION);
	} else if (strcmp (argname[i], "chksum0") == 0) {
	    chksum_present = 1;	/* Ignore the associated value */
	}
    }
    bio_hdrarg_free (argname, argval);
    argname = argval = NULL;
    
    chksum = 0;
    
    /* #Codebooks */
    if (bio_fread (&n_mgau, sizeof(int32), 1, fp, byteswap, &chksum) != 1)
	E_FATAL("fread(%s) (#codebooks) failed\n", file_name);
    if (n_mgau >= MAX_S3MGAUID)
	E_FATAL("%s: #Mixture Gaussians (%d) exceeds limit(%d) enforced by MGAUID type\n",
		file_name, n_mgau, MAX_S3MGAUID);
    
    /* #Features/codebook */
    if (bio_fread (&n_feat, sizeof(int32), 1, fp, byteswap, &chksum) != 1) 
	E_FATAL("fread(%s) (#features) failed\n", file_name);
    if (n_feat != 1) {
	/* Make this non-fatal, so that we can detect semi-continuous models. */
	E_WARN("#Features streams(%d) != 1, assuming semi-continuous models\n", n_feat);
	fclose(fp);
	return -1;
    }
    
    /* #Gaussian densities/feature in each codebook */
    if (bio_fread (&n_density, sizeof(int32), 1, fp, byteswap, &chksum) != 1)
	E_FATAL("fread(%s) (#density/codebook) failed\n", file_name);
    
    /* Vector length of feature stream */
    if (bio_fread (&veclen, sizeof(int32), 1, fp, byteswap, &chksum) != 1)
	E_FATAL("fread(%s) (feature vector-length) failed\n", file_name);
    
    /* #Floats to follow; for the ENTIRE SET of CODEBOOKS */
    if (bio_fread (&n, sizeof(int32), 1, fp, byteswap, &chksum) != 1)
	E_FATAL("fread(%s) (total #floats) failed\n", file_name);
    if (n != n_mgau * n_density * veclen) {
	E_FATAL("%s: #float32s(%d) doesn't match dimensions: %d x %d x %d\n",
		file_name, n, n_mgau, n_density, veclen);
    }
    
    if (type == MGAU_MEAN) {
	/* Allocate memory for mixture gaussian densities */
	g->n_mgau = n_mgau;
	g->max_comp = n_density;
	g->veclen = veclen;
	g->mgau = (mgau_t *) ckd_calloc (n_mgau, sizeof(mgau_t));
	
	buf = (float32 *) ckd_calloc (n, sizeof(float));
	pbuf = (float32 **) ckd_calloc (n_mgau * n_density, sizeof(float32 *));
	
	for (i = 0; i < n_mgau; i++) {
	    g->mgau[i].n_comp = n_density;
	    g->mgau[i].mean = pbuf;
	    
	    for (k = 0; k < n_density; k++) {
		g->mgau[i].mean[k] = buf;
		buf += veclen;
	    }
	    pbuf += n_density;
	}
	
	buf = g->mgau[0].mean[0];	/* Restore buf to original value */
    } else {
	assert (type == MGAU_VAR);
	
	if (g->n_mgau != n_mgau)
	    E_FATAL("#Mixtures(%d) doesn't match that of means(%d)\n", n_mgau, g->n_mgau);
	if (g->max_comp != n_density)
	    E_FATAL("#Components(%d) doesn't match that of means(%d)\n", n_density, g->max_comp);
	if (g->veclen != veclen)
	    E_FATAL("#Vector length(%d) doesn't match that of means(%d)\n", veclen, g->veclen);
	
	buf = (float32 *) ckd_calloc (n, sizeof(float32));
	pbuf = (float32 **) ckd_calloc (n_mgau * n_density, sizeof(float32 *));
	
	for (i = 0; i < n_mgau; i++) {
	    if (g->mgau[i].n_comp != n_density)
		E_FATAL("Mixture %d: #Components(%d) doesn't match that of means(%d)\n",
			i, n_density, g->mgau[i].n_comp);
	    
	    g->mgau[i].var = pbuf;
	    
	    for (k = 0; k < n_density; k++) {
		g->mgau[i].var[k] = buf;
		buf += veclen;
	    }
	    pbuf += n_density;
	}
	
	buf = (float32 *) ckd_calloc (n_mgau * n_density, sizeof(float32));
	for (i = 0; i < n_mgau; i++) {
	    g->mgau[i].lrd = buf;
	    buf += n_density;
	}
	
	buf = g->mgau[0].var[0];	/* Restore buf to original value */
    }
    
    /* Read mixture gaussian densities data */
    if (bio_fread (buf, sizeof(float32), n, fp, byteswap, &chksum) != n)
	E_FATAL("fread(%s) (densitydata) failed\n", file_name);

    if (chksum_present)
	bio_verify_chksum (fp, byteswap, chksum);

    if (fread (&tmp, 1, 1, fp) == 1)
	E_FATAL("%s: More data than expected\n", file_name);
    
    fclose(fp);
    
    E_INFO("%d mixture Gaussians, %d components, veclen %d\n", n_mgau, n_density, veclen);
    
    return 0;
}


static int32 mgau_mixw_read(mgau_model_t *g, char *file_name, float64 mixwfloor)
{
    char **argname, **argval;
    char eofchk;
    FILE *fp;
    int32 byteswap, chksum_present;
    uint32 chksum;
    int32 *buf;
    float32 *pdf;
    int32 i, j, n;
    int32 n_mgau;
    int32 n_feat;
    int32 n_comp;
    int32 n_err;
    
    E_INFO("Reading mixture weights file '%s'\n", file_name);
    
    if ((fp = fopen (file_name, "rb")) == NULL)
      E_FATAL("fopen(%s,rb) failed\n", file_name);
    
    /* Read header, including argument-value info and 32-bit byteorder magic */
    if (bio_readhdr (fp, &argname, &argval, &byteswap) < 0)
	E_FATAL("bio_readhdr(%s) failed\n", file_name);
    
    /* Parse argument-value list */
    chksum_present = 0;
    for (i = 0; argname[i]; i++) {
	if (strcmp (argname[i], "version") == 0) {
	    if (strcmp(argval[i], MGAU_MIXW_VERSION) != 0)
		E_WARN("Version mismatch(%s): %s, expecting %s\n",
			file_name, argval[i], MGAU_MIXW_VERSION);
	} else if (strcmp (argname[i], "chksum0") == 0) {
	    chksum_present = 1;	/* Ignore the associated value */
	}
    }
    bio_hdrarg_free (argname, argval);
    argname = argval = NULL;

    chksum = 0;

    /* Read #senones, #features, #codewords, arraysize */
    if ((bio_fread (&n_mgau, sizeof(int32), 1, fp, byteswap, &chksum) != 1) ||
	(bio_fread (&n_feat, sizeof(int32), 1, fp, byteswap, &chksum) != 1) ||
	(bio_fread (&n_comp, sizeof(int32), 1, fp, byteswap, &chksum) != 1) ||
	(bio_fread (&n, sizeof(int32), 1, fp, byteswap, &chksum) != 1)) {
	E_FATAL("bio_fread(%s) (arraysize) failed\n", file_name);
    }
    if (n_feat != 1)
	E_FATAL("#Features streams(%d) != 1\n", n_feat);
    if (n != n_mgau * n_comp) {
	E_FATAL("%s: #float32s(%d) doesn't match header dimensions: %d x %d\n",
		file_name, i, n_mgau, n_comp);
    }
    if (n_mgau != g->n_mgau)
	E_FATAL("%s: #Mixture Gaussians(%d) doesn't match mean/var parameters(%d)\n",
		n_mgau, g->n_mgau);
    
    buf = (int32 *) ckd_calloc (n_mgau * n_comp, sizeof(int32));
    for (i = 0; i < n_mgau; i++) {
	if (n_comp != mgau_n_comp(g,i))
	    E_FATAL("Mixture %d: #Weights(%d) doesn't match #Gaussian components(%d)\n",
		    i, n_comp, mgau_n_comp(g,i));
	
	g->mgau[i].mixw = buf;
	buf += g->mgau[i].n_comp;
    }
    
    /* Temporary structure to read in floats before conversion to (int32) logs3 */
    pdf = (float32 *) ckd_calloc (n_comp, sizeof(float32));
    
    /* Read mixw data, normalize, floor, convert to logs3 */
    n_err = 0;
    for (i = 0; i < n_mgau; i++) {
	if (bio_fread((void *)pdf, sizeof(float32), n_comp, fp, byteswap, &chksum) != n_comp)
	    E_FATAL("bio_fread(%s) (arraydata) failed\n", file_name);
	
	/* Normalize and floor */
	if (vector_is_zero (pdf, n_comp)) {
	    n_err++;
	    for (j = 0; j < n_comp; j++)
		mgau_mixw(g,i,j) = S3_LOGPROB_ZERO;
	} else {
	    vector_nz_floor (pdf, n_comp, mixwfloor);
	    vector_sum_norm (pdf, n_comp);
	    for (j = 0; j < n_comp; j++)
		mgau_mixw(g,i,j) = (pdf[j] != 0.0) ? logs3(pdf[j]) : S3_LOGPROB_ZERO;
	}
    }
    if (n_err > 0)
	E_ERROR("Weight normalization failed for %d senones\n", n_err);

    ckd_free (pdf);

    if (chksum_present)
	bio_verify_chksum (fp, byteswap, chksum);
    
    if (fread (&eofchk, 1, 1, fp) == 1)
	E_FATAL("More data than expected in %s\n", file_name);

    fclose(fp);

    E_INFO("Read %d x %d mixture weights\n", n_mgau, n_comp);
    
    return 0;
}


/*
 * Compact each mixture Gaussian in the given model by removing any uninitialized components.
 * A component is considered to be uninitialized if its variance is the 0 vector.  Compact by
 * copying the data rather than moving pointers.  Otherwise, malloc pointers could get
 * corrupted.
 */
static void mgau_uninit_compact (mgau_model_t *g)
{
    int32 m, c, c2, n, nm;
    
    E_INFO("Removing uninitialized Gaussian densities\n");
    
    n = 0;
    nm = 0;
    for (m = 0; m < mgau_n_mgau(g); m++) {
	for (c = 0, c2 = 0; c < mgau_n_comp(g,m); c++) {
	    if (! vector_is_zero (mgau_var(g,m,c), mgau_veclen(g))) {
		if (c2 != c) {
		    memcpy (mgau_mean(g,m,c2), mgau_mean(g,m,c),
			    mgau_veclen(g) * sizeof(float32));
		    memcpy (mgau_var(g,m,c2), mgau_var(g,m,c),
			    mgau_veclen(g) * sizeof(float32));
		    mgau_mixw(g,m,c2) = mgau_mixw(g,m,c);
		}
		c2++;
	    } else {
		n++;
	    }
	}
	mgau_n_comp(g,m) = c2;
	if (c2 == 0) {
	    fprintf (stderr, " %d", m);
	    fflush (stderr);
	    nm++;
	}
    }
    if (nm > 0)
	fprintf (stderr, "\n");
    
    if ((nm > 0) || (n > 0))
	E_INFO ("%d densities removed (%d mixtures removed entirely)\n", n, nm);
}


static void mgau_var_floor (mgau_model_t *g, float64 floor)
{
    int32 m, c, i, n;
    
    E_INFO("Applying variance floor\n");
    
    n = 0;
    for (m = 0; m < mgau_n_mgau(g); m++) {
	for (c = 0; c < mgau_n_comp(g,m); c++) {
	    for (i = 0; i < mgau_veclen(g); i++) {
		if (g->mgau[m].var[c][i] < floor) {
		    g->mgau[m].var[c][i] = (float32) floor;
		    n++;
		}
	    }
	}
    }
    
    E_INFO("%d variance values floored\n", n);
}


int32 mgau_var_nzvec_floor (mgau_model_t *g, float64 floor)
{
    int32 m, c, i, n, l;
    float32 *var;
    
    E_INFO("Applying variance floor to non-zero variance vectors\n");
    
    l = mgau_veclen(g);
    
    n = 0;
    for (m = 0; m < mgau_n_mgau(g); m++) {
	for (c = 0; c < mgau_n_comp(g,m); c++) {
	    var = g->mgau[m].var[c];
	    
	    if (! vector_is_zero (var, l)) {
		for (i = 0; i < l; i++) {
		    if (var[i] < floor) {
			var[i] = (float32) floor;
			n++;
		    }
		}
	    }
	}
    }
    
    E_INFO("%d variance values floored\n", n);
    
    return n;
}


/*
 * Some of the Mahalanobis distance computation (between Gaussian density means and given
 * vectors) can be carried out in advance.  (See comment in .h file.)
 */
static int32 mgau_precomp (mgau_model_t *g)
{
    int32 m, c, i;
    float64 lrd;
    
    E_INFO("Precomputing Mahalanobis distance invariants\n");
    
    for (m = 0; m < mgau_n_mgau(g); m++) {
	for (c = 0; c < mgau_n_comp(g,m); c++) {
	    lrd = 0.0;
	    for (i = 0; i < mgau_veclen(g); i++) {
		lrd += log(g->mgau[m].var[c][i]);
		
		/* Precompute this part of the exponential */
		g->mgau[m].var[c][i] = (float32) (1.0 / (g->mgau[m].var[c][i] * 2.0));
	    }
	    
	    lrd += mgau_veclen(g) * log(2.0 * PI);	/* (2pi)^velen */
	    mgau_lrd(g,m,c) = (float32)(-0.5 * lrd);	/* Reciprocal, sqrt */
	}
    }
    
    return 0;
}


/* At the moment, S3 models have the same #means in each codebook and 1 var/mean */
mgau_model_t *mgau_init (char *meanfile, char *varfile, float64 varfloor,
			 char *mixwfile, float64 mixwfloor,
			 int32 precomp)
{
    mgau_model_t *g;
    
    assert (meanfile != NULL);
    
    g = (mgau_model_t *) ckd_calloc (1, sizeof(mgau_model_t));
    
    /* Read means and (diagonal) variances for all mixture gaussians */
    /* We also use this to detect semi-continuous models, in which
     * case we might not have a mixw file, so please keep the
     * assertions below this. */
    if (mgau_file_read (g, meanfile, MGAU_MEAN) == -1) {
	ckd_free(g);
	return NULL;
    }

    assert (varfile != NULL);
    assert (varfloor >= 0.0);
    mgau_file_read (g, varfile, MGAU_VAR);

    assert (mixwfile != NULL);
    assert (mixwfloor >= 0.0);
    mgau_mixw_read (g, mixwfile, mixwfloor);
    
    mgau_uninit_compact (g);		/* Delete uninitialized components */
    
    if (varfloor > 0.0)
	mgau_var_floor (g, varfloor);	/* Variance floor after above compaction */
    
    if (precomp)
	mgau_precomp (g);		/* Precompute Mahalanobis distance invariants */
    
    g->distfloor = logs3_to_log (S3_LOGPROB_ZERO);	/* Floor for Mahalanobis distance values */
    
    return g;
}


int32 mgau_comp_eval (mgau_model_t *g, int32 s, float32 *x, int32 *score)
{
    mgau_t *mgau;
    int32 veclen;
    float32 *m, *v;
    float64 dval, diff, f;
    int32 bs;
    int32 i, c;
    
    veclen = mgau_veclen(g);
    mgau = &(g->mgau[s]);
    f = log_to_logs3_factor();

    bs = MAX_NEG_INT32;
    for (c = 0; c < mgau->n_comp; c++) {
	m = mgau->mean[c];
	v = mgau->var[c];
	dval = mgau->lrd[c];
	
	for (i = 0; i < veclen; i++) {
	    diff = x[i] - m[i];
	    dval -= diff * diff * v[i];
	}
	
	if (dval < g->distfloor)
	    dval = g->distfloor;
	
	score[c] = (int32) (f * dval);
	if (score[c] > bs)
	    bs = score[c];
    }
    
    return bs;
}


int32 mgau_eval (mgau_model_t *g, int32 m, int32 *active, float32 *x)
{
    mgau_t *mgau;
    int32 veclen, score;
    float32 *m1, *m2, *v1, *v2;
    float64 dval1, dval2, diff1, diff2, f;
    int32 i, j, c;
    
    veclen = mgau_veclen(g);
    mgau = &(g->mgau[m]);
    f = log_to_logs3_factor();
    score = S3_LOGPROB_ZERO;
    
    if (! active) {	/* No short list; use all */
	for (c = 0; c < mgau->n_comp-1; c += 2) {	/* Interleave 2 components for speed */
	    m1 = mgau->mean[c];
	    m2 = mgau->mean[c+1];
	    v1 = mgau->var[c];
	    v2 = mgau->var[c+1];
	    dval1 = mgau->lrd[c];
	    dval2 = mgau->lrd[c+1];
	    
	    for (i = 0; i < veclen; i++) {
		diff1 = x[i] - m1[i];
		dval1 -= diff1 * diff1 * v1[i];
		diff2 = x[i] - m2[i];
		dval2 -= diff2 * diff2 * v2[i];
	    }
	    
	    if (dval1 < g->distfloor)	/* Floor */
		dval1 = g->distfloor;
	    if (dval2 < g->distfloor)
		dval2 = g->distfloor;
	    
	    score = logs3_add (score, (int32)(f * dval1) + mgau->mixw[c]);
	    score = logs3_add (score, (int32)(f * dval2) + mgau->mixw[c+1]);
	}
	
	/* Remaining iteration if n_mean odd */
	if (c < mgau->n_comp) {
	    m1 = mgau->mean[c];
	    v1 = mgau->var[c];
	    dval1 = mgau->lrd[c];
	    
	    for (i = 0; i < veclen; i++) {
		diff1 = x[i] - m1[i];
		dval1 -= diff1 * diff1 * v1[i];
	    }
	    
	    if (dval1 < g->distfloor)
		dval1 = g->distfloor;
	    
	    score = logs3_add (score, (int32)(f * dval1) + mgau->mixw[c]);
	}
    } else {
	for (j = 0; active[j] >= 0; j++) {
	    c = active[j];
	    
	    m1 = mgau->mean[c];
	    v1 = mgau->var[c];
	    dval1 = mgau->lrd[c];
	    
	    for (i = 0; i < veclen; i++) {
		diff1 = x[i] - m1[i];
		dval1 -= diff1 * diff1 * v1[i];
	    }
	    
	    if (dval1 < g->distfloor)
		dval1 = g->distfloor;
	    
	    score = logs3_add (score, (int32)(f * dval1) + mgau->mixw[c]);
	}
    }
    
    return score;
}
