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
#include "GstLog.h"
#include <utils/Log.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <media/thread_init.h>
#include <utils/threads.h>
#include <utils/List.h>
#include "GstPlayer.h"
#include "GstPlayerPipeline.h"

namespace android {

// ----------------------------------------------------------------------------
// implement the Gstreamer player
// ----------------------------------------------------------------------------
GstPlayer::GstPlayer()
{
    GST_PLAYER_DEBUG ("Enter\n");
    mDataSourcePath = NULL;
    mGstPlayerPipeline = new GstPlayerPipeline(this);
    if (mGstPlayerPipeline != NULL)
        mInit = OK;
    else
        mInit = NO_INIT;
    mIsDataSourceSet = false;
    mDuration = -1;
    mSurface = NULL;
    GST_PLAYER_DEBUG ("Leave\n");
}

status_t GstPlayer::initCheck()
{
    GST_PLAYER_DEBUG ("mInit = %s\n", (mInit==OK)?"OK":"ERROR");
    return mInit;
}

GstPlayer::~GstPlayer()
{
    GST_PLAYER_DEBUG ("Enter\n");
    if (mGstPlayerPipeline )
    {
        delete mGstPlayerPipeline;
        mGstPlayerPipeline = NULL;
    }
    GST_PLAYER_DEBUG ("Clear Surface.\n");
    if (mSurface != NULL)
    {
        mSurface.clear();
    }
    free(mDataSourcePath);
    GST_PLAYER_DEBUG ("Leave\n");
}

status_t GstPlayer::setAudioStreamType(int type)
{
    mStreamType = type;
    GST_PLAYER_DEBUG ("mStreamType = %d\n", mStreamType);
    return OK;
}

status_t GstPlayer::setDataSource(const char *url)
{
    status_t ret = android::UNKNOWN_ERROR;

    if (mGstPlayerPipeline == NULL || url == NULL)
        return android::UNKNOWN_ERROR;

    if (mGstPlayerPipeline->setDataSource(url) != true)
    {
        GST_PLAYER_ERROR("Cannot set data source\n");
        goto EXIT;
    }

    if (mGstPlayerPipeline->setAudioSink(mAudioSink) != true)
    {
        GST_PLAYER_ERROR("Cannot set audio sink\n");
        goto EXIT;
    }

    ret = OK;
EXIT:    
    return ret;
}

status_t GstPlayer::setDataSource(int fd, int64_t offset, int64_t length)
{
    status_t ret = android::UNKNOWN_ERROR;

    GST_PLAYER_DEBUG("fd: %i, offset: %ld, len: %ld\n", fd, (long)offset, (long)length);

    if (mGstPlayerPipeline->setDataSource(fd, offset, length) != true)
    {
        GST_PLAYER_ERROR("Cannot set data source\n");
        goto EXIT;
    }
    if (mGstPlayerPipeline->setAudioSink(mAudioSink) != true)
    {
        GST_PLAYER_ERROR("Cannot set audio sink\n");
        goto EXIT;
    }

    ret = OK;
EXIT:    
    return ret;
}

status_t GstPlayer::setVideoSurface(const sp<ISurface>& surface)
{
    GST_PLAYER_DEBUG("setVideoSurface(%p)\n", surface.get());
    mSurface = surface;
    return OK;
}

status_t GstPlayer::prepare()
{
    if(mGstPlayerPipeline == NULL)
        return android::UNKNOWN_ERROR;

    return mGstPlayerPipeline->prepare() ? OK : android::UNKNOWN_ERROR;
}


status_t GstPlayer::prepareAsync()
{
    if(mGstPlayerPipeline == NULL)
        return android::UNKNOWN_ERROR;

    return mGstPlayerPipeline->prepareAsync() ? OK : android::UNKNOWN_ERROR;
}

status_t GstPlayer::start()
{
    if(mGstPlayerPipeline == NULL)
        return android::UNKNOWN_ERROR;

    return mGstPlayerPipeline->start() ? OK : android::UNKNOWN_ERROR;
}

status_t GstPlayer::stop()
{
    if(mGstPlayerPipeline == NULL)
        return android::UNKNOWN_ERROR;

    return mGstPlayerPipeline->stop() ? OK : android::UNKNOWN_ERROR;
}

status_t GstPlayer::pause()
{
    if(mGstPlayerPipeline == NULL)
        return android::UNKNOWN_ERROR;

    return mGstPlayerPipeline->pause() ? OK : android::UNKNOWN_ERROR;    
}

bool GstPlayer::isPlaying()
{
    if(mGstPlayerPipeline == NULL)
        return false;
    else
        return mGstPlayerPipeline->isPlaying();
}

status_t GstPlayer::getVideoWidth(int *w)
{
    int h;
    if(mGstPlayerPipeline == NULL)
        return android::UNKNOWN_ERROR;    

    return mGstPlayerPipeline->getVideoSize(w, &h) ?  
        OK : android::UNKNOWN_ERROR;
}

status_t GstPlayer::getVideoHeight(int *h)
{
    int w;
    if(mGstPlayerPipeline == NULL)
        return android::UNKNOWN_ERROR;    

    return mGstPlayerPipeline->getVideoSize(&w, h) ?  
        OK : android::UNKNOWN_ERROR;
}

status_t GstPlayer::getCurrentPosition(int *msec)
{
    if(mGstPlayerPipeline == NULL)
        return android::UNKNOWN_ERROR;    

    return mGstPlayerPipeline->getCurrentPosition(msec) ?  
        OK : android::UNKNOWN_ERROR;
}

status_t GstPlayer::getDuration(int *msec)
{
    if(mGstPlayerPipeline == NULL)
        return android::UNKNOWN_ERROR;    

    return mGstPlayerPipeline->getDuration(msec) ?  
        OK : android::UNKNOWN_ERROR;
}

status_t GstPlayer::seekTo(int msec)
{
    GST_PLAYER_DEBUG ("seekTo(%d)\n", msec);
    // can't always seek to end of streams - so we fudge a little
    if ((msec == mDuration) && (mDuration > 0)) {
        msec--;
        GST_PLAYER_DEBUG ("Seek adjusted 1 msec from end\n");
    }
    if (!mGstPlayerPipeline->seekTo(msec))
    {
        GST_PLAYER_ERROR ("Failed to seekTo() in Gstplayer Pipeline.\n");
        return android::UNKNOWN_ERROR;
    }      
    return OK;
}

status_t GstPlayer::reset()
{
    if(mGstPlayerPipeline == NULL)
        return android::UNKNOWN_ERROR;    

    return mGstPlayerPipeline->reset() ?  OK : android::UNKNOWN_ERROR;
}

status_t GstPlayer::setLooping(int loop)
{
    if(mGstPlayerPipeline == NULL)
        return android::UNKNOWN_ERROR;    

    return mGstPlayerPipeline->setLooping(loop) ?  OK : android::UNKNOWN_ERROR;
}

}; // namespace android

