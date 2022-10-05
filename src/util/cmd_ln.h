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

#include <pocketsphinx.h>
#include <pocketsphinx/export.h>

#include "util/hash_table.h"

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
 * @struct cmd_ln_val_t
 * Configuration parameter structure.
 */
typedef struct cmd_ln_val_s {
    anytype_t val;
    int type;
    char *name;
} cmd_ln_val_t;

/**
 * @name Values for ps_arg_t::type
 */
/* @{ */
/**
 * Bit indicating a required argument.
 */
#define ARG_REQUIRED (1<<0)
/**
 * Integer argument (optional).
 */
#define ARG_INTEGER  (1<<1)
/**
 * Floating point argument (optional).
 */
#define ARG_FLOATING (1<<2)
/**
 * String argument (optional).
 */
#define ARG_STRING   (1<<3)
/**
 * Boolean (true/false) argument (optional).
 */
#define ARG_BOOLEAN  (1<<4)
/**
 * String array argument (optional).
 */
#define ARG_STRING_LIST  (1<<5)

/**
 * Required integer argument.
 */
#define REQARG_INTEGER (ARG_INTEGER | ARG_REQUIRED)
/**
 * Required floating point argument.
 */
#define REQARG_FLOATING (ARG_FLOATING | ARG_REQUIRED)
/**
 * Required string argument.
 */
#define REQARG_STRING (ARG_STRING | ARG_REQUIRED)
/**
 * Required boolean argument.
 */
#define REQARG_BOOLEAN (ARG_BOOLEAN | ARG_REQUIRED)

/* @} */


/**
 * Helper macro to stringify enums and other non-string values for
 * default arguments.
 **/
#define ARG_STRINGIFY(s) ARG_STRINGIFY1(s)
#define ARG_STRINGIFY1(s) #s

/**
 * @struct cmd_ln_t
 * Structure (no longer opaque) used to hold the results of command-line parsing.
 */
typedef struct cmd_ln_s {
    int refcount;
    hash_table_t *ht;
    char **f_argv;
    uint32 f_argc;
    ps_arg_t const *defn;
    char *json;
} cmd_ln_t;

/**
 * Retain ownership of a command-line argument set.
 *
 * @return pointer to retained command-line argument set.
 */
cmd_ln_t *cmd_ln_retain(cmd_ln_t *cmdln);

/**
 * Release a command-line argument set and all associated strings.
 *
 * @return new reference count (0 if freed completely)
 */
int cmd_ln_free_r(cmd_ln_t *cmdln);

/**
 * Parse a list of strings into argumetns.
 *
 * Parse the given list of arguments (name-value pairs) according to
 * the given definitions.  Argument values can be retrieved in future
 * using cmd_ln_access().  argv[0] is assumed to be the program name
 * and skipped.  Any unknown argument name causes a fatal error.  The
 * routine also prints the prevailing argument values (to stderr)
 * after parsing.
 *
 * @note It is currently assumed that the strings in argv are
 *       allocated statically, or at least that they will be valid as
 *       long as the cmd_ln_t returned from this function.
 *       Unpredictable behaviour will result if they are freed or
 *       otherwise become invalidated.
 *
 * @return A cmd_ln_t containing the results of command line parsing,
 *         or NULL on failure.
 **/
POCKETSPHINX_EXPORT
cmd_ln_t *cmd_ln_parse_r(cmd_ln_t *inout_cmdln, /**< In/Out: Previous command-line to update,
                                                     or NULL to create a new one. */
                         ps_arg_t const *defn,	/**< In: Array of argument name definitions */
                         int32 argc,		/**< In: Number of actual arguments */
                         char *argv[],		/**< In: Actual arguments */
                         int32 strict           /**< In: Fail on duplicate or unknown
                                                   arguments, or no arguments? */
    );

#define ps_config_parse_args(defn, argc, argv) \
    cmd_ln_parse_r(NULL, (defn) == NULL ? ps_args() : (defn), argc, argv, FALSE)

/**
 * Parse an arguments file by deliminating on " \r\t\n" and putting each tokens
 * into an argv[] for cmd_ln_parse().
 *
 * @return A cmd_ln_t containing the results of command line parsing, or NULL on failure.
 */
POCKETSPHINX_EXPORT
cmd_ln_t *cmd_ln_parse_file_r(cmd_ln_t *inout_cmdln, /**< In/Out: Previous command-line to update,
                                                     or NULL to create a new one. */
                              ps_arg_t const *defn,   /**< In: Array of argument name definitions*/
                              char const *filename,/**< In: A file that contains all
                                                     the arguments */ 
                              int32 strict         /**< In: Fail on duplicate or unknown
                                                     arguments, or no arguments? */
    );

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
POCKETSPHINX_EXPORT
void cmd_ln_log_help_r (cmd_ln_t *cmdln, const ps_arg_t *defn);

/**
 * Print current configuration values and defaults.
 *
 * @param cmdln  command-line object
 * @param defn array of argument name definitions.
 */
void cmd_ln_log_values_r (cmd_ln_t *cmdln, const ps_arg_t *defn);

cmd_ln_val_t *cmd_ln_val_init(int t, const char *name, const char *str);
void cmd_ln_val_free(cmd_ln_val_t *val);

anytype_t *anytype_from_str(anytype_t *val, int t, const char *str);
anytype_t *anytype_from_int(anytype_t *val, int t, long i);
anytype_t *anytype_from_float(anytype_t *val, int t, double f);

#ifdef __cplusplus
}
#endif

#endif
