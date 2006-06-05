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
 * bio.h -- Sphinx-3 binary file I/O functions.
 * 
 * **********************************************
 * CMU ARPA Speech Project
 *
 * Copyright (c) 1996 Carnegie Mellon University.
 * ALL RIGHTS RESERVED.
 * **********************************************
 * 
 * HISTORY
 * 
 * $Log: bio.h,v $
 * Revision 1.1.1.1  2006/05/23 18:45:02  dhuggins
 * re-importation
 *
 * Revision 1.2  2005/09/01 21:09:54  dhdfu
 * Really, actually, truly consolidate byteswapping operations into
 * byteorder.h.  Where unconditional byteswapping is needed, SWAP_INT32()
 * and SWAP_INT16() are to be used.  The WORDS_BIGENDIAN macro from
 * autoconf controls the functioning of the conditional swap macros
 * (SWAP_?[LW]) whose names and semantics have been regularized.
 * Private, adhoc macros have been removed.
 *
 * Revision 1.1  2004/12/10 16:48:58  rkm
 * Added continuous density acoustic model handling
 *
 * 
 * 22-Nov-2004	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Imported from s3.2, for supporting s3 format continuous
 * 		acoustic models.
 * 
 * 28-Apr-1999	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Created.
 */


#ifndef __BIO_H__
#define __BIO_H__


#include "s2types.h"


#define BYTE_ORDER_MAGIC	(0x11223344)

/*
 * Read binary file format header: has the following format
 *     s3
 *     <argument-name> <argument-value>
 *     <argument-name> <argument-value>
 *     ...
 *     endhdr
 *     4-byte byte-order word used to find file byte ordering relative to host machine.
 * Lines beginning with # are ignored.
 * Memory for name and val allocated by this function; use bio_hdrarg_free to free them.
 * Return value: 0 if successful, -1 otherwise.
 */
int32 bio_readhdr (FILE *fp,		/* In: File to read */
		   char ***name,	/* Out: array of argument name strings read */
		   char ***val,		/* Out: corresponding value strings read */
		   int32 *swap);	/* Out: file needs byteswapping iff (*swap) */

/*
 * Write a simple binary file header, containing only the version string.  Also write
 * the byte order magic word.
 * Return value: 0 if successful, -1 otherwise.
 */
int32 bio_writehdr_version (FILE *fp, char *version);


/*
 * Free name and value strings previously allocated and returned by bio_readhdr.
 */
void bio_hdrarg_free (char **name,	/* In: Array previously returned by bio_readhdr */
		      char **val);	/* In: Array previously returned by bio_readhdr */

/*
 * Like fread but perform byteswapping and accumulate checksum (the 2 extra arguments).
 * But unlike fread, returns -1 if required number of elements (n_el) not read; also,
 * no byteswapping or checksum accumulation is performed in that case.
 */
int32 bio_fread (void *buf,
		 int32 el_sz,
		 int32 n_el,
		 FILE *fp,
		 int32 swap,		/* In: Byteswap iff (swap != 0) */
		 uint32 *chksum);	/* In/Out: Accumulated checksum */

/*
 * Read a 1-d array (fashioned after fread):
 *     4-byte array size (returned in n_el)
 *     memory allocated for the array and read (returned in buf)
 * Byteswapping and checksum accumulation performed as necessary.
 * Fails fatally if expected data not read.
 * Return value: #array elements allocated and read; -1 if error.
 */
int32 bio_fread_1d (void **buf,		/* Out: contains array data; allocated by this
					   function; can be freed using ckd_free */
		    int32 el_sz,	/* In: Array element size */
		    int32 *n_el,	/* Out: #array elements allocated/read */
		    FILE *fp,		/* In: File to read */
		    int32 sw,		/* In: Byteswap iff (swap != 0) */
		    uint32 *ck);	/* In/Out: Accumulated checksum */

/*
 * Read and verify checksum at the end of binary file.  Fails fatally if there is
 * a mismatch.
 */
void bio_verify_chksum (FILE *fp,	/* In: File to read */
			int32 byteswap,	/* In: Byteswap iff (swap != 0) */
			uint32 chksum);	/* In: Value to compare with checksum in file */


#endif
