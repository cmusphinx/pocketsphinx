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
 * mulaw.c -- u-Law audio routines.
 * 
 * HISTORY
 * 
 * $Log: mulaw_base.c,v $
 * Revision 1.1.1.1  2006/05/23 18:45:02  dhuggins
 * re-importation
 *
 * Revision 1.5  2001/12/11 00:24:48  lenzo
 * Acknowledgement in License.
 *
 * Revision 1.4  2001/12/07 17:30:02  lenzo
 * Clean up and remove extra lines.
 *
 * Revision 1.3  2001/12/07 05:09:31  lenzo
 * License.xsxc
 *
 * Revision 1.2  2001/12/07 04:27:35  lenzo
 * License cleanup.  Remove conditions on the names.  Rationale: These
 * conditions don't belong in the license itself, but in other fora that
 * offer protection for recognizeable names such as "Carnegie Mellon
 * University" and "Sphinx."  These changes also reduce interoperability
 * issues with other licenses such as the Mozilla Public License and the
 * GPL.  This update changes the top-level license files and removes the
 * old license conditions from each of the files that contained it.
 * All files in this collection fall under the copyright of the top-level
 * LICENSE file.
 *
 * Revision 1.1  2000/12/12 22:55:55  lenzo
 * Separate out a/d lib.
 *
 * Revision 1.2  2000/12/05 01:45:12  lenzo
 * Restructuring, hear rationalization, warning removal, ANSIfy
 *
 * Revision 1.1.1.1  2000/01/28 22:08:52  lenzo
 * Initial import of sphinx2
 *
 *
 * 
 * 22-Jul-97	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Created from Sunil's version.
 */

#include "s2types.h"
#include "ad.h"
#include "mulaw.h"

void ad_mu2li (int16 *out, unsigned char *in, int32 n_samp)
{
    int32 i;
    
    for (i = 0; i < n_samp; i++)
	out[i] = muLaw[in[i]];
}
