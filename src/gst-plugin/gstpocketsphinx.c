/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 2014 Alpha Cephei Inc.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <gst/gst.h>

#include <sphinxbase/err.h>
#include <sphinxbase/strfuncs.h>

#include "gstpocketsphinx.h"

GST_DEBUG_CATEGORY_STATIC(pocketsphinx_debug);
#define GST_CAT_DEFAULT pocketsphinx_debug


static void
gst_pocketsphinx_set_property(GObject * object, guint prop_id,
                          const GValue * value, GParamSpec * pspec);
static void 
gst_pocketsphinx_get_property(GObject * object, guint prop_id,
                              GValue * value, GParamSpec * pspec);
static GstFlowReturn
gst_pocketsphinx_chain(GstPad * pad, GstObject *parent, GstBuffer * buffer);
static gboolean
gst_pocketsphinx_event(GstPad *pad, GstObject *parent, GstEvent *event);
static void
gst_pocketsphinx_finalize_utt(GstPocketSphinx *ps);
static void
gst_pocketsphinx_finalize(GObject * gobject);

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
    PROP_LMCTL_FILE,
    PROP_LM_NAME,
    PROP_DICT_FILE,
    PROP_MLLR_FILE,
    PROP_FSG_FILE,
    PROP_FSG_MODEL,
    PROP_FWDFLAT,
    PROP_BESTPATH,
    PROP_MAXHMMPF,
    PROP_MAXWPF,
    PROP_BEAM,
    PROP_WBEAM,
    PROP_PBEAM,
    PROP_DSRATIO,
    PROP_LATDIR,
    PROP_DECODER,
    PROP_CONFIGURED
};

/*
 * Static data.
 */

/* Default command line. (will go away soon and be constructed using properties) */
static char *default_argv[] = {
    "gst-pocketsphinx",
};
static const int default_argc = sizeof(default_argv)/sizeof(default_argv[0]);

static GstStaticPadTemplate sink_factory =
    GST_STATIC_PAD_TEMPLATE("sink",
                            GST_PAD_SINK,
                            GST_PAD_ALWAYS,
                            GST_STATIC_CAPS("audio/x-raw, "
		            		    "format = (string) { S16LE }, "
                                            "channels = (int) 1, "
                                            "rate = (int) 16000")
        );

static GstStaticPadTemplate src_factory =
    GST_STATIC_PAD_TEMPLATE("src",
                            GST_PAD_SRC,
                            GST_PAD_ALWAYS,
                            GST_STATIC_CAPS("text/plain")
        );
static guint gst_pocketsphinx_signals[LAST_SIGNAL];

/*
 * Boxing of ps_decoder_t.
 */

GType
ps_decoder_get_type(void)
{
    static GType ps_decoder_type = 0;

    if (G_UNLIKELY(ps_decoder_type == 0)) {
        ps_decoder_type = g_boxed_type_register_static
            ("PSDecoder",
             /* Conveniently, these should just work. */
             (GBoxedCopyFunc) ps_retain,
             (GBoxedFreeFunc) ps_free);
    }

    return ps_decoder_type;
}


/*
 * Class definition
 */

G_DEFINE_TYPE(GstPocketSphinx, gst_pocketsphinx, GST_TYPE_ELEMENT);


static void
gst_pocketsphinx_class_init(GstPocketSphinxClass * klass)
{
    GObjectClass *gobject_class;
    GstElementClass *element_class;;

    gobject_class =(GObjectClass *) klass;
    element_class = (GstElementClass *)klass;

    gobject_class->set_property = gst_pocketsphinx_set_property;
    gobject_class->get_property = gst_pocketsphinx_get_property;
    gobject_class->finalize = gst_pocketsphinx_finalize;

    /* TODO: We will bridge cmd_ln.h properties to GObject
     * properties here somehow eventually. */
    g_object_class_install_property
        (gobject_class, PROP_HMM_DIR,
         g_param_spec_string("hmm", "HMM Directory",
                             "Directory containing acoustic model parameters",
                             NULL,
                             G_PARAM_READWRITE));
    g_object_class_install_property
        (gobject_class, PROP_LM_FILE,
         g_param_spec_string("lm", "LM File",
                             "Language model file",
                             NULL,
                             G_PARAM_READWRITE));
    g_object_class_install_property
        (gobject_class, PROP_LMCTL_FILE,
         g_param_spec_string("lmctl", "LM Control File",
                             "Language model control file (for class LMs)",
                             NULL,
                             G_PARAM_READWRITE));
    g_object_class_install_property
        (gobject_class, PROP_LM_NAME,
         g_param_spec_string("lmname", "LM Name",
                             "Language model name (to select LMs from lmctl)",
                             NULL,
                             G_PARAM_READWRITE));
    g_object_class_install_property
        (gobject_class, PROP_FSG_FILE,
         g_param_spec_string("fsg", "FSG File",
                             "Finite state grammar file",
                             NULL,
                             G_PARAM_READWRITE));
    g_object_class_install_property
        (gobject_class, PROP_FSG_MODEL,
         g_param_spec_pointer("fsg_model", "FSG Model",
                              "Finite state grammar object (fsg_model_t *)",
                              G_PARAM_WRITABLE));
    g_object_class_install_property
        (gobject_class, PROP_DICT_FILE,
         g_param_spec_string("dict", "Dictionary File",
                             "Dictionary File",
                             NULL,
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

    g_object_class_install_property
        (gobject_class, PROP_LATDIR,
         g_param_spec_string("latdir", "Lattice Directory",
                             "Output Directory for Lattices",
                             NULL,
                             G_PARAM_READWRITE));
    g_object_class_install_property
        (gobject_class, PROP_MAXHMMPF,
         g_param_spec_int("maxhmmpf", "Maximum HMMs per frame",
                          "Maximum number of HMMs searched per frame",
                          1, 100000, 1000,
                          G_PARAM_READWRITE));
    g_object_class_install_property
        (gobject_class, PROP_MAXWPF,
         g_param_spec_int("maxwpf", "Maximum words per frame",
                          "Maximum number of words searched per frame",
                          1, 100000, 10,
                          G_PARAM_READWRITE));
    g_object_class_install_property
        (gobject_class, PROP_BEAM,
         g_param_spec_double("beam", "Beam width applied to every frame in Viterbi search",
                          "Beam width applied to every frame in Viterbi search",
                          -1, 1, 1e-48,
                          G_PARAM_READWRITE));
    g_object_class_install_property
        (gobject_class, PROP_PBEAM,
         g_param_spec_double("pbeam", "Beam width applied to phone transitions",
                          "Beam width applied to phone transitions",
                          -1, 1, 1e-48,
                          G_PARAM_READWRITE));
    g_object_class_install_property
        (gobject_class, PROP_WBEAM,
         g_param_spec_double("wbeam", "Beam width applied to word exits",
                          "Beam width applied to phone transitions",
                          -1, 1, 7e-29,
                          G_PARAM_READWRITE));
    g_object_class_install_property
        (gobject_class, PROP_DSRATIO,
         g_param_spec_int("dsratio", "Frame downsampling ratio",
                          "Evaluate acoustic model every N frames",
                          1, 10, 1,
                          G_PARAM_READWRITE));
    g_object_class_install_property
        (gobject_class, PROP_DECODER,
         g_param_spec_boxed("decoder", "Decoder object",
                            "The underlying decoder",
                            PS_DECODER_TYPE,
                            G_PARAM_READABLE));
    g_object_class_install_property
        (gobject_class, PROP_CONFIGURED,
         g_param_spec_boolean("configured", "Finalize configuration",
                              "Set this to finalize configuration",
                              FALSE,
                              G_PARAM_READWRITE));

    gst_pocketsphinx_signals[SIGNAL_PARTIAL_RESULT] = 
        g_signal_new("partial_result",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(GstPocketSphinxClass, partial_result),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__STRING,
                     G_TYPE_NONE,
                     1, G_TYPE_STRING
            );

    gst_pocketsphinx_signals[SIGNAL_RESULT] = 
        g_signal_new("result",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(GstPocketSphinxClass, result),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__STRING,
                     G_TYPE_NONE,
                     1, G_TYPE_STRING
            );

    GST_DEBUG_CATEGORY_INIT(pocketsphinx_debug, "pocketsphinx", 0,
                            "Automatic Speech Recognition");


    gst_element_class_add_pad_template(element_class,
                                       gst_static_pad_template_get(&sink_factory));
    gst_element_class_add_pad_template(element_class,
                                       gst_static_pad_template_get(&src_factory));

    gst_element_class_set_static_metadata(element_class, "PocketSphinx", "Filter/Audio", "Convert speech to text", "CMUSphinx-devel <cmusphinx-devel@lists.sourceforge.net>");

}

static void
gst_pocketsphinx_set_string(GstPocketSphinx *ps,
                            const gchar *key, const GValue *value)
{
    cmd_ln_set_str_r(ps->config, key, g_value_get_string(value));
}

static void
gst_pocketsphinx_set_int(GstPocketSphinx *ps,
                         const gchar *key, const GValue *value)
{
    cmd_ln_set_int32_r(ps->config, key, g_value_get_int(value));
}

static void
gst_pocketsphinx_set_boolean(GstPocketSphinx *ps,
                             const gchar *key, const GValue *value)
{
    cmd_ln_set_boolean_r(ps->config, key, g_value_get_boolean(value));
}

static void
gst_pocketsphinx_set_double(GstPocketSphinx *ps,
                         const gchar *key, const GValue *value)
{
    cmd_ln_set_float_r(ps->config, key, g_value_get_double(value));
}

static void
gst_pocketsphinx_set_property(GObject * object, guint prop_id,
                              const GValue * value, GParamSpec * pspec)
{
    GstPocketSphinx *ps = GST_POCKETSPHINX(object);

    switch (prop_id) {
    case PROP_HMM_DIR:
        gst_pocketsphinx_set_string(ps, "-hmm", value);
        if (ps->ps) {
            /* Reinitialize the decoder with the new acoustic model. */
            ps_reinit(ps->ps, NULL);
        }
        break;
    case PROP_LM_FILE:
        /* FSG and LM are mutually exclusive. */
        gst_pocketsphinx_set_string(ps, "-fsg", NULL);
        gst_pocketsphinx_set_string(ps, "-lmctl", NULL);
        gst_pocketsphinx_set_string(ps, "-lm", value);
        break;
    case PROP_LMCTL_FILE:
        /* FSG and LM are mutually exclusive. */
        gst_pocketsphinx_set_string(ps, "-fsg", NULL);
        gst_pocketsphinx_set_string(ps, "-lmctl", value);
        gst_pocketsphinx_set_string(ps, "-lm", NULL);
        break;
    case PROP_LM_NAME:
        gst_pocketsphinx_set_string(ps, "-fsg", NULL);
        gst_pocketsphinx_set_string(ps, "-lmname", value);
        break;
    case PROP_DICT_FILE:
        gst_pocketsphinx_set_string(ps, "-dict", value);
        if (ps->ps) {
            /* Reinitialize the decoder with the new dictionary. */
            ps_reinit(ps->ps, NULL);
        }
        break;
    case PROP_MLLR_FILE:
        gst_pocketsphinx_set_string(ps, "-mllr", value);
        /* Reinitialize the decoder with the new MLLR transform. */
        if (ps->ps)
            ps_reinit(ps->ps, NULL);
        break;
    case PROP_FSG_MODEL:
        {
            fsg_model_t *fsg = g_value_get_pointer(value);
            const char *name = fsg_model_name(fsg);
            ps_set_fsg(ps->ps, name, fsg);
            ps_set_search(ps->ps, name);
        }
        break;
    case PROP_FSG_FILE:
        /* FSG and LM are mutually exclusive */
        gst_pocketsphinx_set_string(ps, "-lm", NULL);
        gst_pocketsphinx_set_string(ps, "-fsg", value);
        break;
    case PROP_FWDFLAT:
        gst_pocketsphinx_set_boolean(ps, "-fwdflat", value);
        break;
    case PROP_BESTPATH:
        gst_pocketsphinx_set_boolean(ps, "-bestpath", value);
        break;
    case PROP_LATDIR:
        if (ps->latdir)
            g_free(ps->latdir);
        ps->latdir = g_strdup(g_value_get_string(value));
        break;
    case PROP_MAXHMMPF:
        gst_pocketsphinx_set_int(ps, "-maxhmmpf", value);
        break;
    case PROP_MAXWPF:
        gst_pocketsphinx_set_int(ps, "-maxwpf", value);
        break;
    case PROP_BEAM:
        gst_pocketsphinx_set_double(ps, "-beam", value);
        break;
    case PROP_PBEAM:
        gst_pocketsphinx_set_double(ps, "-pbeam", value);
        break;
    case PROP_WBEAM:
        gst_pocketsphinx_set_double(ps, "-wbeam", value);
        break;
    case PROP_DSRATIO:
        gst_pocketsphinx_set_int(ps, "-ds", value);
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
    GstPocketSphinx *ps = GST_POCKETSPHINX(object);

    switch (prop_id) {
    case PROP_DECODER:
        g_value_set_boxed(value, ps->ps);
        break;
    case PROP_CONFIGURED:
        g_value_set_boolean(value, ps->ps != NULL);
        break;
    case PROP_HMM_DIR:
        g_value_set_string(value, cmd_ln_str_r(ps->config, "-hmm"));
        break;
    case PROP_LM_FILE:
        g_value_set_string(value, cmd_ln_str_r(ps->config, "-lm"));
        break;
    case PROP_LMCTL_FILE:
        g_value_set_string(value, cmd_ln_str_r(ps->config, "-lmctl"));
        break;
    case PROP_LM_NAME:
        g_value_set_string(value, cmd_ln_str_r(ps->config, "-lmname"));
        break;
    case PROP_DICT_FILE:
        g_value_set_string(value, cmd_ln_str_r(ps->config, "-dict"));
        break;
    case PROP_MLLR_FILE:
        g_value_set_string(value, cmd_ln_str_r(ps->config, "-mllr"));
        break;
    case PROP_FSG_FILE:
        g_value_set_string(value, cmd_ln_str_r(ps->config, "-fsg"));
        break;
    case PROP_FWDFLAT:
        g_value_set_boolean(value, cmd_ln_boolean_r(ps->config, "-fwdflat"));
        break;
    case PROP_BESTPATH:
        g_value_set_boolean(value, cmd_ln_boolean_r(ps->config, "-bestpath"));
        break;
    case PROP_LATDIR:
        g_value_set_string(value, ps->latdir);
        break;
    case PROP_MAXHMMPF:
        g_value_set_int(value, cmd_ln_int32_r(ps->config, "-maxhmmpf"));
        break;
    case PROP_MAXWPF:
        g_value_set_int(value, cmd_ln_int32_r(ps->config, "-maxwpf"));
        break;
    case PROP_BEAM:
        g_value_set_double(value, cmd_ln_float_r(ps->config, "-beam"));
        break;
    case PROP_PBEAM:
        g_value_set_double(value, cmd_ln_float_r(ps->config, "-pbeam"));
        break;
    case PROP_WBEAM:
        g_value_set_double(value, cmd_ln_float_r(ps->config, "-wbeam"));
        break;
    case PROP_DSRATIO:
        g_value_set_int(value, cmd_ln_int32_r(ps->config, "-ds"));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
gst_pocketsphinx_finalize(GObject * gobject)
{
    GstPocketSphinx *ps = GST_POCKETSPHINX(gobject);

    ps_free(ps->ps);
    cmd_ln_free_r(ps->config);
    g_free(ps->last_result);

    G_OBJECT_CLASS(gst_pocketsphinx_parent_class)->finalize(gobject);
}

static void
gst_pocketsphinx_init(GstPocketSphinx * ps)
{
    ps->sinkpad =
        gst_pad_new_from_static_template(&sink_factory, "sink");
    ps->srcpad =
        gst_pad_new_from_static_template(&src_factory, "src");

    /* Parse default command-line options. */
    ps->config = cmd_ln_parse_r(NULL, ps_args(), default_argc, default_argv, FALSE);
    ps_default_search_args(ps->config);
    ps->ps = ps_init(ps->config);
    if (ps->ps == NULL) {
        GST_ELEMENT_ERROR(GST_ELEMENT(ps), LIBRARY, INIT,
                          ("Failed to initialize PocketSphinx"),
                          ("Failed to initialize PocketSphinx"));
    }

    /* Set up pads. */
    gst_element_add_pad(GST_ELEMENT(ps), ps->sinkpad);
    gst_pad_set_chain_function(ps->sinkpad, gst_pocketsphinx_chain);
    gst_pad_set_event_function(ps->sinkpad, gst_pocketsphinx_event);
    gst_pad_use_fixed_caps(ps->sinkpad);

    gst_element_add_pad(GST_ELEMENT(ps), ps->srcpad);
    gst_pad_use_fixed_caps(ps->srcpad);

    /* Initialize time. */
    ps->last_result_time = 0;
    ps->last_result = NULL;
}

static GstFlowReturn
gst_pocketsphinx_chain(GstPad * pad, GstObject *parent, GstBuffer * buffer)
{
    GstPocketSphinx *ps;
    GstMapInfo info;
    gboolean in_speech;
    
    ps = GST_POCKETSPHINX(parent);

    /* Start an utterance for the first buffer we get */
    if (!ps->listening_started) {
        ps->listening_started = TRUE;
        ps->utt_started = FALSE;
        ps_start_utt(ps->ps);
    }

    gst_buffer_map (buffer, &info, GST_MAP_READ);
    ps_process_raw(ps->ps,
                   (short*) info.data,
                   info.size / sizeof(short),
                   FALSE, FALSE);
    gst_buffer_unmap (buffer, &info);

    in_speech = ps_get_in_speech(ps->ps);
    if (in_speech && !ps->utt_started) {
    	ps->utt_started = TRUE;
    }
    if (!in_speech && ps->utt_started) {
	gst_pocketsphinx_finalize_utt(ps);
    } else if (ps->last_result_time == 0
        /* Get a partial result every now and then, see if it is different. */
        /* Check every 100 milliseconds. */
        || (GST_BUFFER_TIMESTAMP(buffer) - ps->last_result_time) > 100*10*1000) {
        int32 score;
        char const *hyp;

        hyp = ps_get_hyp(ps->ps, &score);
        ps->last_result_time = GST_BUFFER_TIMESTAMP(buffer);
        if (hyp && strlen(hyp) > 0) {
            if (ps->last_result == NULL || 0 != strcmp(ps->last_result, hyp)) {
                g_free(ps->last_result);
                ps->last_result = g_strdup(hyp);
                /* Emit a signal for applications. */
                g_signal_emit(ps, gst_pocketsphinx_signals[SIGNAL_PARTIAL_RESULT],
                              0, hyp);
            }
        }
    }

    gst_buffer_unref(buffer);
    return GST_FLOW_OK;
}


static void
gst_pocketsphinx_finalize_utt(GstPocketSphinx *ps)
{
    GstBuffer *buffer;
    char const *hyp;
    int32 score;

    hyp = NULL;
    if (!ps->listening_started || !ps->utt_started)
	return;
	
    ps_end_utt(ps->ps);
    ps->listening_started = FALSE;
    hyp = ps_get_hyp(ps->ps, &score);

    /* Dump the lattice if requested. */
    if (ps->latdir) {
        char *latfile;
        char uttid[16];

	sprintf(uttid, "%09u", ps->uttno);
	ps->uttno++;
        latfile = string_join(ps->latdir, "/", uttid, ".lat", NULL);
        ps_lattice_t *dag;
        if ((dag = ps_get_lattice(ps->ps)))
            ps_lattice_write(dag, latfile);
        ckd_free(latfile);
    }
    if (hyp) {
        g_signal_emit(ps, gst_pocketsphinx_signals[SIGNAL_RESULT],
    		      0, hyp);
        buffer = gst_buffer_new_and_alloc(strlen(hyp) + 1);
	gst_buffer_fill(buffer, 0, hyp, strlen(hyp));
	gst_buffer_fill(buffer, strlen(hyp), "\n", 1);
	gst_pad_push(ps->srcpad, buffer);
    }
}

static gboolean
gst_pocketsphinx_event(GstPad *pad, GstObject *parent, GstEvent *event)
{
    GstPocketSphinx *ps;

    ps = GST_POCKETSPHINX(parent);

    switch (event->type) {
    case GST_EVENT_EOS:
    {
	gst_pocketsphinx_finalize_utt(ps);
        return gst_pad_event_default(pad, parent, event);
    }
    default:
        return gst_pad_event_default(pad, parent, event);
    }
}

static void
gst_pocketsphinx_log(void *user_data, err_lvl_t lvl, const char *fmt, ...)
{
    static const int gst_level[ERR_MAX] = {GST_LEVEL_DEBUG, GST_LEVEL_INFO,
             GST_LEVEL_INFO, GST_LEVEL_WARNING, GST_LEVEL_ERROR, GST_LEVEL_ERROR};
                 
     va_list ap;
     va_start(ap, fmt);
     gst_debug_log_valist(pocketsphinx_debug, gst_level[lvl], "", "", 0, NULL, fmt, ap);
     va_end(ap);
}


static gboolean
plugin_init(GstPlugin * plugin)
{

    err_set_callback(gst_pocketsphinx_log, NULL);

    if (!gst_element_register(plugin, "pocketsphinx",
                              GST_RANK_NONE, GST_TYPE_POCKETSPHINX))
        return FALSE;
    return TRUE;
}

#define PACKAGE PACKAGE_NAME
GST_PLUGIN_DEFINE(GST_VERSION_MAJOR,
                  GST_VERSION_MINOR,
                  PACKAGE_NAME,
                  "PocketSphinx plugin",
                  plugin_init, PACKAGE_VERSION,
                  "BSD",
                  "PocketSphinx", "http://cmusphinx.sourceforge.net/")
