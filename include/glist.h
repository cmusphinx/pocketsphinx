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
 * glist.h -- Module for maintaining a generic, linear linked-list structure.
 *
 * **********************************************
 * CMU ARPA Speech Project
 *
 * Copyright (c) 1999 Carnegie Mellon University.
 * ALL RIGHTS RESERVED.
 * **********************************************
 * 
 * HISTORY
 * 
 * $Log: glist.h,v $
 * Revision 1.1.1.1  2006/05/23 18:45:02  dhuggins
 * re-importation
 *
 * Revision 1.2  2004/12/10 16:48:55  rkm
 * Added continuous density acoustic model handling
 *
 * 
 * 24-Jun-2004	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Added glist_insert_*() and gnode_free() functions.
 * 
 * 09-Mar-1999	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Added glist_chkdup_*().
 * 
 * 13-Feb-1999	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Created from earlier version.
 */


/*
 * Generic linked-lists maintenance.
 * A convenient little linked-list package, but a double-edged sword: the user must
 * keep track of the data type within the linked list elements.
 * 
 * (C++ would be good for this, but that's a double-edged sword as well.)
 */


#ifndef _LIBUTIL_GLIST_H_
#define _LIBUTIL_GLIST_H_


#include <stdlib.h>
#include "s2types.h"


/* A node in a generic list */
typedef struct gnode_s {
    anytype_t data;		/* See prim_type.h */
    struct gnode_s *next;	/* Next node in list */
} gnode_t;
typedef gnode_t *glist_t;	/* Head of a list of gnodes */


/* Access macros, for convenience */
#define gnode_ptr(g)		((g)->data.ptr)
#define gnode_int32(g)		((g)->data.int32)
#define gnode_uint32(g)		((g)->data.uint32)
#define gnode_float32(g)	((g)->data.float32)
#define gnode_float64(g)	((g)->data.float64)
#define gnode_next(g)		((g)->next)


/*
 * Create and add a new list node, with the given user-defined data, at the HEAD
 * of the given generic list.  Return the new list thus formed.
 * g may be NULL to indicate an initially empty list.
 * (Too bad there's no function overloading.)
 */
glist_t glist_add_ptr (glist_t g, void *ptr);
glist_t glist_add_int32 (glist_t g, int32 val);
glist_t glist_add_uint32 (glist_t g, uint32 val);
glist_t glist_add_float32 (glist_t g, float32 val);
glist_t glist_add_float64 (glist_t g, float64 val);


/*
 * Create and insert a new list node, with the given user-defined data, after
 * the given generic node gn.  gn cannot be NULL.
 * Return ptr to the newly created gnode_t.
 */
gnode_t *glist_insert_ptr (gnode_t *gn, void *ptr);
gnode_t *glist_insert_int32 (gnode_t *gn, int32 val);
gnode_t *glist_insert_uint32 (gnode_t *gn, uint32 val);
gnode_t *glist_insert_float32 (gnode_t *gn, float32 val);
gnode_t *glist_insert_float64 (gnode_t *gn, float64 val);


/*
 * Check the given glist to see if it already contains the given value (of appropriate type).
 * In the case of the ptr, only the pointer values are compared, not the data pointed to by them.
 * Return value: 1 if match found, 0 if not.
 */
int32 glist_chkdup_ptr (glist_t g, void *val);	/* List and value to check for */
int32 glist_chkdup_int32 (glist_t g, int32 val);
int32 glist_chkdup_uint32 (glist_t g, uint32 val);
int32 glist_chkdup_float32 (glist_t g, float32 val);
int32 glist_chkdup_float64 (glist_t g, float64 val);


/*
 * Reverse the order of the given glist.  (glist_add() adds to the head; one might
 * ultimately want the reverse of that.)
 * Return value: The head of the new list.
 * NOTE: The list is reversed "in place"; i.e., no new memory is allocated.
 */
glist_t glist_reverse (glist_t g);


/* Return the number of elements in the given glist_t */
int32 glist_count (glist_t g);


/*
 * Apply the given function to the user-defined data.ptr for each node in the list.
 * (Again, too bad there's no function overloading in C.)
 */
void glist_apply_ptr (glist_t g, void (*func)(void *));
void glist_apply_int32 (glist_t g, void (*func)(int32));
void glist_apply_uint32 (glist_t g, void (*func)(uint32));
void glist_apply_float32 (glist_t g, void (*func)(float32));
void glist_apply_float64 (glist_t g, void (*func)(float64));


/*
 * Free the given node, gn, of a glist, pred being its predecessor in the list.
 * Return ptr to the next node in the list after the freed node.
 */
gnode_t *gnode_free(gnode_t *gn, gnode_t *pred);


/*
 * Free the given generic list; user-defined data contained within is not
 * automatically freed.  The caller must have done that already.
 */
void glist_free (glist_t g);


/*
 * Free the user-defined data (i.e., g->data.ptr) contained at each node of the given
 * glist (using myfree()).  Then free the glist.  "datasize" is the size of the
 * user-defined data at each node, and is needed by myfree().
 */
void glist_myfree (glist_t g, int32 datasize);


/*
 * Return the last node in the given list.
 */
gnode_t *glist_tail (glist_t g);


#endif
