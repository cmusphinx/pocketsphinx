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
#include <math.h>
#include <stdlib.h>

/************************************************************************
 *
 * This work was supported in part by funding from the Defense Advanced 
 * Research Projects Agency and the National Science Foundation of the 
 * United States of America, and the CMU Sphinx Speech Consortium.
 *
 *	This subroutine computes a split-radix FFT for real and		*
 *	even data of length 2n where n = 2^(m). It uses a real FFT 	*
 *	of length n/2 and the algorithm described in "On the Use	*
 *	of Symmetry in FFT Computation" by L. Rabiner in Trans. on	*
 *	ASSP June 1979, pp. 233-239.					*
 *									*
 *	Input format:  x[0], x[1], ... x[n]				*
 *	Output format: x[0], x[1], ... x[n]				*
 *	The result is in place and the transform is also real		*
 *	and even.							*
 *									*
 *	If one desires to compute the inverse FFT one can get it	*
 *	by calling resfft (x, n, m). The actual result is 2*n times	*
 *	the true IFFT.							*
 *									*
 ************************************************************************/

void resfft (float *x, int n, int m)
{
    int	n2, n4, i, j, k, nk;
    float  b0, yt, *y;
    double e;

    void rsfft();

    n2 = n;
    n4 = n2 / 2;
    for (b0 = 0, i = 1; i < n2; i += 2)
	b0 += x[i];
    b0 *= 2;
    y = (float *) malloc (n2 * sizeof (float));
    y[0] = x[0];
    y[n4] = x[n2];
    for (i = 1; i < n4; i++) {
	j = 2 * i;
	yt = x[j + 1] - x[j - 1];
	y[i] = x[j] + yt;
	y[n2 - i] = x[j] - yt;
    }
    rsfft (y, n2, m);
    e = 3.141592653589793 / n;
    for (k = 1; k < n4; k++)
	y[n2 - k] /= 2 * sin (e * k);
    x[0] = y[0] + b0;
    x[n2] = y[0] - b0;
    x[n4] = y[n4];
    for (k = 1; k < n4; k++) {
	nk = n2 - k;
	x[k] = y[k] + y[nk];
	x[nk] = y[k] - y[nk];
    }
    free (y);
}

/************************************************************************
 *
 * This work was supported in part by funding from the Defense Advanced 
 * Research Projects Agency and the National Science Foundation of the 
 * United States of America, and the CMU Sphinx Speech Consortium.
 *
 *	This subroutine computes a split-radix FFT for real data	*
 *	It is a C version of the FORTRAN program in "Real-Valued	*
 *	Fast Fourier Transform Algorithms" by H. Sorensen et al.	*
 *	in Trans. on ASSP, June 1987, pp. 849-863. It uses half 	*
 *	of the operations than its counterpart for complex data.	*
 *									*
 *	Length is n = 2^(m). Decimation in time. Result is in place.	*
 *	Input order:							*
 *	(x[0], x[1], ... x[n - 1])					*
 *	Ouput order: 							*
 *	(Re[0], Re[1], ... Re[n/2], Im[n/2 - 1]...Im[1])		*
 *	The output transform exhibit hermitian symmetry (i.e. real	*
 *	part of transform is even while imaginary part is odd).		*
 *	Hence Im[0] = Im[n/2] = 0; and n memory locations suffice.	*
 *									*
 *	Coded by Alex Acero, March 1989					*
 *									*
 ************************************************************************/

void rsfft (float *x, int n, int m)
{
    int	n1, n2, n4, n8, i0, i1, i2, i3, i4, i5, i6, i7, i8, is, id, i, j, k;
    float  xt, t1, t2, t3, t4, t5, t6;
    double e, a, a3, r1, cc1, cc3, ss1, ss3;

/*  digit reverse counter	*/

    j = 0;
    n1 = n - 1;
    for (i = 0; i < n1; i++) {
	if (i < j) {
	    xt = x[j];
	    x[j] = x[i];
	    x[i] = xt;
	}
	k = n / 2;
	while (k <= j) {
	    j -= k;
	    k /= 2;
	}
	j += k;
    }

/*  length two butterflies	*/

    is = 0;
    id = 4;
    while (is < n - 1) {
	for (i0 = is; i0 < n; i0 += id) {
	    i1 = i0 + 1;
	    r1 = x[i0];
	    x[i0] = r1 + x[i1];
	    x[i1] = r1 - x[i1];
	}
	is = 2 * (id - 1);
	id = 4 * id;
    }

/*  L shaped butterflies	*/

    n2 = 2;
    for (k = 1; k < m; k++) {
	n2 = n2 * 2;
	n4 = n2 / 4;
	n8 = n2 / 8;
	e = 6.283185307179586 / n2;
	is = 0;
	id = 2 * n2;
	while (is < n) {
	    for (i = is; i < n; i += id) {
		i1 = i;
		i2 = i1 + n4;
		i3 = i2 + n4;
		i4 = i3 + n4;
		t1 = x[i4] + x[i3];
		x[i4] = x[i4] - x[i3];
		x[i3] = x[i1] - t1;
		x[i1] = x[i1] + t1;
		if (n4 > 1) {
		    i1 = i1 + n8;
		    i2 = i2 + n8;
		    i3 = i3 + n8;
		    i4 = i4 + n8;
		    t1 = (x[i3] + x[i4]) / sqrt (2.0);
		    t2 = (x[i3] - x[i4]) / sqrt (2.0);
		    x[i4] = x[i2] - t1;
		    x[i3] = - x[i2] - t1;
		    x[i2] = x[i1] - t2;
		    x[i1] = x[i1] + t2;
		}
	    }
	    is = 2 * id - n2;
	    id = 4 * id;
	}
	a = e;
	for (j = 1; j < n8; j++) {
	    a3 = 3 * a;
	    cc1 = cos (a);
	    ss1 = sin (a);
	    cc3 = cos (a3);
	    ss3 = sin (a3);
	    a = (j + 1) * e;
	    is = 0;
	    id = 2 * n2;
	    while (is < n) {
		for (i = is; i < n; i += id) {
		    i1 = i + j;
		    i2 = i1 + n4;
		    i3 = i2 + n4;
		    i4 = i3 + n4;
		    i5 = i + n4 - j;
		    i6 = i5 + n4;
		    i7 = i6 + n4;
		    i8 = i7 + n4;
		    t1 = x[i3] * cc1 + x[i7] * ss1;
		    t2 = x[i7] * cc1 - x[i3] * ss1;
		    t3 = x[i4] * cc3 + x[i8] * ss3;
		    t4 = x[i8] * cc3 - x[i4] * ss3;
		    t5 = t1 + t3;
		    t6 = t2 + t4;
		    t3 = t1 - t3;
		    t4 = t2 - t4;
		    t2 = x[i6] + t6;
		    x[i3] = t6 - x[i6];
		    x[i8] = t2;
		    t2 = x[i2] - t3;
		    x[i7] = - x[i2] - t3;
		    x[i4] = t2;
		    t1 = x[i1] + t5;
		    x[i6] = x[i1] - t5;
		    x[i1] = t1;
		    t1 = x[i5] + t4;
		    x[i5] = x[i5] - t4;
		    x[i2] = t1;
		}
		is = 2 * id - n2;
		id = 4 * id;
	    }
	}
    }
}
