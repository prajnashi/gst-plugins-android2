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

#ifndef _GST_PLAYER_PIPELINE_H_
#define _GST_PLAYER_PIPELINE_H_
#include <utils/Errors.h>
#include <media/mediaplayer.h>
#include <media/MediaPlayerInterface.h>
#include "GstPlayer.h"

#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <gst/gst.h>
#include <gstappsrc.h>

using namespace android;


// The class to handle gst pipeline
class GstPlayerPipeline
{
public:
    GstPlayerPipeline(GstPlayer* gstPlayer);
    ~GstPlayerPipeline();

    bool setDataSource(const char *url);
    bool setDataSource(int fd, int64_t offset, int64_t length);
    bool setAudioSink(sp<MediaPlayerInterface::AudioSink> audiosink);
    bool setVideoSurface(const sp<ISurface>& surface);
    bool prepare();
    bool prepareAsync();
    bool start();
    bool stop();
    bool pause();
    bool isPlaying();
    bool getVideoSize(int *w, int *h);
    bool seekTo(int msec);
    bool getCurrentPosition(int *msec);
    bool getDuration(int *msec);
    bool reset();
    bool setLooping(int loop);

private:
    // static apis
    static gboolean bus_callback (GstBus *bus, GstMessage *msg, gpointer data);
    static void playbin2_found_source(GObject * object, GObject * orig, 
            GParamSpec * pspec, gpointer player_pipeline);
    static void appsrc_need_data(GstAppSrc *src, guint length, 
            gpointer user_data);
    static void appsrc_enough_data(GstAppSrc *src, gpointer user_data);
    static gboolean appsrc_seek_data(GstAppSrc *src, guint64 offset,
            gpointer user_data);
    static void taglist_foreach(const GstTagList *list, const gchar *tag, 
            gpointer user_data);

    // private apis
    bool create_pipeline();
    void delete_pipeline();  

    void handleEos(GstMessage* p_msg);
    void handleError(GstMessage* p_msg);
    void handleTag(GstMessage* p_msg);
    void handleBuffering(GstMessage* p_msg);
    void handleStateChanged(GstMessage* p_msg);
    void handleAsyncDone(GstMessage* p_msg);
    void handleSegmentDone(GstMessage* p_msg);
    void handleDuration(GstMessage* p_msg);
    void handleElement(GstMessage* p_msg);
    void handleApplication(GstMessage* p_msg);

    GstPlayer*  mGstPlayer;

    // gst elements
    GstElement* mPlayBin;
    GstElement* mAudioSink;
    GstElement* mVideoSink;
    GstAppSrc* mAppSource;
    // app source 
    int mFd;
    guint64  mLength;
    guint64  mOffset;
    guint8*  mMemBase;
    // seek
    bool     mSeeking;
    GstState mSeekState;
    // prepare
    bool mAsynchPreparePending;
    // loop
    bool mIsLooping;
    // internal audio sink
    sp<MediaPlayerInterface::AudioSink> mAudioOut;
    // main loop
    GMainLoop*  mMainLoop;
    GThread*      mBusMessageThread;
    pthread_mutex_t  mActionMutex;
}; 

#endif   /*_GST_PLAYER_PIPELINE_H_*/
