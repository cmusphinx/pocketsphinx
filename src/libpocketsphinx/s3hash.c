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
 * s3hash.c -- Hash table module.
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
 * $Log: s3hash.c,v $
 * Revision 1.1.1.1  2006/05/23 18:45:01  dhuggins
 * re-importation
 *
 * Revision 1.1  2004/12/10 16:48:56  rkm
 * Added continuous density acoustic model handling
 *
 * 
 * 22-Nov-2004	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Imported from s3.2, for supporting s3 format continuous
 * 		acoustic models.
 * 
 * 05-May-1999	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon
 * 		Removed hash_key2hash().  Added hash_enter_bkey() and hash_lookup_bkey(),
 * 		and len attribute to hash_entry_t.
 * 
 * 30-Apr-1999	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon
 * 		Added hash_key2hash().
 * 
 * 18-Jun-97	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon
 * 		Included case sensitive/insensitive option.  Removed local, static
 * 		maintenance of all hash tables.
 * 
 * 31-Jul-95	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon
 * 		Created.
 */


#include <string.h>
#include <assert.h>

#include "s3hash.h"
#include "err.h"
#include "ckd_alloc.h"
#include "case.h"


#if 0
static void prime_sieve (int32 max)
{
    char *notprime;
    int32 p, pp;
    
    notprime = (char *) ckd_calloc (max+1, 1);
    p = 2;
    for (;;) {
	printf ("%d\n", p);
	for (pp = p+p; pp <= max; pp += p)
	    notprime[pp] = 1;
	for (++p; (p <= max) && notprime[p]; p++);
	if (p > max)
	    break;
    }
}
#endif


/*
 * HACK!!  Initial hash table size is restricted by this set of primes.  (Of course,
 * collision resolution by chaining will accommodate more entries indefinitely, but
 * efficiency will drop.)
 */
static int32 prime[] = {
    101, 211, 307, 401, 503, 601, 701, 809, 907,
    1009, 1201, 1601, 2003, 2411, 3001, 4001, 5003, 6007, 7001, 8009, 9001,
    10007, 12007, 16001, 20011, 24001, 30011, 40009, 50021, 60013, 70001, 80021, 90001,
    100003, 120011, 160001, 200003, 240007, 300007, 400009, 500009, 600011, 700001, 800011, 900001,
    -1
};


static int32 prime_size (int32 size)
{
    int32 i;
    
    for (i = 0; (prime[i] > 0) && (prime[i] < size); i++);
    if (prime[i] <= 0) {
	E_WARN("Very large hash table requested (%d entries)\n", size);
	--i;
    }
    return (prime[i]);
}


s3hash_table_t *s3hash_new (int32 size, int32 casearg)
{
    s3hash_table_t *h;
    
    h = (s3hash_table_t *) ckd_calloc (1, sizeof(s3hash_table_t));
    h->size = prime_size (size+(size>>1));
    h->nocase = (casearg == S3HASH_CASE_NO);
    h->table = (s3hash_entry_t *) ckd_calloc (h->size, sizeof(s3hash_entry_t));
    /* The above calloc clears h->table[*].key and .next to NULL, i.e. an empty table */

    return h;
}


/*
 * Compute hash value for given key string.
 * Somewhat tuned for English text word strings.
 */
static uint32 key2hash (s3hash_table_t *h, const char *key)
{
    register const char *cp;
    register char c;
    register int32 s;
    register uint32 hash;
    
    hash = 0;
    s = 0;
    
    if (h->nocase) {
	for (cp = key; *cp; cp++) {
	    c = *cp;
	    c = UPPER_CASE(c);
	    hash += c << s;
	    s += 5;
	    if (s >= 25)
		s -= 24;
	}
    } else {
	for (cp = key; *cp; cp++) {
	    hash += (*cp) << s;
	    s += 5;
	    if (s >= 25)
		s -= 24;
	}
    }
    
    return (hash % h->size);
}


static char *makekey (uint8 *data, int32 len, char *key)
{
    int32 i, j;
    
    if (! key)
	key = (char *) ckd_calloc (len*2 + 1, sizeof(char));
    
    for (i = 0, j = 0; i < len; i++, j += 2) {
	key[j] = 'A' + (data[i] & 0x000f);
	key[j+1] = 'J' + ((data[i] >> 4) & 0x000f);
    }
    key[j] = '\0';
    
    return key;
}


static int32 keycmp_nocase (s3hash_entry_t *entry, const char *key)
{
    char c1, c2;
    int32 i;
    const char *str;
    
    str = entry->key;
    for (i = 0; i < entry->len; i++) {
	c1 = *(str++);
	c1 = UPPER_CASE(c1);
	c2 = *(key++);
	c2 = UPPER_CASE(c2);
	if (c1 != c2)
	    return (c1-c2);
    }
    
    return 0;
}


static int32 keycmp_case (s3hash_entry_t *entry, const char *key)
{
    char c1, c2;
    int32 i;
    const char *str;
    
    str = entry->key;
    for (i = 0; i < entry->len; i++) {
	c1 = *(str++);
	c2 = *(key++);
	if (c1 != c2)
	    return (c1-c2);
    }
    
    return 0;
}


/*
 * Lookup chained entries with hash-value hash in table h for given key and return
 * associated value in *val.
 * Return value: 0 if key found in hash table, else -1.
 */
static int32 lookup (s3hash_table_t *h, uint32 hash, const char *key, int32 len, int32 *val)
{
    s3hash_entry_t *entry;
    
    entry = &(h->table[hash]);
    if (entry->key == NULL)
	return -1;
    
    if (h->nocase) {
	while (entry && ((entry->len != len) || (keycmp_nocase (entry, key) != 0)))
	    entry = entry->next;
    } else {
	while (entry && ((entry->len != len) || (keycmp_case (entry, key) != 0)))
	    entry = entry->next;
    }
    
    if (entry) {
	*val = entry->val;
	return 0;
    } else
	return -1;
}


/*
 * Like lookup above, but replace the old associated value with the new value given in *val,
 * before returning the old value in *val.
 * Return value: 0 if key found in hash table, else -1.
 */
static int32 update (s3hash_table_t *h, uint32 hash, const char *key, int32 len, int32 *val)
{
    s3hash_entry_t *entry;
    int32 tmp;
    
    entry = &(h->table[hash]);
    if (entry->key == NULL)
	return -1;
    
    if (h->nocase) {
	while (entry && ((entry->len != len) || (keycmp_nocase (entry, key) != 0)))
	    entry = entry->next;
    } else {
	while (entry && ((entry->len != len) || (keycmp_case (entry, key) != 0)))
	    entry = entry->next;
    }
    
    if (entry) {
	tmp = entry->val;
	entry->val = *val;
	*val = tmp;
	
	return 0;
    } else
	return -1;
}


int32 s3hash_lookup (s3hash_table_t *h, const char *key, int32 *val)
{
    uint32 hash;
    int32 len;
    
    hash = key2hash (h, key);
    len = strlen(key);
    
    return (lookup (h, hash, key, len, val));
}


int32 s3hash_lookup_bkey (s3hash_table_t *h, const char *key, int32 len, int32 *val)
{
    uint32 hash;
    char *str;
    
    str = makekey ((uint8 *)key, len, NULL);
    hash = key2hash (h, str);
    ckd_free (str);
    
    return (lookup (h, hash, key, len, val));
}


int32 s3hash_update (s3hash_table_t *h, const char *key, int32 *val)
{
    uint32 hash;
    int32 len;
    
    hash = key2hash (h, key);
    len = strlen(key);
    
    return (update (h, hash, key, len, val));
}


int32 s3hash_update_bkey (s3hash_table_t *h, const char *key, int32 len, int32 *val)
{
    uint32 hash;
    char *str;
    
    str = makekey ((uint8 *)key, len, NULL);
    hash = key2hash (h, str);
    ckd_free (str);
    
    return (update (h, hash, key, len, val));
}


static int32 enter (s3hash_table_t *h, uint32 hash, const char *key, int32 len, int32 val)
{
    int32 old;
    s3hash_entry_t *cur, *new;
    
    if (lookup (h, hash, key, len, &old) == 0) {
	/* Key already exists */
	return old;
    }
    
    cur = &(h->table[hash]);
    if (cur->key == NULL) {
	/* Empty slot at hashed location; add this entry */
	cur->key = key;
	cur->len = len;
	cur->val = val;
    } else {
	/* Key collision; create new entry and link to hashed location */
	new = (s3hash_entry_t *) ckd_calloc (1, sizeof(s3hash_entry_t));
	new->key = key;
	new->len = len;
	new->val = val;
	new->next = cur->next;
	cur->next = new;
    }

    return val;
}


int32 s3hash_enter (s3hash_table_t *h, const char *key, int32 val)
{
    uint32 hash;
    int32 len;
    
    hash = key2hash (h, key);
    len = strlen(key);
    return (enter (h, hash, key, len, val));
}


int32 s3hash_enter_bkey (s3hash_table_t *h, const char *key, int32 len, int32 val)
{
    uint32 hash;
    char *str;
    
    str = makekey ((uint8 *)key, len, NULL);
    hash = key2hash (h, str);
    ckd_free (str);
    
    return (enter (h, hash, key, len, val));
}


glist_t s3hash_tolist (s3hash_table_t *h, int32 *count)
{
    glist_t g;
    s3hash_entry_t *e;
    int32 i, j;
    
    g = NULL;
    
    j = 0;
    for (i = 0; i < h->size; i++) {
	e = &(h->table[i]);
	
	if (e->key != NULL) {
	    g = glist_add_ptr (g, (void *)e);
	    j++;
	    
	    for (e = e->next; e; e = e->next) {
		g = glist_add_ptr (g, (void *)e);
		j++;
	    }
	}
    }
    
    *count = j;
    
    return g;
}


void s3hash_free (s3hash_table_t *h)
{
    s3hash_entry_t *e, *e2;
    int32 i;
    
    /* Free additional entries created for key collision cases */
    for (i = 0; i < h->size; i++) {
	for (e = h->table[i].next; e; e = e2) {
	    e2 = e->next;
	    ckd_free ((void *) e);
	}
    }
    
    ckd_free ((void *) h->table);
    ckd_free ((void *) h);
}
