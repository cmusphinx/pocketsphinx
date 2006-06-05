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
#include <stdlib.h>
#include <math.h>
#include "ckd_alloc.h"
#include "cdcn.h"
#include "cdcn_internal.h"

/*************************************************************************
 *
 * initialize finds an estimate of the noise vector as the average of all
 * frames whose power is below a threshold. It also computes the average
 * log-energy of the frames whose log-energy is above that threshold
 * (supposedly speech frames).
 * Coded by Alex Acero (acero@s),  November 1989 
 * Modified by Uday Jain, June 95
 *
 *************************************************************************/

static float
initialize (float **data,	/* The observation cepstrum vectors */
	    int	num_frames,		/* Number of frames in utterance */
	    float *noise,		/* Cepstrum vector for the noise */
	    float *tilt,
	    float speech_threshold,	/* Threshold for speech and noise */
	    float **mean,
	    float *prob,
	    float **var,
	    int ncodes,	/* Number of gaussians in distribution */
	    int Ndim	/* Dimensionality of data */
	)
{
	float	noise_ceiling,	/* Threshold to separate speech and noise */
		min,		/* Minimum log-energy in utterance */
		max,			/* Maximum log-energy in utterance */
		speech_power,	/* Average speech power */
		*codemean,
		*localprob;
	int		i,		/* Index all frames in utterance */
		j,		/* Index all coefficients within frame */
		noise_frames,	/* Number of noise frames */
		speech_frames;	/* Number of speech frames */

	codemean = (float *) ckd_calloc(Ndim, sizeof(float));
	localprob = (float *) ckd_calloc(ncodes, sizeof(float));

	/* De-normalize prob w.r.t variance */
	for (i=0;i<ncodes;++i)
	{
		localprob[i] = 1.0;
		for (j = 0; j < Ndim; ++j)
			localprob[i] *= var[i][j];
		localprob[i] = prob[i]*((float)sqrt(localprob[i]));
	}
	/* Initialize tilt */
	for (j = 0; j < Ndim; ++j) {
		tilt[j] = 0;
		codemean[j] = 0;
		for (i = 0; i < ncodes; ++i)
			codemean[j] += localprob[i] * mean[i][j];
	}
	for (i = 0; i < num_frames; ++i)
		for (j = 0; j < Ndim; ++j)
			tilt[j] += data[i][j];

	for (j = 0; j < Ndim; ++j)
		tilt[j] = tilt[j] / (float) num_frames - codemean[j];

	/* Search for the extrema c[0] in the file */
	min = max = data[0][0];
	for (i = 0; i < num_frames; i++) {
		if (data[i][0] < min)
			min = data[i][0];
		if (data[i][0] > max)
			max = data[i][0];
	}

	/* Compute thresholds for noise */
	noise_ceiling = min + (max - min) / 20;

	/* Every frame whose power is below noise_ceiling is considered noise.
	   and every frame above is considered speech */
	noise_frames = 0;
	speech_frames = 0;
	speech_power = 0;
	for (j = 0; j < Ndim; j++)
		noise[j] = 0.0;
	for (i = 0; i < num_frames; i++) {
		if (data[i][0] < noise_ceiling) {
			noise_frames++;
			for (j = 0; j < Ndim; j++)
				noise[j] += data[i][j];
		} else {
			speech_frames++;
			speech_power += data[i][0];
		}
	}
	for (j = 0; j < Ndim; j++)
		noise[j] /= noise_frames;
	speech_power /= speech_frames;

	ckd_free(localprob);
	ckd_free(codemean);

	return (speech_power);
}

/*************************************************************************
 *
 * This function computes ln[exp(x) + 1] which is the non-linear function
 * necessary for the noise subtraction in the cepstral domain.
 *
 * Coded by Alex Acero (acero@s),  November 1989 
 *
 *************************************************************************/

float f1(float x)
{
	float y;

	/* Omit cast to (float) because uClibc lacks logf() - dhuggins@cs */
	y = log(exp((double) x) + 1.0);
	return (y);
}


/*************************************************************************
 *
 * Subroutine correction computes the correction cepstrum vectors for a 
 * codebook given the spectral tilt and the noise cepstrum vectors.
 * For every codeword it finds the corresponding correction vector. That
 * is, every cepstrum vector within a cluster is transformed differently.
 * Coded by Alex Acero (acero@s),  November 1989 
 * Modified by PJM to correct bug in 0th mode correction factor estimate
 * Modified by Bhiksha to eliminate Harcoded Dimensionality
 *
 *************************************************************************/

static
void correction(float *tilt, 	/* The spectral tilt cepstrum vector */
		float *noise, 	/* The noise cepstrum vector */
		float **mean,   /* The means */
		float **corrbook,/* The correction cepstrum vectors */
		int num_codes,  /* The number of gaussians in the distribution */
		int Ndim	/* The dimensionality of the data */
    )
{
	float aux[N + 1];		/* auxiliary vector for FFTs */
	int i,			/* Index for all modes in distribution */
	 j;			/* Index for all coefficients within a frame */

	/* 
	 * The 0th correction is different from the rest of the corrections:
	 * Here we speak of the y = n + s() expansion rather than  y = x + q + r()
	 */
	/* Take direct FFT of -(noise - tilt - codeword) */
	for (j = 0; j < Ndim; j++)
		aux[j] = mean[0][j] + tilt[j] - noise[j];
	for (j = Ndim; j <= N; j++)
		aux[j] = 0.0;
	resfft(aux, N, M);
	/* Process every frequency through non-linear function f1 */
	for (j = 0; j <= N; j++)
		aux[j] = f1(aux[j]);

	/* Take inverse FFT and write result back */
	resfft(aux, N, M);
	for (j = 0; j < Ndim; j++)
		corrbook[0][j] = aux[j] / N2;

	for (i = 1; i < num_codes; i++) {
		/* Take direct FFT of noise - tilt - codeword */
		for (j = 0; j < Ndim; j++)
			aux[j] = noise[j] - tilt[j] - mean[i][j];
		for (j = Ndim; j <= N; j++)
			aux[j] = 0.0;
		resfft (aux, N, M);

		/* Process every frequency through the non-linear function */
		for (j = 0; j <= N; j++)
			aux[j] = f1(aux[j]);

		/* Take inverse FFT and write result back */
		resfft(aux, N, M);
		for (j = 0; j < Ndim; j++)
			corrbook[i][j] = aux[j] / N2;
	}
} 

/*************************************************************************
 *
 * max_q reestimates the tilt cepstrum vector that maximizes the likelihood.
 * It also labels the cleaned speech.
 * Coded by Alex Acero (acero@s),  November 1989 
 * Modified by Bhiksha June 94:
 *     Accounted for variance (originally ignored in the program!?!?!)
 * Modified by PJM to squash bug in noise estimation 1995
 * Modified by Bhiksha to eliminate hard coded dimensionality
 *
 *************************************************************************/

static
float max_q (float **variance,   /* Speech cepstral variances of the modes */
	     float *prob,       /* Ratio of a-priori probabilities of the codes 
				   and the mod of their variances*/
	     float *noise,      /* Cepstrum vector for the noise */
	     float *tilt,       /* Spectral tilt cepstrum */
	     float **mean,   /* The cepstrum mean */
	     float **corrbook,   /* The correction factor's codebook */
	     int  num_codes,    /* Number of codewords in codebook */
	     float **z,          /* The input cepstrum */
	     int num_frames,    /* Number of frames in utterance */
	     int Ndim		/* Dimensionality of data */
	)
{
	float    *newtilt,  /* The new tilt vector */
		*newnoise, /* The new noise vector */
		distance,    /* distance value */
		loglikelihood,    /* The log-likelihood */
		probz,         /* Prob. of z given tilt/noise */
		dennoise,    /* Denominator of noise estimation */
		dentilt,    /* Denominator of tilt estimation */
		pnoise,        /* Probability that frame is noise */
		*qk,		/* An auxiliary vector */
		*qi,  /* Contribution to tilt of frame i */
		*ni,  /* Contribution to noise of frame i */
		fk;        /* Probabilities for different codewords */
	int i,        /* Index frames in utterance */
		j,        /* Index coefficients within frame */
		k;    /* Index codewords in codebook */

	newtilt = (float *) ckd_calloc(Ndim, sizeof(float));
	newnoise = (float *) ckd_calloc(Ndim, sizeof(float));
	qk = (float *) ckd_calloc(Ndim, sizeof(float));
	qi = (float *) ckd_calloc(Ndim, sizeof(float));
	ni = (float *) ckd_calloc(Ndim, sizeof(float));

	/* Initialize newtilt and newnoise */
	for (j = 0; j < Ndim; j++) {
		newtilt[j] = 0.0;
		newnoise[j] = 0.0;
	}
	loglikelihood = 0.0;
	dennoise = 0.0;
	dentilt = 0.0;

	/* Reestimate tilt vector for all frames in utterance */
	for (i = 0; i < num_frames; i++) {
		/* Reestimate noise vector for codeword 0 */
		for (j = 0; j < Ndim; j++)
			qk[j] = z[i][j] - corrbook[0][j];
		distance = dist(qk, noise, variance[0], Ndim);
		fk = (float)exp((double) -distance / 2) * prob[0];
		probz = fk;
		pnoise = fk;
		for (j = 0; j < Ndim; j++) {
			ni[j] = qk[j] * fk;
			qi[j] = 0.0;
		}

		/* Reestimate tilt vector across all codewords */
		for (k = 1; k < num_codes; k++) {
			/* Restimate tilt vector for codeword k */
			for (j = 0; j < Ndim; j++)
				qk[j] =
				    z[i][j] - mean[k][j] - corrbook[k][j];
			distance = dist(qk, tilt, variance[k], Ndim);
			fk = (float)exp((double) -distance / 2) * prob[k];
			probz += fk;
			for (j = 0; j < Ndim; j++)
				qi[j] += qk[j] * fk;
		}
		if (probz != 0.0) {
			/*
			 * Earlier the sign in the loglikelihood used to be negative!! : PJM
			 */
			/* Omit cast to (float) because uClibc lacks logf() - dhuggins@cs */
			loglikelihood += log((double) probz);
			pnoise /= probz;
			dennoise += pnoise;
			dentilt += (1 - pnoise);
			for (j = 0; j < Ndim; j++) {
				newnoise[j] += ni[j] / probz;
				newtilt[j] += qi[j] / probz;
			}
		}
	}

	/* Normalize the estimated tilt vector across codewords */
	for (j = 0; j < Ndim; j++) {
		if (dennoise != 0)
			noise[j] = newnoise[j] / dennoise;
		if (dentilt != 0)
			tilt[j] = newtilt[j] / dentilt;
	}
	loglikelihood /= num_frames;
	/* 
	 * we deactivate this part of the code since it is not needed
	 *
	 * loglikelihood += OFFSET_LIKELIHOOD  ; 
	 *
	 */

	ckd_free(newtilt);
	ckd_free(newnoise);
	ckd_free(qk);
	ckd_free(qi);
	ckd_free(ni);

	return (loglikelihood);
}

/*************************************************************************
 *
 * cdcn_update finds the vectors x, noise
 * and tilt that maximize the a posteriori probability.
 * only one iteration is performed.  this routine can be recalled to 
 * perform multiple iterations if cycles permit.
 * Coded by Alex Acero (acero@s),  November 1989 
 * Modified by Uday Jain, June 95
 *
 *************************************************************************/

float
cdcn_update (float **z,		/* The observed cepstrum vectors */
	     int num_frames,	/* Number of frames in utterance */
	     CDCN_type *cdcn_variables)
{	
	float       distortion;
	float	*noise, *tilt, **means, *prob, **variance, **corrbook;
	int 	num_codes;

	/*
	 * If error, dont bother
	 */
	if (!cdcn_variables->run_cdcn)
		return((float)-1e+30);
        
	/*
	 * Open suitcase
	 */

	noise	= cdcn_variables->noise;
	tilt	= cdcn_variables->tilt;
	means 	= cdcn_variables->means;
	prob	= cdcn_variables->probs;
	variance	= cdcn_variables->variance;
	corrbook	= cdcn_variables->corrbook;
	num_codes	= cdcn_variables->num_codes;

	/*
	 * Initialize if this is the first time the routine is being called
	 */
	if (cdcn_variables->firstcall)
	{
		/* Get initial estimates for noise, tilt, x, y */
		initialize (z,num_frames,noise,tilt,SPEECH_THRESHOLD,means,
			    prob,variance,num_codes, NUM_COEFF);
		correction (tilt, noise, means, corrbook, num_codes, NUM_COEFF);
		cdcn_variables->firstcall = FALSE;
	}

	/*
	 * Compute the correction terms for the means 
	 * Perform one iteration of the estimation of n and q
	 */ 
	distortion = max_q (variance, prob, noise, tilt, means, corrbook, 
			    num_codes, z, num_frames, NUM_COEFF);

	correction (tilt, noise, means, corrbook, num_codes, NUM_COEFF);  
	return (distortion);
}

/*************************************************************************
 *
 * cdcn_converged_update finds the vectors x, noise and tilt that
 * maximize the a posteriori probability, iterating until convergence
 * or maximum number of iterations is reached.
 *
 *************************************************************************/

void
cdcn_converged_update (float **z,		/* The observed cepstrum vectors */
	     int num_frames,	/* Number of frames in utterance */
		       CDCN_type *cdcn_variables,
		       int max_iteration /* max number of iterations, one if you want just one iteration */)
{	
  /* Arbitrary number, has to be less than distortion */
	float       last_distortion = -20.0f;
	float       distortion = -10.0f;
	int iteration = 0;

	/*
	 * If error, dont bother
	 */
	if (!cdcn_variables->run_cdcn)
	  return;
        
	while ((iteration < max_iteration) && (distortion - last_distortion > 0)) {
	  last_distortion = distortion;
	  distortion = cdcn_update(z, num_frames, cdcn_variables);
	  iteration++;
	}
}
