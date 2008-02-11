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
#include <filename.h>
#include <pio.h>

/* Local headers. */
#include "pocketsphinx_internal.h"
#include "cmdln_macro.h"

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
        pocketsphinx_add_file(ps, "-mixw", hmmdir, "mixture_weights");
        pocketsphinx_add_file(ps, "-sendump", hmmdir, "sendump");
        pocketsphinx_add_file(ps, "-kdtree", hmmdir, "kdtrees");
        pocketsphinx_add_file(ps, "-fdict", hmmdir, "noisedict");
        pocketsphinx_add_file(ps, "-featparams", hmmdir, "feat.params");
    }
}

pocketsphinx_t *
pocketsphinx_init(cmd_ln_t *config)
{
    pocketsphinx_t *ps;
    char *fsgfile, *fsgctl = NULL;
    char *lmfile, *lmctl = NULL;

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

    /* Dictionary and triphone mappings (depends on acmod). */
    if ((ps->dict = dict_init(config, ps->acmod->mdef)) == NULL)
        goto error_out;

    /* Determine whether we are starting out in FSG or N-Gram search mode. */
    if ((fsgfile = cmd_ln_str_r(config, "-fsg"))
        || (fsgctl = cmd_ln_str_r(config, "-fsgctlfn"))) {
        /* Initialize the FSG module. */
        ps->fsgs = fsg_search_init(config, ps->lmath,
                                   ps->acmod->mdef, ps->dict);
        if (ps->fsgs == NULL)
            goto error_out;

        /* Load one or more FSGs. */
        if (fsgfile) {
            if (pocketsphinx_load_fsgfile(ps, fsgfile) == NULL)
                goto error_out;
        }
        if (fsgctl) {
            if (pocketsphinx_load_fsgctl(ps, fsgctl, (fsgfile != NULL)) == NULL)
                goto error_out;
        }
    }
    else if ((lmfile = cmd_ln_str_r(config, "-lm"))
             || (lmctl = cmd_ln_str_r(config, "-lmctlfn"))) {
        /* Initialize the N-Gram module. */
        ps->ngs = ngram_search_init(config, ps->acmod, ps->dict);
    }
    /* Otherwise, we will initialize the search whenever the user
     * decides to load an FSG or a language model. */

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

const char *
pocketsphinx_load_fsgfile(pocketsphinx_t *ps, const char *fsgfile)
{
    word_fsg_t *fsg;

    if (ps->fsgs == NULL)
        ps->fsgs = fsg_search_init(ps->config, ps->lmath,
                                   ps->acmod->mdef, ps->dict);

    fsg = word_fsg_readfile(fsgfile, ps->dict, ps->lmath,
                            cmd_ln_boolean_r(ps->config, "-fsgusealtpron"),
                            cmd_ln_boolean_r(ps->config, "-fsgusefiller"),
                            cmd_ln_float32_r(ps->config, "-silpen"),
                            cmd_ln_float32_r(ps->config, "-fillpen"),
                            cmd_ln_float32_r(ps->config, "-lw"));
    if (!fsg)
        return NULL;

    if (!fsg_search_add_fsg(ps->fsgs, fsg)) {
        E_ERROR("Failed to add FSG '%s' to system\n", word_fsg_name(fsg));
        word_fsg_free(fsg);
        return NULL;
    }

    return fsg->name;
}

const char *
pocketsphinx_load_fsgctl(pocketsphinx_t *ps, const char *fsgctlfile, int set_default)
{
    const char *fsgname, *deffsg, *c;
    char *basedir;
    FILE *ctlfp;
    char *fsgfile;
    size_t len;

    if (ps->fsgs == NULL)
        ps->fsgs = fsg_search_init(ps->config, ps->lmath,
                                   ps->acmod->mdef, ps->dict);

    if ((ctlfp = fopen(fsgctlfile, "r")) == NULL) {
        E_ERROR_SYSTEM("Failed to open FSG control file %s");
        return NULL;
    }

    /* Try to find the base directory to append to relative paths in
     * the fsgctl file. */
    if ((c = strrchr(fsgctlfile, '/')) || (c = strrchr(fsgctlfile, '\\'))) {
        /* Include the trailing slash. */
        basedir = ckd_calloc(c - fsgctlfile + 2, 1);
        memcpy(basedir, fsgctlfile, c - fsgctlfile + 1);
    }
    else
        basedir = NULL;

    E_INFO("Reading FSG control file '%s'\n", fsgctlfile);
    if (basedir)
        E_INFO("Will prepend '%s' to unqualified paths\n", basedir);

    deffsg = NULL;
    while ((fsgfile = fread_line(ctlfp, &len))) {
        string_trim(fsgfile, STRING_BOTH);
        if (fsgfile[0] == '#') {/* Comments. */
            ckd_free(fsgfile);
            fsgfile = NULL;
            continue;
        }

        /* Prepend the base dir if necessary. */
        if (basedir && !path_is_absolute(fsgfile)) {
            char *tmp = string_join(basedir, fsgfile, NULL);
            ckd_free(fsgfile);
            fsgfile = tmp;
        }

        fsgname = pocketsphinx_load_fsgfile(ps, fsgfile);
        if (!fsgname) {
            E_ERROR("Error loading FSG file '%s'\n", fsgfile);
            deffsg = NULL;
            goto error_out;
        }
        if (set_default && deffsg == NULL) {
            if (!fsg_search_set_current_fsg(ps->fsgs, fsgname)) {
                E_ERROR("Could not set default fsg '%s'\n", fsgname);
                goto error_out;
            }
            deffsg = fsgname;
        }
        ckd_free(fsgfile);
        fsgfile = NULL;
    }

error_out:
    ckd_free(fsgfile);
    ckd_free(basedir);
    return deffsg;
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
    acmod_start_utt(ps->acmod);

    return 0;
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
    acmod_end_utt(ps->acmod);

    return 0;
}
