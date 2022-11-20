/* -*- c-basic-offset:4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 1999-2022 Carnegie Mellon University.  All rights
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
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 */
/**
 * @file model.h
 * @brief Public API for language models
 *
 * Because doxygen is Bad Software, the actual documentation can only
 * exist in \ref jsgf_t, \ref fsg_model_t, and \ref ngram_model_t.
 * Sorry about that.
 */

#ifndef __PS_MODEL_H__
#define __PS_MODEL_H__

#include <stdio.h>

#include <pocketsphinx/prim_type.h>
#include <pocketsphinx/logmath.h>
#include <pocketsphinx/export.h>

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

/* Forward declaration to avoid include loops */
typedef struct cmd_ln_s ps_config_t;

/**
 * @struct jsgf_t pocketsphinx/model.h
 * @brief JSGF parser
 */
typedef struct jsgf_s jsgf_t;

/**
 * @struct jsgf_rule_t pocketsphinx/model.h
 * @brief Rule in a parsed JSGF grammar.
 */
typedef struct jsgf_rule_s jsgf_rule_t;

/**
 * @struct fsg_model_t pocketsphinx/model.h
 * @brief Finite-state grammar.
 *
 * States are simply integers 0..n_state-1.
 * A transition emits a word and has a given probability of being taken.
 * There can also be null or epsilon transitions, with no associated emitted
 * word.
 */
typedef struct fsg_model_s fsg_model_t;

/**
 * @struct ngram_model_t pocketsphinx/model.h
 * @brief N-Gram based language model.
 */
typedef struct ngram_model_s ngram_model_t;

/**
 * Parse a JSGF grammar from a file.
 *
 * @memberof jsgf_t
 * @param filename the name of the file to parse.
 * @param parent optional parent grammar for this one (NULL, usually).
 * @return new JSGF grammar object, or NULL on failure.
 */
POCKETSPHINX_EXPORT
jsgf_t *jsgf_parse_file(const char *filename, jsgf_t *parent);

/**
 * Parse a JSGF grammar from a string.
 *
 * @memberof jsgf_t
 * @param string 0-terminated string with grammar.
 * @param parent optional parent grammar for this one (NULL, usually).
 * @return new JSGF grammar object, or NULL on failure.
 */
POCKETSPHINX_EXPORT
jsgf_t *jsgf_parse_string(const char *string, jsgf_t *parent);

/**
 * Get the grammar name from the file.
 * @memberof jsgf_t
 */
POCKETSPHINX_EXPORT
char const *jsgf_grammar_name(jsgf_t *jsgf);

/**
 * Free a JSGF grammar.
 * @memberof jsgf_t
 */
POCKETSPHINX_EXPORT
void jsgf_grammar_free(jsgf_t *jsgf);

/**
 * Get a rule by name from a grammar. Name should not contain brackets.
 * @memberof jsgf_t
 */
POCKETSPHINX_EXPORT
jsgf_rule_t *jsgf_get_rule(jsgf_t *grammar, const char *name);

/**
 * Returns the first public rule of the grammar
 * @memberof jsgf_t
 */
POCKETSPHINX_EXPORT
jsgf_rule_t *jsgf_get_public_rule(jsgf_t *grammar);

/**
 * Get the rule name from a rule.
 * @memberof jsgf_rule_t
 */
POCKETSPHINX_EXPORT
char const *jsgf_rule_name(jsgf_rule_t *rule);

/**
 * Test if a rule is public or not.
 * @memberof jsgf_rule_t
 */
POCKETSPHINX_EXPORT
int jsgf_rule_public(jsgf_rule_t *rule);

/**
 * @struct jsgf_rule_iter_t
 * @brief Iterator over rules in a grammar.
 */
typedef struct hash_iter_s jsgf_rule_iter_t;

/**
 * Get an iterator over all rules in a grammar.
 * @memberof jsgf_t
 */
POCKETSPHINX_EXPORT
jsgf_rule_iter_t *jsgf_rule_iter(jsgf_t *grammar);

/**
 * Advance an iterator to the next rule in the grammar.
 * @memberof jsgf_rule_iter_t
 */
POCKETSPHINX_EXPORT
jsgf_rule_iter_t *jsgf_rule_iter_next(jsgf_rule_iter_t *itor);

/**
 * Get the current rule in a rule iterator.
 * @memberof jsgf_rule_iter_t
 */
POCKETSPHINX_EXPORT
jsgf_rule_t *jsgf_rule_iter_rule(jsgf_rule_iter_t *itor);

/**
 * Free a rule iterator (if the end hasn't been reached).
 * @memberof jsgf_rule_iter_t
 */
POCKETSPHINX_EXPORT
void jsgf_rule_iter_free(jsgf_rule_iter_t *itor);

/**
 * Build a Sphinx FSG object from a JSGF rule.
 * @memberof jsgf_t
 */
POCKETSPHINX_EXPORT
fsg_model_t *jsgf_build_fsg(jsgf_t *grammar, jsgf_rule_t *rule,
                            logmath_t *lmath, float32 lw);

/**
 * Read JSGF from file and return FSG object from it.
 *
 * This function looks for a first public rule in jsgf and constructs JSGF from it.
 * @memberof fsg_model_t
 */
POCKETSPHINX_EXPORT
fsg_model_t *jsgf_read_file(const char *file, logmath_t * lmath, float32 lw);

/**
 * Read JSGF from string and return FSG object from it.
 *
 * This function looks for a first public rule in jsgf and constructs JSGF from it.
 * @memberof fsg_model_t
 */
POCKETSPHINX_EXPORT
fsg_model_t *jsgf_read_string(const char *string, logmath_t * lmath, float32 lw);

/**
 * Convert a JSGF rule to Sphinx FSG text form.
 *
 * This does a direct conversion without doing transitive closure on
 * null transitions and so forth.
 * @memberof jsgf_t
 */
POCKETSPHINX_EXPORT
int jsgf_write_fsg(jsgf_t *grammar, jsgf_rule_t *rule, FILE *outfh);

/**
 * Retain ownership of an FSG.
 *
 * @return Pointer to retained FSG.
 * @memberof fsg_model_t
 */
POCKETSPHINX_EXPORT
fsg_model_t *fsg_model_retain(fsg_model_t *fsg);

/**
 * Free the given word FSG.
 *
 * @memberof fsg_model_t
 * @return new reference count (0 if freed completely)
 */
POCKETSPHINX_EXPORT
int fsg_model_free(fsg_model_t *fsg);

/**
 * Read a word FSG from the given file and return a pointer to the structure
 * created.  Return NULL if any error occurred.
 * 
 * File format:
 * 
 * <pre>
 *   Any number of comment lines; ignored
 *   FSG_BEGIN [<fsgname>]
 *   N <#states>
 *   S <start-state ID>
 *   F <final-state ID>
 *   T <from-state> <to-state> <prob> [<word-string>]
 *   T ...
 *   ... (any number of state transitions)
 *   FSG_END
 *   Any number of comment lines; ignored
 * </pre>
 * 
 * The FSG spec begins with the line containing the keyword FSG_BEGIN.
 * It has an optional fsg name string.  If not present, the FSG has the empty
 * string as its name.
 * 
 * Following the FSG_BEGIN declaration is the number of states, the start
 * state, and the final state, each on a separate line.  States are numbered
 * in the range [0 .. <numberofstate>-1].
 * 
 * These are followed by all the state transitions, each on a separate line,
 * and terminated by the FSG_END line.  A state transition has the given
 * probability of being taken, and emits the given word.  The word emission
 * is optional; if word-string omitted, it is an epsilon or null transition.
 * 
 * Comments can also be embedded within the FSG body proper (i.e. between
 * FSG_BEGIN and FSG_END): any line with a # character in col 1 is treated
 * as a comment line.
 * 
 * Return value: a new fsg_model_t structure if the file is successfully
 * read, NULL otherwise.
 * @memberof fsg_model_t
 */
POCKETSPHINX_EXPORT
fsg_model_t *fsg_model_readfile(const char *file, logmath_t *lmath, float32 lw);

/**
 * Like fsg_model_readfile(), but from an already open stream.
 * @memberof fsg_model_t
 */
POCKETSPHINX_EXPORT
fsg_model_t *fsg_model_read(FILE *fp, logmath_t *lmath, float32 lw);

/**
 * Check that an FSG accepts a word sequence
 *
 * @memberof fsg_model_t
 * @param words Whitespace-separated word sequence
 * @return 1 if accepts, 0 if not.
 */
POCKETSPHINX_EXPORT
int fsg_model_accept(fsg_model_t *fsg, char const *words);

/**
 * Write FSG to a file.
 * @memberof fsg_model_t
 */
POCKETSPHINX_EXPORT
void fsg_model_write(fsg_model_t *fsg, FILE *fp);

/**
 * Write FSG to a file.
 * @memberof fsg_model_t
 */
POCKETSPHINX_EXPORT
void fsg_model_writefile(fsg_model_t *fsg, char const *file);

/**
 * Write FSG to a file in AT&T FSM format.
 * @memberof fsg_model_t
 */
POCKETSPHINX_EXPORT
void fsg_model_write_fsm(fsg_model_t *fsg, FILE *fp);

/**
 * Write FSG to a file in AT&T FSM format.
 * @memberof fsg_model_t
 */
POCKETSPHINX_EXPORT
void fsg_model_writefile_fsm(fsg_model_t *fsg, char const *file);

/**
 * Write FSG symbol table to a file (for AT&T FSM)
 * @memberof fsg_model_t
 */
POCKETSPHINX_EXPORT
void fsg_model_write_symtab(fsg_model_t *fsg, FILE *file);

/**
 * Write FSG symbol table to a file (for AT&T FSM)
 * @memberof fsg_model_t
 */
POCKETSPHINX_EXPORT
void fsg_model_writefile_symtab(fsg_model_t *fsg, char const *file);

/**
 * @struct ngram_class_t pocketsphinx/model.h
 * @brief Word class in an N-Gram model.
 */
typedef struct ngram_class_s ngram_class_t;

/**
 * @enum ngram_file_type_e pocketsphinx/model.h
 * @brief File types for N-Gram files
 */
typedef enum ngram_file_type_e {
    NGRAM_INVALID = -1, /**< Not a valid file type. */
    NGRAM_AUTO,  /**< Determine file type automatically. */
    NGRAM_ARPA,  /**< ARPABO text format (the standard). */
    NGRAM_BIN    /**< Sphinx .DMP format. */
} ngram_file_type_t;

#define NGRAM_INVALID_WID -1 /**< Impossible word ID */

/**
 * Read an N-Gram model from a file on disk.
 *
 * @param config Optional pointer to a set of command-line arguments.
 * Recognized arguments are:
 *
 *  - -mmap (boolean) whether to use memory-mapped I/O
 *  - -lw (float32) language weight to apply to the model
 *  - -wip (float32) word insertion penalty to apply to the model
 *
 * @memberof ngram_model_t
 * @param file_name path to the file to read.
 * @param file_type type of the file, or NGRAM_AUTO to determine automatically.
 * @param lmath Log-math parameters to use for probability
 *              calculations.  Ownership of this object is assumed by
 *              the newly created ngram_model_t, and you should not
 *              attempt to free it manually.  If you wish to reuse it
 *              elsewhere, you must retain it with logmath_retain().
 * @return newly created ngram_model_t.
 */
POCKETSPHINX_EXPORT
ngram_model_t *ngram_model_read(ps_config_t *config,
				const char *file_name,
                                ngram_file_type_t file_type,
				logmath_t *lmath);

/**
 * Write an N-Gram model to disk.
 *
 * @memberof ngram_model_t
 * @return 0 for success, <0 on error
 */
POCKETSPHINX_EXPORT
int ngram_model_write(ngram_model_t *model, const char *file_name,
		      ngram_file_type_t format);

/**
 * Guess the file type for an N-Gram model from the filename.
 *
 * @memberof ngram_model_t
 * @return the guessed file type, or NGRAM_INVALID if none could be guessed.
 */
POCKETSPHINX_EXPORT
ngram_file_type_t ngram_file_name_to_type(const char *file_name);

/**
 * Get the N-Gram file type from a string.
 *
 * @memberof ngram_model_t
 * @return file type, or NGRAM_INVALID if no such file type exists.
 */
POCKETSPHINX_EXPORT
ngram_file_type_t ngram_str_to_type(const char *str_name);

/**
 * Get the canonical name for an N-Gram file type.
 *
 * @memberof ngram_model_t
 * @return read-only string with the name for this file type, or NULL
 * if no such type exists.
 */
POCKETSPHINX_EXPORT
char const *ngram_type_to_str(int type);

/**
 * Retain ownership of an N-Gram model.
 *
 * @memberof ngram_model_t
 * @return Pointer to retained model.
 */
POCKETSPHINX_EXPORT
ngram_model_t *ngram_model_retain(ngram_model_t *model);

/**
 * Release memory associated with an N-Gram model.
 *
 * @memberof ngram_model_t
 * @return new reference count (0 if freed completely)
 */
POCKETSPHINX_EXPORT
int ngram_model_free(ngram_model_t *model);

/**
 * @enum ngram_case_e pocketsphinx/model.h
 * @brief Constants for case folding.
 */
typedef enum ngram_case_e {
    NGRAM_UPPER,  /**< Upper case */
    NGRAM_LOWER   /**< Lower case */
} ngram_case_t;

/**
 * Case-fold word strings in an N-Gram model.
 *
 * WARNING: This is not Unicode aware, so any non-ASCII characters
 * will not be converted.
 *
 * @memberof ngram_model_t
 */
POCKETSPHINX_EXPORT
int ngram_model_casefold(ngram_model_t *model, int kase);

/**
 * Apply a language weight, insertion penalty, and unigram weight to a
 * language model.
 *
 * This will change the values output by ngram_score() and friends.
 * This is done for efficiency since in decoding, these are the only
 * values we actually need.  Call ngram_prob() if you want the "raw"
 * N-Gram probability estimate.
 *
 * To remove all weighting, call ngram_apply_weights(model, 1.0, 1.0).
 *
 * @memberof ngram_model_t
 */
POCKETSPHINX_EXPORT
int ngram_model_apply_weights(ngram_model_t *model,
                              float32 lw, float32 wip);

/**
 * Get the current weights from a language model.
 *
 * @memberof ngram_model_t
 * @param model The model in question.
 * @param out_log_wip Output: (optional) logarithm of word insertion penalty.
 * @return language weight.
 */
POCKETSPHINX_EXPORT
float32 ngram_model_get_weights(ngram_model_t *model, int32 *out_log_wip);

/**
 * Get the score (scaled, interpolated log-probability) for a general
 * N-Gram.
 *
 * The argument list consists of the history words (as null-terminated
 * strings) of the N-Gram, <b>in reverse order</b>, followed by NULL.
 * Therefore, if you wanted to get the N-Gram score for "a whole joy",
 * you would call:
 *
 * <pre>
 *  score = ngram_score(model, "joy", "whole", "a", NULL);
 * </pre>
 *
 * This is not the function to use in decoding, because it has some
 * overhead for looking up words.  Use ngram_ng_score(),
 * ngram_tg_score(), or ngram_bg_score() instead.  In the future there
 * will probably be a version that takes a general language model
 * state object, to support suffix-array LM and things like that.
 *
 * If one of the words is not in the LM's vocabulary, the result will
 * depend on whether this is an open or closed vocabulary language
 * model.  For an open-vocabulary model, unknown words are all mapped
 * to the unigram &lt;UNK&gt; which has a non-zero probability and also
 * participates in higher-order N-Grams.  Therefore, you will get a
 * score of some sort in this case.
 *
 * For a closed-vocabulary model, unknown words are impossible and
 * thus have zero probability.  Therefore, if <code>word</code> is
 * unknown, this function will return a "zero" log-probability, i.e. a
 * large negative number.  To obtain this number for comparison, call
 * ngram_zero().
 *
 * @memberof ngram_model_t
 */
POCKETSPHINX_EXPORT
int32 ngram_score(ngram_model_t *model, const char *word, ...);

/**
 * Quick trigram score lookup.
 *
 * @memberof ngram_model_t
 */
POCKETSPHINX_EXPORT
int32 ngram_tg_score(ngram_model_t *model,
                     int32 w3, int32 w2, int32 w1,
                     int32 *n_used);

/**
 * Quick bigram score lookup.
 *
 * @memberof ngram_model_t
 */
POCKETSPHINX_EXPORT
int32 ngram_bg_score(ngram_model_t *model,
                     int32 w2, int32 w1,
                     int32 *n_used);

/**
 * Quick general N-Gram score lookup.
 *
 * @memberof ngram_model_t
 */
POCKETSPHINX_EXPORT
int32 ngram_ng_score(ngram_model_t *model, int32 wid, int32 *history,
                     int32 n_hist, int32 *n_used);

/**
 * Get the "raw" log-probability for a general N-Gram.
 *
 * This returns the log-probability of an N-Gram, as defined in the
 * language model file, before any language weighting, interpolation,
 * or insertion penalty has been applied.
 *
 * @memberof ngram_model_t
 * @note When backing off to a unigram from a bigram or trigram, the
 * unigram weight (interpolation with uniform) is not removed.
 */
POCKETSPHINX_EXPORT
int32 ngram_probv(ngram_model_t *model, const char *word, ...);

/**
 * Get the "raw" log-probability for a general N-Gram.
 *
 * This returns the log-probability of an N-Gram, as defined in the
 * language model file, before any language weighting, interpolation,
 * or insertion penalty has been applied.
 *
 * @note When backing off to a unigram from a bigram or trigram, the
 * unigram weight (interpolation with uniform) is not removed.
 * @memberof ngram_model_t
 */
POCKETSPHINX_EXPORT
int32 ngram_prob(ngram_model_t *model, const char* const *words, int32 n);

/**
 * Quick "raw" probability lookup for a general N-Gram.
 *
 * See documentation for ngram_ng_score() and ngram_apply_weights()
 * for an explanation of this.
 *
 * @memberof ngram_model_t
 */
POCKETSPHINX_EXPORT
int32 ngram_ng_prob(ngram_model_t *model, int32 wid, int32 *history,
                    int32 n_hist, int32 *n_used);

/**
 * Convert score to "raw" log-probability.
 *
 * @note The unigram weight (interpolation with uniform) is not
 * removed, since there is no way to know which order of N-Gram
 * generated <code>score</code>.
 * 
 * @memberof ngram_model_t
 * @param model The N-Gram model from which score was obtained.
 * @param score The N-Gram score to convert
 * @return The raw log-probability value.
 */
POCKETSPHINX_EXPORT
int32 ngram_score_to_prob(ngram_model_t *model, int32 score);

/**
 * Look up numerical word ID.
 *
 * @memberof ngram_model_t
 */
POCKETSPHINX_EXPORT
int32 ngram_wid(ngram_model_t *model, const char *word);

/**
 * Look up word string for numerical word ID.
 *
 * @memberof ngram_model_t
 */
POCKETSPHINX_EXPORT
const char *ngram_word(ngram_model_t *model, int32 wid);

/**
 * Get the unknown word ID for a language model.
 *
 * Language models can be either "open vocabulary" or "closed
 * vocabulary".  The difference is that the former assigns a fixed
 * non-zero unigram probability to unknown words, while the latter
 * does not allow unknown words (or, equivalently, it assigns them
 * zero probability).  If this is a closed vocabulary model, this
 * function will return NGRAM_INVALID_WID.
 *
 * @memberof ngram_model_t
 * @return The ID for the unknown word, or NGRAM_INVALID_WID if none
 * exists.
 */
POCKETSPHINX_EXPORT
int32 ngram_unknown_wid(ngram_model_t *model);

/**
 * Get the "zero" log-probability value for a language model.
 *
 * @memberof ngram_model_t
 */
POCKETSPHINX_EXPORT
int32 ngram_zero(ngram_model_t *model);

/**
 * Get the order of the N-gram model (i.e. the "N" in "N-gram")
 *
 * @memberof ngram_model_t
 */
POCKETSPHINX_EXPORT
int32 ngram_model_get_size(ngram_model_t *model);

/**
 * Get the counts of the various N-grams in the model.
 *
 * @memberof ngram_model_t
 */
POCKETSPHINX_EXPORT
uint32 const *ngram_model_get_counts(ngram_model_t *model);

/**
 * @struct ngram_iter_t pocketsphinx/model.h
 * @brief M-gram (yes, **M**-gram) iterator object.
 *
 * This is an iterator over the N-Gram successors of a given word or
 * N-1-Gram, that is why it is called "M" and not "N".
 */
typedef struct ngram_iter_s ngram_iter_t;

/**
 * Iterate over all M-grams.
 *
 * @memberof ngram_model_t
 * @param model Language model to query.
 * @param m Order of the M-Grams requested minus one (i.e. order of the history)
 * @return An iterator over the requested M, or NULL if no N-grams of
 * order M+1 exist.
 */
POCKETSPHINX_EXPORT
ngram_iter_t *ngram_model_mgrams(ngram_model_t *model, int m);

/**
 * Get an iterator over M-grams pointing to the specified M-gram.
 *
 * @memberof ngram_model_t
 */
POCKETSPHINX_EXPORT
ngram_iter_t *ngram_iter(ngram_model_t *model, const char *word, ...);

/**
 * Get an iterator over M-grams pointing to the specified M-gram.
 *
 * @memberof ngram_model_t
 */
POCKETSPHINX_EXPORT
ngram_iter_t *ngram_ng_iter(ngram_model_t *model, int32 wid, int32 *history, int32 n_hist);

/**
 * Get information from the current M-gram in an iterator.
 *
 * @memberof ngram_iter_t
 * @param out_score Output: Score for this M-gram (including any word
 *                          penalty and language weight).
 * @param out_bowt Output: Backoff weight for this M-gram.
 * @return read-only array of word IDs.
 */
POCKETSPHINX_EXPORT
int32 const *ngram_iter_get(ngram_iter_t *itor,
                            int32 *out_score,
                            int32 *out_bowt);

/**
 * Iterate over all M-gram successors of an M-1-gram.
 *
 * @memberof ngram_iter_t
 * @param itor Iterator pointing to the M-1-gram to get successors of.
 */
POCKETSPHINX_EXPORT
ngram_iter_t *ngram_iter_successors(ngram_iter_t *itor);

/**
 * Advance an M-gram iterator.
 * @memberof ngram_iter_t
 */
POCKETSPHINX_EXPORT
ngram_iter_t *ngram_iter_next(ngram_iter_t *itor);

/**
 * Terminate an M-gram iterator.
 * @memberof ngram_iter_t
 */
POCKETSPHINX_EXPORT
void ngram_iter_free(ngram_iter_t *itor);

/**
 * Add a word (unigram) to the language model.
 *
 * @note The semantics of this are not particularly well-defined for
 * model sets, and may be subject to change.  Currently this will add
 * the word to all of the submodels
 *
 * @memberof ngram_model_t
 * @param model The model to add a word to.
 * @param word Text of the word to add.
 * @param weight Weight of this word relative to the uniform distribution.
 * @return The word ID for the new word.
 */
POCKETSPHINX_EXPORT
int32 ngram_model_add_word(ngram_model_t *model,
                           const char *word, float32 weight);

/**
 * Read a class definition file and add classes to a language model.
 *
 * This function assumes that the class tags have already been defined
 * as unigrams in the language model.  All words in the class
 * definition will be added to the vocabulary as special in-class words.
 * For this reason is is necessary that they not have the same names
 * as any words in the general unigram distribution.  The convention
 * is to suffix them with ":class_tag", where class_tag is the class
 * tag minus the enclosing square brackets.
 *
 * @memberof ngram_model_t
 * @return 0 for success, <0 for error
 */
POCKETSPHINX_EXPORT
int32 ngram_model_read_classdef(ngram_model_t *model,
                                const char *file_name);

/**
 * Add a new class to a language model.
 *
 * If <code>classname</code> already exists in the unigram set for
 * <code>model</code>, then it will be converted to a class tag, and
 * <code>classweight</code> will be ignored.  Otherwise, a new unigram
 * will be created as in ngram_model_add_word().
 * @memberof ngram_model_t
 */
POCKETSPHINX_EXPORT
int32 ngram_model_add_class(ngram_model_t *model,
                            const char *classname,
                            float32 classweight,
                            char **words,
                            const float32 *weights,
                            int32 n_words);

/**
 * Add a word to a class in a language model.
 *
 * @memberof ngram_model_t
 * @param model The model to add a word to.
 * @param classname Name of the class to add this word to.
 * @param word Text of the word to add.
 * @param weight Weight of this word relative to the within-class uniform distribution.
 * @return The word ID for the new word.
 */
POCKETSPHINX_EXPORT
int32 ngram_model_add_class_word(ngram_model_t *model,
                                 const char *classname,
                                 const char *word,
                                 float32 weight);

/**
 * Create a set of language models sharing a common space of word IDs.
 *
 * This function creates a meta-language model which groups together a
 * set of language models, synchronizing word IDs between them.  To
 * use this language model, you can either select a submodel to use
 * exclusively using ngram_model_set_select(), or interpolate
 * between scores from all models.  To do the latter, you can either
 * pass a non-NULL value of the <code>weights</code> parameter, or
 * re-activate interpolation later on by calling
 * ngram_model_set_interp().
 *
 * In order to make this efficient, there are some restrictions on the
 * models that can be grouped together.  The most important (and
 * currently the only) one is that they <strong>must</strong> all
 * share the same log-math parameters.
 *
 * @memberof ngram_model_t
 * @param config Any configuration parameters to be shared between models.
 * @param models Array of pointers to previously created language models.
 * @param names Array of strings to use as unique identifiers for LMs.
 * @param weights Array of weights to use in interpolating LMs, or NULL
 *                for no interpolation.
 * @param n_models Number of elements in the arrays passed to this function.
 */
POCKETSPHINX_EXPORT
ngram_model_t *ngram_model_set_init(ps_config_t *config,
                                    ngram_model_t **models,
                                    char **names,
                                    const float32 *weights,
                                    int32 n_models);

/**
 * Read a set of language models from a control file.
 *
 * This file creates a language model set from a "control file" of
 * the type used in Sphinx-II and Sphinx-III.
 * File format (optional stuff is indicated by enclosing in []):
 * 
 * <pre>
 *   [{ LMClassFileName LMClassFilename ... }]
 *   TrigramLMFileName LMName [{ LMClassName LMClassName ... }]
 *   TrigramLMFileName LMName [{ LMClassName LMClassName ... }]
 *   ...
 * (There should be whitespace around the { and } delimiters.)
 * </pre>
 * 
 * This is an extension of the older format that had only TrigramLMFilenName
 * and LMName pairs.  The new format allows a set of LMClass files to be read
 * in and referred to by the trigram LMs.
 * 
 * No "comments" allowed in this file.
 *
 * @memberof ngram_model_t
 * @param config Configuration parameters.
 * @param lmctlfile Path to the language model control file.
 * @param lmath Log-math parameters to use for probability
 *              calculations.  Ownership of this object is assumed by
 *              the newly created ngram_model_t, and you should not
 *              attempt to free it manually.  If you wish to reuse it
 *              elsewhere, you must retain it with logmath_retain().
 * @return newly created language model set.
 */
POCKETSPHINX_EXPORT
ngram_model_t *ngram_model_set_read(ps_config_t *config,
                                    const char *lmctlfile,
                                    logmath_t *lmath);

/**
 * Returns the number of language models in a set.
 * @memberof ngram_model_t
 */
POCKETSPHINX_EXPORT
int32 ngram_model_set_count(ngram_model_t *set);

/**
 * @struct ngram_model_set_iter_t pocketsphinx/model.h
 * @brief Iterator over language models in a set.
 */
typedef struct ngram_model_set_iter_s ngram_model_set_iter_t;

/**
 * Begin iterating over language models in a set.
 *
 * @memberof ngram_model_t
 * @return iterator pointing to the first language model, or NULL if no models remain.
 */
POCKETSPHINX_EXPORT
ngram_model_set_iter_t *ngram_model_set_iter(ngram_model_t *set);

/**
 * Move to the next language model in a set.
 *
 * @memberof ngram_model_set_iter_t
 * @return iterator pointing to the next language model, or NULL if no models remain.
 */
POCKETSPHINX_EXPORT
ngram_model_set_iter_t *ngram_model_set_iter_next(ngram_model_set_iter_t *itor);

/**
 * Finish iteration over a langauge model set.
 * @memberof ngram_model_set_iter_t
 */
POCKETSPHINX_EXPORT
void ngram_model_set_iter_free(ngram_model_set_iter_t *itor);

/**
 * Get language model and associated name from an iterator.
 *
 * @memberof ngram_model_set_iter_t
 * @param itor the iterator
 * @param lmname Output: string name associated with this language model.
 * @return Language model pointed to by this iterator.
 */
POCKETSPHINX_EXPORT
ngram_model_t *ngram_model_set_iter_model(ngram_model_set_iter_t *itor,
                                          char const **lmname);

/**
 * Select a single language model from a set for scoring.
 *
 * @memberof ngram_model_t
 * @return the newly selected language model, or NULL if no language
 * model by that name exists.
 */
POCKETSPHINX_EXPORT
ngram_model_t *ngram_model_set_select(ngram_model_t *set,
                                      const char *name);

/**
 * Look up a language model by name from a set.
 *
 * @memberof ngram_model_t
 * @return language model corresponding to <code>name</code>, or NULL
 * if no language model by that name exists.
 */
POCKETSPHINX_EXPORT
ngram_model_t *ngram_model_set_lookup(ngram_model_t *set,
                                      const char *name);

/**
 * Get the current language model name, if any.
 * @memberof ngram_model_t
 */
POCKETSPHINX_EXPORT
const char *ngram_model_set_current(ngram_model_t *set);

/**
 * Set interpolation weights for a set and enables interpolation.
 *
 * If <code>weights</code> is NULL, any previously initialized set of
 * weights will be used.  If no weights were specified to
 * ngram_model_set_init(), then a uniform distribution will be used.
 *
 * @memberof ngram_model_t
 */
POCKETSPHINX_EXPORT
ngram_model_t *ngram_model_set_interp(ngram_model_t *set,
                                      const char **names,
                                      const float32 *weights);

/**
 * Add a language model to a set.
 *
 * @memberof ngram_model_t
 * @param set The language model set to add to.
 * @param model The language model to add.
 * @param name The name to associate with this model.
 * @param weight Interpolation weight for this model, relative to the
 *               uniform distribution.  1.0 is a safe value.
 * @param reuse_widmap Reuse the existing word-ID mapping in
 * <code>set</code>.  Any new words present in <code>model</code>
 * will not be added to the word-ID mapping in this case.
 */
POCKETSPHINX_EXPORT
ngram_model_t *ngram_model_set_add(ngram_model_t *set,
                                   ngram_model_t *model,
                                   const char *name,
                                   float32 weight,
                                   int reuse_widmap);

/**
 * Remove a language model from a set.
 *
 * @memberof ngram_model_t
 * @param set The language model set to remove from.
 * @param name The name associated with the model to remove.
 * @param reuse_widmap Reuse the existing word-ID mapping in
 *                     <code>set</code>.
 */
POCKETSPHINX_EXPORT
ngram_model_t *ngram_model_set_remove(ngram_model_t *set,
                                      const char *name,
                                      int reuse_widmap);

/**
 * Set the word-to-ID mapping for this model set.
 * @memberof ngram_model_t
 */
POCKETSPHINX_EXPORT
void ngram_model_set_map_words(ngram_model_t *set,
                               const char **words,
                               int32 n_words);

/**
 * Query the word-ID mapping for the current language model.
 *
 * @memberof ngram_model_t
 * @return the local word ID in the current language model, or
 * NGRAM_INVALID_WID if <code>set_wid</code> is invalid or
 * interpolation is enabled.
 */
POCKETSPHINX_EXPORT
int32 ngram_model_set_current_wid(ngram_model_t *set,
                                  int32 set_wid);

/**
 * Test whether a word ID corresponds to a known word in the current
 * state of the language model set.
 *
 * @memberof ngram_model_t
 * @return If there is a current language model, returns non-zero if
 * <code>set_wid</code> corresponds to a known word in that language
 * model.  Otherwise, returns non-zero if <code>set_wid</code>
 * corresponds to a known word in any language model.
 */
POCKETSPHINX_EXPORT
int32 ngram_model_set_known_wid(ngram_model_t *set, int32 set_wid);

/**
 * Flush any cached N-Gram information
 * @memberof ngram_model_t
 */
POCKETSPHINX_EXPORT
void ngram_model_flush(ngram_model_t *lm);

#ifdef __cplusplus
}
#endif

#endif /* __PS_MODEL_H__ */
