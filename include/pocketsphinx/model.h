/* -*- c-basic-offset:4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 2022 David Huggins-Daines.  All rights reserved.
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
 */

#ifndef __PS_MODEL_H__
#define __PS_MODEL_H__

#include <pocketsphinx/prim_type.h>
#include <pocketsphinx/export.h>

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

/**
 * @struct jsgf_t
 * @brief JSGF parser
 */
typedef struct jsgf_s jsgf_t;

/**
 * @struct jsgf_rule_t
 * @brief Rule in a parsed JSGF grammar.
 */
typedef struct jsgf_rule_s jsgf_rule_t;

/**
 * @struct fsg_model_t
 * @brief Finite-state grammar.
 *
 * States are simply integers 0..n_state-1.
 * A transition emits a word and has a given probability of being taken.
 * There can also be null or epsilon transitions, with no associated emitted
 * word.
 */
typedef struct fsg_model_s fsg_model_t;

/**
 * @struct ngram_model_t
 * @brief N-Gram based language model.
 */
typedef struct ngram_model_s ngram_model_t;

/**
 * Parse a JSGF grammar from a file.
 *
 * @param filename the name of the file to parse.
 * @param parent optional parent grammar for this one (NULL, usually).
 * @return new JSGF grammar object, or NULL on failure.
 */
jsgf_t *jsgf_parse_file(const char *filename, jsgf_t *parent);

/**
 * Parse a JSGF grammar from a string.
 *
 * @param 0-terminated string with grammar.
 * @param parent optional parent grammar for this one (NULL, usually).
 * @return new JSGF grammar object, or NULL on failure.
 */
jsgf_t *jsgf_parse_string(const char *string, jsgf_t *parent);

/**
 * Get the grammar name from the file.
 */
char const *jsgf_grammar_name(jsgf_t *jsgf);

/**
 * Free a JSGF grammar.
 */
void jsgf_grammar_free(jsgf_t *jsgf);

/**
 * Get a rule by name from a grammar. Name should not contain brackets.
 */
jsgf_rule_t *jsgf_get_rule(jsgf_t *grammar, const char *name);

/**
 * Returns the first public rule of the grammar
 */ 
jsgf_rule_t *jsgf_get_public_rule(jsgf_t *grammar);

/**
 * Get the rule name from a rule.
 */
char const *jsgf_rule_name(jsgf_rule_t *rule);

/**
 * Test if a rule is public or not.
 */
int jsgf_rule_public(jsgf_rule_t *rule);

/**
 * Iterator over rules in a grammar.
 */
typedef struct hash_iter_s jsgf_rule_iter_t;

/**
 * Get an iterator over all rules in a grammar.
 */
jsgf_rule_iter_t *jsgf_rule_iter(jsgf_t *grammar);

/**
 * Advance an iterator to the next rule in the grammar.
 */
jsgf_rule_iter_t *jsgf_rule_iter_next(jsgf_rule_iter_t *itor);

/**
 * Get the current rule in a rule iterator.
 */
jsgf_rule_t *jsgf_rule_iter_rule(jsgf_rule_iter_t *itor);

/**
 * Free a rule iterator (if the end hasn't been reached).
 */
void jsgf_rule_iter_free(jsgf_rule_iter_t *itor);

/**
 * Build a Sphinx FSG object from a JSGF rule.
 */
fsg_model_t *jsgf_build_fsg(jsgf_t *grammar, jsgf_rule_t *rule,
                            logmath_t *lmath, float32 lw);

/**
 * Build a Sphinx FSG object from a JSGF rule.
 *
 * This differs from jsgf_build_fsg() in that it does not do closure
 * on epsilon transitions or any other postprocessing.  For the time
 * being this is necessary in order to write it to a file.
 */
fsg_model_t *jsgf_build_fsg_raw(jsgf_t *grammar, jsgf_rule_t *rule,
                                logmath_t *lmath, float32 lw);

/**
 * Read JSGF from file and return FSG object from it.
 *
 * This function looks for a first public rule in jsgf and constructs JSGF from it.
 */
fsg_model_t *jsgf_read_file(const char *file, logmath_t * lmath, float32 lw);

/**
 * Read JSGF from string and return FSG object from it.
 *
 * This function looks for a first public rule in jsgf and constructs JSGF from it.
 */
fsg_model_t *jsgf_read_string(const char *string, logmath_t * lmath, float32 lw);

/**
 * Convert a JSGF rule to Sphinx FSG text form.
 *
 * This does a direct conversion without doing transitive closure on
 * null transitions and so forth.
 */
int jsgf_write_fsg(jsgf_t *grammar, jsgf_rule_t *rule, FILE *outfh);


#endif /* __PS_MODEL_H__ */
