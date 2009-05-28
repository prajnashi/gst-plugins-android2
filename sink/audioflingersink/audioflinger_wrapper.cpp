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
#define ENABLE_GST_PLAYER_LOG
#include <media/AudioTrack.h>
#include <utils/Log.h>
#include <AudioFlinger.h>
#include <MediaPlayerInterface.h>
#include <MediaPlayerService.h>
#include "audioflinger_wrapper.h"
#include <glib/glib.h>
#include <GstLog.h>

using namespace android;

typedef struct _AudioFlingerDevice
{
  AudioTrack* audio_track;
  bool init;
  sp<MediaPlayerBase::AudioSink> audio_sink;
} AudioFlingerDevice;


/* commonly used macro */
#define AUDIO_FLINGER_DEVICE(handle) ((AudioFlingerDevice*)handle)
#define AUDIO_FLINGER_DEVICE_TRACK(handle) \
    (AUDIO_FLINGER_DEVICE(handle)->audio_track)
#define AUDIO_FLINGER_DEVICE_SINK(handle) \
    (AUDIO_FLINGER_DEVICE(handle)->audio_sink)


AudioFlingerDeviceHandle audioflinger_device_create()
{
  AudioFlingerDevice* audiodev = NULL;
  AudioTrack *audiotr = NULL;

  // create a new instance of AudioFlinger 
  audiodev = new AudioFlingerDevice;
  if (audiodev == NULL) {
    GST_PLAYER_ERROR("Error to create AudioFlingerDevice\n");
    return NULL;
  }

  // create AudioTrack
  audiotr = new AudioTrack ();
  if (audiotr == NULL) {
    GST_PLAYER_ERROR("Error to create AudioTrack\n");
    return NULL;
  }

  audiodev->init = false;
  audiodev->audio_track = (AudioTrack *) audiotr;
  audiodev->audio_sink = 0;
  GST_PLAYER_DEBUG("Create AudioTrack successfully\n");

  return (AudioFlingerDeviceHandle)audiodev;
}

AudioFlingerDeviceHandle audioflinger_device_open(void* audio_sink)
{
  AudioFlingerDevice* audiodev = NULL;

  // audio_sink shall be an MediaPlayerBase::AudioSink instance
  if(audio_sink == NULL)
    return NULL;

  // create a new instance of AudioFlinger 
  audiodev = new AudioFlingerDevice;
  if (audiodev == NULL) {
    GST_PLAYER_ERROR("Error to create AudioFlingerDevice\n");
    return NULL;
  }

  // set AudioSink
  audiodev->audio_track = NULL;
  audiodev->audio_sink = (MediaPlayerBase::AudioSink*)audio_sink;
  audiodev->init = false;
  GST_PLAYER_DEBUG("Open AudioSink successfully\n");

  return (AudioFlingerDeviceHandle)audiodev;    
}

int audioflinger_device_set (AudioFlingerDeviceHandle handle, 
  int streamType, int channelCount, uint32_t sampleRate, int bufferCount)
{
  status_t status = NO_ERROR;

  int format = AudioSystem::PCM_16_BIT;

  if (handle == NULL)
      return -1;

  if(AUDIO_FLINGER_DEVICE_TRACK(handle)) {
    // bufferCount is not the number of internal buffer, but the internal
    // buffer size 
    status = AUDIO_FLINGER_DEVICE_TRACK(handle)->set(streamType, sampleRate, 
        format, channelCount, bufferCount);
    GST_PLAYER_DEBUG("Set AudioTrack, status: %d, streamType: %d, sampleRate: %d, "
        "channelCount: %d, bufferCount: %d\n", status, streamType, sampleRate, 
        channelCount, bufferCount);
  }
  else if(AUDIO_FLINGER_DEVICE_SINK(handle).get()) {
    status = AUDIO_FLINGER_DEVICE_SINK(handle)->open(sampleRate, channelCount, 
        format, bufferCount);
    GST_PLAYER_DEBUG("Set AudioSink, status: %d, streamType: %d, sampleRate: %d," 
        "channelCount: %d, bufferCount: %d\n", status, streamType, sampleRate, 
        channelCount, bufferCount);
  }

  if (status != NO_ERROR) 
    return -1;

  AUDIO_FLINGER_DEVICE(handle)->init = true;

  return 0;
}

void audioflinger_device_release (AudioFlingerDeviceHandle handle)
{
  if (handle == NULL)
    return;

  GST_PLAYER_DEBUG("Enter\n");
  if (AUDIO_FLINGER_DEVICE_TRACK(handle))  {
    GST_PLAYER_DEBUG("Release AudioTrack\n");
    delete AUDIO_FLINGER_DEVICE_TRACK(handle);
  }


  if (AUDIO_FLINGER_DEVICE_SINK(handle).get()) {
    GST_PLAYER_DEBUG("Release AudioSink\n");
    AUDIO_FLINGER_DEVICE_SINK(handle).clear();
  }
  
  delete AUDIO_FLINGER_DEVICE(handle);
}


void audioflinger_device_start (AudioFlingerDeviceHandle handle)
{
  if (handle == NULL || AUDIO_FLINGER_DEVICE(handle)->init == false)
    return;

  if (AUDIO_FLINGER_DEVICE_TRACK(handle))  {
    AUDIO_FLINGER_DEVICE_TRACK(handle)->start();
  }
  else {
    AUDIO_FLINGER_DEVICE_SINK(handle)->start();
  }
}

void audioflinger_device_stop (AudioFlingerDeviceHandle handle)
{
  if (handle == NULL || AUDIO_FLINGER_DEVICE(handle)->init == false)
    return;

  if (AUDIO_FLINGER_DEVICE_TRACK(handle))  {
    AUDIO_FLINGER_DEVICE_TRACK(handle)->stop();
  }
  else {
    AUDIO_FLINGER_DEVICE_SINK(handle)->stop();
  }
}

void audioflinger_device_flush (AudioFlingerDeviceHandle handle)
{
  if (handle == NULL || AUDIO_FLINGER_DEVICE(handle)->init == false)
    return;

  if (AUDIO_FLINGER_DEVICE_TRACK(handle))  {
    AUDIO_FLINGER_DEVICE_TRACK(handle)->flush();
  }
  else {
    AUDIO_FLINGER_DEVICE_SINK(handle)->flush();
  }
}

void audioflinger_device_pause (AudioFlingerDeviceHandle handle)
{
  if (handle == NULL || AUDIO_FLINGER_DEVICE(handle)->init == false)
    return;

  if (AUDIO_FLINGER_DEVICE_TRACK(handle))  {
    AUDIO_FLINGER_DEVICE_TRACK(handle)->pause();
  }
  else {
    AUDIO_FLINGER_DEVICE_SINK(handle)->pause();
  }  
}

void audioflinger_device_mute (AudioFlingerDeviceHandle handle, int mute)
{
  if (handle == NULL || AUDIO_FLINGER_DEVICE(handle)->init == false)
    return;

  if (AUDIO_FLINGER_DEVICE_TRACK(handle))  {
    AUDIO_FLINGER_DEVICE_TRACK(handle)->mute((bool)mute);
  }
  else {
    // do nothing here, because the volume/mute is set in media service layer
  }    
}

int audioflinger_device_muted (AudioFlingerDeviceHandle handle)
{
  if (handle == NULL || AUDIO_FLINGER_DEVICE(handle)->init == false)
      return -1;

  if (AUDIO_FLINGER_DEVICE_TRACK(handle))  {
    return (int) AUDIO_FLINGER_DEVICE_TRACK(handle)->muted ();
  }
  else {
    // do nothing here, because the volume/mute is set in media service layer
    return -1;
  }      
}


void audioflinger_device_set_volume (AudioFlingerDeviceHandle handle, float left,
    float right)
{
  if (handle == NULL || AUDIO_FLINGER_DEVICE(handle)->init == false)
    return;

  if (AUDIO_FLINGER_DEVICE_TRACK(handle))  {
    AUDIO_FLINGER_DEVICE_TRACK(handle)->setVolume (left, right);
  }
  else {
    // do nothing here, because the volume/mute is set in media service layer
    return;
  }        
}

ssize_t audioflinger_device_write (AudioFlingerDeviceHandle handle, const void *buffer,
    size_t size)
{
  if (handle == NULL || AUDIO_FLINGER_DEVICE(handle)->init == false)
    return -1;
  if (AUDIO_FLINGER_DEVICE_TRACK(handle))  {
    return AUDIO_FLINGER_DEVICE_TRACK(handle)->write(buffer, size);
  }
  else {
    return AUDIO_FLINGER_DEVICE_SINK(handle)->write(buffer, size);
  }  
}

int audioflinger_device_frameCount (AudioFlingerDeviceHandle handle)
{
  if (handle == NULL || AUDIO_FLINGER_DEVICE(handle)->init == false)
    return -1;
  if (AUDIO_FLINGER_DEVICE_TRACK(handle))  {
    return (int)AUDIO_FLINGER_DEVICE_TRACK(handle)->frameCount();
  }
  else {
    return (int)AUDIO_FLINGER_DEVICE_SINK(handle)->frameCount();
  } 
}

int audioflinger_device_frameSize (AudioFlingerDeviceHandle handle)
{
  if (handle == NULL || AUDIO_FLINGER_DEVICE(handle)->init == false)
    return -1;
  if (AUDIO_FLINGER_DEVICE_TRACK(handle))  {
    return (int)AUDIO_FLINGER_DEVICE_TRACK(handle)->frameSize();
  }
  else {
    return (int)AUDIO_FLINGER_DEVICE_SINK(handle)->frameSize();
  }   
}

int64_t audioflinger_device_latency (AudioFlingerDeviceHandle handle)
{
  if (handle == NULL || AUDIO_FLINGER_DEVICE(handle)->init == false)
    return -1;
  if (AUDIO_FLINGER_DEVICE_TRACK(handle))  {
    return (int64_t)AUDIO_FLINGER_DEVICE_TRACK(handle)->latency();
  }
  else {
    return (int64_t)AUDIO_FLINGER_DEVICE_SINK(handle)->latency();
  }     
}

int audioflinger_device_format (AudioFlingerDeviceHandle handle)
{
  if (handle == NULL || AUDIO_FLINGER_DEVICE(handle)->init == false)
    return -1;
  if (AUDIO_FLINGER_DEVICE_TRACK(handle))  {
    return (int)AUDIO_FLINGER_DEVICE_TRACK(handle)->format();
  }
  else {
    // do nothing here, MediaPlayerBase::AudioSink doesn't provide format()
    // interface
    return -1;
  }       
}

int audioflinger_device_channelCount (AudioFlingerDeviceHandle handle)
{
  if (handle == NULL || AUDIO_FLINGER_DEVICE(handle)->init == false)
    return -1;
  if (AUDIO_FLINGER_DEVICE_TRACK(handle))  {
    return (int)AUDIO_FLINGER_DEVICE_TRACK(handle)->channelCount();
  }
  else {
    return (int)AUDIO_FLINGER_DEVICE_SINK(handle)->channelCount();
  }  
}

uint32_t audioflinger_device_sampleRate (AudioFlingerDeviceHandle handle)
{
  if (handle == NULL || AUDIO_FLINGER_DEVICE(handle)->init == false)
    return 0;
  if (AUDIO_FLINGER_DEVICE_TRACK(handle))  {
    return (int)AUDIO_FLINGER_DEVICE_TRACK(handle)->sampleRate();
  }
  else {
    // do nothing here, MediaPlayerBase::AudioSink doesn't provide sampleRate()
    // interface
    return -1;
  }
}
