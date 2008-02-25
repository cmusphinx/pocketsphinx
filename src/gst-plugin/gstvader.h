/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
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


#ifndef __GST_VADER_H__
#define __GST_VADER_H__


#include <gst/gst.h>
#include <gst/gstevent.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define GST_TYPE_VADER \
  (gst_vader_get_type())
#define GST_VADER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_VADER,GstVader))
#define GST_VADER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_VADER,GstVaderClass))
#define GST_IS_VADER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_VADER))
#define GST_IS_VADER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_VADER))

/* Custom events inserted in the stream at start and stop of cuts. */
#define GST_EVENT_VADER_START \
	GST_EVENT_MAKE_TYPE(146, GST_EVENT_TYPE_DOWNSTREAM | GST_EVENT_TYPE_SERIALIZED)
#define GST_EVENT_VADER_STOP \
	GST_EVENT_MAKE_TYPE(147, GST_EVENT_TYPE_DOWNSTREAM | GST_EVENT_TYPE_SERIALIZED)

typedef struct _GstVader GstVader;
typedef struct _GstVaderClass GstVaderClass;

struct _GstVader
{
  GstElement element;

  GstPad *sinkpad, *srcpad;

  double threshold_level;       /* level below which to cut */
  double threshold_length;      /* how long signal has to remain
                                 * below this level before cutting */

  double silent_run_length;     /* how long has it been below threshold ? */
  gboolean silent;
  gboolean silent_prev;

  double pre_length;            /* how long can the pre-record buffer be ? */
  double pre_run_length;        /* how long is it currently ? */
  GList *pre_buffer;            /* list of GstBuffers in pre-record buffer */
  gboolean leaky;               /* do we leak an overflowing prebuffer ? */

  gboolean have_caps;           /* did we get the needed caps yet ? */
  gint width;                   /* bit width of data */
  long max_sample;              /* maximum sample value */
};

struct _GstVaderClass
{
  GstElementClass parent_class;
  void (*cut_start) (GstVader* filter);
  void (*cut_stop) (GstVader* filter);
};

GType gst_vader_get_type (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GST_STEREO_H__ */
