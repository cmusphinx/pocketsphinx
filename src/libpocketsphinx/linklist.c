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
 * linklist.c -- generic module for efficient memory management of 
 * linked list elements of various sizes; a separate list for each 
 * size.  Elements must be a multiple of a pointer size.
 * 
 * HISTORY
 * 
 * $Log: linklist.c,v $
 * Revision 1.1.1.1  2006/05/23 18:44:59  dhuggins
 * re-importation
 *
 * Revision 1.10  2005/07/02 12:55:40  egouvea
 * Changed void* with char* to fix compilation problem when compiling with gcc4.0
 *
 * Revision 1.9  2004/12/10 16:48:56  rkm
 * Added continuous density acoustic model handling
 *
 * Revision 1.8  2004/05/18 19:02:40  egouvea
 * Removed line including malloc.h, already defined in stdlib.h, and added initial support for macosx
 *
 * Revision 1.7  2001/12/11 00:24:48  lenzo
 * Acknowledgement in License.
 *
 * Revision 1.6  2001/12/07 17:30:02  lenzo
 * Clean up and remove extra lines.
 *
 * Revision 1.5  2001/12/07 05:09:30  lenzo
 * License.xsxc
 *
 * Revision 1.4  2001/12/07 04:27:35  lenzo
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
 * Revision 1.3  2001/03/29 21:09:41  lenzo
 * Getting the compile under windows back to normal
 *
 * Revision 1.2  2000/12/05 01:45:12  lenzo
 * Restructuring, hear rationalization, warning removal, ANSIfy
 *
 * Revision 1.1.1.1  2000/01/28 22:08:50  lenzo
 * Initial import of sphinx2
 *
 *
 * 
 * 15-May-95	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon.
 * 		Added "static" declaration to list[] and n_list.
 * 
 * 27-Apr-94	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon.
 * 		Created.
 */

#include <stdio.h>
#include <stdlib.h>

#include "s2types.h"
#include "err.h"

#define QUIT(x)		{fprintf x; exit(-1);}

#define MAX_LIST	16
#define MAX_ALLOC	40944

/*
 * Elements are seen as structures with an array of void pointers.  So element size
 * must be integral muliple of (void *).
 */
typedef struct list_s {
    void **freelist;	/* ptr to first element in freelist */
    int32 elem_size;	/* #(char *) in element */
    int32 n_malloc;	/* #elements to malloc if run out of free elments */
} list_t;
static list_t list[MAX_LIST];
static int32 n_list = 0;

void *listelem_alloc (int32 elem_size)
{
    int32 i, j;
    void **cpp;
    char *cp;
    
    for (i = 0; i < n_list; i++) {
	if (list[i].elem_size == elem_size)
	    break;
    }
    if (i >= n_list) {
	/* New list element size encountered, create new list entry */
	if (n_list >= MAX_LIST)
	    E_FATAL("Increase MAX_LIST\n");
	
	if (elem_size > MAX_ALLOC)
	    E_FATAL("Increase MAX_ALLOC to %d\n", elem_size);

	if ((elem_size % sizeof(char *)))
	    E_FATAL("Element size (%d) not multiple of (char *)\n",
		    elem_size);
	
	list[n_list].freelist = NULL;
	list[n_list].elem_size = elem_size;
	list[n_list].n_malloc = MAX_ALLOC / elem_size;
	i = n_list++;
    }

    if (list[i].freelist == NULL) {
	cpp = list[i].freelist = (void **) malloc (list[i].n_malloc * elem_size);
	cp = (char *) cpp;
	for (j = list[i].n_malloc-1; j > 0; --j) {
	    cp += elem_size;
	    *cpp = (void *)cp;
	    cpp = (void **)cp;
	}
	*cpp = NULL;
    }
    
    cp = (char *) list[i].freelist;
    list[i].freelist = *(list[i].freelist);
    return ((void *)cp);
}

void listelem_free (void *elem, int32 elem_size)
{
    int32 i;
    void **cpp;
    
    for (i = 0; i < n_list; i++) {
	if (list[i].elem_size == elem_size)
	    break;
    }
    if (i >= n_list)
	E_FATAL("elem_size (%d) not in known list\n", elem_size);
    
    cpp = elem;
    *cpp = list[i].freelist;
    list[i].freelist = cpp;
}
