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

#ifndef ANDROID_GSTPLAYER_H
#define ANDROID_GSTPLAYER_H

#include <utils/Errors.h>
#include <media/MediaPlayerInterface.h>

#define GST_CONFIG_FILE "/sdcard/gst.conf"

class GstPlayerPipeline;

namespace android {

// GstPlayer
// This is the wrapper layer to integrate gst player into android media 
// player service. To do that, we change
//  .../frameworks/base/media/libmediaplayerservice/MediaPlayerService.cpp 
// to load gstplayer or pvplayer by using a config file (gst_conf).
//
class GstPlayer : public MediaPlayerInterface
{
public:
    GstPlayer();
    virtual             ~GstPlayer();

    virtual status_t    initCheck();
    virtual status_t    setAudioStreamType(int type);
    virtual status_t    setDataSource(const char *url);
    virtual status_t    setDataSource(int fd, int64_t offset, int64_t length);
    virtual status_t    setVideoSurface(const sp<ISurface>& surface);
    virtual status_t    prepare();
    virtual status_t    prepareAsync();
    virtual status_t    start();
    virtual status_t    stop();
    virtual status_t    pause();
    virtual bool        isPlaying();
    virtual status_t    getVideoWidth(int *w);
    virtual status_t    getVideoHeight(int *h);
    virtual status_t    seekTo(int msec);
    virtual status_t    getCurrentPosition(int *msec);
    virtual status_t    getDuration(int *msec);
    virtual status_t    reset();
    virtual status_t    setLooping(int loop);
    virtual player_type playerType() { return GST_PLAYER; }

    // make available to GstPlayerPipeline
    void sendEvent(int msg, int ext1=0, int ext2=0) { MediaPlayerBase::sendEvent(msg, ext1, ext2); }

private:

    GstPlayerPipeline*   mGstPlayerPipeline;
    char *               mDataSourcePath;
    bool                 mIsDataSourceSet;
    sp<ISurface>         mSurface;
    status_t             mInit;
    int                  mDuration;
    int                  mStreamType;
};

}; // namespace android

#endif // ANDROID_GSTPLAYER_H
