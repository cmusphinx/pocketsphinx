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

/* HASH.C
 *------------------------------------------------------------*
 * SYNOPSIS
 *	hash_add(ht,sym,val)	- add sym to ht with val
 *	hash_lookup(ht,sym,val) - find sym in ht, put val into val.
 *	list_t* hash_to_list(ht) - convert ht to a list.
 *------------------------------------------------------------*
 * DESCRIPTION
 *	The hash_* subroutines implement a simple symbolic
 * hashing scheme where values are associated with symbols.
 * You can add sym,val pairs to the table and later look them up.
 * Entries in the hash table cannot be deleted. The hash table
 * will grow to accomadate an arbitary (with in limits) no. of
 * entries.
 *------------------------------------------------------------*
 * BUGS
 *	It would be trivial to add a deletion function.
 *------------------------------------------------------------*
 * HISTORY
 * 
 * $Log: hash.c,v $
 * Revision 1.1.1.1  2006/05/23 18:44:59  dhuggins
 * re-importation
 *
 * Revision 1.12  2005/07/21 20:03:53  egouvea
 * Fixed bug 1154352, seg fault when extended ascii is the only letter in
 * the word.
 *
 * Revision 1.11  2004/12/10 16:48:56  rkm
 * Added continuous density acoustic model handling
 *
 * 
 * 27-Aug-94  M K Ravishankar (rkm@cs) at Carnegie-Mellon University
 * 	Changed hash_lookup to be case-insensitive.
 * 
 * 18-Feb-88  Fil Alleva (faa) at Carnegie-Mellon University
 *	Created.
 *
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>

#include "s2types.h"
#include "list.h"
#include "hash.h"
#include "err.h"

#define ERR_ARG		1
#define ERR_MALLOC	2

int32 hash_count = 0;
int32 rehash_count = 0;
int32 hash_rebuild = 0;
int32 hash_rebuild_ent = 0;

static int exception (char const *rname, char const *s, int exc);
static int32 hash_in(hash_t *ht, char const *sym, caddr_t val);
static int32 next_prime(int32 p);

extern int mystrcasecmp(char const *, char const *);

/* HASH_ADD
 *------------------------------------------------------------*
 * DESCRIPTION
 *	Add 'sym' to the hash table 'ht' with the associated
 * value 'val'. If 'sym' is already in the table then its
 * 'val' == the new 'val', otherwise there is a conflict resulting
 * in a fatal error.
 */
int32
hash_add (hash_t *ht, char const *sym, caddr_t val)
{
    static char const *rname = "hash_add";

    if ((ht == 0) || (sym == 0))
	return exception (rname, "sym", ERR_ARG);

#ifdef DEBUG
    printf ("%s: %x %s %d\n", rname, ht, sym, val);
    printf ("%s: inuse %d, size %d\n", rname, ht->inuse, ht->size);
#endif
    
    /*
     * Make sure the Hash table isn't too full
     */
    if ((2 * ht->inuse) >= ht->size) {
	int32 i;
	int32 old_size = ht->size;
	hent_t *tab, *old_tab = ht->tab;

	if ((ht->inuse == 0) && (ht->size_hint > 0))
	    ht->size = next_prime ((ht->size_hint * 2) + ht->size_hint + 2);
	else
	    ht->size = next_prime ((ht->inuse * 2) + ht->inuse + 2);

        ht->inuse = 0;
	ht->tab = (hent_t *) calloc ((size_t)ht->size, sizeof (hent_t));
	
	if (ht->tab == 0)
	    return exception (rname, sym, ERR_MALLOC);
	/*
	 * Create new hash table from the old one.
	 */
	for (tab = old_tab, i = 0; i < old_size; i++, tab++) {
	    if (tab->obj)
		hash_in (ht, tab->obj, tab->val);
	}
	/*
	 * Delete old hash table
	 */
	free (old_tab);
 	hash_rebuild++; hash_rebuild_ent += ht->inuse;
    }
    /*
     * Hash 'sym' into ht
     */
    if (hash_in (ht, sym, val)) {
	E_FATAL ("\n%s: Error: [%s] hash conflict\nThere are two entries in the dictionary for [%s]\nPlease change or remove one of them and re-run.\n\n", rname, sym, sym);
    }
    return (0);
}

/* HASH_FREE
 *------------------------------------------------------------*
 * DESCRIPTION
 *	Free the hash table.
 * NB.
 *	This routine doesn't free the objects.
 */
int
hash_free (hash_t *ht)
{
    static char const *rname = "hash_free";

    if (ht == 0)
	exception (rname, "", ERR_ARG);

    free (ht->tab);
    ht->tab = 0;
    ht->size = 0;
    ht->inuse = 0;

    return 0;
}

/* HASH_LOOKUP
 *------------------------------------------------------------*
 * DESCRIPTION
 *	Lookup 'sym' in 'ht'. If 'sym' is found return its
 * assocoated value in *val and return 0, indicating we found
 * the 'sym' in the table. If we didn't find the symbol return
 * -1, and put the value of the key int32 *val.
 * 
 * NOTE: CASE-INSENSITIVE!!
 */
int32
hash_lookup (hash_t *ht, char const *sym, caddr_t *val)
{
    static char const *rname = "hash_lookup";
    register char const *cp;
    register unsigned char c;
    register uint32    key;
    register int32    i;

    if ((ht == 0) || (sym == 0) || (val == 0))
	return (exception (rname, sym, ERR_ARG));

    key = 0;
    i = -1;
    cp = sym;
/* kal 18-Jul-2000 - replaced with patch from Jose Renau 
    do {
	c = *cp++;
	if ((c >= 'a') && (c <= 'z'))
	    c -= 32;
	key += c << (0xF & i--);
    } while (*cp);
*/
    do {
      c = tolower(*cp++);
      c -='a' & 0x0F;
      key = (key<<3) ^ c;
    } while(*cp);

    hash_count++;
rehash:
    if (ht->size == 0) {
	/*
	 * The hash table hasn't been built yet so this entry isn't in there
	 */
	if (val)
	  *val = (caddr_t) key;
	return (-1);
    }

    key %= ht->size;

    if (ht->tab[key].obj == 0) {
	*val = (caddr_t) key;
	return (-1);
    }

    if (mystrcasecmp(ht->tab[key].obj, sym) == 0) {
	*val = ht->tab[key].val;
	return (0);
    }
    key++;
    rehash_count++;
    goto rehash;
}

/* HASH_IN
 *------------------------------------------------------------*
 * DESRIPTION
 *	Lookup up 'sym'. If it is not in 'ht->tab' then add it.
 * Otherwise compare the 'val' for 'sym' from the table with
 * the one passed in. If they differ return -1, conflict, other
 * wise return 0.
 */
static int32
hash_in (hash_t *ht, char const *sym, caddr_t val)
{
    static char const *rname = "hash_in";
    caddr_t key;

    if ((ht == 0) || (sym == 0))
	return (exception (rname, sym, ERR_ARG));
    
    if (hash_lookup(ht, sym, &key)) {
        /* if lookup fails to find sym, key contains the hashed locn */
	ht->tab[(int32)key].obj = sym;
	ht->tab[(int32)key].val = val;
	ht->inuse++;
    }
    else
    {
      if (key != val)
	return (-1);
    }
    return (0);
}

/* EXCEPTION
 *------------------------------------------------------------*
 */
static int
exception (char const *rname, char const *s, int exc)
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

static int32
next_prime (int32 p)
{
        register int32 k;

again:
        for (k = 2; k <= p/2; k++)
	    if ((p / k)*k == p)
	        break;
	if (k <= p/2) {
	    p++;
	    goto again;
	}
	return p;
}

list_t *hash_to_list (hash_t *ht)
/*------------------------------------------------------------*
 * convert the hash table 'ht' to a list_t and return it.
 */
{
    int32 i;
    list_t* list;

    list = new_list();
    list->size_hint = ht->size+1;
    
    for (i = 0; i < ht->size; i++) {
	if (ht->tab[i].obj) {
	    list_insert (list, ht->tab[i].val);
	}
    }
    assert (list->in_use == ht->inuse);
    return (list);
}
