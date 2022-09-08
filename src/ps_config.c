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


#include <sphinxbase/err.h>
#include <sphinxbase/cmd_ln.h>
#include <sphinxbase/strfuncs.h>

#include "pocketsphinx_internal.h"
#include "cmdln_macro.h"
#include "jsmn.h"

static const arg_t ps_args_def[] = {
    POCKETSPHINX_OPTIONS,
    CMDLN_EMPTY_OPTION
};

arg_t const *
ps_args(void)
{
    return ps_args_def;
}

ps_config_t *
ps_config_init(const char *command)
{
    ps_config_t *config = ckd_calloc(1, sizeof(*config));
    if (command)
        ps_config_set_command(config, command);
    return config;
}

ps_config_t *
ps_config_retain(ps_config_t *config)
{
    ++config->refcount;
    return config;
}

int
ps_config_free(ps_config_t *config)
{
    if (config == NULL)
        return 0;
    if (--config->refcount > 0)
        return config->refcount;
    if (config->command)
        ckd_free(config->command);
    if (config->ht) {
        glist_t entries;
        gnode_t *gn;
        int32 n;

        entries = hash_table_tolist(config->ht, &n);
        for (gn = entries; gn; gn = gnode_next(gn)) {
            hash_entry_t *e = (hash_entry_t *)gnode_ptr(gn);
            cmd_ln_val_free((cmd_ln_val_t *)e->val);
        }
        glist_free(entries);
        hash_table_free(config->ht);
        config->ht = NULL;
    }

    if (config->f_argv) {
        int32 i;
        for (i = 0; i < (int32)config->f_argc; ++i) {
            ckd_free(config->f_argv[i]);
        }
        ckd_free(config->f_argv);
        config->f_argv = NULL;
        config->f_argc = 0;
    }
    ckd_free(config);
    return 0;
}

ps_config_t *
ps_config_parse_args(ps_config_t *config, int argc, char *argv[])
{
    int32 i, j, n;
    hash_table_t *defidx = NULL;
    int new_config = FALSE;

    /* Construct command-line object */
    if (config == NULL) {
        new_config = TRUE;
        config = ps_config_init(NULL);
    }
    config->defn = ps_args_def;

    /* Build a hash table for argument definitions */
    defidx = hash_table_new(50, 0);
    for (n = 0; config->defn[n].name; n++) {
        void *v;

        v = hash_table_enter(defidx, config->defn[n].name, (void *)&config->defn[n]);
        if (v != &config->defn[n]) {
            E_ERROR("Duplicate argument name in definition: %s\n", config->defn[n].name);
            goto error;
        }
    }

    /* Allocate memory for argument values */
    if (config->ht == NULL)
        config->ht = hash_table_new(n, 0 /* argument names are case-sensitive */ );

    /* Parse command line arguments (name-value pairs) */
    for (j = 0; j < argc; j += 2) {
        arg_t *argdef;
        cmd_ln_val_t *val;
        void *v;

        if (hash_table_lookup(defidx, argv[j], &v) < 0) {
            E_ERROR("Unknown argument name '%s'\n", argv[j]);
            goto error;
        }
        argdef = (arg_t *)v;

        /* Enter argument value */
        if (j + 1 >= argc) {
            E_ERROR("Argument value for '%s' missing\n", argv[j]);
            goto error;
        }

        if (argdef == NULL)
            val = cmd_ln_val_init(ARG_STRING, argv[j], argv[j + 1]);
        else {
            if ((val = cmd_ln_val_init(argdef->type, argv[j], argv[j + 1])) == NULL) {
                E_ERROR("Bad argument value for %s: %s\n", argv[j],
                        argv[j + 1]);
                goto error;
            }
        }

        if ((v = hash_table_enter(config->ht, val->name, (void *)val)) !=
            (void *)val) {
            cmd_ln_val_free(val);
            E_ERROR("Duplicate argument name in arguments: %s\n",
                    argdef->name);
            goto error;
        }
    }

    /* Fill in default values, if any, for unspecified arguments */
    for (i = 0; i < n; i++) {
        cmd_ln_val_t *val;
        void *v;

        if (hash_table_lookup(config->ht, config->defn[i].name, &v) < 0) {
            if ((val = cmd_ln_val_init(config->defn[i].type,
                                       config->defn[i].name,
                                       config->defn[i].deflt)) == NULL) {
                E_ERROR
                    ("Bad default argument value for %s: %s\n",
                     config->defn[i].name, config->defn[i].deflt);
                goto error;
            }
            hash_table_enter(config->ht, val->name, (void *)val);
        }
    }

    /* Check for required arguments; exit if any missing */
    j = 0;
    for (i = 0; i < n; i++) {
        if (config->defn[i].type & ARG_REQUIRED) {
            void *v;
            if (hash_table_lookup(config->ht, config->defn[i].name, &v) != 0)
                E_ERROR("Missing required argument %s\n", config->defn[i].name);
        }
    }
    if (j > 0) {
        goto error;
    }

    if (argc == 1) {
        E_ERROR("No arguments given\n");
        if (defidx)
            hash_table_free(defidx);
        if (new_config)
            ps_config_free(config);
        return NULL;
    }

    hash_table_free(defidx);
    return config;

  error:
    if (defidx)
        hash_table_free(defidx);
    if (new_config)
        ps_config_free(config);
    E_ERROR("Failed to parse arguments list\n");
    return NULL;
}

ps_config_t *
ps_config_parse_json(ps_config_t *config, const char *command,
                     const char *json)
{
    return NULL;
}

const char *
ps_config_serialize_json(ps_config_t *config)
{
    return NULL;
}

const char *
ps_config_command(ps_config_t *config)
{
    return config->command;
}

const char *
ps_config_set_command(ps_config_t *config, const char *command)
{
    if (config->command)
        ckd_free(config->command);
    if (command)
        config->command = ckd_salloc(command);
    else
        config->command = NULL;
    return config->command;
}

ps_type_t
ps_config_typeof(ps_config_t *config, char const *name)
{
    void *val;
    if (hash_table_lookup(config->ht, name, &val) < 0)
        return 0;
    if (val == NULL)
        return 0;
    return ((cmd_ln_val_t *)val)->type;
}

const anytype_t *
ps_config_get(ps_config_t *config, const char *name)
{
    void *val;
    if (hash_table_lookup(config->ht, name, &val) < 0)
        return NULL;
    if (val == NULL)
        return NULL;
    return &((cmd_ln_val_t *)val)->val;
}

char const *
ps_config_str(cmd_ln_t *config, char const *name)
{
    cmd_ln_val_t *val;
    val = cmd_ln_access_r(config, name);
    if (val == NULL)
        return NULL;
    if (!(val->type & ARG_STRING)) {
        E_ERROR("Argument %s does not have string type\n", name);
        return NULL;
    }
    return (char const *)val->val.ptr;
}

long
ps_config_int(cmd_ln_t *config, char const *name)
{
    cmd_ln_val_t *val;
    val = cmd_ln_access_r(config, name);
    if (val == NULL)
        return 0L;
    if (!(val->type & (ARG_INTEGER | ARG_BOOLEAN))) {
        E_ERROR("Argument %s does not have integer type\n", name);
        return 0L;
    }
    return val->val.i;
}

int
ps_config_bool(ps_config_t *config, char const *name)
{
    long val = ps_config_int(config, name);
    return val != 0;
}

double
ps_config_float(ps_config_t *config, char const *name)
{
    cmd_ln_val_t *val;
    val = cmd_ln_access_r(config, name);
    if (val == NULL)
        return 0.0;
    if (!(val->type & ARG_FLOATING)) {
        E_ERROR("Argument %s does not have floating-point type\n", name);
        return 0.0;
    }
    return val->val.fl;
}

const anytype_t *
ps_config_set(ps_config_t *config, const char *name, const anytype_t *val, ps_type_t t)
{
    cmd_ln_val_t *cval;
    cval = cmd_ln_access_r(config, name);
    if (cval == NULL) {
        E_ERROR("Unknown argument: %s\n", name);
        return NULL;
    }
    if (!(cval->type & t)) {
        E_ERROR("Argument %s has incompatible type\n", name, cval->type);
        return NULL;
    }
    if (cval->type & ARG_STRING) {
        if (cval->val.ptr)
            ckd_free(cval->val.ptr);
        if (val != NULL)
            cval->val.ptr = ckd_salloc(val->ptr);
    }
    else {
        if (val == NULL)
            memset(&cval->val, 0, sizeof(cval->val));
        else
            memcpy(&cval->val, val, sizeof(cval->val));
    }
    return &cval->val;
}

const anytype_t *
ps_config_set_str(ps_config_t *config, char const *name, char const *val)
{
    anytype_t cval;
    cval.ptr = (void *)val;
    return ps_config_set(config, name, &cval, ARG_STRING);
}

const anytype_t *
ps_config_set_int(ps_config_t *config, char const *name, long val)
{
    anytype_t cval;
    cval.i = val;
    return ps_config_set(config, name, &cval, ARG_INTEGER | ARG_BOOLEAN);
}

const anytype_t *
ps_config_set_float(ps_config_t *config, char const *name, double val)
{
    anytype_t cval;
    cval.fl = val;
    return ps_config_set(config, name, &cval, ARG_FLOATING);
}

const anytype_t *
ps_config_set_bool(ps_config_t *config, char const *name, int val)
{
    return ps_config_set_int(config, name, val);
}
