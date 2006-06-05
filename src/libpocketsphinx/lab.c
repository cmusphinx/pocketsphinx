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
 * lab.c -- File I/O for label files
 *
 * HISTORY
 * 
 * $Log: lab.c,v $
 * Revision 1.1.1.1  2006/05/23 18:44:59  dhuggins
 * re-importation
 *
 * Revision 1.8  2004/12/10 16:48:56  rkm
 * Added continuous density acoustic model handling
 *
 * 
 * 23-Oct-01    Kevin A. Lenzo (lenzo@cs.cmu.edu) Fixed the
 *              magic numbers in save_labs, but it's still hard-coded.
 *              This needs to be calculated from the sample rate and shift.
 *
 * 28-Mar-00	Alan W Black (awb@cs.cmu.edu) Created.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "s2types.h"
#include "CM_macros.h"
#include "basic_types.h"
#include "search_const.h"
#include "err.h"
#include "log.h"
#include "scvq.h"
#include "msd.h"
#include "fbs.h"

#include "time_align.h"
#include "s2params.h"

/* FIXME: prototype this in an internal header file somewhere. */
int save_labs(SEGMENT_T *segs,
	      int num_entries,
	      const char *dirname,
	      const char *filename,
	      const char *extname,
	      const char *labtype)
{
    int i;
    FILE *labfd;
    char *path;

    path=(char *)malloc(strlen(dirname)+strlen(filename)+strlen(extname)+4);
    sprintf(path,"%s/%s.%s",dirname,filename,extname);

    if ((labfd = fopen(path,"w")) == NULL)
    {
	E_ERROR("Failed to open label file: %s\n", path);
	free(path);
	exit(1);
    }
    
    if (strcmp(labtype,"xlabel") == 0)
    {
	fprintf(labfd,"#\n");
	for (i=0; i<num_entries; i++)
	{
	  fprintf(labfd,"%0.6f 125 %s ; %d\n", 
		  /*		    segs[i].end * 0.00625,  */
		  segs[i].end * 0.01,
		  segs[i].name,
		  segs[i].score);
	}
    }
/*    else if (strcmp(labtype,"something else") == 0) */
/*    {                                               */
/*    }                                               */
    else  
    {   /* some CMU internal format -- does any one use this */
	printf("%20s %4s %4s %s\n",
	       "Phone", "Beg", "End", "Acoustic Score");
	for (i=0; i<num_entries; i++)
	{
	    fprintf(labfd,"%20s %4d %4d %12d\n",
		    segs[i].name, segs[i].start, segs[i].end, segs[i].score);
	}
    }
    free(path);
    fclose(labfd);
    return 0;
}
