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
/* PCONF.H
 *------------------------------------------------------------*
 * Copyright 1988, Fil Alleva and Carnegie Mellon University.
 * ALL RIGHTS RESERVED.
 *------------------------------------------------------------*
 * HISTORY
 * $Log: pconf.h,v $
 * Revision 1.1.1.1  2006/05/23 18:45:03  dhuggins
 * re-importation
 *
 * Revision 1.5  2004/03/01 00:00:48  egouvea
 * Fixed refs to win32, and clean up files used by autoconf/automake
 *
 * Revision 1.4  2001/12/11 00:24:48  lenzo
 * Acknowledgement in License.
 *
 * Revision 1.3  2001/12/07 17:30:02  lenzo
 * Clean up and remove extra lines.
 *
 * Revision 1.2  2001/12/07 05:09:30  lenzo
 * License.xsxc
 *
 * Revision 1.1  2000/12/05 01:33:29  lenzo
 * files added or moved
 *
 * Revision 1.1.1.1  2000/01/28 22:09:07  lenzo
 * Initial import of sphinx2
 *
 *
 * Revision 1.1  1993/03/23  14:57:57  eht
 * Initial revision
 *
 */

#ifndef _PCONF_
#define _PCONF_

#include <sys/types.h>

#ifdef WIN32
#include <posixwin32.h>
#endif

typedef enum {NOTYPE,
	      BYTE, SHORT, INT, LONG,
	      U_BYTE, U_SHORT, U_INT, U_LONG,
	      FLOAT, DOUBLE,
	      BOOL, CHAR, STRING,
	      DATA_SRC
} arg_t;

typedef enum {
	SRC_NONE, SRC_HSA, SRC_VQFILE, SRC_CEPFILE, SRC_ADCFILE
} data_src_t;

typedef union _ptr {
    char	*CharP;
    char	*ByteP;
    u_char	*UByteP;
    short	*ShortP;
    u_short	*UShortP;
    int		*IntP;
    u_int	*UIntP;
    long	*LongP;
    u_long	*ULongP;
    float	*FloatP;
    double	*DoubleP;
    int		*BoolP;
    char	**StringP;
    data_src_t	*DataSrcP;

} ptr_t;

typedef struct _config {
	char const *LongName;		/* Long Name */
	char const *Doc;		/* Documentation string */
	char const *swtch;		/* Switch Name */
	arg_t	arg_type;		/* Argument Type */
	caddr_t	var;			/* Pointer to the variable */
} config_t;

typedef struct _InternalConfig {
	char const *LongName;		/* Long Name */
	char const *Doc;		/* Documentation string */
	char const *swtch;		/* Switch Name */
	arg_t	arg_type;		/* Argument Type */
	ptr_t	var;			/* Pointer to the variable */
} Config_t;

int pconf(int argc, char *argv[],
	  config_t *config_p, char **display, char **geometry,
	  char * (*GetDefault)(char const *, char const *));
int ppconf(int argc, char *argv[],
	   config_t *config_p, char **display, char **geometry,
	   char * (*GetDefault)(char const *, char const *), char last);
void pusage(char *prog, Config_t *cp);

#endif /* _PCONF_ */
