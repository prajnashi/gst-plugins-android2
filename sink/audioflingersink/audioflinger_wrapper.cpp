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

#include <media/AudioTrack.h>
#include <utils/Log.h>
#include <AudioFlinger.h>
#include "audioflinger_wrapper.h"
#include <glib/glib.h>

using namespace android;

/* commonly used macro */
#define DEV_TRACK(audiodev) ((AudioTrack *) audiodev->audio_device)

AudioFlingerDevice *
audioflinger_device_create (int streamType, int channelCount,
    uint32_t sampleRate, int bufferCount)
{
  AudioFlingerDevice *audiodev = NULL;
  int format = AudioSystem::PCM_16_BIT;
  AudioTrack *audiotr = NULL;
  status_t status = NO_ERROR;

  /* create a new instance of AudioFlinger */
  AudioFlinger::instantiate ();

  audiodev = (AudioFlingerDevice *) malloc (sizeof (AudioFlingerDevice));
  if (audiodev == NULL) {
    return NULL;
  }

  /* create AudioTrack */
  audiotr = new AudioTrack ();
  if (audiotr == NULL) {
    return NULL;
  }

  /* bufferCount is not the number of internal buffer, but the internal
   * buffer size
   */
  status =
      audiotr->set (streamType, sampleRate, format, channelCount, bufferCount);
  if (status != NO_ERROR) {
    return NULL;
  }

  audiodev->streamType = streamType;
  audiodev->channelCount = channelCount;
  audiodev->sampleRate = sampleRate;
  audiodev->bufferCount = bufferCount;
  audiodev->audio_device = (AudioTrack *) audiotr;

  return audiodev;
}

int
audioflinger_device_release (AudioFlingerDevice * audiodev)
{
  g_return_val_if_fail (audiodev != NULL, 0);

  if (audiodev->audio_device != NULL)
    delete ((AudioTrack *) audiodev->audio_device);

  free (audiodev);

  return 0;
}


void
audioflinger_device_start (AudioFlingerDevice * audiodev)
{
  g_return_if_fail (audiodev != NULL);

  DEV_TRACK(audiodev)->start();
}

void
audioflinger_device_stop (AudioFlingerDevice * audiodev)
{
  g_return_if_fail (audiodev != NULL);

  DEV_TRACK(audiodev)->stop();
}

int
audioflinger_device_stoped (AudioFlingerDevice * audiodev)
{
  g_return_val_if_fail (audiodev != NULL, 0);

  return (int) DEV_TRACK(audiodev)->stopped ();
}

void
audioflinger_device_flush (AudioFlingerDevice * audiodev)
{
  g_return_if_fail (audiodev != NULL);

  DEV_TRACK(audiodev)->flush();
}

void
audioflinger_device_pause (AudioFlingerDevice * audiodev)
{
  g_return_if_fail (audiodev != NULL);

  DEV_TRACK(audiodev)->pause();
}

void
audioflinger_device_mute (AudioFlingerDevice * audiodev, int mute)
{
  g_return_if_fail (audiodev != NULL);

  DEV_TRACK(audiodev)->mute((bool) mute);
}

int
audioflinger_device_muted (AudioFlingerDevice * audiodev)
{
  g_return_val_if_fail (audiodev != NULL, 0);

  return (int) DEV_TRACK(audiodev)->muted ();
}


void
audioflinger_device_set_volume (AudioFlingerDevice * audiodev, float left,
    float right)
{
  g_return_if_fail (audiodev != NULL);

  DEV_TRACK(audiodev)->setVolume (left, right);
}

ssize_t
audioflinger_device_write (AudioFlingerDevice * audiodev, const void *buffer,
    size_t size)
{
  g_return_val_if_fail (audiodev != NULL, -1);
  g_return_val_if_fail (buffer != NULL, -1);

  return DEV_TRACK(audiodev)->write (buffer, size);
}

int
audioflinger_device_frameCount (AudioFlingerDevice * audiodev)
{
  g_return_val_if_fail (audiodev != NULL, -1);

  return DEV_TRACK(audiodev)->frameCount ();
}

int
audioflinger_device_frameSize (AudioFlingerDevice * audiodev)
{
  g_return_val_if_fail (audiodev != NULL, -1);

  return DEV_TRACK(audiodev)->frameSize ();
}

int64_t
audioflinger_device_latency (AudioFlingerDevice * audiodev)
{
  g_return_val_if_fail (audiodev != NULL, -1);

  return (int64_t) DEV_TRACK(audiodev)->latency();
}

int
audioflinger_device_streamType (AudioFlingerDevice * audiodev)
{
  g_return_val_if_fail (audiodev != NULL, -1);

  return DEV_TRACK(audiodev)->streamType();
}

int
audioflinger_device_format (AudioFlingerDevice * audiodev)
{
  g_return_val_if_fail (audiodev != NULL, -1);

  return DEV_TRACK(audiodev)->format();
}

int
audioflinger_device_channelCount (AudioFlingerDevice * audiodev)
{
  g_return_val_if_fail (audiodev != NULL, -1);

  return DEV_TRACK(audiodev)->channelCount();
}

uint32_t
audioflinger_device_sampleRate (AudioFlingerDevice * audiodev)
{
  g_return_val_if_fail (audiodev != NULL, -1);

  return DEV_TRACK(audiodev)->sampleRate();
}
