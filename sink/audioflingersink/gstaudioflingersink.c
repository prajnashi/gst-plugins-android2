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

/**
 * SECTION:element-audioflindersink
 *
 * This element lets you output sound using the Audio Flinger system in Android
 *
 * Note that you should almost always use generic audio conversion elements
 * like audioconvert and audioresample in front of an audiosink to make sure
 * your pipeline works under all circumstances (those conversion elements will
 * act in passthrough-mode if no conversion is necessary).
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "gstaudioflingersink.h"

#undef PCM_DUMP
#define DEFAULT_BUFFERTIME (500*GST_MSECOND) / (GST_USECOND)
#define DEFAULT_LATENCYTIME (50*GST_MSECOND) / (GST_USECOND)
#define DEFAULT_VOLUME 0.7

/*
 * PROPERTY_ID
 */
enum 
{
  PROP_NULL,
  PROP_VOLUME,
  PROP_MUTE,
  PROP_DUMP_PCM,
};

GST_DEBUG_CATEGORY_STATIC (audioflinger_debug);
#define GST_CAT_DEFAULT audioflinger_debug

/* elementfactory information */
static const GstElementDetails gst_audioflinger_sink_details =
GST_ELEMENT_DETAILS ("Audio Sink (AudioFlinger)",
    "Sink/Audio",
    "Output to android's AudioFlinger",
    "Prajnashi S <prajnashi@gmail.com>, "
    "Prajnashi S <prajnashi@gmail.com>");

static void gst_audioflinger_sink_base_init (gpointer g_class);
static void gst_audioflinger_sink_class_init (GstAudioFlingerSinkClass * klass);
static void gst_audioflinger_sink_init (GstAudioFlingerSink * audioflinger_sink);

static void gst_audioflinger_sink_dispose (GObject * object);
static void gst_audioflinger_sink_finalise (GObject * object);

static void gst_audioflinger_sink_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_audioflinger_sink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);

static GstCaps *gst_audioflinger_sink_getcaps (GstBaseSink * bsink);

static gboolean gst_audioflinger_sink_open (GstAudioSink * asink);
static gboolean gst_audioflinger_sink_close (GstAudioSink * asink);
static gboolean gst_audioflinger_sink_prepare (GstAudioSink * asink,
    GstRingBufferSpec * spec);
static gboolean gst_audioflinger_sink_unprepare (GstAudioSink * asink);
static guint gst_audioflinger_sink_write (GstAudioSink * asink, gpointer data,
    guint length);
static guint gst_audioflinger_sink_delay (GstAudioSink * asink);
static void gst_audioflinger_sink_reset (GstAudioSink * asink);
static GstStateChangeReturn gst_audioflinger_sink_change_state (GstElement *element, GstStateChange transition);
static void gst_audioflinger_sink_set_mute (GstAudioFlingerSink * audioflinger_sink, int mute);
static void gst_audioflinger_sink_set_volume (GstAudioFlingerSink * audioflinger_sink, float volume);


static GstStaticPadTemplate audioflingersink_sink_factory =
    GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw-int, "
        "endianness = (int) { " G_STRINGIFY (G_BYTE_ORDER) " }, "
        "signed = (boolean) { TRUE, FALSE }, "
        "width = (int) 16, "
        "depth = (int) 16, "
        "rate = (int) [ 1, MAX ], " "channels = (int) [ 1, 2 ]; "
        "audio/x-raw-int, "
        "signed = (boolean) { TRUE, FALSE }, "
        "width = (int) 8, "
        "depth = (int) 8, "
        "rate = (int) [ 1, MAX ], " "channels = (int) [ 1, 2 ]")
    );

static GstElementClass *parent_class = NULL;

GType
gst_audioflinger_sink_get_type (void)
{
  static GType audioflingersink_type = 0;

  if (!audioflingersink_type) {
    static const GTypeInfo audioflingersink_info = {
      sizeof (GstAudioFlingerSinkClass),
      gst_audioflinger_sink_base_init,
      NULL,
      (GClassInitFunc) gst_audioflinger_sink_class_init,
      NULL,
      NULL,
      sizeof (GstAudioFlingerSink),
      0,
      (GInstanceInitFunc) gst_audioflinger_sink_init,
    };

    audioflingersink_type =
        g_type_register_static (GST_TYPE_AUDIO_SINK, "GstAudioFlingerSink",
        &audioflingersink_info, 0);
  }

  return audioflingersink_type;
}

static void
gst_audioflinger_sink_dispose (GObject * object)
{
  GstAudioFlingerSink *audioflinger_sink = GST_AUDIOFLINGERSINK (object);

  if (audioflinger_sink->probed_caps) {
    gst_caps_unref (audioflinger_sink->probed_caps);
    audioflinger_sink->probed_caps = NULL;
  }

  if (audioflinger_sink->dump_file_location) {
    g_free(audioflinger_sink->dump_file_location);
    audioflinger_sink->dump_file_location = NULL;
  }

  if (audioflinger_sink->dump_file) {
    fclose(audioflinger_sink->dump_file);
    audioflinger_sink->dump_file = NULL;
  }
  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gst_audioflinger_sink_base_init (gpointer g_class)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (g_class);

  gst_element_class_set_details (element_class, &gst_audioflinger_sink_details);

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&audioflingersink_sink_factory));
  GST_DEBUG_CATEGORY_INIT(audioflinger_debug,"audioflingersink",0,"audioflinger sink trace");
}

static void
gst_audioflinger_sink_class_init (GstAudioFlingerSinkClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstBaseSinkClass *gstbasesink_class;
  GstBaseAudioSinkClass *gstbaseaudiosink_class;
  GstAudioSinkClass *gstaudiosink_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstbasesink_class = (GstBaseSinkClass *) klass;
  gstbaseaudiosink_class = (GstBaseAudioSinkClass *) klass;
  gstaudiosink_class = (GstAudioSinkClass *) klass;

  parent_class = g_type_class_peek_parent (klass);

  gobject_class->dispose = GST_DEBUG_FUNCPTR (gst_audioflinger_sink_dispose);
  gobject_class->finalize = GST_DEBUG_FUNCPTR (gst_audioflinger_sink_finalise);
  gobject_class->get_property = GST_DEBUG_FUNCPTR (gst_audioflinger_sink_get_property);
  gobject_class->set_property = GST_DEBUG_FUNCPTR (gst_audioflinger_sink_set_property);

  gstelement_class->change_state = GST_DEBUG_FUNCPTR (gst_audioflinger_sink_change_state);
  gstbasesink_class->get_caps = GST_DEBUG_FUNCPTR (gst_audioflinger_sink_getcaps);

  gstaudiosink_class->open = GST_DEBUG_FUNCPTR (gst_audioflinger_sink_open);
  gstaudiosink_class->close = GST_DEBUG_FUNCPTR (gst_audioflinger_sink_close);
  gstaudiosink_class->prepare = GST_DEBUG_FUNCPTR (gst_audioflinger_sink_prepare);
  gstaudiosink_class->unprepare = GST_DEBUG_FUNCPTR (gst_audioflinger_sink_unprepare);
  gstaudiosink_class->write = GST_DEBUG_FUNCPTR (gst_audioflinger_sink_write);
  gstaudiosink_class->delay = GST_DEBUG_FUNCPTR (gst_audioflinger_sink_delay);
  gstaudiosink_class->reset = GST_DEBUG_FUNCPTR (gst_audioflinger_sink_reset);

  /* Install properties */
  g_object_class_install_property(gobject_class, PROP_MUTE,
     g_param_spec_uint ("mute", "Mute",
	    "control volume to mute",0,0xFFFFFFFF,0,G_PARAM_READWRITE));
  g_object_class_install_property(gobject_class, PROP_VOLUME,
     g_param_spec_float ("volume", "Volume",
	     "control volume size",0,0xFFFFFFFF,0,G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_DUMP_PCM,
      g_param_spec_string ("dump", "Dump file location",
          "Dump raw PCM file to location", NULL,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
gst_audioflinger_sink_init (GstAudioFlingerSink * audioflinger_sink)
{
  GST_DEBUG_OBJECT (audioflinger_sink, "initializing audioflinger_sink");

  audioflinger_sink->audioflinger_device = NULL;
  audioflinger_sink->m_volume = DEFAULT_VOLUME;
  audioflinger_sink->m_mute = 0;
  audioflinger_sink->dump_file_location = NULL;
  audioflinger_sink->dump_file = NULL;
}

static void
gst_audioflinger_sink_finalise (GObject * object)
{
  GstAudioFlingerSink *audioflinger_sink = GST_AUDIOFLINGERSINK (object);

  if (audioflinger_sink->audioflinger_device != NULL)
  {
      audioflinger_device_release (audioflinger_sink->audioflinger_device);
      audioflinger_sink->audioflinger_device = NULL;
  }
  G_OBJECT_CLASS (parent_class)->finalize ((GObject *) (object));
}

static void
gst_audioflinger_sink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstAudioFlingerSink *audioflinger_sink;

  audioflinger_sink = GST_AUDIOFLINGERSINK (object);
  if (!audioflinger_sink)
  {
    return;
  }
  int val_int=0;
  float val_float = 0;
  gchar* val_str = NULL;


  switch (prop_id) {
    case PROP_MUTE:
      val_int = g_value_get_uint(value);
      gst_audioflinger_sink_set_mute(audioflinger_sink, val_int);
	  break;
	case PROP_VOLUME:
      val_float = g_value_get_float(value);
	  gst_audioflinger_sink_set_volume(audioflinger_sink, val_float);
	  break;
    case PROP_DUMP_PCM:
      val_str = g_value_get_string(value);
      audioflinger_sink->dump_file_location = g_strdup(val_str);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_audioflinger_sink_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstAudioFlingerSink *audioflinger_sink;
  audioflinger_sink = GST_AUDIOFLINGERSINK (object);
  if (!audioflinger_sink)
  {
    return;
  }
  
  switch (prop_id) {
    case PROP_MUTE:
      g_value_set_uint (value, audioflinger_sink->m_mute);
      break;
	case PROP_VOLUME:
	  g_value_set_float (value, audioflinger_sink->m_volume);
	  break;
    case PROP_DUMP_PCM:
	  g_value_set_string (value, audioflinger_sink->dump_file_location);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static GstCaps *
gst_audioflinger_sink_getcaps (GstBaseSink * bsink)
{
  GstAudioFlingerSink *audioflinger_sink;
  GstCaps *caps;

  audioflinger_sink = GST_AUDIOFLINGERSINK (bsink);

  if (audioflinger_sink->audioflinger_device == NULL) {
    caps = gst_caps_copy (gst_pad_get_pad_template_caps (GST_BASE_SINK_PAD
            (bsink)));
  } else if (audioflinger_sink->probed_caps) {
    caps = gst_caps_copy (audioflinger_sink->probed_caps);
  } else {
      caps = gst_caps_new_any ();
    if (caps && !gst_caps_is_empty (caps)) {
      audioflinger_sink->probed_caps = gst_caps_copy (caps);
    }
  }

  return caps;
}

static gint
ilog2 (gint x)
{
  /* well... hacker's delight explains... */
  x = x | (x >> 1);
  x = x | (x >> 2);
  x = x | (x >> 4);
  x = x | (x >> 8);
  x = x | (x >> 16);
  x = x - ((x >> 1) & 0x55555555);
  x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
  x = (x + (x >> 4)) & 0x0f0f0f0f;
  x = x + (x >> 8);
  x = x + (x >> 16);
  return (x & 0x0000003f) - 1;
}

static gboolean
gst_audioflinger_sink_open (GstAudioSink * asink)
{
  GstAudioFlingerSink *audioflinger;
  int mode;

  audioflinger = GST_AUDIOFLINGERSINK (asink);
  if (!audioflinger)
  {
     return FALSE;
  }

  GstBaseAudioSink *baseaudiosink = (GstBaseAudioSink*)audioflinger;
  baseaudiosink->buffer_time = DEFAULT_BUFFERTIME;
  baseaudiosink->latency_time = DEFAULT_LATENCYTIME; 

  return TRUE;
}

static gboolean
gst_audioflinger_sink_close (GstAudioSink * asink)
{
  audioflinger_device_stop(GST_AUDIOFLINGERSINK (asink)->audioflinger_device);
  if (GST_AUDIOFLINGERSINK (asink)->audioflinger_device != NULL)
  {
     audioflinger_device_release(GST_AUDIOFLINGERSINK (asink)->audioflinger_device);
	 GST_AUDIOFLINGERSINK (asink)->audioflinger_device = NULL;
  } 
  return TRUE;
}

static gboolean
gst_audioflinger_sink_prepare (GstAudioSink * asink, GstRingBufferSpec * spec)
{
  GstAudioFlingerSink *audioflinger;

  audioflinger = GST_AUDIOFLINGERSINK (asink);

  /*
   * FIXME:
   * In test, I found android audio flinger cannot work with 8bit audio.
   * So, only 16 bit PCM is supported here. 
   */
  if (spec->width != 16)
    goto dodgy_width;

  if (audioflinger->audioflinger_device == NULL)
  {
     /* we keep the audio flinger internal buffer size as  spec->segsize */
     audioflinger->audioflinger_device = 
       audioflinger_device_create(3,spec->channels,spec->rate,spec->segsize);
	 if (audioflinger->audioflinger_device == NULL)
	 {
       goto wrong_format;
	 }
  }

  gst_audioflinger_sink_set_volume(audioflinger, audioflinger->m_volume);
  gst_audioflinger_sink_set_mute(audioflinger,audioflinger->m_mute);

  spec->bytes_per_sample = (spec->width / 8) * spec->channels;
  audioflinger->bytes_per_sample = (spec->width / 8) * spec->channels;

  GST_DEBUG_OBJECT (audioflinger, 
    "channels: %d, rate: %d, width: %d, got segsize: %d, segtotal: %d, frame count: %d, frame size: %d",
    spec->channels, spec->rate, spec->width, spec->segsize, spec->segtotal,
    audioflinger_device_frameCount(audioflinger->audioflinger_device),
    audioflinger_device_frameSize(audioflinger->audioflinger_device)
    );

  /* check if dump raw PCM */
  if (audioflinger->dump_file_location) {
    GST_DEBUG_OBJECT (audioflinger, "open file to dump raw PCM: %s",
       audioflinger->dump_file_location);
    audioflinger->dump_file = fopen (audioflinger->dump_file_location,"wb");
    if (audioflinger->dump_file == NULL)  {
      GST_ERROR_OBJECT (audioflinger, "cannot open file to dump raw PCM: %s", 
        audioflinger->dump_file_location);
    }
  }

  return TRUE;

  /* ERRORS */
wrong_format:
  {
    GST_ELEMENT_ERROR (audioflinger, RESOURCE, SETTINGS, (NULL),
        ("Unable to get format %d", spec->format));
    return FALSE;
  }
dodgy_width:
  {
    GST_ELEMENT_ERROR (audioflinger, RESOURCE, SETTINGS, (NULL),
        ("unexpected width %d", spec->width));
    return FALSE;
  }
}

static gboolean
gst_audioflinger_sink_unprepare (GstAudioSink * asink)
{
  GstAudioFlingerSink *audioflinger;

  audioflinger = GST_AUDIOFLINGERSINK (asink);

  /* close raw PCM file */
  if (audioflinger->dump_file)  {
    GST_DEBUG_OBJECT (audioflinger, "close file to dump raw PCM");
    fclose (audioflinger->dump_file);
    audioflinger->dump_file = NULL;
  }

  if (!gst_audioflinger_sink_close (asink))
    goto couldnt_close;

  if (!gst_audioflinger_sink_open (asink))
    goto couldnt_reopen;

  return TRUE;

  /* ERRORS */
couldnt_close:
  {
    GST_DEBUG_OBJECT (asink, "Could not close the audio device");
    return FALSE;
  }
couldnt_reopen:
  {
    GST_DEBUG_OBJECT (asink, "Could not reopen the audio device");
    return FALSE;
  }
}

static guint
gst_audioflinger_sink_write (GstAudioSink * asink, gpointer data, guint length)
{
  GstAudioFlingerSink *audioflinger;
  guint ret = 0;
  
  audioflinger = GST_AUDIOFLINGERSINK (asink);

  /* dump raw PCM to file */
  if(audioflinger->dump_file)
  {
    fwrite(data,length,1,audioflinger->dump_file);
    fflush(audioflinger->dump_file);
  }

  GST_INFO_OBJECT (audioflinger, "write length=%d", length);
  ret = audioflinger_device_write (audioflinger->audioflinger_device, data, length);

  GST_INFO_OBJECT (audioflinger, "written=%u", ret);
  
  return ret;
}

static guint
gst_audioflinger_sink_delay (GstAudioSink * asink)
{
  GstAudioFlingerSink *audioflinger;
  gint delay = 0;
  gint ret;

  audioflinger = GST_AUDIOFLINGERSINK (asink);

  /*
   * TODO:
   * Call audioflinger_device_latency() to get delay
   */
  return delay / audioflinger->bytes_per_sample;
}

static void
gst_audioflinger_sink_reset (GstAudioSink * asink)
{
  /*
   * TODO:
   * reopen device here?
   */
}

static void 
gst_audioflinger_sink_set_mute (GstAudioFlingerSink * audioflinger_sink, int mute)
{
   GST_DEBUG_OBJECT(audioflinger_sink,"set PROP_MUTE = %d\n",mute);
   if (audioflinger_sink->audioflinger_device != NULL)
   {
      audioflinger_device_mute(audioflinger_sink->audioflinger_device,mute);
   }
   audioflinger_sink->m_mute = mute;
}

static void 
gst_audioflinger_sink_set_volume (GstAudioFlingerSink * audioflinger_sink, float volume)
{
   GST_DEBUG_OBJECT(audioflinger_sink,"set PROP_VOLUME = %f\n",volume);
   if (audioflinger_sink->audioflinger_device != NULL)
   {
      audioflinger_device_set_volume(audioflinger_sink->audioflinger_device,volume,volume);
   }

   audioflinger_sink->m_volume = volume;
}


static GstStateChangeReturn 
gst_audioflinger_sink_change_state (GstElement *element, GstStateChange transition)
{
   GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
   GstAudioFlingerSink *audioflinger_sink = GST_AUDIOFLINGERSINK (element);

   switch (transition) {
      case GST_STATE_CHANGE_NULL_TO_READY:
	       GST_DEBUG_OBJECT(audioflinger_sink,"GST_STATE_CHANGE_NULL_TO_READY\n");
	       break;
	  case GST_STATE_CHANGE_READY_TO_PAUSED:
           GST_DEBUG_OBJECT(audioflinger_sink,"GST_STATE_CHANGE_READY_TO_PAUSED\n");
		   break;
      case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
	       GST_DEBUG_OBJECT(audioflinger_sink,"GST_STATE_CHANGE_PAUSED_TO_PLAYING\n");
		   if (audioflinger_device_stoped(audioflinger_sink->audioflinger_device) != 0)
			{
			  audioflinger_device_start(audioflinger_sink->audioflinger_device);
		    }
			
		   break;
	  default:
	       break;
   }

   ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);
   if (G_UNLIKELY (ret == GST_STATE_CHANGE_FAILURE))
	   goto activate_failed;
   
   switch (transition) {
      case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
	       GST_DEBUG_OBJECT(audioflinger_sink,"GST_STATE_CHANGE_PLAYING_TO_PAUSED\n");
		   if (audioflinger_device_stoped(audioflinger_sink->audioflinger_device) == 0)
		   {
              audioflinger_device_pause(audioflinger_sink->audioflinger_device); 
		   }
		   break;
	  case GST_STATE_CHANGE_PAUSED_TO_READY:
	       GST_DEBUG_OBJECT(audioflinger_sink,"GST_STATE_CHANGE_PAUSED_TO_READY\n");
		   break;
	  case GST_STATE_CHANGE_READY_TO_NULL:
	       GST_DEBUG_OBJECT(audioflinger_sink,"GST_STATE_CHANGE_READY_TO_NULL\n");
		   break;
	  default:
	       break;
   }
   return ret;
   // errors
activate_failed:
   {
     GST_DEBUG_OBJECT (audioflinger_sink,"element failed to change states -- activation problem?");
	 return GST_STATE_CHANGE_FAILURE;
   }
}

static gboolean
plugin_init (GstPlugin *plugin)
{
  return gst_element_register (plugin,"audioflingersink",GST_RANK_NONE,GST_TYPE_AUDIOFLINGERSINK);
}

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR,GST_VERSION_MINOR,"audioflingersink","audioflinger sink audio",plugin_init,VERSION,"LGPL","GStreamer","http://gstreamer.net/")
