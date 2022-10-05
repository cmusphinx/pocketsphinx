/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 2009 Carnegie Mellon University.  All rights 
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
/**
 * \file sphinx_lm_convert.c
 * Language model conversion tool.
 */
#include <pocketsphinx.h>
#include "pocketsphinx_internal.h"
#include "lm/ngram_model.h"
#include "util/ckd_alloc.h"
#include "util/cmd_ln.h"
#include "util/ckd_alloc.h"
#include "util/pio.h"
#include "util/strfuncs.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

static const ps_arg_t defn[] = {
  { "help",
    ARG_BOOLEAN,
    "no",
    "Shows the usage of the tool"},

  { "logbase",
    ARG_FLOATING,
    "1.0001",
    "Base in which all log-likelihoods calculated" },

  { "i",
    REQARG_STRING,
    NULL,
    "Input language model file (required)"},

  { "o",
    REQARG_STRING,
    NULL,
    "Output language model file (required)"},

  { "ifmt",
    ARG_STRING,
    NULL,
    "Input language model format (will guess if not specified)"},

  { "ofmt",
    ARG_STRING,
    NULL,
    "Output language model file (will guess if not specified)"},

  { "case",
    ARG_STRING,
    NULL,
    "Ether 'lower' or 'upper' - case fold to lower/upper case (NOT UNICODE AWARE)" },

  { "mmap",
    ARG_BOOLEAN,
    "no",
    "Use memory-mapped I/O for reading binary LM files"},

  { NULL, 0, NULL, NULL }
};

static void
usagemsg(char *pgm)
{
    E_INFO("Usage: %s -i <input.lm> \\\n", pgm);
    E_INFOCONT("\t[-ifmt txt] [-ofmt dmp]\n");
    E_INFOCONT("\t-o <output.lm.DMP>\n");

    exit(0);
}


int
main(int argc, char *argv[])
{
	cmd_ln_t *config;
	ngram_model_t *lm = NULL;
	logmath_t *lmath;
        int itype, otype;
        char const *kase;

	if ((config = cmd_ln_parse_r(NULL, defn, argc, argv, TRUE)) == NULL) {
            /* This probably just means that we got no arguments. */
            err_set_loglevel(ERR_INFO);
            cmd_ln_log_help_r(NULL, defn);
            return 1;
        }
		
	if (ps_config_bool(config, "help")) {
	    usagemsg(argv[0]);
	}

	/* Create log math object. */
	if ((lmath = logmath_init
	     (ps_config_float(config, "logbase"), 0, 0)) == NULL) {
		E_FATAL("Failed to initialize log math\n");
	}
	
	if (ps_config_str(config, "i") == NULL || ps_config_str(config, "i") == NULL) {
            E_ERROR("Please specify both input and output models\n");
            goto error_out;
        }	    
	
	/* Load the input language model. */
        if (ps_config_str(config, "ifmt")) {
            if ((itype = ngram_str_to_type(ps_config_str(config, "ifmt")))
                == NGRAM_INVALID) {
                E_ERROR("Invalid input type %s\n", ps_config_str(config, "ifmt"));
                goto error_out;
            }
            lm = ngram_model_read(config, ps_config_str(config, "i"),
                                  itype, lmath);
        }
        else {
            lm = ngram_model_read(config, ps_config_str(config, "i"),
                                  NGRAM_AUTO, lmath);
	}

	if (lm == NULL) {
	    E_ERROR("Failed to read the model from the file '%s'\n", ps_config_str(config, "i"));
	    goto error_out;
	}

        /* Guess or set the output language model type. */
        if (ps_config_str(config, "ofmt")) {
            if ((otype = ngram_str_to_type(ps_config_str(config, "ofmt")))
                == NGRAM_INVALID) {
                E_ERROR("Invalid output type %s\n", ps_config_str(config, "ofmt"));
                goto error_out;
            }
        }
        else {
            otype = ngram_file_name_to_type(ps_config_str(config, "o"));
        }

        /* Case fold if requested. */
        if ((kase = ps_config_str(config, "case"))) {
            if (0 == strcmp(kase, "lower")) {
                ngram_model_casefold(lm, NGRAM_LOWER);
            }
            else if (0 == strcmp(kase, "upper")) {
                ngram_model_casefold(lm, NGRAM_UPPER);
            }
            else {
                E_ERROR("Unknown value for -case: %s\n", kase);
                goto error_out;
            }
        }

        /* Write the output language model. */
        if (ngram_model_write(lm, ps_config_str(config, "o"), otype) != 0) {
            E_ERROR("Failed to write language model in format %s to %s\n",
                    ngram_type_to_str(otype), ps_config_str(config, "o"));
            goto error_out;
        }

        /* That's all folks! */
        ngram_model_free(lm);
        if (lmath) {
            logmath_free(lmath);
        }
        if (config) {
            ps_config_free(config);
        }
	return 0;

error_out:
        ngram_model_free(lm);
        if (lmath) {
            logmath_free(lmath);
        }
        if (config) {
            ps_config_free(config);
        }
	return 1;
}
