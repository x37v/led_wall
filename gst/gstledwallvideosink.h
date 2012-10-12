/* GStreamer
 * Copyright (C) 2012 FIXME <fixme@example.com>
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

#ifndef _GST_LED_WALL_VIDEO_SINK_H_
#define _GST_LED_WALL_VIDEO_SINK_H_

#include <gst/video/gstvideosink.h>

G_BEGIN_DECLS

#define GST_TYPE_LED_WALL_VIDEO_SINK   (gst_led_wall_video_sink_get_type())
#define GST_LED_WALL_VIDEO_SINK(obj)   (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_LED_WALL_VIDEO_SINK,GstLedWallVideoSink))
#define GST_LED_WALL_VIDEO_SINK_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_LED_WALL_VIDEO_SINK,GstLedWallVideoSinkClass))
#define GST_IS_LED_WALL_VIDEO_SINK(obj)   (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_LED_WALL_VIDEO_SINK))
#define GST_IS_LED_WALL_VIDEO_SINK_CLASS(obj)   (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_LED_WALL_VIDEO_SINK))

typedef struct _GstLedWallVideoSink GstLedWallVideoSink;
typedef struct _GstLedWallVideoSinkClass GstLedWallVideoSinkClass;

struct _GstLedWallVideoSink
{
  GstVideoSink base_ledwallvideosink;

  GstPad *sinkpad;
  guint8 *led_buffer;

  gchar *device;

  gint requested_rows;
  gint requested_cols;

  gint rows;
  gint cols;
};

struct _GstLedWallVideoSinkClass
{
  GstVideoSinkClass base_ledwallvideosink_class;
};

GType gst_led_wall_video_sink_get_type (void);

G_END_DECLS

#endif
