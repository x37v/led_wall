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
 * Free Software Foundation, Inc., 51 Franklin Street, Suite 500,
 * Boston, MA 02110-1335, USA.
 */
/**
 * SECTION:element-gstledwallvideosink
 *
 * The ledwallvideosink element outputs RGB frames a USB connected
 * LED video wall.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch --gst-plugin-path=. -v videotestsrc ! ledwallvideosink
 * ]|
 * FIXME Describe what the pipeline does.
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

//#define TO_PGM

#include <string.h>
#include <gst/gst.h>
#include <gst/video/gstvideosink.h>
#include <gst/video/video.h>
#include "gstledwallvideosink.h"
#include "color.h"
#include "led_driver.h"

GST_DEBUG_CATEGORY_STATIC (gst_led_wall_video_sink_debug_category);
#define GST_CAT_DEFAULT gst_led_wall_video_sink_debug_category

#define DEFAULT_DEVICE "/dev/ttyACM0"
#define DEFAULT_ROWS 64
#define DEFAULT_COLS (8 * 12)

enum {
  PROP_0,
  PROP_COLUMNS,
  PROP_ROWS,
  PROP_DEVICE
};

/* prototypes */
static void gst_led_wall_video_sink_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_led_wall_video_sink_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);

static void gst_led_wall_video_sink_dispose (GObject * object);
static void gst_led_wall_video_sink_finalize (GObject * object);
static gboolean gst_led_wall_video_sink_setcaps(GstBaseSink * bsink, GstCaps * caps);
static GstCaps * gst_led_wall_video_sink_getcaps(GstBaseSink * bsink);
static GstStateChangeReturn gst_led_wall_video_sink_change_state (GstElement * element,
    GstStateChange transition);

static GstFlowReturn gst_led_wall_video_sink_show_frame (GstBaseSink * bsink, GstBuffer * buf);

static inline
void map_to_image(GstLedWallVideoSink *sink, int ledcol, int ledrow, int *videocol, int *videorow, int video_width, int video_height) {
  *videorow = (ledrow * video_height) / sink->rows;
  *videocol = (ledcol * video_width) / sink->cols;

  //removed remapping aspect ratio
  //*videorow = (ledrow * (video_height - ((8 * height) / led_rows))) / led_rows + (height * 4) / led_rows;
  //*videocol = (ledcol * (video_width - ((10 * video_width) / led_columns))) / led_columns + (5 * video_width) / led_columns;

  if (*videorow < 0)
    *videorow = 0;
  else if (*videorow >= video_height)
    *videorow = video_height - 1;

  if (*videocol < 0)
    *videocol = 0;
  else if (*videocol >= video_width)
    *videocol = video_width - 1;
}

/* pad templates */

static GstStaticPadTemplate gst_led_wall_video_sink_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (
      "video/x-raw-rgb, "
        "framerate = (fraction) [ 0, MAX ], "
        "width = (int) 96, "
        "height = (int) 64; "
        //should we have these enabled below?
        //bpp=(int)32, depth=(int)24, endianness=(int)4321, red_mask=(int)65280, green_mask=(int)16711680, blue_mask=(int)-16777216
        //unsure about framerate based on USB capabilities
        //framerate=(fraction)[ 0/1, 2147483647/1 ]
      )
    );


/* class initialization */

#define DEBUG_INIT(bla) \
  GST_DEBUG_CATEGORY_INIT (gst_led_wall_video_sink_debug_category, "ledwallvideosink", 0, \
      "debug category for ledwallvideosink element");

GST_BOILERPLATE_FULL (GstLedWallVideoSink, gst_led_wall_video_sink,
    GstVideoSink, GST_TYPE_VIDEO_SINK, DEBUG_INIT);

static void
gst_led_wall_video_sink_base_init (gpointer g_class)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (g_class);

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_led_wall_video_sink_sink_template));

  gst_element_class_set_details_simple(element_class,
    "GstLedWallVideoSink",
    "Sink/Video/Device",
    "Sink for writing video to the LED wall",
    "Alex Norman <alex@x37v.info>");
/*
  gst_element_class_set_metadata (element_class, "GstLedWallVideoSink",
      "Sink", "Sink for writing video to the LED wall", "Alex Norman <alex@x37v.info>");
*/
}

static void
gst_led_wall_video_sink_class_init (GstLedWallVideoSinkClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  //GstVideoSinkClass *video_sink_class = GST_VIDEO_SINK_CLASS (klass);
  GstBaseSinkClass *gstbasesink_class = (GstBaseSinkClass *) klass;
  GstElementClass *gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_led_wall_video_sink_set_property;
  gobject_class->get_property = gst_led_wall_video_sink_get_property;

  gobject_class->dispose = gst_led_wall_video_sink_dispose;
  gobject_class->finalize = gst_led_wall_video_sink_finalize;

  gstbasesink_class->render = GST_DEBUG_FUNCPTR (gst_led_wall_video_sink_show_frame);
  gstbasesink_class->preroll = GST_DEBUG_FUNCPTR (gst_led_wall_video_sink_show_frame);

  gstbasesink_class->get_caps = GST_DEBUG_FUNCPTR (gst_led_wall_video_sink_getcaps);
  gstbasesink_class->set_caps = GST_DEBUG_FUNCPTR (gst_led_wall_video_sink_setcaps);

  gstelement_class->change_state =
      GST_DEBUG_FUNCPTR (gst_led_wall_video_sink_change_state);

  g_object_class_install_property (gobject_class, PROP_DEVICE,
      g_param_spec_string ("device", "Output device",
          "Location of the serial device for output", DEFAULT_DEVICE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_COLUMNS,
      g_param_spec_int ("columns", "LED columns",
          "Number of pixel columns to output", 0, G_MAXINT,
          DEFAULT_COLS, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_ROWS,
      g_param_spec_int ("rows", "LED rows",
          "Number of pixel rows to output", 0, G_MAXINT,
          DEFAULT_ROWS, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

}

static void
gst_led_wall_video_sink_init (GstLedWallVideoSink * ledwallvideosink,
    GstLedWallVideoSinkClass * ledwallvideosink_class G_GNUC_UNUSED)
{

  ledwallvideosink->sinkpad =
      gst_pad_new_from_static_template (&gst_led_wall_video_sink_sink_template,
      "sink");

  ledwallvideosink->device = g_strdup (DEFAULT_DEVICE);
  ledwallvideosink->requested_rows = ledwallvideosink->rows = DEFAULT_ROWS;
  ledwallvideosink->requested_cols = ledwallvideosink->cols = DEFAULT_COLS;
}

void
gst_led_wall_video_sink_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GstLedWallVideoSink *sink = GST_LED_WALL_VIDEO_SINK (object);

  switch (property_id) {
    case PROP_DEVICE:
      g_free (sink->device);
      sink->device = g_value_dup_string (value);
      break;
    case PROP_ROWS:
      sink->requested_rows = g_value_get_int (value);
      break;
    case PROP_COLUMNS:
      sink->requested_cols = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_led_wall_video_sink_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GstLedWallVideoSink *sink = GST_LED_WALL_VIDEO_SINK (object);

  switch (property_id) {
    case PROP_DEVICE:
      g_value_set_string (value, sink->device);
      break;
    case PROP_ROWS:
      g_value_set_int (value, sink->requested_rows);
      break;
    case PROP_COLUMNS:
      g_value_set_int (value, sink->requested_cols);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_led_wall_video_sink_dispose (GObject * object)
{
  /* GstLedWallVideoSink *ledwallvideosink = GST_LED_WALL_VIDEO_SINK (object); */
  /* clean up as possible.  may be called multiple times */
  G_OBJECT_CLASS (parent_class)->dispose (object);
}

void
gst_led_wall_video_sink_finalize (GObject * object)
{
  GstLedWallVideoSink *sink = GST_LED_WALL_VIDEO_SINK (object);
  g_free (sink->device);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


static GstFlowReturn gst_led_wall_video_sink_show_frame(GstBaseSink * bsink, GstBuffer * buf)
{
  GstLedWallVideoSink *sink = GST_LED_WALL_VIDEO_SINK (bsink);
  //guint8 * data = GST_BUFFER_DATA (buf);
  unsigned int width = GST_VIDEO_SINK_WIDTH(sink);
  unsigned int height = GST_VIDEO_SINK_HEIGHT(sink);
  int num_leds = sink->rows * sink->cols;
  int led_rows = sink->rows;

  //printf("width %d height %d size: %d\n", width, height, GST_BUFFER_SIZE(buf));
  //printf("%d\n", data[0]);
  
#ifdef TO_PGM
  //pgm
  printf("P2\n%d %d\n255\n", width, height);
  for (unsigned int row = 0; row < height; row++) {
    for (unsigned int col = 0; col < width; col++) {
      guint8 intensity = data[row * 2 * width + col * 2];
      printf("%d ", (int)intensity);
    }
  }
  printf("\n");
#else
  for (int i = 0; i < num_leds; i++) {
    int row, col;
    int r,g,b;

    //figure out our led row/column
    col = i / led_rows;
    if (i % 128 >= led_rows)
      row = i % led_rows;
    else
      row = 63 - (i % led_rows);

    //find the location row+column in the image
    map_to_image(sink, col, row, &col, &row, width, height);

#if 0
    int y, cb, cr;
    int y_off;
    //figure out the y, cr, cb
    y_off = 2 * col + (2 * row*width);
    y = data[y_off];

    if (col % 2 == 0) {
      cb = data[y_off + 1];
      cr = data[y_off + 3];
    } else {
      cb = data[y_off - 1];
      cr = data[y_off + 1];
    }

    //convert to rgb
    ycrcb2rgb(y, cb, cr, &r, &g, &b);
#else
    //direct rgb
    guint32 * rgba = (guint32 *)GST_BUFFER_DATA (buf);
    guint32 pixel = rgba[col + row * width];
    r = (pixel >> 16) & 0xFF;
    g = (pixel >>  8) & 0xFF;
    b = (pixel >>  0) & 0xFF;
    //r = data[y_off + 1];
    //g = data[y_off + 2];
    //b = data[y_off + 0];
#endif

    //the colors are weird in the led output
    sink->led_buffer[0 + i * 3] = gamma_map(g);
    sink->led_buffer[1 + i * 3] = gamma_map(r);
    sink->led_buffer[2 + i * 3] = gamma_map(b);
  }
  led_write_buffer(sink->led_buffer);
#endif

  return GST_FLOW_OK;
}

static GstCaps *
gst_led_wall_video_sink_getcaps (GstBaseSink * bsink) {
  GstLedWallVideoSink *ledwallvideosink = GST_LED_WALL_VIDEO_SINK (bsink);

  /*
  if (ledwallvideosink->xcontext)
    return gst_caps_ref (xvimagesink->xcontext->caps);
    */

//#ifndef TO_PGM
//  printf("get caps\n");
//#endif

  return gst_caps_copy (gst_pad_get_pad_template_caps (GST_VIDEO_SINK_PAD (ledwallvideosink)));
}

static gboolean
gst_led_wall_video_sink_setcaps (GstBaseSink * bsink, GstCaps * caps)
{
  GstLedWallVideoSink *ledwallvideosink = GST_LED_WALL_VIDEO_SINK (bsink);
  GstStructure *structure = gst_caps_get_structure (caps, 0);
  gboolean ret;

  gint width, height;
  ret = gst_structure_get_int (structure, "width", &width);
  ret &= gst_structure_get_int (structure, "height", &height);
//#ifndef TO_PGM
//  printf("width %d height %d\n", width, height);
//#endif

  GST_VIDEO_SINK_WIDTH(ledwallvideosink) = width;
  GST_VIDEO_SINK_HEIGHT(ledwallvideosink) = height;

  //fps = gst_structure_get_value (structure, "framerate");

#if 0
  GstXvImageSink *xvimagesink;
  GstStructure *structure;
  GstCaps *intersection;
  guint32 im_format = 0;
  gboolean ret;
  gint video_width, video_height;
  gint video_par_n, video_par_d;        /* video's PAR */
  gint display_par_n, display_par_d;    /* display's PAR */
  GValue display_ratio = { 0, };        /* display w/h ratio */
  const GValue *caps_par;
  const GValue *fps;
  gint num, den;

  xvimagesink = GST_XVIMAGESINK (bsink);

  GST_DEBUG_OBJECT (xvimagesink,
      "In setcaps. Possible caps %" GST_PTR_FORMAT ", setting caps %"
      GST_PTR_FORMAT, xvimagesink->xcontext->caps, caps);

  intersection = gst_caps_intersect (xvimagesink->xcontext->caps, caps);
  GST_DEBUG_OBJECT (xvimagesink, "intersection returned %" GST_PTR_FORMAT,
      intersection);
  if (gst_caps_is_empty (intersection)) {
    return FALSE;
  }

  gst_caps_unref (intersection);

  structure = gst_caps_get_structure (caps, 0);
  ret = gst_structure_get_int (structure, "width", &video_width);
  ret &= gst_structure_get_int (structure, "height", &video_height);
  fps = gst_structure_get_value (structure, "framerate");
  ret &= (fps != NULL);

  if (!ret)
    return FALSE;

  xvimagesink->fps_n = gst_value_get_fraction_numerator (fps);
  xvimagesink->fps_d = gst_value_get_fraction_denominator (fps);

  xvimagesink->video_width = video_width;
  xvimagesink->video_height = video_height;
  im_format = gst_xvimagesink_get_format_from_caps (xvimagesink, caps);
  if (im_format == 0) {
    return FALSE;
  }

  /* get aspect ratio from caps if it's present, and
   * convert video width and height to a display width and height
   * using wd / hd = wv / hv * PARv / PARd
   * the ratio wd / hd will be stored in display_ratio */
  g_value_init (&display_ratio, GST_TYPE_FRACTION);

  /* get video's PAR */
  caps_par = gst_structure_get_value (structure, "pixel-aspect-ratio");
  if (caps_par) {
    video_par_n = gst_value_get_fraction_numerator (caps_par);
    video_par_d = gst_value_get_fraction_denominator (caps_par);
  } else {
    video_par_n = 1;
    video_par_d = 1;
  }
  /* get display's PAR */
  if (xvimagesink->par) {
    display_par_n = gst_value_get_fraction_numerator (xvimagesink->par);
    display_par_d = gst_value_get_fraction_denominator (xvimagesink->par);
  } else {
    display_par_n = 1;
    display_par_d = 1;
  }

  gst_value_set_fraction (&display_ratio,
      video_width * video_par_n * display_par_d,
      video_height * video_par_d * display_par_n);

  num = gst_value_get_fraction_numerator (&display_ratio);
  den = gst_value_get_fraction_denominator (&display_ratio);
  GST_DEBUG_OBJECT (xvimagesink,
      "video width/height: %dx%d, calculated display ratio: %d/%d",
      video_width, video_height, num, den);

  /* now find a width x height that respects this display ratio.
   * prefer those that have one of w/h the same as the incoming video
   * using wd / hd = num / den */

  /* start with same height, because of interlaced video */
  /* check hd / den is an integer scale factor, and scale wd with the PAR */
  if (video_height % den == 0) {
    GST_DEBUG_OBJECT (xvimagesink, "keeping video height");
    GST_VIDEO_SINK_WIDTH (xvimagesink) = video_height * num / den;
    GST_VIDEO_SINK_HEIGHT (xvimagesink) = video_height;
  } else if (video_width % num == 0) {
    GST_DEBUG_OBJECT (xvimagesink, "keeping video width");
    GST_VIDEO_SINK_WIDTH (xvimagesink) = video_width;
    GST_VIDEO_SINK_HEIGHT (xvimagesink) = video_width * den / num;
  } else {
    GST_DEBUG_OBJECT (xvimagesink, "approximating while keeping video height");
    GST_VIDEO_SINK_WIDTH (xvimagesink) = video_height * num / den;
    GST_VIDEO_SINK_HEIGHT (xvimagesink) = video_height;
  }
  GST_DEBUG_OBJECT (xvimagesink, "scaling to %dx%d",
      GST_VIDEO_SINK_WIDTH (xvimagesink), GST_VIDEO_SINK_HEIGHT (xvimagesink));

  /* Notify application to set xwindow id now */
  if (!xvimagesink->xwindow) {
    gst_x_overlay_prepare_xwindow_id (GST_X_OVERLAY (xvimagesink));
  }

  /* Creating our window and our image with the display size in pixels */
  g_assert (GST_VIDEO_SINK_WIDTH (xvimagesink) > 0);
  g_assert (GST_VIDEO_SINK_HEIGHT (xvimagesink) > 0);
  if (!xvimagesink->xwindow)
    xvimagesink->xwindow = gst_xvimagesink_xwindow_new (xvimagesink,
        GST_VIDEO_SINK_WIDTH (xvimagesink),
        GST_VIDEO_SINK_HEIGHT (xvimagesink));

  /* We renew our xvimage only if size or format changed;
   * the xvimage is the same size as the video pixel size */
  if ((xvimagesink->xvimage) &&
      ((im_format != xvimagesink->xvimage->im_format) ||
          (video_width != xvimagesink->xvimage->width) ||
          (video_height != xvimagesink->xvimage->height))) {
    GST_DEBUG_OBJECT (xvimagesink,
        "old format %" GST_FOURCC_FORMAT ", new format %" GST_FOURCC_FORMAT,
        GST_FOURCC_ARGS (xvimagesink->xcontext->im_format),
        GST_FOURCC_ARGS (im_format));
    GST_DEBUG_OBJECT (xvimagesink, "renewing xvimage");
    gst_buffer_unref (GST_BUFFER (xvimagesink->xvimage));
    xvimagesink->xvimage = NULL;
  }

  xvimagesink->xcontext->im_format = im_format;

#endif
  return TRUE;
}

static gboolean
open_port (GstLedWallVideoSink *sink)
{
  int num_leds = sink->rows * sink->cols;
  char *devices[] = { sink->device, "/dev/ttyUSB000", "/dev/ttyACM1", NULL };
  char **device;

  for (device = devices; *device != NULL; device++) {
    if (led_open_output(*device, num_leds)) {
      printf("opened %s\n", *device);
      return TRUE;
    }
    printf("cannot open output %s\n", *device);
  }

  GST_ELEMENT_ERROR (sink, CORE, STATE_CHANGE, (NULL),
      ("Failed to open output device %s", sink->device));
  return FALSE;
}

static GstStateChangeReturn
gst_led_wall_video_sink_change_state (GstElement * element, GstStateChange transition)
{
  GstLedWallVideoSink *sink = GST_LED_WALL_VIDEO_SINK (element);
  GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      sink->rows = sink->requested_rows;
      sink->cols = sink->requested_cols;

      if (!open_port (sink))
        goto error;
      sink->led_buffer = g_malloc0 (3 * sink->cols * sink->rows);
      break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      break;
    case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
      break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      break;
    case GST_STATE_CHANGE_READY_TO_NULL:
      /* clean up object here */
      g_free (sink->led_buffer);
      sink->led_buffer = NULL;

      led_close();
      break;
    default:
      break;
  }

  return ret;
error:
  return GST_STATE_CHANGE_FAILURE;
}

static gboolean
plugin_init (GstPlugin * plugin)
{

  return gst_element_register (plugin, "ledwallvideosink", GST_RANK_NONE,
      GST_TYPE_LED_WALL_VIDEO_SINK);
}

#ifndef VERSION
#define VERSION "0.0.1"
#endif
#ifndef PACKAGE
#define PACKAGE "FIXME_package"
#endif
#ifndef PACKAGE_NAME
#define PACKAGE_NAME "FIXME_led_wall"
#endif
#ifndef GST_PACKAGE_ORIGIN
#define GST_PACKAGE_ORIGIN "http://x37v.info/"
#endif

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "ledwallvideosink",
    "Led wall video sink",
    plugin_init, VERSION, "LGPL", PACKAGE_NAME, GST_PACKAGE_ORIGIN)

