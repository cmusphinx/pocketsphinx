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
#include "config_macro.h"
#include "jsmn.h"

static const ps_arg_t ps_args_def[] = {
    POCKETSPHINX_OPTIONS,
    CMDLN_EMPTY_OPTION
};

ps_arg_t const *
ps_args(void)
{
    return ps_args_def;
}

ps_config_t *
ps_config_init(const ps_arg_t *defn)
{
    ps_config_t *config = ckd_calloc(1, sizeof(*config));
    int i, ndef;
    
    config->refcount = 1;
    if (defn)
        config->defn = defn;
    else
        config->defn = ps_args_def;
    for (ndef = 0; config->defn[ndef].name; ndef++)
        ;
    config->ht = hash_table_new(ndef, FALSE);
    for (i = 0; i < ndef; i++) {
        cmd_ln_val_t *val;
        if ((val = cmd_ln_val_init(config->defn[i].type,
                                   config->defn[i].name,
                                   config->defn[i].deflt)) == NULL) {
            E_ERROR
                ("Bad default argument value for %s: %s\n",
                 config->defn[i].name, config->defn[i].deflt);
            continue;
        }
        hash_table_enter(config->ht, val->name, (void *)val);
    }
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
    if (config->json)
        ckd_free(config->json);
    ckd_free(config);
    return 0;
}

static const char *searches[] = {
    "lm",
    "jsgf",
    "fsg",
    "keyphrase",
    "kws",
    "allphone",
    "lmctl"
};
static const int nsearches = sizeof(searches)/sizeof(searches[0]);
    
int
ps_config_validate(ps_config_t *config)
{
    int i, found = 0;
    for (i = 0; i < nsearches; ++i) {
        if (ps_config_str(config, searches[i]) != NULL)
            if (++found > 1)
                break;
    }
    if (found > 1) {
        int len = strlen("Only one of ");
        char *msg;
        for (i = 0; i < nsearches; ++i)
            len += strlen(searches[i]) + 2;
        len += strlen("can be enabled at a time in config\n");
        msg = ckd_malloc(len + 1);
        strcpy(msg, "Only one of ");
        for (i = 0; i < nsearches; ++i) {
            strcat(msg, searches[i]);
            strcat(msg, ", ");
        }
        strcat(msg, "can be enabled at a time in config\n");
        E_ERROR(msg);
        ckd_free(msg);
        return -1;
    }
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
    case 0:
        errstr = "JSON string appears to be empty";
        break;
    default:
        errstr = "Unknown error";
    }
    E_ERROR("JSON parsing failed: %s\n", errstr);
}

size_t
unescape(char *out, const char *in, size_t len)
{
   char *ptr = out;
   size_t i;

   for (i = 0; i < len; ++i) {
      int c = in[i];
      if (c == '\\') {
          switch (in[i+1]) {
          case '"':  *ptr++ = '"'; i++; break;
          case '\\': *ptr++ = '\\'; i++; break;
          case 'b':  *ptr++ = '\b'; i++; break;
          case 'f':  *ptr++ = '\f'; i++; break;
          case 'n': *ptr++ = '\n'; i++; break;
          case 'r': *ptr++ = '\r'; i++; break;
          case 't': *ptr++ = '\t'; i++; break;
          default:
              E_WARN("Unsupported escape sequence \\%c\n", in[i+1]);
              *ptr++ = c;
          }
      }
      else {
          *ptr++ = c;
      }
   }
   *ptr = '\0';
   return ptr - out;
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
        if ((config = ps_config_init(NULL)) == NULL)
            return NULL;
        new_config = TRUE;
    }
    jsmn_init(&parser);
    jslen = strlen(json);
    if ((ntok = jsmn_parse(&parser, json, jslen, NULL, 0)) <= 0) {
        json_error(ntok);
        goto error_out;
    }
    /* Need to reset the parser before second pass! */
    jsmn_init(&parser);
    tokens = ckd_calloc(ntok, sizeof(*tokens));
    if ((i = jsmn_parse(&parser, json, jslen, tokens, ntok)) != ntok) {
        json_error(i);
        goto error_out;
    }
    /* Accept missing top-level object. */
    i = 0;
    if (tokens[0].type == JSMN_OBJECT)
        ++i;
    while (i < ntok) {
        key = ckd_malloc(tokens[i].end - tokens[i].start + 1);
        unescape(key, json + tokens[i].start, tokens[i].end - tokens[i].start);
        if (tokens[i].type != JSMN_STRING && tokens[i].type != JSMN_PRIMITIVE) {
            E_ERROR("Expected string or primitive key, got %s\n", key);
            goto error_out;
        }
        if (++i == ntok) {
            E_ERROR("Missing value for %s\n", key);
            goto error_out;
        }
        val = ckd_malloc(tokens[i].end - tokens[i].start + 1);
        unescape(val, json + tokens[i].start, tokens[i].end - tokens[i].start);
        if (ps_config_set_str(config, key, val) == NULL) {
            E_ERROR("Unknown or invalid parameter %s\n", key);
            goto error_out;
        }
        ckd_free(key);
        ckd_free(val);
        key = val = NULL;
        ++i;
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

/* Following two functions are:
 *
 * Copyright (C) 2014 James McLaughlin.  All rights reserved.
 * https://github.com/udp/json-builder
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
static size_t measure_string (unsigned int length,
                              const char *str)
{
   unsigned int i;
   size_t measured_length = 0;

   for(i = 0; i < length; ++ i)
   {
      int c = str[i];

      switch (c)
      {
      case '"':
      case '\\':
      case '\b':
      case '\f':
      case '\n':
      case '\r':
      case '\t':

         measured_length += 2;
         break;

      default:

         ++ measured_length;
         break;
      };
   };

   return measured_length;
}

#define PRINT_ESCAPED(c) do {  \
   *buf ++ = '\\';             \
   *buf ++ = (c);              \
} while(0);                    \

static size_t serialize_string(char *buf,
                               unsigned int length,
                               const char *str)
{
   char *orig_buf = buf;
   unsigned int i;

   for(i = 0; i < length; ++ i)
   {
      int c = str [i];

      switch (c)
      {
      case '"':   PRINT_ESCAPED ('\"');  continue;
      case '\\':  PRINT_ESCAPED ('\\');  continue;
      case '\b':  PRINT_ESCAPED ('b');   continue;
      case '\f':  PRINT_ESCAPED ('f');   continue;
      case '\n':  PRINT_ESCAPED ('n');   continue;
      case '\r':  PRINT_ESCAPED ('r');   continue;
      case '\t':  PRINT_ESCAPED ('t');   continue;

      default:

         *buf ++ = c;
         break;
      };
   };

   return buf - orig_buf;
}
/* End code from json-builder */

static int
serialize_key(char *ptr, int maxlen, const char *key)
{
    int slen, len = 0;
    if (ptr) {
        *ptr++ = '\t';
        *ptr++ = '"';
        maxlen -= 2;
    }
    len += 2; /* \t\" */
    if (ptr) {
        assert(maxlen > 0);
        slen = serialize_string(ptr, strlen(key), key);
        ptr += slen;
        maxlen -= slen;
    }
    else {
        slen = measure_string(strlen(key), key);
    }
    len += slen;
    
    if (ptr) {
        assert(maxlen > 0);
        *ptr++ = '"';
        *ptr++ = ':';
        *ptr++ = ' ';
        maxlen -= 3;
    }
    len += 3; /* "\": " */
    return len;
}

static int
serialize_value(char *ptr, int maxlen, const char *val)
{
    int slen, len = 0;
    if (ptr) {
        *ptr++ = '"';
        maxlen--;
    }
    len++; /* \" */
    if (ptr) {
        assert(maxlen > 0);
        slen = serialize_string(ptr, strlen(val), val);
        ptr += slen;
        maxlen -= slen;
    }
    else {
        slen = measure_string(strlen(val), val);
    }
    len += slen;
    
    if (ptr) {
        assert(maxlen > 0);
        *ptr++ = '"';
        *ptr++ = ',';
        *ptr++ = '\n';
        maxlen -= 3;
    }
    len += 3; /* "\",\n" */
    return len;
}

static int
build_json(ps_config_t *config, char *json, int len)
{
    hash_iter_t *itor;
    char *ptr = json;
    int l, rv = 0;

    if ((l = snprintf(ptr, len, "{\n")) < 0)
        return -1;
    rv += l;
    if (ptr) {
        len -= l;
        ptr += l;
    }
    for (itor = hash_table_iter(config->ht); itor;
         itor = hash_table_iter_next(itor)) {
        const char *key = hash_entry_key(itor->ent);
        cmd_ln_val_t *cval = hash_entry_val(itor->ent);
        if (cval->type & ARG_STRING && cval->val.ptr == NULL)
            continue;
        if ((l = serialize_key(ptr, len, key)) < 0)
            return -1;
        rv += l;
        if (ptr) {
            len -= l;
            ptr += l;
        }
        if (cval->type & ARG_STRING) {
            if ((l = serialize_value(ptr, len,
                                     (char *)cval->val.ptr)) < 0)
                return -1;
        }
        else if (cval->type & ARG_INTEGER) {
            if ((l = snprintf(ptr, len, "%ld,\n",
                              cval->val.i)) < 0)
                return -1;
        }
        else if (cval->type & ARG_BOOLEAN) {
            if ((l = snprintf(ptr, len, "%s,\n",
                              cval->val.i ? "true" : "false")) < 0)
                return -1;
        }
        else if (cval->type & ARG_FLOATING) {
            if ((l = snprintf(ptr, len, "%g,\n",
                              cval->val.fl)) < 0)
                return -1;
        }
        else {
            E_ERROR("Unknown type %d for parameter %s\n",
                    cval->type, key);
        }
        rv += l;
        if (ptr) {
            len -= l;
            ptr += l;
        }
    }
    /* Back off last comma because JSON is awful */
    if (ptr && ptr > json + 1) {
        len += 2;
        ptr -= 2;
    }
    if ((l = snprintf(ptr, len, "\n}\n")) < 0)
        return -1;
    rv += l;
    if (ptr) {
        len -= l;
        ptr += l;
    }
    return rv;
}
        
const char *
ps_config_serialize_json(ps_config_t *config)
{
    int len = build_json(config, NULL, 0);
    if (len < 0)
        return NULL;
    if (config->json)
        ckd_free(config->json);
    config->json = ckd_malloc(len + 1);
    if (build_json(config, config->json, len + 1) != len) {
        ckd_free(config->json);
        config->json = NULL;
    }
    return config->json;
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
    const ps_arg_t *arg;
    if (cval == NULL) {
        E_ERROR("Unknown parameter %s\n", name);
        return NULL;
    }
    /* FIXME: Perhaps cmd_ln_val_t should store a pointer to ps_arg_t */
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
        val->ptr = ckd_malloc(len + 1);
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
        val->ptr = ckd_malloc(len + 1);
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
