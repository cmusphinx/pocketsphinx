/* ====================================================================
 * Copyright (c) 1996-2004 Carnegie Mellon University.  All rights 
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

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "s2types.h"
#include "fixpoint.h"
#include "fe.h"
#include "log.h"
#include "fe_internal.h"

/*
  31 Jan 00 mseltzer - changed rounding of filter edges to -not- use 
                        rint() function. 
   3 Dec 99 mseltzer - corrected inverse DCT-2 
                        period is 1/NumFilts not 1/(2*NumFilts)
			added "beta" factor in summation
	  	     - changed mel filter bank construction so that 
                        left,right,center freqs are rounded to DFT 
                        points before filter is constructed
  
*/
                        

int32 fe_build_melfilters(melfb_t *MEL_FB)
{    
    int32 i, whichfilt, start_pt;
    float32 leftfr, centerfr, rightfr, fwidth, height, *filt_edge;
    float32 melmax, melmin, dmelbw, freq, dfreq, leftslope,rightslope;

    /*estimate filter coefficients*/
    MEL_FB->filter_coeffs = (mfcc_t **)fe_create_2d(MEL_FB->num_filters,
						    MEL_FB->fft_size, sizeof(mfcc_t));
    MEL_FB->left_apex = (mfcc_t *) calloc(MEL_FB->num_filters,sizeof(mfcc_t));
    MEL_FB->width = (int32 *) calloc(MEL_FB->num_filters,sizeof(int32));
    
    if (MEL_FB->doublewide==ON)
	filt_edge = (float32 *) calloc(MEL_FB->num_filters+4,sizeof(float32));
    else	
	filt_edge = (float32 *) calloc(MEL_FB->num_filters+2,sizeof(float32));

    if (MEL_FB->filter_coeffs==NULL || MEL_FB->left_apex==NULL
	|| MEL_FB->width==NULL || filt_edge==NULL){
	fprintf(stderr,"memory alloc failed in fe_build_mel_filters()\n...exiting\n");
	exit(0);
    }
    
    dfreq = MEL_FB->sampling_rate/(float32)MEL_FB->fft_size;
    
    melmax = fe_mel(MEL_FB->upper_filt_freq);
    melmin = fe_mel(MEL_FB->lower_filt_freq);
    dmelbw = (melmax-melmin)/(MEL_FB->num_filters+1);

    if (MEL_FB->doublewide==ON){
	melmin = melmin-dmelbw;
	melmax = melmax+dmelbw;
	if ((fe_melinv(melmin)<0) ||
	    (fe_melinv(melmax)>MEL_FB->sampling_rate/2)){
	    fprintf(stderr,"Out of Range: low  filter edge = %f (%f)\n",fe_melinv(melmin),0.0);
	    fprintf(stderr,"              high filter edge = %f (%f)\n",fe_melinv(melmax),MEL_FB->sampling_rate/2);
	    fprintf(stderr,"exiting...\n");
	    exit(0);
	}
    }    
    
    if (MEL_FB->doublewide==ON){
        for (i=0;i<=MEL_FB->num_filters+3; ++i){
	    filt_edge[i] = fe_melinv(i*dmelbw + melmin);
        }
    }
    else {
	for (i=0;i<=MEL_FB->num_filters+1; ++i){
	    filt_edge[i] = fe_melinv(i*dmelbw + melmin);   
	}
    }
    
    for (whichfilt=0;whichfilt<MEL_FB->num_filters; ++whichfilt) {
      /*line triangle edges up with nearest dft points... */
      if (MEL_FB->doublewide==ON){
        leftfr   = (float32)((int32)((filt_edge[whichfilt]/dfreq)+0.5))*dfreq;
        centerfr = (float32)((int32)((filt_edge[whichfilt+2]/dfreq)+0.5))*dfreq;
        rightfr  = (float32)((int32)((filt_edge[whichfilt+4]/dfreq)+0.5))*dfreq;
      }else{
        leftfr   = (float32)((int32)((filt_edge[whichfilt]/dfreq)+0.5))*dfreq;
        centerfr = (float32)((int32)((filt_edge[whichfilt+1]/dfreq)+0.5))*dfreq;
        rightfr  = (float32)((int32)((filt_edge[whichfilt+2]/dfreq)+0.5))*dfreq;
      }
#ifdef FIXED_POINT
      MEL_FB->left_apex[whichfilt] = (int32)(leftfr + 0.5);
#else /* FIXED_POINT */
      MEL_FB->left_apex[whichfilt] = leftfr;
#endif /* FIXED_POINT */
      fwidth = rightfr - leftfr;
      
      /* 2/fwidth for triangles of area 1 */
      height = 2/(float32)fwidth;
      if (centerfr != leftfr) {
	leftslope = height/(centerfr-leftfr);
      }
      if (centerfr != rightfr) {
	rightslope = height/(centerfr-rightfr);
      }

      /* Round to the nearest integer instead of truncating and adding
         one, which breaks if the divide is already an integer */      
      start_pt = (int32)(leftfr/dfreq + 0.5);
      freq = (float32)start_pt*dfreq;
      i=0;
      
      while (freq<centerfr){
#ifdef FIXED_POINT
        MEL_FB->filter_coeffs[whichfilt][i] = 
	  LOG((freq-leftfr)*leftslope);
#else /* FIXED_POINT */
        MEL_FB->filter_coeffs[whichfilt][i] = 
	  FLOAT2MFCC((freq-leftfr)*leftslope);
#endif /* FIXED_POINT */
        freq += dfreq;
        i++;
      }
      /* If the two floats are equal, the leftslope computation above
         results in Inf, so we handle the case here. */
      if (freq==centerfr){
#ifdef FIXED_POINT
	MEL_FB->filter_coeffs[whichfilt][i] =
	  LOG(height);
#else /* FIXED_POINT */
	MEL_FB->filter_coeffs[whichfilt][i] =
	  FLOAT2MFCC(height);
#endif /* FIXED_POINT */
        freq += dfreq;
        i++;
      }
      while (freq<rightfr){
#ifdef FIXED_POINT
        MEL_FB->filter_coeffs[whichfilt][i] =
	  LOG((freq-rightfr)*rightslope);
#else /* FIXED_POINT */
        MEL_FB->filter_coeffs[whichfilt][i] =
	  FLOAT2MFCC((freq-rightfr)*rightslope);
#endif /* FIXED_POINT */
        freq += dfreq;
        i++;
      }
      MEL_FB->width[whichfilt] = i;
    }
    
    free(filt_edge);
    return(0);
}

int32 fe_compute_melcosine(melfb_t *MEL_FB)
{

    float32 period, freq;
    int32 i,j;
    
    period = (float32)2*MEL_FB->num_filters;

    if ((MEL_FB->mel_cosine = (mfcc_t **) fe_create_2d(MEL_FB->num_cepstra,MEL_FB->num_filters,
						       sizeof(mfcc_t)))==NULL){
	fprintf(stderr,"memory alloc failed in fe_compute_melcosine()\n...exiting\n");
	exit(0);
    }
    
    
    for (i=0; i<MEL_FB->num_cepstra; i++) {
	freq = 2*(float32)M_PI*(float32)i/period;
	for (j=0;j< MEL_FB->num_filters;j++) {
	    float64 cosine;

	    cosine = cos((float64)(freq*(j+0.5)));
	    MEL_FB->mel_cosine[i][j] = FLOAT2MFCC(cosine);
	}
    }    

    return(0);
}


float32 fe_mel(float32 x)
{
    return (float32)(2595.0*log10(1.0+x/700.0));
}

float32 fe_melinv(float32 x)
{
    return (float32)(700.0*(pow(10.0,x/2595.0) - 1.0));
}

void fe_pre_emphasis(int16 const *in, frame_t *out, int32 len,
		     float32 factor, int16 prior)
{
    int32 i;
#ifdef FIXED_POINT
#ifdef FIXED16
    int32 fxd_factor = (int32)(factor * (float32)(1<<DEFAULT_RADIX));
    out[0] = in[0] - ((int32)(prior * fxd_factor) >> DEFAULT_RADIX);

    for (i = 1; i < len; ++i) {
	out[i] = in[i] - ((int32)(in[i-1] * fxd_factor) >> DEFAULT_RADIX);
    }
#else
    int32 fxd_factor = (int32)(factor * (float32)(1<<DEFAULT_RADIX));
    out[0] = (int32)(in[0] << DEFAULT_RADIX) - (int32)(prior * fxd_factor);

    for (i = 1; i < len; ++i) {
	out[i] = (int32)(in[i] << DEFAULT_RADIX) - (int32)(in[i-1] * fxd_factor);
    }
#endif
#else /* !FIXED_POINT */
    out[0] = (frame_t)in[0]-factor*(frame_t)prior;
    for (i=1; i<len;i++) {
	out[i] = (frame_t)in[i] - factor*(frame_t)in[i-1];
    }
#endif /* !FIXED_POINT */
}

void fe_short_to_frame(int16 const *in, frame_t *out, int32 len)
{
    int32 i;
    
#ifdef  FIXED_POINT
    for (i=0;i<len;i++)
      out[i] = (int32)in[i] << DEFAULT_RADIX;
#else /* FIXED_POINT */
    for (i=0;i<len;i++)
      out[i] = (frame_t)in[i];
#endif /* FIXED_POINT */
}

void fe_create_hamming(window_t *in, int32 in_len)
{
    int i;
     
    if (in_len>1)
	for (i=0; i<in_len; i++) {
	    in[i] = FLOAT2MFCC(0.54 - 0.46*cos(2*M_PI*i/((float64)in_len-1.0))); 
	}

    return;
}


void fe_hamming_window(frame_t *in, window_t *window, int32 in_len)
{
    int i;
    
    if (in_len>1)
	for (i=0; i<in_len; i++) {
		in[i] = MFCCMUL(in[i], window[i]);
	}

    return;
}


void fe_frame_to_fea(fe_t *FE, frame_t *in, mfcc_t *fea)
{
    powspec_t *spec, *mfspec;
    
    if (FE->FB_TYPE == MEL_SCALE){
	spec = (powspec_t *)calloc(FE->FFT_SIZE, sizeof(powspec_t));
	mfspec = (powspec_t *)calloc(FE->MEL_FB->num_filters, sizeof(powspec_t));

	if (spec==NULL || mfspec==NULL){
	    fprintf(stderr,"memory alloc failed in fe_frame_to_fea()\n...exiting\n");
	    exit(0);
	}
	
 	fe_spec_magnitude(in, FE->FRAME_SIZE, spec, FE->FFT_SIZE);
	fe_mel_spec(FE, spec, mfspec);
	fe_mel_cep(FE, mfspec, fea);
	
	free(spec);
	free(mfspec);	
    }
    else {
	fprintf(stderr,"MEL SCALE IS CURRENTLY THE ONLY IMPLEMENTATION!\n");
	exit(0);
    }
    
}

void fe_spec_magnitude(frame_t const *data, int32 data_len, powspec_t *spec, int32 fftsize)
{
    int32 j,wrap;
    int32 fftorder;
    frame_t *fft;

    fft = (frame_t *) calloc(fftsize,sizeof(frame_t));
    memcpy(fft, data, data_len*sizeof(frame_t));
    if (data_len > fftsize)  /*aliasing */
	for (wrap = 0, j = fftsize; j < data_len; wrap++,j++)
	    fft[wrap] += data[j];
    for (fftorder = 0, j = fftsize; j>1; fftorder++, j>>= 1)
	    ;
    fe_fft_real(fft, fftsize, fftorder);
    
    for (j=0; j <= fftsize/2; j++)
    {
#ifdef FIXED_POINT
	uint32 r = abs(fft[j]);
	uint32 i = abs(fft[fftsize-j]);
#ifdef FIXED16
	int32 rr = INTLOG(r) * 2;
	int32 ii = INTLOG(i) * 2;
#else
	int32 rr = FIXLOG(r) * 2;
	int32 ii = FIXLOG(i) * 2;
#endif

	spec[j] = ADD(rr, ii);
#else /* !FIXED_POINT */
	spec[j] = fft[j]*fft[j] + fft[fftsize-j]*fft[fftsize-j];
#endif /* !FIXED_POINT */
    }

    free(fft);
    return;
}

void fe_mel_spec(fe_t *FE, powspec_t const *spec, powspec_t *mfspec)
{
    int32 whichfilt, start, i;
#ifdef FIXED_POINT
    int32 dfreq = FE->SAMPLING_RATE/FE->FFT_SIZE;
#else /* FIXED_POINT */
    float32 dfreq = FE->SAMPLING_RATE/(float32)FE->FFT_SIZE;
#endif /* FIXED_POINT */
    
    for (whichfilt = 0; whichfilt<FE->MEL_FB->num_filters; whichfilt++){
#ifdef FIXED_POINT
	/* There is an implicit floor here instead of rounding, hope
	 * it doesn't break stuff... */
	start = FE->MEL_FB->left_apex[whichfilt]/dfreq;
#else /* FIXED_POINT */
        /* Round to the nearest integer instead of truncating and
	 adding one, which breaks if the divide is already an
	 integer */      
	start = (int32)(FE->MEL_FB->left_apex[whichfilt]/dfreq + 0.5);
#endif /* FIXED_POINT */
	/* FIXME: It's not clear why this works for FIXED_POINT, since
	 * it ought to be MIN_LOG rather than zero.  But, it does. */
	mfspec[whichfilt] = 0;
#ifdef FIXED_POINT
	for (i=0; i< FE->MEL_FB->width[whichfilt]; i++)
	    mfspec[whichfilt] = ADD(mfspec[whichfilt],
				    spec[start+i] + FE->MEL_FB->filter_coeffs[whichfilt][i]);
#else /* !FIXED_POINT */
	for (i=0; i< FE->MEL_FB->width[whichfilt]; i++)
	    mfspec[whichfilt] +=
		    spec[start+i] * FE->MEL_FB->filter_coeffs[whichfilt][i];
#endif /* !FIXED_POINT */
    }
}

void fe_mel_cep(fe_t *FE, powspec_t *mfspec, mfcc_t *mfcep)
{
    int32 i,j;
    int32 period;
    int32 beta;

#ifdef FIXED_POINT
    /* powspec_t is unsigned, while mfcc_t is signed.  We are going to
     * modify mfspec in place to put it in log-domain so it's
     * important for it to become signed here. */
    mfcc_t *mflogspec = (mfcc_t *)mfspec;
#else /* FIXED_POINT */
    powspec_t *mflogspec = mfspec;
#endif /* FIXED_POINT */

    period = FE->MEL_FB->num_filters;

    for (i=0;i<FE->MEL_FB->num_filters; ++i) {
#if defined(FIXED_POINT)
	/* Convert it to fixed-point natural log from integer S2 log */
	if (mfspec[i]>0)
	    mflogspec[i] = LOG_TO_FIXLN(mflogspec[i]);
	else /* FIXME: As above, I honestly don't know why this works. */
	    mflogspec[i] = INT_MAX;
#else /* !FIXED_POINT */
	if (mfspec[i]>0)
	    mflogspec[i] = log(mfspec[i]);
	else
	    mflogspec[i] = -1.0e+5;
#endif /* !FIXED_POINT */
    }
    
    for (i=0; i< FE->NUM_CEPSTRA; ++i){
	mfcep[i] = 0;
	for (j=0;j<FE->MEL_FB->num_filters; j++){
	    if (j==0)
		beta = 1; /* 0.5 */
	    else
		beta = 2; /* 1.0 */
	    mfcep[i] += MFCCMUL(mflogspec[j],
			       FE->MEL_FB->mel_cosine[i][j]) * beta;
	}
	mfcep[i] /= (frame_t)period * 2;
    }
    return;
}

int32 fe_fft(complex const *in, complex *out, int32 N, int32 invert)
{
    int32
        s, k,			/* as above				*/
        lgN;			/* log2(N)				*/
    complex
        *f1, *f2,			/* pointers into from array		*/
        *t1, *t2,			/* pointers into to array		*/
        *ww;			/* pointer into w array			*/
    complex
        *from, *to,		/* as above				*/
        wwf2,			/* temporary for ww*f2			*/
        *exch,			/* temporary for exchanging from and to	*/
        *wEnd;			/* to keep ww from going off end	*/

    /* Cache the weight array and scratchpad for all FFTs of the same
     * order (we could actually do better than this, but we'd need a
     * different algorithm). */
    static complex *w;
    static complex *buffer; /* from and to flipflop btw out and buffer */  
    static int32 lastN;

    /* check N, compute lgN						*/
    for (k = N, lgN = 0; k > 1; k /= 2, lgN++)
    {
	if (k%2 != 0 || N < 0)
	{
	    fprintf(stderr, "fft: N must be a power of 2 (is %d)\n", N);
	    return(-1);
	}
    }

    /* check invert							*/
    if (!(invert == 1 || invert == -1))
    {
	fprintf(stderr, "fft: invert must be either +1 or -1 (is %d)\n", invert);
	return(-1);
    }

    /* Initialize weights and scratchpad buffer.  This will cause a
     * slow startup and "leak" a small, constant amount of memory,
     * don't worry about it. */
    if (lastN != N)
    {
	if (buffer) free(buffer);
	if (w) free(w);
	buffer = (complex *)calloc(N, sizeof(complex));
	w = (complex *) calloc(N/2, sizeof(complex));
	/* w = exp(-2*PI*i/N), w[k] = w^k					*/
	for (k = 0; k < N/2; k++)
	{
	    float64 x = -2*M_PI*invert*k/N;
	    w[k].r = FLOAT2MFCC(cos(x));
	    w[k].i = FLOAT2MFCC(sin(x));
	}
	lastN = N;
    }

    wEnd = &w[N/2];

    /* Initialize scratchpad pointers. */
    if (lgN%2 == 0)
    {
	from = out;
	to = buffer;
    }
    else
    {
	to = out;
	from = buffer;
    }
    memcpy(from, in, N * sizeof(*in));
  
    /* go for it!								*/
    for (k = N/2; k > 0; k /= 2)
    {
        for (s = 0; s < k; s++)
        {
            /* initialize pointers						*/
            f1 = &from[s]; f2 = &from[s+k];
            t1 = &to[s]; t2 = &to[s+N/2];
            ww = &w[0];
            /* compute <s,k>							*/
            while (ww < wEnd)
            {
                /* wwf2 = ww * f2							*/
		wwf2.r = MFCCMUL(f2->r, ww->r) - MFCCMUL(f2->i, ww->i);
		wwf2.i = MFCCMUL(f2->r, ww->i) + MFCCMUL(f2->i, ww->r);
                /* t1 = f1 + wwf2							*/
                t1->r = f1->r + wwf2.r;
                t1->i = f1->i + wwf2.i;
                /* t2 = f1 - wwf2							*/
                t2->r = f1->r - wwf2.r;
                t2->i = f1->i - wwf2.i;
                /* increment							*/
                f1 += 2 * k; f2 += 2 * k;
                t1 += k; t2 += k;
                ww += k;
            }
        }
        exch = from; from = to; to = exch;
    }

    /* Normalize for inverse FFT (not used but hey...) */
    if (invert == -1)
    {
	for (s = 0; s<N; s++)
	{
	    from[s].r = in[s].r / N;
	    from[s].i = in[s].i / N;

	}
    }

    return(0);
}

/* Translated from the FORTRAN (obviously) from "Real-Valued Fast
 * Fourier Transform Algorithms" by Henrik V. Sorensen et al., IEEE
 * Transactions on Acoustics, Speech, and Signal Processing, vol. 35,
 * no.6.  Optimized to use a static array of sine/cosines.
 */
int32 fe_fft_real(frame_t *x, int n, int m)
{
	int32 i, j, k, n1, n2, n4, i1, i2, i3, i4;
	frame_t t1, t2, xt, cc, ss;
	static frame_t *ccc, *sss;
	static int32 lastn;

	if (ccc == NULL || n != lastn) {
		free(ccc);
		free(sss);
		ccc = calloc(n/4, sizeof(*ccc));
		sss = calloc(n/4, sizeof(*sss));
		for (i = 0; i < n/4; ++i) {
			float64 a;

			a = 2*M_PI*i/n;
			
#ifdef FIXED16
			ccc[i] = cos(a)*32768;
			sss[i] = sin(a)*32768;
#else
			ccc[i] = FLOAT2MFCC(cos(a));
			sss[i] = FLOAT2MFCC(sin(a));
#endif
		}
		lastn = n;
	}

	j = 0;
	n1 = n-1;
	for (i = 0; i < n1; ++i) {
		if (i < j) {
			xt = x[j];
			x[j] = x[i];
			x[i] = xt;
		}
		k = n/2;
		while (k <= j) {
			j -= k;
			k /= 2;
		}
		j += k;
	}
	for (i = 0; i < n; i += 2) {
		xt = x[i];
		x[i] = xt + x[i+1];
		x[i+1] = xt - x[i+1];
	}
	n2 = 0;
	for (k = 1; k < m; ++k) {
		n4 = n2;
		n2 = n4+1;
		n1 = n2+1;
		for (i = 0; i < n; i += (1<<n1)) {
			xt = x[i];
			x[i] = xt + x[i+(1<<n2)];
			x[i+(1<<n2)] = xt - x[i+(1<<n2)];
			x[i+(1<<n4)+(1<<n2)] = -x[i+(1<<n4)+(1<<n2)];
			for (j = 1; j < (1<<n4); ++j) {
				i1 = i + j;
				i2 = i - j + (1<<n2);
				i3 = i + j + (1<<n2);
				i4 = i - j + (1<<n1);

				/* a = 2*M_PI * j / n1; */
				/* cc = cos(a); ss = sin(a); */
				cc = ccc[j<<(m-n1)];
				ss = sss[j<<(m-n1)];
#ifdef FIXED16
				t1 = (((int32)x[i3] * cc) >> 15)
				    + (((int32)x[i4] * ss) >> 15);
				t2 = ((int32)x[i3] * ss >> 15)
				    - (((int32)x[i4] * cc) >> 15);
#else
				t1 = MFCCMUL((mfcc_t)x[i3], (mfcc_t)cc)
				    + MFCCMUL((mfcc_t)x[i4], (mfcc_t)ss);
				t2 = MFCCMUL((mfcc_t)x[i3], (mfcc_t)ss)
				    - MFCCMUL((mfcc_t)x[i4], (mfcc_t)cc);
#endif
				x[i4] = x[i2] - t2;
				x[i3] = -x[i2] - t2;
				x[i2] = x[i1] - t1;
				x[i1] = x[i1] + t1;
			}
		}
	}
	return 0;
}


void *fe_create_2d(int32 d1, int32 d2, int32 elem_size)
{
    char *store;
    void **out;
    int32 i, j;
    store = calloc(d1 * d2, elem_size);

    if (store == NULL) {
	fprintf(stderr,"fe_create_2d failed\n");
	return(NULL);
    }
    
    out = calloc(d1, sizeof(void *));

    if (out == NULL) {
	fprintf(stderr,"fe_create_2d failed\n");
	free(store);
	return(NULL);
    }
    
    for (i = 0, j = 0; i < d1; i++, j += d2) {
	out[i] = store + (j * elem_size);
    }

    return out;
}

void fe_free_2d(void *arr)
{
    if (arr != NULL){
	/* FIXME: memory leak */
	free(((void **)arr)[0]);
	free(arr);
    }
    
}

void fe_parse_general_params(param_t const *P, fe_t *FE)
{

    if (P->SAMPLING_RATE != 0) 
	FE->SAMPLING_RATE = P->SAMPLING_RATE;
    else
	FE->SAMPLING_RATE = DEFAULT_SAMPLING_RATE;

    if (P->FRAME_RATE != 0) 
	FE->FRAME_RATE = P->FRAME_RATE;
    else 
	FE->FRAME_RATE = DEFAULT_FRAME_RATE;
    
    if (P->WINDOW_LENGTH != 0) 
	FE->WINDOW_LENGTH = P->WINDOW_LENGTH;
    else 
	FE->WINDOW_LENGTH = (float32) DEFAULT_WINDOW_LENGTH;
    
    if (P->FB_TYPE != 0) 
	FE->FB_TYPE = P->FB_TYPE;
    else 
	FE->FB_TYPE = DEFAULT_FB_TYPE;
 
    if (P->PRE_EMPHASIS_ALPHA != 0) 
	FE->PRE_EMPHASIS_ALPHA = P->PRE_EMPHASIS_ALPHA;
    else 
	FE->PRE_EMPHASIS_ALPHA = (float32) DEFAULT_PRE_EMPHASIS_ALPHA;
 
    if (P->NUM_CEPSTRA != 0) 
	FE->NUM_CEPSTRA = P->NUM_CEPSTRA;
    else 
	FE->NUM_CEPSTRA = DEFAULT_NUM_CEPSTRA;

    if (P->FFT_SIZE != 0) 
	FE->FFT_SIZE = P->FFT_SIZE;
    else 
	FE->FFT_SIZE = DEFAULT_FFT_SIZE;
 
}

void fe_parse_melfb_params(param_t const *P, melfb_t *MEL)
{
    if (P->SAMPLING_RATE != 0) 
	MEL->sampling_rate = P->SAMPLING_RATE;
    else 
	MEL->sampling_rate = DEFAULT_SAMPLING_RATE;

    if (P->FFT_SIZE != 0) 
	MEL->fft_size = P->FFT_SIZE;
    else {
      if (MEL->sampling_rate == BB_SAMPLING_RATE)
	MEL->fft_size = DEFAULT_BB_FFT_SIZE;
      if (MEL->sampling_rate == NB_SAMPLING_RATE)
	MEL->fft_size = DEFAULT_NB_FFT_SIZE;
      else 
	MEL->fft_size = DEFAULT_FFT_SIZE;
    }
 
    if (P->NUM_CEPSTRA != 0) 
	MEL->num_cepstra = P->NUM_CEPSTRA;
    else 
	MEL->num_cepstra = DEFAULT_NUM_CEPSTRA;
 
    if (P->NUM_FILTERS != 0)	
	MEL->num_filters = P->NUM_FILTERS;
    else {
        if (MEL->sampling_rate == BB_SAMPLING_RATE)
	    MEL->num_filters = DEFAULT_BB_NUM_FILTERS;
        else if (MEL->sampling_rate == NB_SAMPLING_RATE)
            MEL->num_filters = DEFAULT_NB_NUM_FILTERS;
        else {
            fprintf(stderr,"Please define the number of MEL filters needed\n");
            fprintf(stderr,"Modify include/fe.h and fe_sigproc.c\n");
            fflush(stderr); exit(0);
        }
    }

    if (P->UPPER_FILT_FREQ != 0)	
	MEL->upper_filt_freq = P->UPPER_FILT_FREQ;
    else{
        if (MEL->sampling_rate == BB_SAMPLING_RATE)
	    MEL->upper_filt_freq = (float32) DEFAULT_BB_UPPER_FILT_FREQ;
        else if (MEL->sampling_rate == NB_SAMPLING_RATE)
            MEL->upper_filt_freq = (float32) DEFAULT_NB_UPPER_FILT_FREQ;
        else {
            fprintf(stderr,"Please define the upper filt frequency needed\n");
            fprintf(stderr,"Modify include/fe.h and fe_sigproc.c\n");
            fflush(stderr); exit(0);
        }
    } 

    if (P->LOWER_FILT_FREQ != 0)	
	MEL->lower_filt_freq = P->LOWER_FILT_FREQ;
    else {
        if (MEL->sampling_rate == BB_SAMPLING_RATE)
	    MEL->lower_filt_freq = (float32) DEFAULT_BB_LOWER_FILT_FREQ;
        else if (MEL->sampling_rate == NB_SAMPLING_RATE)
            MEL->lower_filt_freq = (float32) DEFAULT_NB_LOWER_FILT_FREQ;
        else {
            fprintf(stderr,"Please define the lower filt frequency needed\n");
            fprintf(stderr,"Modify include/fe.h and fe_sigproc.c\n");
            fflush(stderr); exit(0);
        }
    } 

    if (P->doublebw == ON)
	MEL->doublewide = ON;
    else
	MEL->doublewide = OFF;

}

void fe_print_current(fe_t *FE)
{
    fprintf(stderr,"Current FE Parameters:\n");
    fprintf(stderr,"\tSampling Rate:             %f\n",FE->SAMPLING_RATE);
    fprintf(stderr,"\tFrame Size:                %d\n",FE->FRAME_SIZE);
    fprintf(stderr,"\tFrame Shift:               %d\n",FE->FRAME_SHIFT);
    fprintf(stderr,"\tFFT Size:                  %d\n",FE->FFT_SIZE);
    fprintf(stderr,"\tNumber of Overflow Samps:  %d\n",FE->NUM_OVERFLOW_SAMPS);
    fprintf(stderr,"\tStart Utt Status:          %d\n",FE->START_FLAG);
}

