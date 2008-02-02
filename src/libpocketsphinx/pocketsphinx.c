/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 2008 Carnegie Mellon University.  All rights
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

/* System headers. */
#include <stdio.h>

/* SphinxBase headers. */
#include <err.h>
#include <strfuncs.h>

/* Local headers. */
#include "pocketsphinx_internal.h"
#include "cmdln_macro.h"
#include "search.h"

static const arg_t ps_args_def[] = {
    input_cmdln_options(),
    waveform_to_cepstral_command_line_macro(),
    output_cmdln_options(),
    am_cmdln_options(),
    lm_cmdln_options(),
    dictionary_cmdln_options(),
    fsg_cmdln_options(),
    beam_cmdln_options(),
    search_cmdln_options(),
    CMDLN_EMPTY_OPTION
};

/* I'm not sure what the portable way to do this is. */
static int
file_exists(const char *path)
{
    FILE *tmp;

    tmp = fopen(path, "rb");
    if (tmp) fclose(tmp);
    return (tmp != NULL);
}

static void
pocketsphinx_add_file(pocketsphinx_t *ps, const char *arg,
                      const char *hmmdir, const char *file)
{
    char *tmp = string_join(hmmdir, "/", file, NULL);

    if (cmd_ln_str_r(ps->config, arg) == NULL && file_exists(tmp)) {
        cmd_ln_set_str_r(ps->config, arg, tmp);
        ps->strings = glist_add_ptr(ps->strings, tmp);
    }
}

static void
pocketsphinx_init_defaults(pocketsphinx_t *ps)
{
    char *hmmdir;

    /* Get acoustic model filenames and add them to the command-line */
    if ((hmmdir = cmd_ln_str_r(ps->config, "-hmm")) != NULL) {
        pocketsphinx_add_file(ps, "-mdef", hmmdir, "mdef");
        pocketsphinx_add_file(ps, "-mean", hmmdir, "means");
        pocketsphinx_add_file(ps, "-var", hmmdir, "variances");
        pocketsphinx_add_file(ps, "-tmat", hmmdir, "transition_matrices");
        pocketsphinx_add_file(ps, "-sendump", hmmdir, "sendump");
        pocketsphinx_add_file(ps, "-kdtree", hmmdir, "kdtrees");
        pocketsphinx_add_file(ps, "-fdict", hmmdir, "noisedict");
    }
}

pocketsphinx_t *
pocketsphinx_init(cmd_ln_t *config)
{
    pocketsphinx_t *ps;

    /* First initialize the structure itself */
    ps = ckd_calloc(1, sizeof(*ps));
    ps->config = config;

    /* Fill in some default arguments. */
    pocketsphinx_init_defaults(ps);

    /* Logmath computation (used in acmod and search) */
    ps->lmath = logmath_init
        ((float64)cmd_ln_float32_r(config, "-logbase"), 0, FALSE);

    /* Acoustic model (this is basically everything that
     * uttproc.c, senscr.c, and others used to do) */
    if ((ps->acmod = acmod_init(config, ps->lmath, NULL, NULL)) == NULL)
        goto error_out;

    /* Dictionary and triphone mappings. */
    if ((ps->dict = dict_init(config, ps->acmod->mdef)) == NULL)
        goto error_out;

    return ps;
error_out:
    pocketsphinx_free(ps);
    return NULL;
}

arg_t const *
pocketsphinx_args(void)
{
    return ps_args_def;
}

void
pocketsphinx_free(pocketsphinx_t *ps)
{
    gnode_t *gn;

    for (gn = ps->strings; gn; gn = gnode_next(gn))
        ckd_free(gnode_ptr(gn));
    glist_free(ps->strings);
    dict_free(ps->dict);
    cmd_ln_free_r(ps->config);
    ckd_free(ps);
}

cmd_ln_t *
pocketsphinx_get_config(pocketsphinx_t *ps)
{
    return ps->config;
}

int
pocketsphinx_run_ctl_file(pocketsphinx_t *ps,
			  char const *ctlfile)
{
    return -1;
}

int
pocketsphinx_start_utt(pocketsphinx_t *ps)
{
    return -1;
}

int
pocketsphinx_process_raw(pocketsphinx_t *ps,
			 int16 const *data,
			 int32 n_samples,
			 int do_search,
			 int full_utt)
{
    return -1;
}

int
pocketsphinx_process_cep(pocketsphinx_t *ps,
			 mfcc_t const **data,
			 int32 n_frames,
			 int no_search,
			 int full_utt)
{
    return -1;
}

int
pocketsphinx_end_utt(pocketsphinx_t *ps)
{
    return -1;
}
