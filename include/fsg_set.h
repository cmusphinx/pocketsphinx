/* -*- c-basic-offset:4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 1999-2008 Carnegie Mellon University.  All rights
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
/**
 * @file fsg_set.h Public functions for FSG decoding.
 */

#ifndef __FSG_SET_H__
#define __FSG_SET_H__

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

/* SphinxBase headers we need. */
#include <sphinxbase/fsg_model.h>
#include <sphinxbase/hash_table.h>
#include <sphinxbase/jsgf.h>

/* PocketSphinx headers. */
#include <pocketsphinx_export.h>

/**
 * Set of finite-state grammars used in decoding.
 */
typedef struct fsg_search_s fsg_set_t;

/**
 * Iterator over finite-state grammars.
 */
typedef hash_iter_t fsg_set_iter_t;

/**
 * Get an iterator over all finite-state grammars in a set.
 */
POCKETSPHINX_EXPORT
fsg_set_iter_t *fsg_set_iter(fsg_set_t *fsgs);

/**
 * Advance an iterator to the next grammar in the set.
 */
POCKETSPHINX_EXPORT
fsg_set_iter_t *fsg_set_iter_next(fsg_set_iter_t *itor);

/**
 * Get the current rule in an FSG iterator.
 */
POCKETSPHINX_EXPORT
fsg_model_t *fsg_set_iter_fsg(fsg_set_iter_t *itor);

/**
 * Free an FSG iterator (if the end hasn't been reached).
 */
POCKETSPHINX_EXPORT
void fsg_set_iter_free(fsg_set_iter_t *itor);

/**
 * Lookup the FSG associated with the given name.
 *
 * @return The FSG associated with name, or NULL if no match found.
 */
POCKETSPHINX_EXPORT
fsg_model_t *fsg_set_get_fsg(fsg_set_t *fsgs, char const *name);

/**
 * Add the given FSG to the collection of FSGs known to this search object.
 *
 * The given fsg is simply added to the collection.  It is not automatically
 * made the currently active one.
 *
 * @param name Name to associate with this FSG.  If NULL, this will be
 *            taken from the FSG structure.
 * @return A pointer to the FSG associated with name, or NULL on
 *         failure.  If this pointer is not equal to wfsg, then there
 *         was a name conflict, and wfsg was not added.
 */
POCKETSPHINX_EXPORT
fsg_model_t *fsg_set_add(fsg_set_t *fsgs, char const *name, fsg_model_t *wfsg);

/**
 * Delete the given FSG from the known collection.
 */
POCKETSPHINX_EXPORT
fsg_model_t *fsg_set_remove(fsg_set_t *fsgs, fsg_model_t *wfsg);

/**
 * Like fsg_set_del_fsg(), but identifies the FSG by its name.
 */
POCKETSPHINX_EXPORT
fsg_model_t *fsg_set_remove_byname(fsg_set_t *fsgs, char const *name);

/**
 * Switch to a new FSG (identified by its string name).
 *
 * @return Pointer to new FSG, or NULL on failure.
 */
POCKETSPHINX_EXPORT
fsg_model_t *fsg_set_select(fsg_set_t *fsgs, const char *name);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __FSG_SET_H__ */
