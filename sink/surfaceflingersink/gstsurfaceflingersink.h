
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
#ifndef __GST_FBDEVSINK_H__
#define __GST_FBDEVSINK_H__

#include <gst/gst.h>
#include <gst/video/gstvideosink.h>
#include <gst/video/video.h>
#include "surfaceflinger_wrap.h"

G_BEGIN_DECLS

#define GST_TYPE_SURFACEFLINGERSINK \
  (gst_surfaceflinger_sink_get_type())
#define GST_SURFACEFLINGERSINK(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_SURFACEFLINGERSINK,GstSurfaceFlingerSink))
#define GST_SURFACEFLINGERSINK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_SURFACEFLINGERSINK,GstSurfaceFlingerSinkClass))
#define GST_IS_SURFACEFLINGERSINK(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_SURFACEFLINGERSINK))
#define GST_IS_SURFACEFLINGERSINK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_SURFACEFLINGERSINK))

typedef struct _GstSurfaceFlingerSink GstSurfaceFlingerSink;
typedef struct _GstSurfaceFlingerSinkClass GstSurfaceFlingerSinkClass;

struct _GstSurfaceFlingerSink {
  GstVideoSink videosink;
  gpointer isurface;
  gint pixel_format;
  VideoFlingerDeviceHandle videodev;
  int width, height;
  int fps_n, fps_d;
};

struct _GstSurfaceFlingerSinkClass {
  GstBaseSinkClass parent_class;
};

GType gst_surfaceflinger_sink_get_type(void);

G_END_DECLS

#endif /* __GST_FBDEVSINK_H__ */
