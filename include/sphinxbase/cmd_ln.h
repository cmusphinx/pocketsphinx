/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
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
 * cmd_ln.h -- Command line argument parsing.
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
 * 15-Jul-1997	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Added required arguments types.
 * 
 * 07-Dec-96	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Created, based on Eric's implementation.  Basically, combined several
 *		functions into one, eliminated validation, and simplified the interface.
 */


#ifndef _LIBUTIL_CMD_LN_H_
#define _LIBUTIL_CMD_LN_H_

#include <stdio.h>
#include <stdarg.h>

#include <sphinxbase/prim_type.h>
#include <sphinxbase/hash_table.h>

/**
 * @file cmd_ln.h
 * @brief Command-line and other configuration parsing and handling.
 *  
 * Configuration parameters, optionally parsed from the command line.
 */
  

#ifdef __cplusplus
extern "C" {
#endif
#if 0
/* Fool Emacs. */
}
#endif

/**
 * @struct arg_t
 * Argument definition structure.
 */
typedef struct arg_s {
	char const *name;   /**< Name of the command line switch */
	int type;           /**< Type of the argument in question */
	char const *deflt;  /**< Default value (as a character string), or NULL if none */
	char const *doc;    /**< Documentation/description string */
} arg_t;

/**
 * @struct cmd_ln_val_t
 * Configuration parameter structure.
 */
typedef struct cmd_ln_val_s {
    anytype_t val;
    int type;
    char *name;
} cmd_ln_val_t;

/**
 * @struct cmd_ln_t
 * Structure (no longer opaque) used to hold the results of command-line parsing.
 */
typedef struct cmd_ln_s {
    int refcount;
    hash_table_t *ht;
    char **f_argv;
    uint32 f_argc;
    arg_t const *defn;
    char *command;
} cmd_ln_t;

/**
 * Access the value and metadata for a configuration parameter.
 *
 * This structure is owned by the cmd_ln_t, assume that you must copy
 * anything inside it, including strings, if you wish to retain it,
 * and should never free it manually.
 *
 * @param cmdln Command-line object.
 * @param name the command-line flag to retrieve.
 * @return the value and metadata associated with <tt>name</tt>, or
 *         NULL if <tt>name</tt> does not exist.  You must use
 *         cmd_ln_exists_r() to distinguish between cases where a
 *         value is legitimately NULL and where the corresponding flag
 *         is unknown.
 */
cmd_ln_val_t *cmd_ln_access_r(cmd_ln_t *cmdln, char const *name);

cmd_ln_val_t * cmd_ln_val_init(int t, const char *name, const char *str);
void cmd_ln_val_free(cmd_ln_val_t *val);

/**
 * Parse an arguments file by deliminating on " \r\t\n" and putting each tokens
 * into an argv[] for cmd_ln_parse().
 *
 * @return A cmd_ln_t containing the results of command line parsing, or NULL on failure.
 */
cmd_ln_t *cmd_ln_parse_file_r(cmd_ln_t *inout_cmdln, /**< In/Out: Previous command-line to update,
                                                     or NULL to create a new one. */
                              arg_t const *defn,   /**< In: Array of argument name definitions*/
                              char const *filename,/**< In: A file that contains all
                                                     the arguments */ 
                              int32 strict         /**< In: Fail on duplicate or unknown
                                                     arguments, or no arguments? */
    );

/**
 * Set a string in a command-line object even if it is not present in argument
 * description. Useful for setting extra values computed from configuration, propagated
 * to other parts.
 *
 * @param cmdln Command-line object.
 * @param name The command-line flag to set.
 * @param str String value to set.  The command-line object does not
 *            retain ownership of this pointer.
 */
void cmd_ln_set_str_extra_r(cmd_ln_t *cmdln, char const *name, char const *str);

/**
 * Print a help message listing the valid argument names, and the associated
 * attributes as given in defn.
 *
 * @param cmdln command-line object
 * @param defn array of argument name definitions.
 */
void cmd_ln_log_help_r (cmd_ln_t *cmdln, const arg_t *defn);

/**
 * Print current configuration values and defaults.
 *
 * @param cmdln  command-line object
 * @param defn array of argument name definitions.
 */
void cmd_ln_log_values_r (cmd_ln_t *cmdln, const arg_t *defn);


#ifdef __cplusplus
}
#endif

#endif
