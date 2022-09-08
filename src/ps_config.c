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

#include "pocketsphinx_internal.h"

#include "jsmn.h"

ps_config_t *
ps_config_init(const char *command)
{
    return NULL;
}

ps_config_t *
ps_config_retain(ps_config_t *config)
{
    return config;
}

int
ps_config_free(ps_config_t *config)
{
    return 0;
}

ps_config_t *
ps_config_parse_args(ps_config_t *config, int argc, char *argv[])
{
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
    return NULL;
}

const char *
ps_config_set_command(ps_config_t *config, const char *command)
{
    return NULL;
}

ps_type_t
ps_config_typeof(ps_config_t *config, char const *name)
{
    return 0;
}

const anytype_t *
ps_config_get(ps_config_t *config, const char *name)
{
    return NULL;
}

const anytype_t *
ps_config_set(ps_config_t *config, const char *name, const anytype_t *val)
{
    return NULL;
}

long
ps_config_int(ps_config_t *config, const char *name)
{
    return 0;
}

double
ps_config_float(ps_config_t *config, const char *name)
{
    return 0.0;
}

const char *
ps_config_str(ps_config_t *config, const char *name)
{
    return NULL;
}

const anytype_t *
ps_config_set_int(ps_config_t *config, const char *name, long val)
{
    return NULL;
}

const anytype_t *
ps_config_set_float(ps_config_t *config, const char *name, double val)
{
    return NULL;
}

const anytype_t *
ps_config_set_str(ps_config_t *config, const char *name, const char *val)
{
    return NULL;
}
