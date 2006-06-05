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
 * Semi-continuous codebook read routines
 *
 * HISTORY
 * 
 * $Log: sc_cbook_r.c,v $
 * Revision 1.1.1.1  2006/05/23 18:45:01  dhuggins
 * re-importation
 *
 * Revision 1.7  2004/12/10 16:48:56  rkm
 * Added continuous density acoustic model handling
 *
 * 
 * 19-Nov-97  M K Ravishankar (rkm@cs) at Carnegie-Mellon University
 * 	Added ability to read power variance file if it exists.
 * 
 * 08-Oct-91  Eric Thayer (eht) at Carnegie-Mellon University
 *	Created from system by Xuedong Huang
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "s2types.h"
#include "log.h"
#include "scvq.h"
#include "cepio.h"
#include "sc_vq_internal.h"
#include "ckd_alloc.h"
#include "bio.h"
#include "err.h"
#include "cont_mgau.h"

#define MGAU_PARAM_VERSION	"1.0"	/* Sphinx-3 file format version for mean/var */

static int32 fLenMap[NUM_FEATURES] = {
    CEP_VECLEN, DCEP_VECLEN, POW_VECLEN, CEP_VECLEN
    };

/*
 * reads either a raw .vec (mean) file or a raw .var (variance) file.
 * more computation is done after a raw .var is read.
 */
static int32 readCBFile(feat_t feat, float32 **CB, char *CBFile)
{
  int32 len;

  /* do some sanity checks on the args. Define NDEBUG to disable */
  assert(CB != NULL);
  assert(CBFile != NULL);
  if (cep_read_bin(CB, &len, CBFile)) {
    perror ("readCBFile:");
    fprintf(stderr, "readCBFile: couldn't read codebook %s\n", CBFile);
    return -1;
  }
#ifndef NDEBUG
  /* sanity check codebook size */
  switch (feat) {
  case CEP_FEAT:
    assert(len / (sizeof(float32)*CEP_VECLEN) == NUM_ALPHABET);
    break;
  case DCEP_FEAT:
    assert(len / (sizeof(float32)*DCEP_VECLEN) == NUM_ALPHABET);
    break;
  case DDCEP_FEAT:
    assert(len / (sizeof(float32)*CEP_VECLEN) == NUM_ALPHABET);
    break;
  case POW_FEAT:
    assert(len / (sizeof(float32)*POW_VECLEN) == NUM_ALPHABET);
    break;
  }
#endif
  return len / sizeof(float32);
}

static float64 vFloor = DEF_VAR_FLOOR;	/* variance floor */

void setVarFloor(double aVal)
{
  vFloor = aVal;
}

/* no processing on mean file, just read it */
int32 readMeanCBFile(feat_t feat, mean_t **CB, char *MeanCBFile)
{
  int len;
#ifdef FIXED_POINT
  int i;
  float32 *CBPf;
  fixed32 *CBPi;
#endif

  /* do some sanity checks on the args. Define NDEBUG to disable */
  assert(((int32)feat < NUM_FEATURES) && ((int32)feat >= 0));
  assert(MeanCBFile != NULL);
  assert(CB != NULL);
  
  if ((len = readCBFile(feat, (float32 **)CB, MeanCBFile)) < 0) return -1;
#ifdef FIXED_POINT
  /* convert means to fixed-point */
  CBPf = *(float32 **)CB;
  CBPi = *CB;
  for (i = 0; i < len; ++i, ++CBPi, ++CBPf)
    *CBPi = FLOAT2FIX(*CBPf);
#endif
  return 0;
}

static double two_pi = 6.2831852;	/* almost M_PI * 2.0; */

/*
 * Arguments:
 * 	feat	- used to identify parameters for a given feature.
 *	det	- det array for the feature (NUM_ALPHABET in length)
 *	CB	- integer array for the feature codebook.
 *	VarCBFile - path to raw var CB for the given feature, feat.
 */
int32 readVarCBFile(feat_t feat, int32 *det, var_t **CB, char *VarCBFile)
{
    int32 vecLen, i, j;
    int32 d;
    var_t *CBP;

    /* do some sanity checks on the args. Define NDEBUG to disable */
    assert(((int32)feat < NUM_FEATURES) && ((int32)feat >= 0));
    assert(det != NULL);
    assert(CB != NULL);
    assert(VarCBFile != NULL);
    /* no automatically generated power variance file.  see setPowVar(). */

    if (readCBFile(feat, (float32 **)CB, VarCBFile) < 0) return -1;
    CBP = *CB;
    vecLen = fLenMap[(int32)feat];
    if (feat != DCEP_FEAT) {
	for (i = 0; i < NUM_ALPHABET; i++) {
	    d = 0;
	    /* Discard C0, but don't discard POW_FEAT[0]!
	       dhuggins@cs.cmu.edu, 2005-08-27 */
	    if (feat == POW_FEAT)
	      j = 0;
	    else {
	      j = 1;
	      *CBP++ = 0;
	    }
	    for (; j < vecLen; j++, CBP++) {
		float64 fvar;

		/* Always do these pre-calculations in floating point */
		fvar = *(float32 *)CBP;
		if (fvar < vFloor) fvar = vFloor;
		d += LOG (1 / sqrt(fvar * two_pi));
		*CBP = (var_t)(1.0 / (2.0 * fvar * LOG_BASE));
	    }
	    *det++ = d;
	}
    }
    else {
	for (i = 0; i < NUM_ALPHABET; i++) {
	    d = 0;
	    for (j = 1, *CBP++ = 0; j < vecLen; j++, CBP++) {
		float64 fvar;

		fvar = *(float32 *)CBP;
#if 0
		/* This is a hack from xdh. no int32er used.
		 * might be used in the future. Should parameterize.
		 */
		if (j > CEP_VECLEN) fvar /= DCEP_LONGWEIGHT;
#endif
		if (fvar < vFloor) fvar = vFloor;
		d += LOG (1.0 / sqrt(fvar * two_pi));
		*CBP = (var_t)(1.0 / (2.0 * fvar * LOG_BASE));
	    }
	    *det++ = d;
	}
    }
    return 0;
}

#define POWER_VARIANCE 0.05

int32 setPowVar(int32 *det, var_t **CB, double pow_var)
{
  var_t *CBP;
  int32 i = NUM_ALPHABET;

  /* do some sanity checks on the args. Define NDEBUG to disable */
  assert(CB != NULL);
  assert(det != NULL);

  CBP = *CB = (var_t *) malloc(POW_VECLEN*sizeof(var_t)*NUM_ALPHABET);
  if (CBP == NULL) {
    perror("readCBFiles: allocating power variances vectors\n");
    return -1;
  }
  do {
#ifdef NO_DIV_POWVAR_BY_2
    *CBP++ = (var_t)(pow_var/LOG_BASE);
    *CBP++ = (var_t)(1.0/LOG_BASE);
    *CBP++ = (var_t)(0.125/LOG_BASE);
    *det++ = LOG(pow_var * 1.0 * 0.125);
#else
    *CBP++ = (var_t)(pow_var/(2.0 * LOG_BASE));
    *CBP++ = (var_t)(1.0/(2.0 * LOG_BASE));
    *CBP++ = (var_t)(0.125/(2.0 * LOG_BASE));
    *det++ = LOG(pow_var * 1.0 * 0.125)/2.0;
#endif
  } while (--i);

  return 0;
}

/* Read a Sphinx3 mean or variance file. */
int32
s3_read_mgau(char *file_name, float32 **cb)
{
    char tmp;
    FILE *fp;
    int32 i, blk, n;
    int32 n_mgau;
    int32 n_feat;
    int32 n_density;
    int32 veclen[4];
    int32 byteswap, chksum_present;
    char **argname, **argval;
    uint32 chksum;
    
    E_INFO("Reading S3 mixture gaussian file '%s'\n", file_name);
    
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
    if (n_mgau != 1)
	E_FATAL("%s: #codebooks (%d) != 1\n", file_name, n_mgau);
    
    /* #Features/codebook */
    if (bio_fread (&n_feat, sizeof(int32), 1, fp, byteswap, &chksum) != 1)
	E_FATAL("fread(%s) (#features) failed\n", file_name);
    if (n_feat != 4)
	E_FATAL("#Features streams(%d) != 4\n", n_feat);
    
    /* #Gaussian densities/feature in each codebook */
    if (bio_fread (&n_density, sizeof(int32), 1, fp, byteswap, &chksum) != 1)
	E_FATAL("fread(%s) (#density/codebook) failed\n", file_name);
    if (n_density != NUM_ALPHABET)
	E_FATAL("%s: Number of densities per feature(%d) != %d\n",
		file_name, n_mgau, NUM_ALPHABET);
    
    /* Vector length of feature stream */
    if (bio_fread (&veclen, sizeof(int32), 4, fp, byteswap, &chksum) != 4)
	E_FATAL("fread(%s) (feature vector-length) failed\n", file_name);
    for (i = 0, blk = 0; i < 4; ++i)
	blk += veclen[i];
    
    /* #Floats to follow; for the ENTIRE SET of CODEBOOKS */
    if (bio_fread (&n, sizeof(int32), 1, fp, byteswap, &chksum) != 1)
	E_FATAL("fread(%s) (total #floats) failed\n", file_name);
    if (n != n_mgau * n_density * blk)
	E_FATAL("%s: #float32s(%d) doesn't match dimensions: %d x %d x %d\n",
		file_name, n, n_mgau, n_density, blk);

    /* This gets a bit tricky, as SCVQ expects C0 to exist in
     * the three cepstra streams even though it isn't used (this can
     * be fixed but isn't yet). */
    for (i = 0; i < 4; ++i) {
	int j;

	cb[i] = (float32 *) ckd_calloc (NUM_ALPHABET * fLenMap[i], sizeof(float32));

	if (veclen[i] == fLenMap[i]) {/* Not true except for POW_FEAT */
	    if (bio_fread(cb[i], sizeof(float32), NUM_ALPHABET * fLenMap[i],
			  fp, byteswap, &chksum) != NUM_ALPHABET * fLenMap[i])
		E_FATAL("fread(%s, %d) of feat %d failed\n", file_name,
			NUM_ALPHABET * fLenMap[i], i);
	}
	else if (veclen[i] < fLenMap[i]) {
	    for (j = 0; j < NUM_ALPHABET; ++j) {
		if (bio_fread((cb[i] + j * fLenMap[i]) + (fLenMap[i] - veclen[i]),
			      sizeof(float32), veclen[i],
			      fp, byteswap, &chksum) != veclen[i])
		    E_FATAL("fread(%s, %d) in feat %d failed\n", file_name,
			    veclen[i], i);
	    }
	}
	else
	    E_FATAL("%s: feature %d length %d is not <= expected %d\n",
		    file_name, i, veclen[i], fLenMap[i]);
    }

    if (chksum_present)
	bio_verify_chksum (fp, byteswap, chksum);

    if (fread (&tmp, 1, 1, fp) == 1)
	E_FATAL("%s: More data than expected\n", file_name);
    
    fclose(fp);
    
    E_INFO("%d mixture Gaussians, %d components, veclen %d\n", n_mgau, n_density, blk);
    
    return n;
}

int32
s3_precomp(mean_t **means, var_t **vars, int32 **dets)
{
    int feat;

    for (feat = 0; feat < 4; ++feat) {
	float32 *fmp;
	mean_t *mp;
	var_t *vp;
	int32 *dp, vecLen, i;
	
	vecLen = fLenMap[(int32)feat];
	fmp = (float32 *)means[feat];
	mp = means[feat];
	vp = vars[feat];
	dp = dets[feat];

	for (i = 0; i < NUM_ALPHABET; ++i) {
	    int32 d, j;

	    d = 0;
	    for (j = 0; j < vecLen; ++j, ++vp, ++mp, ++fmp) {
		float64 fvar;

#ifdef FIXED_POINT
		*mp = FLOAT2FIX(*fmp);
#endif
		/* Omit C0 for cepstral features (but not pow_feat!) */
		if (j == 0 && feat != POW_FEAT) {
		    *vp = 0;
		    continue;
		}

		/* Always do these pre-calculations in floating point */
		fvar = *(float32 *)vp;
		if (fvar < vFloor) fvar = vFloor;
		d += LOG (1 / sqrt(fvar * two_pi));
		*vp = (var_t)(1.0 / (2.0 * fvar * LOG_BASE));
	    }
	    *dp++ = d;
	}
    }
    return 0;
}
