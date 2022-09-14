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


#include <pocketsphinx.h>

#include "util/cmd_ln.h"
#include "util/strfuncs.h"

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
ps_config_init(void)
{
    ps_config_t *config = ckd_calloc(1, sizeof(*config));
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

void
json_error(int err)
{
    const char *errstr;
    switch (err) {
    case JSMN_ERROR_INVAL:
        errstr = "JSMN_ERROR_INVAL - bad token, JSON string is corrupted";
        break;
    case JSMN_ERROR_NOMEM:
        errstr = "JSMN_ERROR_NOMEM - not enough tokens, JSON string is too large";
        break;
    case JSMN_ERROR_PART:
        errstr = "JSMN_ERROR_PART - JSON string is too short, expecting more JSON data";
        break;
    default:
        errstr = "Unknown error";
    }
    E_ERROR("JSON parsing failed: %s\n", errstr);
}

ps_config_t *
ps_config_parse_json(ps_config_t *config,
                     const char *json)
{
    jsmn_parser parser;
    jsmntok_t *tokens = NULL;
    char *key = NULL, *val = NULL;
    int i, jslen, ntok, new_config = FALSE;

    if (json == NULL)
        return NULL;
    if (config == NULL) {
        if ((config = ps_config_init()) == NULL)
            return NULL;
        new_config = TRUE;
    }
    jsmn_init(&parser);
    jslen = strlen(json);
    if ((ntok = jsmn_parse(&parser, json, jslen, NULL, 0)) < 0) {
        json_error(ntok);
        goto error_out;
    }
    tokens = ckd_calloc(ntok, sizeof(*tokens));
    if ((ntok = jsmn_parse(&parser, json, jslen, tokens, ntok)) < 0) {
        json_error(ntok);
        goto error_out;
    }
    if (ntok == 0 || tokens[0].type != JSMN_OBJECT) {
        E_ERROR("Expected JSON object at top level\n");
        goto error_out;
    }
    /* JSMN is really a tokenizer more than a parser, but that is all
     * you need for JSON. */
    for (i = 1; i < ntok; ++i) {
        key = ckd_calloc(tokens[i].end - tokens[i].start + 1, 1);
        memcpy(key, json + tokens[i].start, tokens[i].end - tokens[i].start);
        if (tokens[i].type != JSMN_STRING) {
            E_ERROR("Expected string key, got %s\n", key);
            goto error_out;
        }
        if (i + 1 == ntok) {
            E_ERROR("Missing value for %s\n", key);
            goto error_out;
        }
        val = ckd_calloc(tokens[i+1].end - tokens[i+1].start + 1, 1);
        memcpy(val, json + tokens[i+1].start, tokens[i+1].end - tokens[i+1].start);
        if (ps_config_set_str(config, key, val) == NULL) {
            E_ERROR("Unknown or invalid parameter %s\n", key);
            goto error_out;
        }
        ckd_free(key);
        ckd_free(val);
        key = val = NULL;
    }

    ckd_free(key);
    ckd_free(val);
    ckd_free(tokens);
    return config;

error_out:
    if (key)
        ckd_free(key);
    if (val)
        ckd_free(val);
    if (tokens)
        ckd_free(tokens);
    if (new_config)
        ps_config_free(config);
    return NULL;
}

const char *
ps_config_serialize_json(ps_config_t *config)
{
    return NULL;
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

static const anytype_t *
ps_config_unset(ps_config_t *config, char const *name)
{
    cmd_ln_val_t *cval = cmd_ln_access_r(config, name);
    const arg_t *arg;
    if (cval == NULL) {
        E_ERROR("Unknown parameter %s\n", name);
        return NULL;
    }
    /* FIXME: Perhaps cmd_ln_val_t should store a pointer to arg_t */
    for (arg = config->defn; arg->name; ++arg) {
        if (0 == strcmp(arg->name, name)) {
            if (anytype_from_str(&cval->val, cval->type, arg->deflt) == NULL)
                return NULL;
            break;
        }
    }
    if (arg->name == NULL) {
        E_ERROR("No definition found for %s\n", name);
        return NULL;
    }
    return &cval->val;
}

const anytype_t *
ps_config_set(ps_config_t *config, const char *name, const anytype_t *val, ps_type_t t)
{
    if (val == NULL) {
        /* Restore default parameter */
        return ps_config_unset(config, name);
    }
    else if (t & ARG_STRING)
        return ps_config_set_str(config, name, val->ptr);
    else if (t & ARG_INTEGER)
        return ps_config_set_int(config, name, val->i);
    else if (t & ARG_BOOLEAN)
        return ps_config_set_bool(config, name, val->i);
    else if (t & ARG_FLOATING)
        return ps_config_set_bool(config, name, val->fl);
    else {
        E_ERROR("Value has unknown type %d\n", name, t);
        return NULL;
    }
}

anytype_t *
anytype_from_str(anytype_t *val, int t, const char *str)
{
    if (val == NULL)
        return NULL;
    if (str == NULL) {
        if (val->ptr && (t & ARG_STRING))
            ckd_free(val->ptr);
        memset(val, 0, sizeof(*val));
        return val;
    }
    if (strlen(str) == 0)
        return NULL;
    switch (t) {
    case ARG_INTEGER:
    case REQARG_INTEGER:
        if (sscanf(str, "%ld", &val->i) != 1)
            return NULL;
        break;
    case ARG_FLOATING:
    case REQARG_FLOATING:
        val->fl = atof_c(str);
        break;
    case ARG_BOOLEAN:
    case REQARG_BOOLEAN:
        if ((str[0] == 'y') || (str[0] == 't') ||
            (str[0] == 'Y') || (str[0] == 'T') || (str[0] == '1')) {
            val->i = TRUE;
        }
        else if ((str[0] == 'n') || (str[0] == 'f') ||
                 (str[0] == 'N') || (str[0] == 'F') |
                 (str[0] == '0')) {
            val->i = FALSE;
        }
        else {
            E_ERROR("Unparsed boolean value '%s'\n", str);
            return NULL;
        }
        break;
    case ARG_STRING:
    case REQARG_STRING:
        if (val->ptr)
            ckd_free(val->ptr);
        val->ptr = ckd_salloc(str);
        break;
    default:
        E_ERROR("Unknown argument type: %d\n", t);
        return NULL;
    }
    return val;
}

const anytype_t *
ps_config_set_str(ps_config_t *config, char const *name, char const *val)
{
    cmd_ln_val_t *cval = cmd_ln_access_r(config, name);
    if (cval == NULL) {
        E_ERROR("Unknown parameter %s\n", name);
        return NULL;
    }
    if (anytype_from_str(&cval->val, cval->type, val) == NULL) {
        return NULL;
    }
    return &cval->val;
}

anytype_t *
anytype_from_int(anytype_t *val, int t, long i)
{
    if (val == NULL)
        return NULL;
    switch (t) {
    case ARG_INTEGER:
    case REQARG_INTEGER:
        val->i = i;
        break;
    case ARG_FLOATING:
    case REQARG_FLOATING:
        val->fl = (double)i;
        break;
    case ARG_BOOLEAN:
    case REQARG_BOOLEAN:
        val->i = (i != 0);
        break;
    case ARG_STRING:
    case REQARG_STRING: {
        int len = snprintf(NULL, 0, "%ld", i);
        if (len < 0) {
            E_ERROR_SYSTEM("snprintf() failed");
            return NULL;
        }
        val->ptr = ckd_calloc(len + 1, 1);
        if (snprintf(val->ptr, len + 1, "%ld", i) != len) {
            E_ERROR_SYSTEM("snprintf() failed");
            return NULL;
        }
        break;
    }
    default:
        E_ERROR("Unknown argument type: %d\n", t);
        return NULL;
    }
    return val;
}

const anytype_t *
ps_config_set_int(ps_config_t *config, char const *name, long val)
{
    cmd_ln_val_t *cval = cmd_ln_access_r(config, name);
    if (cval == NULL) {
        E_ERROR("Unknown parameter %s\n", name);
        return NULL;
    }
    if (anytype_from_int(&cval->val, cval->type, val) == NULL) {
        return NULL;
    }
    return &cval->val;
}

anytype_t *
anytype_from_float(anytype_t *val, int t, double f)
{
    if (val == NULL)
        return NULL;
    switch (t) {
    case ARG_INTEGER:
    case REQARG_INTEGER:
        val->i = (long)f;
        break;
    case ARG_FLOATING:
    case REQARG_FLOATING:
        val->fl = f;
        break;
    case ARG_BOOLEAN:
    case REQARG_BOOLEAN:
        val->i = (f != 0.0);
        break;
    case ARG_STRING:
    case REQARG_STRING: {
        int len = snprintf(NULL, 0, "%g", f);
        if (len < 0) {
            E_ERROR_SYSTEM("snprintf() failed");
            return NULL;
        }
        val->ptr = ckd_calloc(len + 1, 1);
        if (snprintf(val->ptr, len + 1, "%g", f) != len) {
            E_ERROR_SYSTEM("snprintf() failed");
            return NULL;
        }
        break;
    }
    default:
        E_ERROR("Unknown argument type: %d\n", t);
        return NULL;
    }
    return val;
}

const anytype_t *
ps_config_set_float(ps_config_t *config, char const *name, double val)
{
    cmd_ln_val_t *cval = cmd_ln_access_r(config, name);
    if (cval == NULL) {
        E_ERROR("Unknown parameter %s\n", name);
        return NULL;
    }
    if (anytype_from_float(&cval->val, cval->type, val) == NULL) {
        return NULL;
    }
    return &cval->val;
}

const anytype_t *
ps_config_set_bool(ps_config_t *config, char const *name, int val)
{
    return ps_config_set_int(config, name, val != 0);
}
