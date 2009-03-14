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

/*
 * This file defines APIs to convert C++ AudioFlinder/AudioTrack
 * interface to C interface
 */
#ifndef __AUDIOFLINGER_WRAPPER_H__
#define __AUDIOFLINGER_WRAPPER_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _AudioFlingerDevice
{
  void *audio_device;
  int streamType;
  int channelCount;
  uint32_t sampleRate;
  int bufferCount;
} AudioFlingerDevice;

AudioFlingerDevice * audioflinger_device_create(int streamType,
    int channelCount,uint32_t sampleRate,int bufferCount);

int audioflinger_device_release(AudioFlingerDevice * audiodev);

void audioflinger_device_start(AudioFlingerDevice * audiodev);

void audioflinger_device_stop(AudioFlingerDevice * audiodev);

int  audioflinger_device_stoped(AudioFlingerDevice * audiodev);

ssize_t  audioflinger_device_write(AudioFlingerDevice * audiodev, 
    const void* buffer, size_t size);

void audioflinger_device_flush(AudioFlingerDevice * audiodev);

void audioflinger_device_pause(AudioFlingerDevice * audiodev);

void audioflinger_device_mute(AudioFlingerDevice * audiodev, int mute);

int  audioflinger_device_muted(AudioFlingerDevice * audiodev);

void audioflinger_device_set_volume(AudioFlingerDevice * audiodev, 
    float left, float right);

int audioflinger_device_frameCount(AudioFlingerDevice * audiodev);

int audioflinger_device_frameSize(AudioFlingerDevice * audiodev);

int64_t audioflinger_device_latency(AudioFlingerDevice * audiodev);

int audioflinger_device_streamType(AudioFlingerDevice * audiodev);
 
int audioflinger_device_format(AudioFlingerDevice * audiodev);

int audioflinger_device_channelCount(AudioFlingerDevice * audiodev);

uint32_t  audioflinger_device_sampleRate(AudioFlingerDevice * audiodev);

#ifdef __cplusplus
}
#endif

#endif /* __AUDIOFLINGER_WRAPPER_H__ */
