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
/* LIST.C
 *
 * HISTORY
 * 14-Oct-92 Eric Thayer (eht+@cmu.edu) Carnegie Mellon University
 *	added formal dec for sym in list_insert().
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "s2types.h"
#include "list.h"

#define ERR_ARG		1
#define ERR_MALLOC	2

static int32 exception();

/* NEW_LIST
 *-----------------------------------------------------------*
 * DESCRIPTION
 *	Currently all we do is a calloc
 */
list_t *new_list (void)
{
    return (list_t *) calloc (1, sizeof(list_t));
}

/* LIST_ADD
 *------------------------------------------------------------*
 * DESCRIPTION
 */
int
list_add (list_t *list, caddr_t sym, int32 idx)
{
    static char const  *rname = "list_add";

    if (list == 0)
	return (exception (rname, "list", ERR_ARG));

    /*
     * Make sure the new entry fits in the List 
     */
    if (idx >= list->size) {
	/* HACK!!  Why the hardwired 64? */
	list->size = (list->size_hint > idx+64) ? list->size_hint : idx+64;

	if (list->list == 0)
	    list->list = (caddr_t *) malloc (sizeof (caddr_t) * list->size);
	else
	    list->list = (caddr_t *) realloc (list->list, sizeof (caddr_t) * list->size);
	if (list->list == 0)
	    return (exception (rname, sym, ERR_MALLOC));
	list->in_use = idx;
    }
    /*
     * Add 'sym' into list 
     */
    list->list[idx] = sym;

    return (0);
}

/* LIST_LOOKUP
 *------------------------------------------------------------*
 * DESCRIPTION
 */
caddr_t
list_lookup (list_t const *list, int32 idx)
{
    static char const *rname = "list_lookup";

    if ((list == 0) || (idx >= list->size) || (idx < 0))
	return ((caddr_t) exception (rname, "idx", ERR_ARG));

    return (list->list[idx]);
}

void list_insert (list_t *list, caddr_t sym)
/*-------------------*
 * Add sym to list at the in_use position and increment in_use.
 */
{
    list_add (list, sym, list->in_use);
    list->in_use++;
}

void list_unique_insert (list_t *list, caddr_t sym)
/*-------------------*
 * Add sym to list at the in_use position and increment in_use.
 */
{
    int32 i;

    for (i = 0; i < list->in_use; i++)
	if (sym == list->list[i])
	    return;

    list_add (list, sym, list->in_use);
    list->in_use++;
}

/* LIST_FREE
 *------------------------------------------------------------*
 * DESCRIPTION
 *	Free the list list.
 * NB.
 *	This routine doesn't free the objects.
 */
int
list_free (list_t *list)
{
    static char const *rname = "listFree";

    if (list == 0)
	return (exception (rname, "", ERR_ARG));

    free (list->list);
    list->list = 0;
    list->size = 0;
    list->in_use = 0;
    return 0;
}

int32
list_index (list_t const *list, caddr_t sym)
{
    int32 i;

    for (i = 0; i < list->in_use; i++)
      if (sym == list->list[i])
	return (i);

    fprintf (stderr, "listIndex: failed on %ld\n", (unsigned long) sym);
    exit (-1);
}

int32 listLength (list_t const *list)
{
    return list->in_use;
}

/* EXCEPTION
 *------------------------------------------------------------*
 */
static int32
exception (char *rname, char *s, int32 exc)
{
    switch (exc) {
	case ERR_ARG:
	    fprintf (stderr, "%s: Bad Argument [%s]\n", rname, s);
	    exit (-1);
	    break;
	case ERR_MALLOC:
	    fprintf (stderr, "%s: Malloc failed [%s]\n", rname, s);
	    exit (-1);
	    break;
	default:
	    fprintf (stderr, "%s: [%s] Unknown Exception[%d]\n", rname, s,
		     exc);
    }
    return -1;
}

void listWrite (FILE *fs, list_t const *list)
{
    fwrite (&list->in_use, sizeof (int32), 1, fs);
    fwrite (list->list, sizeof (caddr_t), list->in_use, fs);
}

void listRead (FILE *fs, list_t *list)
{
    if (list == 0) {
	fprintf (stderr, "listRead: bad argument\n");
	exit (-1);
    }
    fread (&list->in_use, sizeof (int32), 1, fs);
    list->list = (caddr_t *) calloc (sizeof(caddr_t), list->in_use);
    list->size = list->in_use;
    fread (list->list, sizeof (caddr_t), list->in_use, fs);
}
