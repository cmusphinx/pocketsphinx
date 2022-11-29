/* -*- c-basic-offset:4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 2014 Alpha Cephei Inc..  All rights
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
 * THIS SOFTWARE IS PROVIDED BY ALPHA CEPHEI INC. ``AS IS'' AND 
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
 * @file search.h
 * @brief Search modules 
 *
 * User can configure several "search" objects with different grammars
 * and language models and switch them in runtime to provide
 * interactive experience for the user.
 *
 * There are different possible search modes:
 * 
 * <ul>
 * <li>keyphrase - efficiently looks for keyphrase and ignores other speech. allows to configure detection threshold.</li>
 * <li>grammar - recognizes speech according to JSGF grammar. Unlike keyphrase grammar search doesn't ignore words which are not in grammar but tries to recognize them.</li>
 * <li>ngram/lm - recognizes natural speech with a language model.</li>
 * <li>allphone - recognizes phonemes with a phonetic language model.</li>
 * <li>align - creates time alignments for a fixed word sequence.</li>
 * </ul>
 * 
 * Each search module has a name and can be referenced by name. These
 * names are application-specific. The function ps_activate_search()
 * activates a search module previously added by a name. Only one
 * search module can be activated at time.
 *
 * To add the search module one needs to point to the grammar/language
 * model describing the search. The location of the grammar is
 * specific to the application.
 * 
 * The exact design of a searches depends on your application. For
 * example, you might want to listen for activation keyphrase first
 * and once keyphrase is recognized switch to ngram search to
 * recognize actual command. Once you have recognized the command, you
 * can switch to grammar search to recognize the confirmation and then
 * switch back to keyphrase listening mode to wait for another
 * command.
 *
 * If only a simple recognition is required it is sufficient to add a
 * single search or just configure the required mode with
 * configuration options.
 *
 * Because Doxygen is Bad Software, the actual function definitions
 * can only exist in \ref ps_decoder_t.  Sorry about that.
 */

#ifndef __PS_SEARCH_H__
#define __PS_SEARCH_H__

#include <pocketsphinx/model.h>
#include <pocketsphinx/alignment.h>

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

/**
 * @struct ps_search_iter_t pocketsphinx/search.h
 * @brief Iterator over search modules.
 */
typedef struct ps_search_iter_s ps_search_iter_t;

/* Forward-declare this because header files are an atrocity. */
typedef struct ps_decoder_s ps_decoder_t;

/**
 * Actives search with the provided name.
 *
 * @memberof ps_decoder_t
 * @param name Name of search module to activate. This must have been
 * previously added by either ps_add_fsg(), ps_add_lm(), or
 * ps_add_kws().  If NULL, it will re-activate the default search,
 * which is useful when running second-pass alignment, for instance.
 * @return 0 on success, -1 on failure
 */
POCKETSPHINX_EXPORT
int ps_activate_search(ps_decoder_t *ps, const char *name);

/**
 * Returns name of current search in decoder
 *
 * @memberof ps_decoder_t
 * @see ps_activate_search
 */
POCKETSPHINX_EXPORT 
const char* ps_current_search(ps_decoder_t *ps);

/**
 * Removes a search module and releases its resources.
 *
 * Removes a search module previously added with
 * using ps_add_fsg(), ps_add_lm(), ps_add_kws(), etc.
 *
 * @memberof ps_decoder_t
 * @see ps_add_fsg
 * @see ps_add_lm
 * @see ps_add_kws
 */
POCKETSPHINX_EXPORT
int ps_remove_search(ps_decoder_t *ps, const char *name);

/**
 * Returns iterator over current searches 
 *
 * @memberof ps_decoder_t
 * @see ps_activate_search
 */
POCKETSPHINX_EXPORT
ps_search_iter_t *ps_search_iter(ps_decoder_t *ps);

/**
 * Updates search iterator to point to the next position.
 * 
 * @memberof ps_search_iter_t
 * This function automatically frees the iterator object upon reaching
 * the final entry.
 * @see ps_activate_search
 */
POCKETSPHINX_EXPORT
ps_search_iter_t *ps_search_iter_next(ps_search_iter_t *itor);

/**
 * Retrieves the name of the search the iterator points to.
 *
 * @memberof ps_search_iter_t
 * @see ps_activate_search
 */
POCKETSPHINX_EXPORT
const char* ps_search_iter_val(ps_search_iter_t *itor);

/**
 * Delete an unfinished search iterator
 *
 * @memberof ps_search_iter_t
 * @see ps_activate_search
 */
POCKETSPHINX_EXPORT
void ps_search_iter_free(ps_search_iter_t *itor);

/**
 * Updates search iterator to point to the next position.
 * 
 * @memberof ps_search_iter_t
 * This function automatically frees the iterator object upon reaching
 * the final entry.
 * @see ps_activate_search
 */
POCKETSPHINX_EXPORT
const char* ps_search_iter_val(ps_search_iter_t *itor);

/**
 * Get the language model or lmset object associated with a search.
 *
 * @memberof ps_decoder_t
 * @arg name Name of language model search, or NULL for current search.
 * @return The language model (possibly set of language models) object
 *         for this decoder.  The decoder retains ownership of this
 *         pointer, so you should not attempt to free it manually.
 *         Use ngram_model_retain() if you wish to reuse it elsewhere.
 */
POCKETSPHINX_EXPORT 
ngram_model_t *ps_get_lm(ps_decoder_t *ps, const char *name);

/**
 * Adds new search based on N-gram language model.
 *
 * Associates N-gram search with the provided name. The search can be activated
 * using ps_activate_search().
 *
 * @memberof ps_decoder_t
 * @see ps_activate_search.
 */ 
POCKETSPHINX_EXPORT
int ps_add_lm(ps_decoder_t *ps, const char *name, ngram_model_t *lm);

/**
 * Adds new search based on N-gram language model.
 *
 * Convenient method to load N-gram model and create a search.
 * 
 * @memberof ps_decoder_t
 * @see ps_add_lm
 */
POCKETSPHINX_EXPORT
int ps_add_lm_file(ps_decoder_t *ps, const char *name, const char *path);

/**
 * Get the finite-state grammar set object associated with a search.
 *
 * @memberof ps_decoder_t
 * @arg name Name of FSG search, or NULL for current search.
 * @return The current FSG set object for this decoder, or
 *         NULL if `name` does not correspond to an FSG search.
 */
POCKETSPHINX_EXPORT
fsg_model_t *ps_get_fsg(ps_decoder_t *ps, const char *name);

/**
 * Adds new search based on finite state grammar.
 *
 * Associates FSG search with the provided name. The search can be
 * activated using ps_activate_search().
 *
 * @memberof ps_decoder_t
 * @see ps_activate_search
 */
POCKETSPHINX_EXPORT
int ps_add_fsg(ps_decoder_t *ps, const char *name, fsg_model_t *fsg);

/**
 * Adds new search using JSGF model.
 *
 * Convenient method to load JSGF model and create a search.
 *
 * @memberof ps_decoder_t
 * @see ps_add_fsg
 */
POCKETSPHINX_EXPORT
int ps_add_jsgf_file(ps_decoder_t *ps, const char *name, const char *path);

/**
 * Adds new search using JSGF model.
 *
 * Convenience method to parse JSGF model from string and create a search.
 *
 * @memberof ps_decoder_t
 * @see ps_add_fsg
 */
POCKETSPHINX_EXPORT
int ps_add_jsgf_string(ps_decoder_t *ps, const char *name, const char *jsgf_string);

/**
 * Get the keyphrase associated with a KWS search
 *
 * @memberof ps_decoder_t
 * @arg name Name of KWS search, or NULL for current search.
 * @return The current keyphrase to spot, or NULL if `name` does not
 * correspond to a KWS search
 */
POCKETSPHINX_EXPORT 
const char* ps_get_kws(ps_decoder_t *ps, const char *name);

/**
 * Adds keyphrases from a file to spotting
 *
 * Associates KWS search with the provided name. The search can be activated
 * using ps_activate_search().
 *
 * @memberof ps_decoder_t
 * @see ps_activate_search
 */
POCKETSPHINX_EXPORT 
int ps_add_kws(ps_decoder_t *ps, const char *name, const char *keyfile);

/**
 * Adds new keyphrase to spot
 *
 * Associates KWS search with the provided name. The search can be activated
 * using ps_activate_search().
 *
 * @memberof ps_decoder_t
 * @see ps_activate_search
 */
POCKETSPHINX_EXPORT 
int ps_add_keyphrase(ps_decoder_t *ps, const char *name, const char *keyphrase);

/**
 * Adds new search based on phone N-gram language model.
 *
 * Associates N-gram search with the provided name. The search can be activated
 * using ps_activate_search().
 *
 * @memberof ps_decoder_t
 * @see ps_activate_search.
 */ 
POCKETSPHINX_EXPORT
int ps_add_allphone(ps_decoder_t *ps, const char *name, ngram_model_t *lm);

/**
 * Adds new search based on phone N-gram language model.
 *
 * Convenient method to load N-gram model and create a search.
 * 
 * @memberof ps_decoder_t
 * @see ps_add_allphone
 */
POCKETSPHINX_EXPORT
int ps_add_allphone_file(ps_decoder_t *ps, const char *name, const char *path);

/**
 * Set up decoder to force-align a word sequence.
 *
 * Unlike the `ps_add_*` functions, this activates the search module
 * immediately, since force-alignment is nearly always a single shot.
 * Currently "under the hood" this is an FSG search but you shouldn't
 * depend on that.  The search module activated is *not* the default
 * search, so you can return to that one by calling ps_activate_search
 * with `NULL`.
 *
 * Decoding proceeds as normal, though only this word sequence will be
 * recognized, with silences and alternate pronunciations inserted.
 * Word alignments are available with ps_seg_iter().  To obtain
 * phoneme or state segmentations, you must subsequently call
 * ps_set_alignment() and re-run decoding.  It's tough son, but it's life.
 *
 * @memberof ps_decoder_t
 * @param ps Decoder
 * @param words String containing whitespace-separated words for alignment.
 *              These words are assumed to exist in the current dictionary.
 * 
 */
POCKETSPHINX_EXPORT
int ps_set_align_text(ps_decoder_t *ps, const char *words);

/**
 * Set up decoder to run phone and state-level alignment.
 *
 * Unlike the `ps_add_*` functions, this activates the search module
 * immediately, since force-alignment is nearly always a single shot.
 *
 * To align, run or re-run decoding as usual, then call
 * ps_get_alignment() to get the resulting alignment.  Note that if
 * you call this function *before* rerunning decoding, you can obtain
 * the phone and state sequence, but the durations will be invalid
 * (phones and states will inherit the parent word's duration).
 *
 * @memberof ps_decoder_t
 * @param ps Decoder object.
 * @param al Usually NULL, which means to construct an alignment from
 *           the current search hypothesis (this does not work with
 *           allphone or keyword spotting).  You can also pass a
 *           ps_alignment_t here if you have one.  The search will
 *           retain but not copy it, so after running decoding it will
 *           be updated with new durations.  You can set starts and
 *           durations for words or phones (not states) to constrain
 *           the alignment.
 * @return 0 for success, -1 for error (if there is no search
 *         hypothesis, or it cannot be aligned due to missing word
 *         IDs)
 */
POCKETSPHINX_EXPORT
int ps_set_alignment(ps_decoder_t *ps, ps_alignment_t *al);

/**
 * Get the alignment associated with the current search module.
 *
 * As noted above, if decoding has not been run, this will contain
 * invalid durations, but that may still be useful if you just want to
 * know the state sequence.
 *
 * @memberof ps_decoder_t
 * @return Current alignment, or NULL if none.  This pointer is owned
 *         by the decoder, so you must call ps_alignment_retain() on
 *         it if you wish to keep it outside the lifetime of the
 *         decoder.
 */
POCKETSPHINX_EXPORT
ps_alignment_t *ps_get_alignment(ps_decoder_t *ps);

#ifdef __cplusplus
}
#endif

#endif /* __PS_SEARCH_H__ */
