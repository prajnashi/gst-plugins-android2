/* GStreamer
 * Copyright (C) <2009> Prajnashi S <prajnashi@gmail.com>
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
#include "config.h"
#endif

#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <stdlib.h>

#include <unistd.h>

#include "gstsurfaceflingersink.h"

GST_DEBUG_CATEGORY (gst_surfaceflinger_sink_debug);
#define GST_CAT_DEFAULT gst_surfaceflinger_sink_debug

/* elementfactory information */
static const GstElementDetails gst_surfaceflinger_sink_details =
GST_ELEMENT_DETAILS ("android's surface flinger sink",
    "Sink/Video",
    "A linux framebuffer videosink",
    "Prajnashi S <prajnashi@gmail.com>");

enum
{
  ARG_0,
  PROP_SURFACE,
};

static void gst_surfaceflinger_sink_base_init (gpointer g_class);
static void gst_surfaceflinger_sink_class_init (GstSurfaceFlingerSinkClass * klass);
static void gst_surfaceflinger_sink_get_times (GstBaseSink * basesink,
    GstBuffer * buffer, GstClockTime * start, GstClockTime * end);

static gboolean gst_surfaceflinger_sink_setcaps (GstBaseSink * bsink, GstCaps * caps);

static GstFlowReturn gst_surfaceflinger_sink_render (GstBaseSink * bsink,
    GstBuffer * buff);
static gboolean gst_surfaceflinger_sink_start (GstBaseSink * bsink);
static gboolean gst_surfaceflinger_sink_stop (GstBaseSink * bsink);

static void gst_surfaceflinger_sink_init (GstSurfaceFlingerSink * surfacesink);
static void gst_surfaceflinger_sink_finalize (GObject * object);
static void gst_surfaceflinger_sink_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_surfaceflinger_sink_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec);
static GstStateChangeReturn
gst_surfaceflinger_sink_change_state (GstElement * element, GstStateChange transition);

static GstCaps *gst_surfaceflinger_sink_getcaps (GstBaseSink * bsink);

static GstVideoSinkClass *parent_class = NULL;

/* TODO: support more pixel form in the future */
#define GST_SURFACE_TEMPLATE_CAPS GST_VIDEO_CAPS_RGB_16

static void
gst_surfaceflinger_sink_base_init (gpointer g_class)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (g_class);

  static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE ("sink",
      GST_PAD_SINK,
      GST_PAD_ALWAYS,
      GST_STATIC_CAPS (GST_SURFACE_TEMPLATE_CAPS)
      );

      gst_element_class_set_details (element_class, &gst_surfaceflinger_sink_details);
      gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&sink_template));
}


static void
gst_surfaceflinger_sink_get_times (GstBaseSink * basesink, GstBuffer * buffer,
    GstClockTime * start, GstClockTime * end)
{
    GstSurfaceFlingerSink *surfacesink;

    surfacesink = GST_SURFACEFLINGERSINK (basesink);

    if (GST_BUFFER_TIMESTAMP_IS_VALID (buffer)) 
    {
        *start = GST_BUFFER_TIMESTAMP (buffer);
        if (GST_BUFFER_DURATION_IS_VALID (buffer)) 
        {
            *end = *start + GST_BUFFER_DURATION (buffer);
        } 
        else 
        {
            if (surfacesink->fps_n > 0) 
            {
                *end = *start +
                gst_util_uint64_scale_int (GST_SECOND, surfacesink->fps_d,
                surfacesink->fps_n);
            }
        }
    }
}

static GstCaps *
gst_surfaceflinger_sink_getcaps (GstBaseSink * bsink)
{
    GstSurfaceFlingerSink *surfacesink;
    GstCaps *caps;

    surfacesink = GST_SURFACEFLINGERSINK (bsink);
    caps = gst_caps_from_string (GST_SURFACE_TEMPLATE_CAPS);
  
    return caps;
}

static gboolean
gst_surfaceflinger_sink_setcaps (GstBaseSink * bsink, GstCaps * vscapslist)
{
    GstSurfaceFlingerSink *surfacesink;
    GstStructure *structure;
    const GValue *fps;

    surfacesink = GST_SURFACEFLINGERSINK (bsink);

    GST_DEBUG_OBJECT (surfacesink, "caps after linked: %s",  gst_caps_to_string(vscapslist));

    structure = gst_caps_get_structure (vscapslist, 0);
  
    surfacesink->pixel_format  = VIDEO_FLINGER_RGB_565;
    fps = gst_structure_get_value (structure, "framerate");
    surfacesink->fps_n = gst_value_get_fraction_numerator (fps);
    surfacesink->fps_d = gst_value_get_fraction_denominator (fps);

    gst_structure_get_int (structure, "width", &surfacesink->width);
    gst_structure_get_int (structure, "height", &surfacesink->height);

    GST_DEBUG_OBJECT (surfacesink, "framerate=%d/%d", surfacesink->fps_n, surfacesink->fps_d);
    GST_DEBUG_OBJECT (surfacesink, "register framebuffers: width=%d,  height=%d,  pixel_format=%d",  
        surfacesink->width, surfacesink->height, surfacesink->pixel_format);

    /* register frame buffer */
    videoflinger_device_register_framebuffers(
        surfacesink->videodev, surfacesink->width, 
        surfacesink->height, surfacesink->pixel_format);

    GST_DEBUG_OBJECT (surfacesink, "gst_surfaceflinger_sink_setcaps return true");
    return TRUE;
}


static GstFlowReturn
gst_surfaceflinger_sink_render (GstBaseSink * bsink, GstBuffer * buf)
{
    GstSurfaceFlingerSink *surfacesink;

    surfacesink = GST_SURFACEFLINGERSINK (bsink);
    GST_DEBUG_OBJECT(surfacesink, "post buffer=%p, size=%d", GST_BUFFER_DATA (buf), GST_BUFFER_SIZE (buf));
    
    /* post frame buffer */
    videoflinger_device_post(surfacesink->videodev, GST_BUFFER_DATA (buf), GST_BUFFER_SIZE (buf));

    GST_DEBUG_OBJECT (surfacesink, "gst_surfaceflinger_sink_render return GST_FLOW_OK");
    return GST_FLOW_OK;
}

static gboolean
gst_surfaceflinger_sink_start (GstBaseSink * bsink)
{
    GstSurfaceFlingerSink *surfacesink;

    surfacesink = GST_SURFACEFLINGERSINK (bsink);
  
    /* release previous video device */
    if (surfacesink->videodev != NULL) 
    {
        videoflinger_device_release ( surfacesink->videodev);
        surfacesink->videodev = NULL;
    }

    /* create a new video device */
    GST_DEBUG_OBJECT (surfacesink, "ISurface = %p", surfacesink->isurface);
    surfacesink->videodev = videoflinger_device_create(surfacesink->isurface);
    if ( surfacesink->videodev == NULL)
    {
        GST_ERROR_OBJECT (surfacesink, "Failed to create video device.");
        return FALSE;
    }    
  
    GST_DEBUG_OBJECT (surfacesink, "gst_surfaceflinger_sink_start return TRUE");
    return TRUE;
}

static gboolean
gst_surfaceflinger_sink_stop (GstBaseSink * bsink)
{
    GstSurfaceFlingerSink *surfacesink;

    surfacesink = GST_SURFACEFLINGERSINK (bsink);

    if (surfacesink->videodev != NULL) 
    {
        videoflinger_device_release ( surfacesink->videodev);
        surfacesink->videodev = NULL;
    }

    return TRUE;
}

static void
gst_surfaceflinger_sink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
    GstSurfaceFlingerSink *surfacesink;

    surfacesink = GST_SURFACEFLINGERSINK (object);

    switch (prop_id) 
    {
    case PROP_SURFACE:
        surfacesink->isurface = g_value_get_pointer(value);
        GST_DEBUG_OBJECT (surfacesink, "set property: ISureface = %p",  surfacesink->isurface);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}


static void
gst_surfaceflinger_sink_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
    GstSurfaceFlingerSink *surfacesink;

    surfacesink = GST_SURFACEFLINGERSINK (object);

    switch (prop_id) 
    {
    case PROP_SURFACE:
        g_value_set_pointer (value, surfacesink->isurface);
        GST_DEBUG_OBJECT (surfacesink, "get property: ISurface = %p.",  surfacesink->isurface);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

/* TODO: delete me later */
static GstStateChangeReturn
gst_surfaceflinger_sink_change_state (GstElement * element, GstStateChange transition)
{
    GstSurfaceFlingerSink *surfacesink;
    GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;

    g_return_val_if_fail (GST_IS_SURFACEFLINGERSINK (element), GST_STATE_CHANGE_FAILURE);
    surfacesink = GST_SURFACEFLINGERSINK (element);

    ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

    switch (transition) 
    {
    default:
      break;
    }
    return ret;
}

static gboolean
plugin_init (GstPlugin * plugin)
{
    GST_DEBUG_CATEGORY_INIT (gst_surfaceflinger_sink_debug, "surfaceflingersink",
      0, "Video sink plugin");

    if (!gst_element_register (plugin, "surfaceflingersink", GST_RANK_NONE,
          GST_TYPE_SURFACEFLINGERSINK))
    {
        return FALSE;
    }

    return TRUE;
}

static void
gst_surfaceflinger_sink_class_init (GstSurfaceFlingerSinkClass * klass)
{
    GObjectClass *gobject_class;
    GstElementClass *gstelement_class;
    GstBaseSinkClass *gstvs_class;

    gobject_class = (GObjectClass *) klass;
    gstelement_class = (GstElementClass *) klass;
    gstvs_class = (GstBaseSinkClass *) klass;

    parent_class = g_type_class_peek_parent (klass);

    gobject_class->set_property = gst_surfaceflinger_sink_set_property;
    gobject_class->get_property = gst_surfaceflinger_sink_get_property;
    gobject_class->finalize = gst_surfaceflinger_sink_finalize;

    gstelement_class->change_state =
        GST_DEBUG_FUNCPTR (gst_surfaceflinger_sink_change_state);

    g_object_class_install_property (gobject_class, PROP_SURFACE,
        g_param_spec_pointer("surface", "Surface",
        "The pointer of ISurface interface", G_PARAM_READWRITE));

    gstvs_class->set_caps = GST_DEBUG_FUNCPTR (gst_surfaceflinger_sink_setcaps);
    gstvs_class->get_caps = GST_DEBUG_FUNCPTR (gst_surfaceflinger_sink_getcaps);
    gstvs_class->get_times = GST_DEBUG_FUNCPTR (gst_surfaceflinger_sink_get_times);
    gstvs_class->preroll = GST_DEBUG_FUNCPTR (gst_surfaceflinger_sink_render);
    gstvs_class->render = GST_DEBUG_FUNCPTR (gst_surfaceflinger_sink_render);
    gstvs_class->start = GST_DEBUG_FUNCPTR (gst_surfaceflinger_sink_start);
    gstvs_class->stop = GST_DEBUG_FUNCPTR (gst_surfaceflinger_sink_stop);
}

static void
gst_surfaceflinger_sink_init (GstSurfaceFlingerSink * surfacesink)
{
    surfacesink->isurface = NULL;
    surfacesink->videodev = NULL;

    surfacesink->fps_n = 0;
    surfacesink->fps_d = 1;
    surfacesink->width = 320;
    surfacesink->height = 240;
    surfacesink->pixel_format = -1;
}

static void
gst_surfaceflinger_sink_finalize (GObject * object)
{
    GstSurfaceFlingerSink *surfacesink = GST_SURFACEFLINGERSINK (object);

    G_OBJECT_CLASS (parent_class)->finalize (object);
}

GType
gst_surfaceflinger_sink_get_type (void)
{
    static GType surfacesink_type = 0;

    if (!surfacesink_type) {
        static const GTypeInfo surfacesink_info = {
            sizeof (GstSurfaceFlingerSinkClass),
            gst_surfaceflinger_sink_base_init,
            NULL,
            (GClassInitFunc) gst_surfaceflinger_sink_class_init,
            NULL,
            NULL,
            sizeof (GstSurfaceFlingerSink),
            0,
            (GInstanceInitFunc) gst_surfaceflinger_sink_init,
            };

        surfacesink_type =
            g_type_register_static (GST_TYPE_BASE_SINK, "GstSurfaceFlingerSink",
            &surfacesink_info, 0);
    }
    return surfacesink_type;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR, GST_VERSION_MINOR, 
    "surfaceflingersink", "android surface flinger sink", 
    plugin_init, VERSION,"LGPL","GStreamer","http://gstreamer.net/")
