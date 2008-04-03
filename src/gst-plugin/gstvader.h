/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
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
#if 0
}
#endif

#define GST_TYPE_VADER				\
    (gst_vader_get_type())
#define GST_VADER(obj)                                          \
    (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_VADER,GstVader))
#define GST_VADER_CLASS(klass)						\
    (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_VADER,GstVaderClass))
#define GST_IS_VADER(obj)                               \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_VADER))
#define GST_IS_VADER_CLASS(klass)                       \
    (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_VADER))

/* Custom events inserted in the stream at start and stop of cuts. */
#define GST_EVENT_VADER_START						\
    GST_EVENT_MAKE_TYPE(146, GST_EVENT_TYPE_DOWNSTREAM | GST_EVENT_TYPE_SERIALIZED)
#define GST_EVENT_VADER_STOP						\
    GST_EVENT_MAKE_TYPE(147, GST_EVENT_TYPE_DOWNSTREAM | GST_EVENT_TYPE_SERIALIZED)

typedef struct _GstVader GstVader;
typedef struct _GstVaderClass GstVaderClass;

/* Maximum frame size over which VAD is calculated. */
#define VADER_FRAME 512
/* Number of frames over which to vote on speech/non-speech decision. */
#define VADER_WINDOW 5

struct _GstVader
{
    GstElement element;

    GstPad *sinkpad, *srcpad;

    GStaticRecMutex mtx;          /**< Lock used in setting parameters. */

    gboolean window[VADER_WINDOW];/**< Voting window of speech/silence decisions. */
    gboolean silent;		  /**< Current state of the filter. */
    gboolean silent_prev;	  /**< Previous state of the filter. */
    GList *pre_buffer;            /**< list of GstBuffers in pre-record buffer */
    guint64 silent_run_length;    /**< How much silence have we endured so far? */
    guint64 pre_run_length;       /**< How much pre-silence have we endured so far? */

    gint threshold_level;         /**< Silence threshold level (Q15, adaptive). */
    guint64 threshold_length;     /**< Minimum silence for cutting, in nanoseconds. */
    guint64 pre_length;           /**< Pre-buffer to add on silence->speech transition. */

    gboolean auto_threshold;      /**< Set threshold automatically. */
    gint silence_mean;            /**< Mean RMS power of silence frames. */
    gint silence_stddev;          /**< Variance in RMS power of silence frames. */
    gint silence_frames;          /**< Number of frames used in estimating mean/variance */
};

struct _GstVaderClass
{
    GstElementClass parent_class;
    void (*vader_start) (GstVader* filter);
    void (*vader_stop) (GstVader* filter);
};

GType gst_vader_get_type (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GST_STEREO_H__ */
