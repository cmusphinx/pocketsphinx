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
 * 
 * HISTORY
 * 
 * $Log: log.h,v $
 * Revision 1.1.1.1  2006/05/23 18:45:02  dhuggins
 * re-importation
 *
 * Revision 1.6  2004/12/10 16:48:55  rkm
 * Added continuous density acoustic model handling
 *
 * 
 * 16-May-96	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Created from Fil Alleva's original.
 */


#ifndef _LOG_H_
#define _LOG_H_

#include <math.h>

extern int16 Addition_Table[];
extern int32 Table_Size;

#define BASE		1.0001
#define LOG_BASE	9.9995e-5
#define MIN_LOG		-690810000
#define MIN_DOUBLE	1e-300
/*
 * Note that we are always dealing with values between 0.0 and 1.0 so the
 * log(x) is < 0.0 so the rounding uses -0.5 instead of +0.5
 */
#define LOG(x)	((x == 0.0) ? MIN_LOG :					\
			      ((x > 1.0) ?				\
				 (int32) ((log (x) / LOG_BASE) + 0.5) :	\
				 (int32) ((log (x) / LOG_BASE) - 0.5)))

#define EXP(x)	(exp ((double) (x) * LOG_BASE))

/* tkharris++ for additional overflow check */
#define ADD(x,y) ((x) > (y) ? \
                  (((y) <= MIN_LOG ||(x)-(y)>=Table_Size ||(x) - +(y)<0) ? \
		           (x) : Addition_Table[(x) - (y)] + (x))	\
		   : \
		  (((x) <= MIN_LOG ||(y)-(x)>=Table_Size ||(y) - +(x)<0) ? \
		          (y) : Addition_Table[(y) - (x)] + (y)))

#define FAST_ADD(res, x, y, table, table_size)		\
{							\
	int32 _d = (x) - (y);				\
	if (_d > 0) { /* x >= y */			\
		if (_d >= (table_size))			\
			res = (x);			\
		else					\
			res = (table)[_d] + (x);	\
	} else { /* x < y */				\
		if (-_d >= (table_size))		\
			res = (y);			\
		else					\
			res = (table)[-_d] + (y);	\
	}						\
}

/*
 * Note that we always deal with quantities < 0.0, there for we round
 * using -0.5 instead of +0.5
 */ 
#define LOG10TOLOG(x)	((int32)((x * (2.30258509 / LOG_BASE)) - 0.5))

/*
 * In terms of already shifted and negated quantities (i.e. dealing with
 * 8-bit quantized values):
 */
#define LOG_ADD(p1,p2)	(logadd_tbl[(p1<<8)+(p2)])

extern unsigned char logadd_tbl[];

#endif /* _LOG_H_ */
