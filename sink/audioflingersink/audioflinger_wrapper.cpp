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

using namespace android;


AudioFlingerDevice * audioflinger_device_create(int streamType,int channelCount,uint32_t sampleRate,int bufferCount)
{
    AudioFlingerDevice *audiodev = NULL;
    int format = AudioSystem::PCM_16_BIT;
    AudioTrack *audiotr = NULL;
    status_t status = NO_ERROR;

    /* create a new instance of AudioFlinger */
    AudioFlinger::instantiate();   

    audiodev = (AudioFlingerDevice*)malloc(sizeof(AudioFlingerDevice));   
    if (audiodev == NULL)
    {
        return NULL;
    }
    
    /* create AudioTrack */
    audiotr = new AudioTrack(); 
    if(audiotr == NULL )
    {
        return NULL;
    }    

    /* bufferCount is not the number of internal buffer, but the internal
     * buffer size
     */
    status = audiotr->set(streamType,sampleRate,format,channelCount,bufferCount);
    if(status != NO_ERROR)
    {
        return NULL;
    }
   
    audiodev->streamType = streamType;
    audiodev->channelCount = channelCount;
    audiodev->sampleRate = sampleRate;
    audiodev->bufferCount = bufferCount;
    audiodev->audio_device = (AudioTrack *)audiotr;

    return audiodev;   
}

int audioflinger_device_release(AudioFlingerDevice * audiodev)
{
    if (audiodev == NULL)
    {
        return -1;
    } 

    if (audiodev->audio_device != NULL)
    {
        delete((AudioTrack *)audiodev->audio_device);
        audiodev->audio_device = NULL;
    }

    audiodev->streamType = -1;
    audiodev->channelCount = 0;
    audiodev->sampleRate = 0;
    audiodev->bufferCount = 0;  

    free(audiodev);
    audiodev = NULL;

    return 0;
}


void audioflinger_device_start(AudioFlingerDevice * audiodev)
{
    if (audiodev == NULL)
    {
        return;
    }

    AudioTrack *audiotr = NULL;
    audiotr = (AudioTrack *)audiodev->audio_device;

    audiotr->start();
}

void audioflinger_device_stop(AudioFlingerDevice * audiodev)
{
    if (audiodev == NULL)
    {
        return;
    }

    AudioTrack *audiotr = NULL;
    audiotr = (AudioTrack *)audiodev->audio_device;

    audiotr->stop();
}

int  audioflinger_device_stoped(AudioFlingerDevice * audiodev)
{
    if (audiodev == NULL)
    {
        return -1;
    }
   
    AudioTrack *audiotr = NULL;
    audiotr = (AudioTrack *)audiodev->audio_device;
 
    return ((audiotr->stopped() == true) ? 1:0);
}

void audioflinger_device_flush(AudioFlingerDevice * audiodev)
{
    if (audiodev == NULL)
    {
        return;
    }

    AudioTrack *audiotr = NULL;
    audiotr = (AudioTrack *)audiodev->audio_device;

    audiotr->flush();
}

void audioflinger_device_pause(AudioFlingerDevice * audiodev)
{
    if (audiodev == NULL)
    {
        return;
    }

    AudioTrack *audiotr = NULL;
    audiotr = (AudioTrack *)audiodev->audio_device;

    audiotr->pause();
}

void audioflinger_device_mute(AudioFlingerDevice * audiodev, int mute)
{
    if (audiodev == NULL)
    {
        return;
    }

    AudioTrack *audiotr = NULL;
    bool mute_bool = false;
    audiotr = (AudioTrack *)audiodev->audio_device;
    if (mute == 0)
    {
        mute_bool = false;
    }
    if (mute == 1)
    {
        mute_bool = true;
    }
    audiotr->mute(mute_bool);
}

int audioflinger_device_muted(AudioFlingerDevice * audiodev)
{
    if (audiodev == NULL)
    {
        return -1;
    }
    AudioTrack *audiotr = NULL;
    audiotr = (AudioTrack *)audiodev->audio_device;
  
    return ((audiotr->muted() == true) ? 1:0);
}


void audioflinger_device_set_volume(AudioFlingerDevice * audiodev, float left, float right)
{
    if (audiodev == NULL)
    {
        return;
    }

    AudioTrack *audiotr = (AudioTrack *)audiodev->audio_device;

    audiotr->setVolume(left,right);
}

ssize_t  audioflinger_device_write(AudioFlingerDevice * audiodev, const void* buffer, size_t size)
{
    if (audiodev == NULL || buffer == NULL)
    {
        return -1;
    }
   
    size_t writesize = -1;
    AudioTrack *audiotr = NULL;
    audiotr = (AudioTrack *)audiodev->audio_device;

    writesize = audiotr->write(buffer, size);

    return writesize;
}

int audioflinger_device_frameCount(AudioFlingerDevice * audiodev)
{
    if (audiodev == NULL)
    {
        return -1;
    }
   
    int framecount = 0;
    AudioTrack *audiotr = NULL;
    audiotr = (AudioTrack *)audiodev->audio_device;

    framecount = audiotr->frameCount();

    return framecount;
}

int audioflinger_device_frameSize(AudioFlingerDevice * audiodev)
{
    if (audiodev == NULL)
    {
        return -1;
    }
   
    int framesize = 0;
    AudioTrack *audiotr = NULL;
    audiotr = (AudioTrack *)audiodev->audio_device;

    framesize = audiotr->frameSize();

    return framesize;
}



int64_t audioflinger_device_latency(AudioFlingerDevice * audiodev)
{
    if (audiodev == NULL)
    {
        return -1;
    }

    nsecs_t latency_time = 0;
    AudioTrack *audiotr = NULL;
    audiotr = (AudioTrack *)audiodev->audio_device;

    latency_time = audiotr->latency();

    return (int64_t)latency_time;
}

int audioflinger_device_streamType(AudioFlingerDevice * audiodev)
{
    if (audiodev == NULL)
    {
        return -1;
    }

    int streamtype;
    AudioTrack *audiotr = NULL;
    audiotr = (AudioTrack *)audiodev->audio_device;

    streamtype = audiotr->streamType();

    return streamtype;
}

int audioflinger_device_format(AudioFlingerDevice * audiodev)
{
    if (audiodev == NULL)
    {
        return -1;
    }

    int format;
    AudioTrack *audiotr = NULL;
    audiotr = (AudioTrack *)audiodev->audio_device;

    format = audiotr->format();

    return format;
}

int audioflinger_device_channelCount(AudioFlingerDevice * audiodev)
{
    if (audiodev == NULL)
    {
        return -1;
    }

    int channelcount;
    AudioTrack *audiotr = NULL;
    audiotr = (AudioTrack *)audiodev->audio_device;

    channelcount = audiotr->channelCount();

    return channelcount;
}

uint32_t  audioflinger_device_sampleRate(AudioFlingerDevice * audiodev)
{
    if (audiodev == NULL)
    {
        return 0;
    }

    uint32_t  samplerate;
    AudioTrack *audiotr = NULL;
    audiotr = (AudioTrack *)audiodev->audio_device;

    samplerate = audiotr->sampleRate();

    return samplerate;
}
