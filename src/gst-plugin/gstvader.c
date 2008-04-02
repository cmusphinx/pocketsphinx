/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
 * Copyright (C) 2002,2003,2005
 *           Thomas Vander Stichele <thomas at apestaart dot org>
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

/*
 * Modified version of the "cutter" element to do better at VAD.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <string.h>
#include <gst/gst.h>
#include <gst/audio/audio.h>
#include "gstvader.h"
#include "math.h"

GST_DEBUG_CATEGORY_STATIC(vader_debug);
#define GST_CAT_DEFAULT vader_debug

static const GstElementDetails vader_details =
    GST_ELEMENT_DETAILS("VAD element",
                        "Filter/Editor/Audio",
                        "Voice Activity DEtectoR to split audio into non-silent bits",
                        "Thomas <thomas@apestaart.org>, David Huggins-Daines <dhuggins@cs.cmu.edu>");

static GstStaticPadTemplate vader_src_factory =
    GST_STATIC_PAD_TEMPLATE("src",
                            GST_PAD_SRC,
                            GST_PAD_ALWAYS,
                            GST_STATIC_CAPS("audio/x-raw-int, "
                                            "rate = (int) [ 1, MAX ], "
                                            "channels = (int) [ 1, MAX ], "
                                            "endianness = (int) BYTE_ORDER, "
                                            "width = (int) 16, "
                                            "depth = (int) 16, "
                                            "signed = (boolean) true")
        );

static GstStaticPadTemplate vader_sink_factory =
    GST_STATIC_PAD_TEMPLATE("sink",
                            GST_PAD_SINK,
                            GST_PAD_ALWAYS,
                            GST_STATIC_CAPS("audio/x-raw-int, "
                                            /* FIXME: Actually we want this to be negotiable... */
                                            "rate = (int) 8000, "
                                            "channels = (int) 1, "
                                            "endianness = (int) BYTE_ORDER, "
                                            "width = (int) 16, "
                                            "depth = (int) 16, "
                                            "signed = (boolean) true")
        );

enum
{
    SIGNAL_VADER_START,
    SIGNAL_VADER_STOP,
    LAST_SIGNAL
};

static guint gst_vader_signals[LAST_SIGNAL];

enum
{
    PROP_0,
    PROP_THRESHOLD,
    PROP_AUTO_THRESHOLD,
    PROP_RUN_LENGTH,
    PROP_PRE_LENGTH
};

GST_BOILERPLATE(GstVader, gst_vader, GstElement, GST_TYPE_ELEMENT);

static void gst_vader_set_property(GObject * object, guint prop_id,
                                   const GValue * value, GParamSpec * pspec);
static void gst_vader_get_property(GObject * object, guint prop_id,
                                   GValue * value, GParamSpec * pspec);

static GstFlowReturn gst_vader_chain(GstPad * pad, GstBuffer * buffer);

static void
gst_vader_base_init(gpointer g_class)
{
    GstElementClass *element_class = GST_ELEMENT_CLASS(g_class);

    gst_element_class_add_pad_template(element_class,
                                       gst_static_pad_template_get(&vader_src_factory));
    gst_element_class_add_pad_template(element_class,
                                       gst_static_pad_template_get(&vader_sink_factory));
    gst_element_class_set_details(element_class, &vader_details);
}

static void
gst_vader_class_init(GstVaderClass * klass)
{
    GObjectClass *gobject_class;
    GstElementClass *gstelement_class;

    gobject_class = (GObjectClass *) klass;
    gstelement_class = (GstElementClass *) klass;

    gobject_class->set_property = gst_vader_set_property;
    gobject_class->get_property = gst_vader_get_property;

    g_object_class_install_property
        (G_OBJECT_CLASS(klass), PROP_THRESHOLD,
         g_param_spec_double("threshold", "Threshold",
                             "Volume threshold for speech/silence decision",
                             -1.0, 1.0, 256.0/32768.0, G_PARAM_READWRITE));
    g_object_class_install_property
        (G_OBJECT_CLASS(klass), PROP_AUTO_THRESHOLD,
         g_param_spec_boolean("auto-threshold", "Automatic Threshold",
                             "Set speech/silence threshold automatically",
                             FALSE, G_PARAM_READWRITE));
    g_object_class_install_property
        (G_OBJECT_CLASS(klass), PROP_RUN_LENGTH,
         g_param_spec_uint64("run-length", "Run length",
                             "Length of drop below threshold before cut_stop (in nanoseconds)",
                             0, G_MAXUINT64, (guint64)(0.5 * GST_SECOND), G_PARAM_READWRITE));
    g_object_class_install_property
        (G_OBJECT_CLASS(klass), PROP_PRE_LENGTH,
         g_param_spec_uint64("pre-length", "Pre-recording buffer length",
                             "Length of pre-recording buffer (in nanoseconds)",
                             0, G_MAXUINT64, (guint64)(0.5 * GST_SECOND), G_PARAM_READWRITE));

    gst_vader_signals[SIGNAL_VADER_START] = 
        g_signal_new("vader_start",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(GstVaderClass, vader_start),
                     NULL, NULL,
                     gst_marshal_VOID__INT64,
                     G_TYPE_NONE,
                     1, G_TYPE_UINT64
            );

    gst_vader_signals[SIGNAL_VADER_STOP] = 
        g_signal_new("vader_stop",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(GstVaderClass, vader_stop),
                     NULL, NULL,
                     gst_marshal_VOID__INT64,
                     G_TYPE_NONE,
                     1, G_TYPE_UINT64
            );

    GST_DEBUG_CATEGORY_INIT(vader_debug, "vader", 0, "Voice Activity Detection");
}

static void
gst_vader_init(GstVader * filter, GstVaderClass * g_class)
{
    filter->sinkpad =
        gst_pad_new_from_static_template(&vader_sink_factory, "sink");
    filter->srcpad =
        gst_pad_new_from_static_template(&vader_src_factory, "src");

    filter->threshold_level = 256;
    filter->threshold_length = (guint64)(0.5 * GST_SECOND);
    filter->auto_threshold = FALSE;
    filter->silence_mean = 0;
    filter->silence_stddev = 0;
    filter->silence_frames = 0;

    memset(filter->window, 0, VADER_WINDOW * sizeof(*filter->window));
    filter->silent = TRUE;
    filter->silent_prev = TRUE;
    filter->silent_run_length = 0;

    filter->pre_buffer = NULL;
    filter->pre_length = (guint64)(0.5 * GST_SECOND);
    filter->pre_run_length = 0;

    gst_element_add_pad(GST_ELEMENT(filter), filter->sinkpad);
    gst_pad_set_chain_function(filter->sinkpad, gst_vader_chain);
    gst_pad_use_fixed_caps(filter->sinkpad);

    gst_element_add_pad(GST_ELEMENT(filter), filter->srcpad);
    gst_pad_use_fixed_caps(filter->srcpad);
}

static GstMessage *
gst_vader_message_new(GstVader * c, gboolean above, GstClockTime timestamp)
{
    GstStructure *s;

    s = gst_structure_new("vader",
                          "above", G_TYPE_BOOLEAN, above,
                          "timestamp", GST_TYPE_CLOCK_TIME, timestamp, NULL);

    return gst_message_new_element(GST_OBJECT(c), s);
}

static GstEvent *
gst_vader_event_new(GstVader *c, GstEventType type, GstClockTime timestamp)
{
    GstEvent *e;

    e = gst_event_new_custom(type, NULL);
    GST_EVENT_TIMESTAMP(e) = timestamp;

    return e;
}

static guint
compute_normed_power(gint16 *in_data, guint num_samples)
{
    guint i, shift, sumsq;

    sumsq = 0;
    shift = 0;
    for (i = 0; i < num_samples; ++i) {
        guint sq;

        sq = in_data[i] * in_data[i];
        sumsq += (sq >> shift);
        /* Prevent overflows. */
        while (sumsq > 0x10000) {
            sumsq >>= 1;
            shift += 1;
        }
    }

    /* Normalize it to Q15 (this is equivalent to dividing by (1<<30)
     * then multiplying by (1<<15)). */
    if (shift > 15)
        return (sumsq << (shift - 15)) / num_samples;
    else
        return (sumsq / num_samples) >> (15 - shift);
}

/**
 * Calculate Q15 square root z of a Q15 number x.
 *
 * This is equal to 32768 \sqrt(\frac{x}{32768}), which is equal to
 * 2^{7.5} x^{0.5}, so:
 *
 * If y = log_2(x)
 * z = 2^{7.5 + 0.5y} = 2^{7.5 + 0.5y_{odd}} 2^{0.5y_{remainder})
 *   = 2^{7.5 + 0.5y_{odd}} + 2^{7.5 + 0.5y_{odd}} (2^{0.5y_{remainder}) - 1)
 *
 * Therefore the factor 2^{0.5y_{remainder}) - 1 can be stored in a
 * table.  Since 0 <= y_{remainder} < 2, this table has size 2^N -
 * 2^{N-2} for some value of N (7 is a pretty good one...)
 */
#define REMTAB_SIZE 96
static const guint16 remtab[REMTAB_SIZE] = {
0, 508, 1008, 1501, 1987, 2467, 2940, 3406, 3867, 4322, 4772, 5216, 5655, 6090, 6519, 6944, 7364, 7780, 8191, 8599, 9003, 9402, 9798, 10191, 10579, 10965, 11347, 11725, 12101, 12473, 12843, 13209, 13572, 13933, 14291, 14646, 14999, 15349, 15696, 16041, 16384, 16724, 17061, 17397, 17730, 18062, 18391, 18717, 19042, 19365, 19686, 20005, 20322, 20637, 20950, 21261, 21571, 21879, 22185, 22490, 22792, 23093, 23393, 23691, 23987, 24282, 24576, 24867, 25158, 25447, 25734, 26020, 26305, 26588, 26870, 27151, 27430, 27708, 27985, 28261, 28535, 28808, 29080, 29350, 29620, 29888, 30155, 30422, 30686, 30950, 31213, 31475, 31735, 31995, 32253, 32511
};
static guint
fixpoint_sqrt_q15(guint x)
{
    guint z;
    int log2, scale, idx;

    /* 0 and one are special cases since they have no closest odd
     * power of 2. */
    if (x == 0)
        return 0;
    else if (x == 1)
        return 181; /* 32768 * sqrt(1.0/32768) */

    /* Compute nearest log2. */
    for (log2 = 31; log2 > 0; --log2)
        if (x & (1<<log2))
            break;
    /* Find nearest odd log2. */
    if ((log2 & 1) == 0)
        log2 -= 1;
    /* Find index into remtab. */
    idx = x - (1<<log2);
    /* Scale it to fit remtab. */
    scale = (1<<(log2 + 2)) - (1<<log2);
    idx = idx * REMTAB_SIZE / scale;
    /* Base of square root. */
    z = 1<<(8 + log2 / 2);
    /* Add remainder. */
    return z + ((z * remtab[idx]) >> 15);
}

/**
 * Very approximate fixed-point square root (for big numbers only!)
 *
 * Really simple, sqrt(x) = 2^{\frac{\log_2 x}{2}}.  So approximate
 * \log_2 x, then divide it by two, and exponentiate :)
 */
static guint
fixpoint_bogus_sqrt(guint x)
{
    int log2;

    /* Compute nearest log2. */
    for (log2 = 31; log2 > 0; --log2)
        if (x & (1<<log2))
            break;
    /* Return "square root" */
    return 1<<(log2/2+1);
}

static GstFlowReturn
gst_vader_chain(GstPad * pad, GstBuffer * buf)
{
    GstVader *filter;
    gint16 *in_data;
    guint num_samples;
    GstBuffer *prebuf;            /* pointer to a prebuffer element */
    gint i, vote;
    guint power, rms;

    g_return_val_if_fail(pad != NULL, GST_FLOW_ERROR);
    g_return_val_if_fail(GST_IS_PAD(pad), GST_FLOW_ERROR);
    g_return_val_if_fail(buf != NULL, GST_FLOW_ERROR);

    filter = GST_VADER(GST_OBJECT_PARENT(pad));
    g_return_val_if_fail(filter != NULL, GST_FLOW_ERROR);
    g_return_val_if_fail(GST_IS_VADER(filter), GST_FLOW_ERROR);

    in_data = (gint16 *) GST_BUFFER_DATA(buf);
    num_samples = GST_BUFFER_SIZE(buf) / 2;

    filter->silent_prev = filter->silent;

    /* If we are in auto-threshold mode, check to see if we have
     * enough data to estimate a threshold.  (FIXME: we should be
     * estimating at the sample level rather than the frame level,
     * probably) */
    if (filter->threshold_level == -1) {
        if (filter->silence_frames > 5) {
            filter->silence_mean /= filter->silence_frames;
            filter->silence_stddev /= filter->silence_frames;
            filter->silence_stddev -= filter->silence_mean * filter->silence_mean;
            filter->silence_stddev = fixpoint_bogus_sqrt(filter->silence_stddev);
            /* Set threshold three standard deviations from the mean. */
            filter->threshold_level = filter->silence_mean + 3 * filter->silence_stddev;
            GST_DEBUG_OBJECT(filter, "silence_mean %d stddev %d auto_threshold %d\n",
                             filter->silence_mean, filter->silence_stddev,
                             filter->threshold_level);
        }
    }

    /* Divide buffer into reasonably sized parts. */
    for (i = 0; i < num_samples; i += VADER_FRAME) {
        gint frame_len, j;

        frame_len = MIN(num_samples - i, VADER_FRAME);
        power = compute_normed_power(in_data + i, frame_len);
        rms = fixpoint_sqrt_q15(power);

        /* If we are in auto-threshold mode, don't do any voting etc. */
        if (filter->threshold_level == -1) {
            filter->silence_mean += rms;
            filter->silence_stddev += rms * rms;
            filter->silence_frames += 1;
            GST_DEBUG_OBJECT(filter, "silence_mean_acc %d silence_stddev_acc %d frames %d\n",
                             filter->silence_mean, filter->silence_stddev, filter->silence_frames);
            continue;
        }
        /* Shift back window values. */
        memmove(filter->window, filter->window + 1,
                (VADER_WINDOW - 1) * sizeof(*filter->window));

        /* Decide if this buffer is silence or not. */
        if (rms > filter->threshold_level)
            filter->window[VADER_WINDOW-1] = TRUE;
        else
            filter->window[VADER_WINDOW-1] = FALSE;

        /* Vote on whether we have entered a region of non-silence. */
        vote = 0;
        for (j = 0; j < VADER_WINDOW; ++j)
            vote += filter->window[j];

        GST_DEBUG_OBJECT(filter, "frame_len %d rms power %d threshold %d vote %d\n",
                         frame_len, rms, filter->threshold_level, vote);

        if (vote > VADER_WINDOW / 2) {
            filter->silent_run_length = 0;
            filter->silent = FALSE;
        }
        else {
            filter->silent_run_length
                += gst_audio_duration_from_pad_buffer(filter->sinkpad, buf);
        }

        if (filter->silent_run_length > filter->threshold_length)
            /* it has been silent long enough, flag it */
            filter->silent = TRUE;
    }

    /* has the silent status changed ? if so, send right signal
     * and, if from silent -> not silent, flush pre_record buffer
     */
    if (filter->silent != filter->silent_prev) {
        if (filter->silent) {
            /* Sound to silence transition. */
            GstMessage *m =
                gst_vader_message_new(filter, FALSE, GST_BUFFER_TIMESTAMP(buf));
            GstEvent *e =
                gst_vader_event_new(filter, GST_EVENT_VADER_STOP, GST_BUFFER_TIMESTAMP(buf));
            GST_DEBUG_OBJECT(filter, "signaling CUT_STOP");
            gst_element_post_message(GST_ELEMENT(filter), m);
            /* Insert a custom event in the stream to mark the end of a cut. */
            gst_pad_push_event(filter->srcpad, e);
            /* FIXME: That event's timestamp is wrong... as is this one. */
            g_signal_emit(filter, gst_vader_signals[SIGNAL_VADER_STOP],
                          0, GST_BUFFER_TIMESTAMP(buf));
        } else {
            /* Silence to sound transition. */
            GstClockTime ts = GST_BUFFER_TIMESTAMP(buf);
            gint count = 0;
            GstMessage *m;
            GstEvent *e;

            GST_DEBUG_OBJECT(filter, "signaling CUT_START");
            /* Use the first pre_buffer's timestamp for the signal if possible. */
            if (filter->pre_buffer) {
                prebuf = (g_list_first(filter->pre_buffer))->data;
                ts = GST_BUFFER_TIMESTAMP(prebuf);
            }
            g_signal_emit(filter, gst_vader_signals[SIGNAL_VADER_START],
                          0, ts);
            m = gst_vader_message_new(filter, TRUE, ts);
            e = gst_vader_event_new(filter, GST_EVENT_VADER_START, ts);
            gst_element_post_message(GST_ELEMENT(filter), m);
            /* Insert a custom event in the stream to mark the beginning of a cut. */
            gst_pad_push_event(filter->srcpad, e);

            /* first of all, flush current buffer */
            GST_DEBUG_OBJECT(filter, "flushing buffer of length %" GST_TIME_FORMAT,
                             GST_TIME_ARGS(filter->pre_run_length));
            while (filter->pre_buffer) {
                prebuf = (g_list_first(filter->pre_buffer))->data;
                filter->pre_buffer = g_list_remove(filter->pre_buffer, prebuf);
                gst_pad_push(filter->srcpad, prebuf);
                ++count;
            }
            GST_DEBUG_OBJECT(filter, "flushed %d buffers", count);
            filter->pre_run_length = 0;
        }
    }
    /* now check if we have to send the new buffer to the internal buffer cache
     * or to the srcpad */
    if (filter->silent) {
        filter->pre_buffer = g_list_append(filter->pre_buffer, buf);
        filter->pre_run_length +=
            gst_audio_duration_from_pad_buffer(filter->sinkpad, buf);
        while (filter->pre_run_length > filter->pre_length) {
            prebuf = (g_list_first(filter->pre_buffer))->data;
            g_assert(GST_IS_BUFFER(prebuf));
            filter->pre_buffer = g_list_remove(filter->pre_buffer, prebuf);
            filter->pre_run_length -=
                gst_audio_duration_from_pad_buffer(filter->sinkpad, prebuf);
            gst_buffer_unref(prebuf);
        }
    } else {
        gst_pad_push(filter->srcpad, buf);
    }

    return GST_FLOW_OK;
}

static void
gst_vader_set_property(GObject * object, guint prop_id,
                       const GValue * value, GParamSpec * pspec)
{
    GstVader *filter;

    g_return_if_fail(GST_IS_VADER(object));
    filter = GST_VADER(object);

    switch (prop_id) {
    case PROP_THRESHOLD:
        filter->threshold_level = (gint)(g_value_get_double(value) * 32768.0);
        break;
    case PROP_AUTO_THRESHOLD:
        filter->auto_threshold = g_value_get_boolean(value);
        if (filter->auto_threshold) {
            /* We have to be in silence mode to calibrate. */
            filter->silent = TRUE;
            /* Reset counters and such. */
            filter->threshold_level = -1;
            memset(filter->window, 0, sizeof(*filter->window) * VADER_WINDOW);
            filter->silence_mean = 0;
            filter->silence_stddev = 0;
            filter->silence_frames = 0;
        }
        break;
    case PROP_RUN_LENGTH:
        filter->threshold_length = g_value_get_uint64(value);
        break;
    case PROP_PRE_LENGTH:
        filter->pre_length = g_value_get_uint64(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
gst_vader_get_property(GObject * object, guint prop_id,
                       GValue * value, GParamSpec * pspec)
{
    GstVader *filter;

    g_return_if_fail(GST_IS_VADER(object));
    filter = GST_VADER(object);

    switch (prop_id) {
    case PROP_RUN_LENGTH:
        g_value_set_uint64(value, filter->threshold_length);
        break;
    case PROP_PRE_LENGTH:
        g_value_set_uint64(value, filter->pre_length);
        break;
    case PROP_THRESHOLD:
        g_value_set_double(value, (gdouble)filter->threshold_level / 32768.0);
        break;
    case PROP_AUTO_THRESHOLD:
        g_value_set_boolean(value, filter->auto_threshold);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}
