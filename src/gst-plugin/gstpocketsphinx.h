/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* Based on gstsphinxsink.h:
 *
 * Copyright (C) 2007  Nickolay V. Shmyrev  <nshmyrev@yandex.ru>
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
 * GStreamer
 * Copyright 2005 Thomas Vander Stichele <thomas@apestaart.org>
 * Copyright 2005 Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Alternatively, the contents of this file may be used under the
 * GNU Lesser General Public License Version 2.1 (the "LGPL"), in
 * which case the following provisions apply instead of the ones
 * mentioned above:
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

#ifndef __GST_POCKETSPHINX_H__
#define __GST_POCKETSPHINX_H__

#include <gst/gst.h>
#include <gst/base/gstbasesink.h>
#include <gst/base/gstadapter.h>

#include <sphinx_config.h>
#include <fe.h>
#include <fbs.h>
#include <cont_ad.h>

G_BEGIN_DECLS

/* #defines don't like whitespacey bits */
#define GST_TYPE_POCKETSPHINX                   \
    (gst_pocketsphinx_get_type())
#define GST_POCKETSPHINX(obj)                                           \
    (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_POCKETSPHINX,GstPocketSphinx))
#define GST_POCKETSPHINX_CLASS(klass)                                   \
    (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_POCKETSPHINX,GstPocketSphinxClass))
#define GST_IS_POCKETSPHINX(obj)                                \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_POCKETSPHINX))
#define GST_IS_POCKETSPHINX_CLASS(klass)                        \
    (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_POCKETSPHINX))

typedef struct _GstPocketSphinx      GstPocketSphinx;
typedef struct _GstPocketSphinxClass GstPocketSphinxClass;

typedef struct _GstSphinxSinkAd {
    GstPocketSphinx *self;
    int32 dummy;
    int32 sps;
    int32 bps;
    int32 calibrated;
    int32 initialized;
    int32 calibrate_started;
    int32 listening;
} GstSphinxSinkAd;

struct _GstPocketSphinx
{
    GstBaseSink basesink;
    cont_ad_t *cont;
    GstSphinxSinkAd ad;
    gint32 last_ts;
  
    GstAdapter *adapter;  

    /* Hash table to store copies of property strings */
    GHashTable *arghash;
};

struct _GstPocketSphinxClass 
{
    GstBaseSinkClass parent_class;

    /* signals */
    void (*initialization)  (GstElement *element);
    void (*after_initialization)  (GstElement *element);
    void (*calibration)     (GstElement *element);
    void (*ready)           (GstElement *element);
    void (*listening)       (GstElement *element);
    void (*partial_result)  (GstElement *element, const gchar *hyp_str);
    void (*result)          (GstElement *element, const gchar *hyp_str);
};

GType gst_pocketsphinx_get_type (void);

G_END_DECLS

#endif /* __GST_POCKETSPHINX_H__ */
