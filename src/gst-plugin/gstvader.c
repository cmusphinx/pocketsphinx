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
 * This is just the "cutter" element with some extra stuff.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <gst/gst.h>
#include <gst/audio/audio.h>
#include "gstvader.h"
#include "math.h"

GST_DEBUG_CATEGORY_STATIC (vader_debug);
#define GST_CAT_DEFAULT vader_debug

#define VADER_DEFAULT_THRESHOLD_LEVEL    0.05
#define VADER_DEFAULT_THRESHOLD_LENGTH  (500 * GST_MSECOND)
#define VADER_DEFAULT_PRE_LENGTH        (200 * GST_MSECOND)

static const GstElementDetails vader_details =
GST_ELEMENT_DETAILS ("VAD element",
    "Filter/Editor/Audio",
    "Voice Activity DEtectoR to split audio into non-silent bits",
    "Thomas <thomas@apestaart.org>, David Huggins-Daines <dhuggins@cs.cmu.edu>");

static GstStaticPadTemplate vader_src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw-int, "
        "rate = (int) [ 1, MAX ], "
        "channels = (int) [ 1, MAX ], "
        "endianness = (int) BYTE_ORDER, "
        "width = (int) { 8, 16 }, "
        "depth = (int) { 8, 16 }, " "signed = (boolean) true")
    );

static GstStaticPadTemplate vader_sink_factory =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw-int, "
        "rate = (int) [ 1, MAX ], "
        "channels = (int) [ 1, MAX ], "
        "endianness = (int) BYTE_ORDER, "
        "width = (int) { 8, 16 }, "
        "depth = (int) { 8, 16 }, " "signed = (boolean) true")
    );

enum
{
  PROP_0,
  PROP_THRESHOLD,
  PROP_THRESHOLD_DB,
  PROP_RUN_LENGTH,
  PROP_PRE_LENGTH,
  PROP_LEAKY
};

GST_BOILERPLATE (GstVader, gst_vader, GstElement, GST_TYPE_ELEMENT);

static void gst_vader_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_vader_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static GstFlowReturn gst_vader_chain (GstPad * pad, GstBuffer * buffer);

void gst_vader_get_caps (GstPad * pad, GstVader * filter);

static void
gst_vader_base_init (gpointer g_class)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (g_class);

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&vader_src_factory));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&vader_sink_factory));
  gst_element_class_set_details (element_class, &vader_details);
}

static void
gst_vader_class_init (GstVaderClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_vader_set_property;
  gobject_class->get_property = gst_vader_get_property;

  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_THRESHOLD,
      g_param_spec_double ("threshold", "Threshold",
          "Volume threshold before trigger",
          -G_MAXDOUBLE, G_MAXDOUBLE, 0.0, G_PARAM_READWRITE));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_THRESHOLD_DB,
      g_param_spec_double ("threshold-dB", "Threshold (dB)",
          "Volume threshold before trigger (in dB)",
          -G_MAXDOUBLE, G_MAXDOUBLE, 0.0, G_PARAM_READWRITE));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_RUN_LENGTH,
      g_param_spec_uint64 ("run-length", "Run length",
          "Length of drop below threshold before cut_stop (in nanoseconds)",
          0, G_MAXUINT64, 0, G_PARAM_READWRITE));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_PRE_LENGTH,
      g_param_spec_uint64 ("pre-length", "Pre-recording buffer length",
          "Length of pre-recording buffer (in nanoseconds)",
          0, G_MAXUINT64, 0, G_PARAM_READWRITE));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_LEAKY,
      g_param_spec_boolean ("leaky", "Leaky",
          "do we leak buffers when below threshold ?",
          0, G_PARAM_READWRITE));

  GST_DEBUG_CATEGORY_INIT (vader_debug, "vader", 0, "Audio cutting");
}

static void
gst_vader_init (GstVader * filter, GstVaderClass * g_class)
{
  filter->sinkpad =
      gst_pad_new_from_static_template (&vader_sink_factory, "sink");
  filter->srcpad =
      gst_pad_new_from_static_template (&vader_src_factory, "src");

  filter->threshold_level = VADER_DEFAULT_THRESHOLD_LEVEL;
  filter->threshold_length = VADER_DEFAULT_THRESHOLD_LENGTH;
  filter->silent_run_length = 0 * GST_SECOND;
  filter->silent = TRUE;
  filter->silent_prev = FALSE;  /* previous value of silent */

  filter->pre_length = VADER_DEFAULT_PRE_LENGTH;
  filter->pre_run_length = 0 * GST_SECOND;
  filter->pre_buffer = NULL;
  filter->leaky = TRUE;

  gst_element_add_pad (GST_ELEMENT (filter), filter->sinkpad);
  gst_pad_set_chain_function (filter->sinkpad, gst_vader_chain);
  gst_pad_use_fixed_caps (filter->sinkpad);

  gst_element_add_pad (GST_ELEMENT (filter), filter->srcpad);
  gst_pad_use_fixed_caps (filter->srcpad);
}

static GstMessage *
gst_vader_message_new (GstVader * c, gboolean above, GstClockTime timestamp)
{
  GstStructure *s;

  s = gst_structure_new ("vader",
      "above", G_TYPE_BOOLEAN, above,
      "timestamp", GST_TYPE_CLOCK_TIME, timestamp, NULL);

  return gst_message_new_element (GST_OBJECT (c), s);
}

static GstEvent *
gst_vader_event_new (GstVader *c, GstEventType type, GstClockTime timestamp)
{
  GstEvent *e;

  e = gst_event_new_custom(type, NULL);
  GST_EVENT_TIMESTAMP (e) = timestamp;

  return e;
}

/* Calculate the Normalized Cumulative Square over a buffer of the given type
 * and over all channels combined */

#define DEFINE_VADER_CALCULATOR(TYPE, RESOLUTION)                            \
static void inline                                                            \
gst_vader_calculate_##TYPE (TYPE * in, guint num,                            \
                            double *NCS)                                      \
{                                                                             \
  register int j;                                                             \
  double squaresum = 0.0;           /* square sum of the integer samples */   \
  register double square = 0.0;     /* Square */                              \
  gdouble normalizer;               /* divisor to get a [-1.0, 1.0] range */  \
                                                                              \
  *NCS = 0.0;                       /* Normalized Cumulative Square */        \
                                                                              \
  normalizer = (double) (1 << (RESOLUTION * 2));                              \
                                                                              \
  for (j = 0; j < num; j++)                                                   \
  {                                                                           \
    square = ((double) in[j]) * in[j];                                        \
    squaresum += square;                                                      \
  }                                                                           \
                                                                              \
                                                                              \
  *NCS = squaresum / normalizer;                                              \
}

DEFINE_VADER_CALCULATOR (gint16, 15);
DEFINE_VADER_CALCULATOR (gint8, 7);


static GstFlowReturn
gst_vader_chain (GstPad * pad, GstBuffer * buf)
{
  GstVader *filter;
  gint16 *in_data;
  guint num_samples;
  gdouble NCS = 0.0;            /* Normalized Cumulative Square of buffer */
  gdouble RMS = 0.0;            /* RMS of signal in buffer */
  gdouble NMS = 0.0;            /* Normalized Mean Square of buffer */
  GstBuffer *prebuf;            /* pointer to a prebuffer element */

  g_return_val_if_fail (pad != NULL, GST_FLOW_ERROR);
  g_return_val_if_fail (GST_IS_PAD (pad), GST_FLOW_ERROR);
  g_return_val_if_fail (buf != NULL, GST_FLOW_ERROR);

  filter = GST_VADER (GST_OBJECT_PARENT (pad));
  g_return_val_if_fail (filter != NULL, GST_FLOW_ERROR);
  g_return_val_if_fail (GST_IS_VADER (filter), GST_FLOW_ERROR);

  if (!filter->have_caps)
    gst_vader_get_caps (pad, filter);

  in_data = (gint16 *) GST_BUFFER_DATA (buf);
#if 0
  GST_LOG_OBJECT (filter, "length of prerec buffer: %" GST_TIME_FORMAT,
      GST_TIME_ARGS (filter->pre_run_length));
#endif

  /* calculate mean square value on buffer */
  switch (filter->width) {
    case 16:
      num_samples = GST_BUFFER_SIZE (buf) / 2;
      gst_vader_calculate_gint16 (in_data, num_samples, &NCS);
      NMS = NCS / num_samples;
      break;
    case 8:
      num_samples = GST_BUFFER_SIZE (buf);
      gst_vader_calculate_gint8 ((gint8 *) in_data, num_samples, &NCS);
      NMS = NCS / num_samples;
      break;
    default:
      /* this shouldn't happen */
      g_warning ("no mean square function for width %d\n", filter->width);
      break;
  }

  filter->silent_prev = filter->silent;

  RMS = sqrt (NMS);
  /* if RMS below threshold, add buffer length to silent run length count
   * if not, reset
   */
#if 0
  GST_LOG_OBJECT (filter, "buffer stats: NMS %f, RMS %f, audio length %f", NMS,
      RMS,
      gst_guint64_to_gdouble (gst_audio_duration_from_pad_buffer (filter->
              sinkpad, buf)));
#endif
  if (RMS < filter->threshold_level)
    filter->silent_run_length +=
        gst_guint64_to_gdouble (gst_audio_duration_from_pad_buffer (filter->
            sinkpad, buf));
  else {
    filter->silent_run_length = 0 * GST_SECOND;
    filter->silent = FALSE;
  }

  if (filter->silent_run_length > filter->threshold_length)
    /* it has been silent long enough, flag it */
    filter->silent = TRUE;

  /* has the silent status changed ? if so, send right signal
   * and, if from silent -> not silent, flush pre_record buffer
   */
  if (filter->silent != filter->silent_prev) {
    if (filter->silent) {
      /* Sound to silence transition. */
      GstMessage *m =
          gst_vader_message_new (filter, FALSE, GST_BUFFER_TIMESTAMP (buf));
      GstEvent *e =
	gst_vader_event_new (filter, GST_EVENT_VADER_STOP, GST_BUFFER_TIMESTAMP(buf));
      GST_DEBUG_OBJECT (filter, "signaling CUT_STOP");
      gst_element_post_message (GST_ELEMENT (filter), m);
      /* Insert a custom event in the stream to mark the end of a cut. */
      gst_pad_push_event (filter->srcpad, e);
    } else {
      /* Silence to sound transition. */
      gint count = 0;
      GstMessage *m =
          gst_vader_message_new (filter, TRUE, GST_BUFFER_TIMESTAMP (buf));
      GstEvent *e =
	      gst_vader_event_new (filter, GST_EVENT_VADER_START, GST_BUFFER_TIMESTAMP(buf));

      GST_DEBUG_OBJECT (filter, "signaling CUT_START");
      gst_element_post_message (GST_ELEMENT (filter), m);
      /* Insert a custom event in the stream to mark the beginning of a cut. */
      gst_pad_push_event (filter->srcpad, e);

      /* first of all, flush current buffer */
      GST_DEBUG_OBJECT (filter, "flushing buffer of length %" GST_TIME_FORMAT,
          GST_TIME_ARGS (filter->pre_run_length));
      while (filter->pre_buffer) {
        prebuf = (g_list_first (filter->pre_buffer))->data;
        filter->pre_buffer = g_list_remove (filter->pre_buffer, prebuf);
        gst_pad_push (filter->srcpad, prebuf);
        ++count;
      }
      GST_DEBUG_OBJECT (filter, "flushed %d buffers", count);
      filter->pre_run_length = 0 * GST_SECOND;
    }
  }
  /* now check if we have to send the new buffer to the internal buffer cache
   * or to the srcpad */
  if (filter->silent) {
    filter->pre_buffer = g_list_append (filter->pre_buffer, buf);
    filter->pre_run_length +=
        gst_guint64_to_gdouble (gst_audio_duration_from_pad_buffer (filter->
            sinkpad, buf));
    while (filter->pre_run_length > filter->pre_length) {
      prebuf = (g_list_first (filter->pre_buffer))->data;
      g_assert (GST_IS_BUFFER (prebuf));
      filter->pre_buffer = g_list_remove (filter->pre_buffer, prebuf);
      filter->pre_run_length -=
          gst_guint64_to_gdouble (gst_audio_duration_from_pad_buffer (filter->
              sinkpad, prebuf));
      /* only pass buffers if we don't leak */
      if (!filter->leaky)
        gst_pad_push (filter->srcpad, prebuf);
      else
        gst_buffer_unref (prebuf);
    }
  } else {
    gst_pad_push (filter->srcpad, buf);
  }

  return GST_FLOW_OK;
}

static void
gst_vader_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstVader *filter;

  g_return_if_fail (GST_IS_VADER (object));
  filter = GST_VADER (object);

  switch (prop_id) {
    case PROP_THRESHOLD:
      filter->threshold_level = g_value_get_double (value);
      GST_DEBUG ("DEBUG: set threshold level to %f", filter->threshold_level);
      break;
    case PROP_THRESHOLD_DB:
      /* set the level given in dB
       * value in dB = 20 * log (value)
       * values in dB < 0 result in values between 0 and 1
       */
      filter->threshold_level = pow (10, g_value_get_double (value) / 20);
      GST_DEBUG_OBJECT (filter, "set threshold level to %f",
          filter->threshold_level);
      break;
    case PROP_RUN_LENGTH:
      /* set the minimum length of the silent run required */
      filter->threshold_length =
          gst_guint64_to_gdouble (g_value_get_uint64 (value));
      break;
    case PROP_PRE_LENGTH:
      /* set the length of the pre-record block */
      filter->pre_length = gst_guint64_to_gdouble (g_value_get_uint64 (value));
      break;
    case PROP_LEAKY:
      /* set if the pre-record buffer is leaky or not */
      filter->leaky = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_vader_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstVader *filter;

  g_return_if_fail (GST_IS_VADER (object));
  filter = GST_VADER (object);

  switch (prop_id) {
    case PROP_RUN_LENGTH:
      g_value_set_uint64 (value, filter->threshold_length);
      break;
    case PROP_THRESHOLD:
      g_value_set_double (value, filter->threshold_level);
      break;
    case PROP_THRESHOLD_DB:
      g_value_set_double (value, 20 * log (filter->threshold_level));
      break;
    case PROP_PRE_LENGTH:
      g_value_set_uint64 (value, filter->pre_length);
      break;
    case PROP_LEAKY:
      g_value_set_boolean (value, filter->leaky);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

void
gst_vader_get_caps (GstPad * pad, GstVader * filter)
{
  GstCaps *caps;
  GstStructure *structure;

  caps = gst_pad_get_caps (pad);
  /* FIXME : Please change this to a better warning method ! */
  g_assert (caps != NULL);
  structure = gst_caps_get_structure (caps, 0);
  gst_structure_get_int (structure, "width", &filter->width);
  filter->max_sample = 1 << (filter->width - 1);        /* signed */
  filter->have_caps = TRUE;

  gst_caps_unref (caps);
}
