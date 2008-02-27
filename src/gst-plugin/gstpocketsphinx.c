/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 2007 Carnegie Mellon University.  All rights
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
 * Author: David Huggins-Daines <dhuggins@cs.cmu.edu>
 */
/* Originally based on gstsphinxsink.c from gnome-voice-control:
 *
 * Copyright (C) 2007  Nickolay V. Shmyrev  <nshmyrev@yandex.ru>
 *
 * gstsphinxsink.c:
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string.h>
#include <gst/gst.h>
#include <sphinx_config.h>

#include "gstpocketsphinx.h"
#include "gstvader.h"

GST_DEBUG_CATEGORY_STATIC(pocketsphinx_debug);
#define GST_CAT_DEFAULT pocketsphinx_debug

enum
{
    SIGNAL_PARTIAL_RESULT,
    SIGNAL_RESULT,
    LAST_SIGNAL
};

enum
{
    PROP_0,
    PROP_HMM_DIR,
    PROP_LM_FILE,
    PROP_DICT_FILE,
    PROP_FSG_FILE,
    PROP_S2_FSG,
    PROP_FWDFLAT,
    PROP_BESTPATH
};

/* Default command line. (will go away soon and be constructed using properties) */
static char *default_argv[] = {
    "gst-pocketsphinx",
    "-hmm", POCKETSPHINX_PREFIX "/share/pocketsphinx/model/hmm/wsj1",
    "-lm", POCKETSPHINX_PREFIX "/share/pocketsphinx/model/lm/swb/swb.lm.DMP",
    "-dict", POCKETSPHINX_PREFIX "/share/pocketsphinx/model/lm/swb/swb.dic",
    "-samprate", "8000",
    "-nfft", "256",
    "-fwdflat", "no",
    "-bestpath", "no"
};
static const int default_argc = sizeof(default_argv)/sizeof(default_argv[0]);

static GstStaticPadTemplate sink_factory =
    GST_STATIC_PAD_TEMPLATE("sink",
                            GST_PAD_SINK,
                            GST_PAD_ALWAYS,
                            GST_STATIC_CAPS("audio/x-raw-int, "
                                            "width = (int) 16, "
                                            "depth = (int) 16, "
                                            "signed = (boolean) true, "
                                            "endianness = (int) BYTE_ORDER, "
                                            "channels = (int) 1, "
                                            "rate = (int) 8000")
        );

static GstStaticPadTemplate src_factory =
    GST_STATIC_PAD_TEMPLATE("src",
                            GST_PAD_SRC,
                            GST_PAD_ALWAYS,
                            GST_STATIC_CAPS("text/plain")
        );
	
static void gst_pocketsphinx_set_property(GObject * object, guint prop_id,
                                          const GValue * value, GParamSpec * pspec);
static void gst_pocketsphinx_get_property(GObject * object, guint prop_id,
                                          GValue * value, GParamSpec * pspec);
static GstFlowReturn gst_pocketsphinx_chain(GstPad * pad, GstBuffer * buffer);
static gboolean gst_pocketsphinx_event(GstPad *pad, GstEvent *event);

static guint gst_pocketsphinx_signals[LAST_SIGNAL];

GST_BOILERPLATE (GstPocketSphinx, gst_pocketsphinx, GstElement, GST_TYPE_ELEMENT);

static void
gst_pocketsphinx_base_init(gpointer gclass)
{
    static const GstElementDetails element_details = {
        "PocketSphinx",
        "Filter/Audio",
        "Convert speech to text",
        "David Huggins-Daines <dhuggins@cs.cmu.edu>"
    };
    GstElementClass *element_class = GST_ELEMENT_CLASS(gclass);

    gst_element_class_add_pad_template(element_class,
                                       gst_static_pad_template_get(&sink_factory));
    gst_element_class_add_pad_template(element_class,
                                       gst_static_pad_template_get(&src_factory));
    gst_element_class_set_details(element_class, &element_details);
}

static void
string_disposal(gpointer key, gpointer value, gpointer user_data)
{
    g_free(value);
}

static void
gst_pocketsphinx_finalize(GObject * gobject)
{
    GstPocketSphinx *ps = GST_POCKETSPHINX(gobject);

    g_hash_table_foreach(ps->arghash, string_disposal, NULL);
    g_hash_table_destroy(ps->arghash);

    GST_CALL_PARENT(G_OBJECT_CLASS, finalize,(gobject));
}

static void
gst_pocketsphinx_class_init(GstPocketSphinxClass * klass)
{
    GObjectClass *gobject_class;

    gobject_class =(GObjectClass *) klass;

    gobject_class->set_property = gst_pocketsphinx_set_property;
    gobject_class->get_property = gst_pocketsphinx_get_property;
    gobject_class->finalize = GST_DEBUG_FUNCPTR(gst_pocketsphinx_finalize);

    /* TODO: We will bridge cmd_ln.h properties to GObject
     * properties here somehow eventually. */
    g_object_class_install_property
        (gobject_class, PROP_HMM_DIR,
         g_param_spec_string("hmm", "HMM Directory",
                             "Directory containing acoustic model parameters",
                             POCKETSPHINX_PREFIX "/share/pocketsphinx/model/hmm/wsj1",
                             G_PARAM_READWRITE));
    g_object_class_install_property
        (gobject_class, PROP_LM_FILE,
         g_param_spec_string("lm", "LM File",
                             "Language model file",
                             POCKETSPHINX_PREFIX "/share/pocketsphinx/model/lm/swb/swb.lm.DMP",
                             G_PARAM_READWRITE));
    g_object_class_install_property
        (gobject_class, PROP_FSG_FILE,
         g_param_spec_string("fsg", "FSG File",
                             "Finite state grammar file",
                             NULL,
                             G_PARAM_READWRITE));
    g_object_class_install_property
        (gobject_class, PROP_S2_FSG,
         g_param_spec_pointer("s2_fsg", "FSG Pointer",
                              "Finite state grammar pointer (s2_fsg_t *)",
                              G_PARAM_WRITABLE));
    g_object_class_install_property
        (gobject_class, PROP_DICT_FILE,
         g_param_spec_string("dict", "Dictionary File",
                             "Dictionary File",
                             POCKETSPHINX_PREFIX "/share/pocketsphinx/model/lm/swb/swb.dic",
                             G_PARAM_READWRITE));

    g_object_class_install_property
        (gobject_class, PROP_FWDFLAT,
         g_param_spec_boolean("fwdflat", "Flat Lexicon Search",
                              "Enable Flat Lexicon Search",
                              FALSE,
                              G_PARAM_READWRITE));
    g_object_class_install_property
        (gobject_class, PROP_BESTPATH,
         g_param_spec_boolean("bestpath", "Graph Search",
                              "Enable Graph Search",
                              FALSE,
                              G_PARAM_READWRITE));

    gst_pocketsphinx_signals[SIGNAL_PARTIAL_RESULT] = 
        g_signal_new("partial_result",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(GstPocketSphinxClass, partial_result),
                     NULL, NULL,
                     gst_marshal_VOID__STRING,
                     G_TYPE_NONE,
                     1, G_TYPE_STRING
            );

    gst_pocketsphinx_signals[SIGNAL_RESULT] = 
        g_signal_new("result",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(GstPocketSphinxClass, result),
                     NULL, NULL,
                     gst_marshal_VOID__STRING,
                     G_TYPE_NONE,
                     1, G_TYPE_STRING
            );

    GST_DEBUG_CATEGORY_INIT(pocketsphinx_debug, "pocketsphinx", 0,
                            "Automatic Speech Recognition");
}

static void
gst_pocketsphinx_set_string(GstPocketSphinx *ps,
                            const gchar *key, const GValue *value)
{
    gchar *oldstr, *newstr;

    if (value != NULL)
        newstr = g_strdup(g_value_get_string(value));
    else
        newstr = NULL;
    if ((oldstr = g_hash_table_lookup(ps->arghash, key)))
        g_free(oldstr);
    cmd_ln_set_str(key, newstr);
    g_hash_table_foreach(ps->arghash, (gpointer)key, newstr);
}

static void
gst_pocketsphinx_set_boolean(GstPocketSphinx *ps,
                             const gchar *key, const GValue *value)
{
    cmd_ln_set_boolean(key, g_value_get_boolean(value));
}

static void
gst_pocketsphinx_set_property(GObject * object, guint prop_id,
                              const GValue * value, GParamSpec * pspec)
{
    GstPocketSphinx *ps = GST_POCKETSPHINX(object);

    switch (prop_id) {
    case PROP_HMM_DIR:
    {
        gst_pocketsphinx_set_string(ps, "-hmm", value);
        if (ps->inited) {
            /* Reinitialize the decoder with the new language model. */
            fbs_end();
            fbs_init(0, NULL);
        }
        break;
    }
    case PROP_LM_FILE:
        /* FSG and LM are mutually exclusive. */
        gst_pocketsphinx_set_string(ps, "-fsg", NULL);
        gst_pocketsphinx_set_string(ps, "-lm", value);
        if (ps->inited) {
            /* Switch to this new LM. */
            lm_read(g_value_get_string(value),
                    g_value_get_string(value),
                    cmd_ln_float32("-lw"),
                    cmd_ln_float32("-uw"),
                    cmd_ln_float32("-wip"));
            uttproc_set_lm(g_value_get_string(value));
        }
        break;
    case PROP_DICT_FILE:
        gst_pocketsphinx_set_string(ps, "-dict", value);
        break;
    case PROP_FSG_FILE:
        /* FSG and LM are mutually exclusive */
        gst_pocketsphinx_set_string(ps, "-lm", NULL);
        gst_pocketsphinx_set_string(ps, "-fsg", value);
        if (ps->inited) {
            /* Switch to this new FSG. */
            char *fsgname;
            fsgname = uttproc_load_fsgfile((char *)
                                           g_value_get_string(value));
            if (fsgname)
                uttproc_set_fsg(fsgname);
        }
        break;
    case PROP_S2_FSG:
    {
        s2_fsg_t *fsg = g_value_get_pointer(value);

        uttproc_del_fsg(fsg->name);
        uttproc_load_fsg(g_value_get_pointer(value),
                         cmd_ln_boolean("-fsgusealtpron"),
                         cmd_ln_boolean("-fsgusefiller"),
                         cmd_ln_float32("-silpen"),
                         cmd_ln_float32("-fillpen"),
                         cmd_ln_float32("-lw"));
        uttproc_set_fsg(fsg->name);
        break;
    }
    case PROP_FWDFLAT:
        gst_pocketsphinx_set_boolean(ps, "-fwdflat", value);
        break;
    case PROP_BESTPATH:
        gst_pocketsphinx_set_boolean(ps, "-bestpath", value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        return;
    }
}

static void
gst_pocketsphinx_get_property(GObject * object, guint prop_id,
                              GValue * value, GParamSpec * pspec)
{
    switch (prop_id) {
    case PROP_HMM_DIR:
        g_value_set_string(value, cmd_ln_str("-hmm"));
        break;
    case PROP_LM_FILE:
        g_value_set_string(value, cmd_ln_str("-lm"));
        break;
    case PROP_DICT_FILE:
        g_value_set_string(value, cmd_ln_str("-dict"));
        break;
    case PROP_FSG_FILE:
        g_value_set_string(value, cmd_ln_str("-fsg"));
        break;
    case PROP_FWDFLAT:
        g_value_set_boolean(value, cmd_ln_boolean("-fwdflat"));
        break;
    case PROP_BESTPATH:
        g_value_set_boolean(value, cmd_ln_boolean("-bestpath"));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
gst_pocketsphinx_init(GstPocketSphinx * ps,
                      GstPocketSphinxClass * gclass)
{
    ps->sinkpad =
        gst_pad_new_from_static_template(&sink_factory, "sink");
    ps->srcpad =
        gst_pad_new_from_static_template(&src_factory, "src");

    /* Create the hash table to store argument strings. */
    ps->arghash = g_hash_table_new(g_str_hash, g_str_equal);

    /* Parse default command-line options. */
    cmd_ln_parse(fbs_get_args(), default_argc, default_argv, FALSE);

    /* Set up pads. */
    gst_element_add_pad(GST_ELEMENT(ps), ps->sinkpad);
    gst_pad_set_chain_function(ps->sinkpad, gst_pocketsphinx_chain);
    gst_pad_set_event_function(ps->sinkpad, gst_pocketsphinx_event);
    gst_pad_use_fixed_caps(ps->sinkpad);

    gst_element_add_pad(GST_ELEMENT(ps), ps->srcpad);
    gst_pad_use_fixed_caps(ps->srcpad);
}

static GstFlowReturn
gst_pocketsphinx_chain(GstPad * pad, GstBuffer * buffer)
{
    GstPocketSphinx *ps;

    ps = GST_POCKETSPHINX(GST_OBJECT_PARENT(pad));

    /* Start an utterance for the first buffer we get (i.e. we assume
     * that the VADER is "leaky") */
    if (!ps->listening) {
        ps->listening = TRUE;
        uttproc_begin_utt(NULL);
    }
    uttproc_rawdata((short *)GST_BUFFER_DATA(buffer),
                    GST_BUFFER_SIZE(buffer) / sizeof(short), TRUE);
    return GST_FLOW_OK;
}

static gboolean
gst_pocketsphinx_event(GstPad *pad, GstEvent *event)
{
    GstPocketSphinx *ps;

    ps = GST_POCKETSPHINX(GST_OBJECT_PARENT(pad));

    /* Pick out VAD events. */
    switch (event->type) {
    case GST_EVENT_NEWSEGMENT:
        /* Initialize the decoder once the audio starts. (HACK) */
        fbs_init(0, NULL);
        ps->inited = TRUE;
        return gst_pad_event_default(pad, event);
    case GST_EVENT_VADER_START:
        ps->listening = TRUE;
        uttproc_begin_utt(NULL);
        /* Forward this event. */
        return gst_pad_event_default(pad, event);
    case GST_EVENT_EOS:
    case GST_EVENT_VADER_STOP: {
        GstBuffer *buffer;
        int32 frm;
        char *hyp;

        ps->listening = FALSE;
        uttproc_end_utt();
        uttproc_result(&frm, &hyp, TRUE);
        /* Forward this result in a buffer. */
        buffer = gst_buffer_new_and_alloc(strlen(hyp) + 1);
        strcpy((char *)GST_BUFFER_DATA(buffer), hyp);
        GST_BUFFER_TIMESTAMP(buffer) = GST_EVENT_TIMESTAMP(event);
        gst_buffer_set_caps(buffer, GST_PAD_CAPS(ps->srcpad));
        gst_pad_push(ps->srcpad, buffer);

        /* Forward this event. */
        return gst_pad_event_default(pad, event);
    }
    default:
        /* Don't bother with other events. */
        return gst_pad_event_default(pad, event);
    }
}

static gboolean
plugin_init(GstPlugin * plugin)
{
    if (!gst_element_register(plugin, "pocketsphinx",
                              GST_RANK_NONE, GST_TYPE_POCKETSPHINX))
        return FALSE;
    if (!gst_element_register(plugin, "vader",
                              GST_RANK_NONE, GST_TYPE_VADER))
        return FALSE;
    return TRUE;
}

#define VERSION PACKAGE_VERSION
#define PACKAGE PACKAGE_NAME
GST_PLUGIN_DEFINE(GST_VERSION_MAJOR,
                  GST_VERSION_MINOR,
                  "pocketsphinx",
                  "PocketSphinx plugin",
                  plugin_init, VERSION,
                  "LGPL", "PocketSphinx", "http://pocketsphinx.org/")
