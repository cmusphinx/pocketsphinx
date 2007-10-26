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
 * log.h -- LOG based probability ops
 * 
 * **********************************************
 * CMU ARPA Speech Project
 *
 * Copyright (c) 1996 Carnegie Mellon University.
 * **********************************************
 */


#ifndef _LOG_H_
#define _LOG_H_

#include <math.h>

/* Pull in the definitions from sphinxbase and redefine them for convenience here */
#include <fixpoint.h>
#include <logmath.h>

/* Global log-domain addition table. */
logmath_t *lmath;

#define BASE		logmath_get_base(lmath)
#define LOG(x) 		logmath_log(lmath,x)
#define EXP(x)		logmath_exp(lmath,x)
#define ADD(x,y)	logmath_add(lmath,x,y)
#define LOG10TOLOG(x)	logmath_log10_to_log(lmath,x)
#define MIN_LOG         logmath_get_zero(lmath)

/*
 * In terms of already shifted and negated quantities (i.e. dealing with
 * 8-bit quantized values):
 */
#define LOG_ADD(p1,p2)	(logadd_tbl[(p1<<8)+(p2)])

extern const unsigned char logadd_tbl[];

#endif /* _LOG_H_ */
