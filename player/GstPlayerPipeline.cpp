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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "GstPlayerPipeline.h"



using namespace android;

// always enable gst log
#define ENABLE_GST_LOG

#define MSG_QUIT_MAINLOOP   "application/x-quit-mainloop"
#define MSG_PAUSE           "application/x-pause"

#define INIT_LOCK(pMutex)   pthread_mutex_init(pMutex, NULL)
#define LOCK(pMutex)        pthread_mutex_lock(pMutex)
#define UNLOCK(pMutex)      pthread_mutex_unlock(pMutex)
#define DELETE_LOCK(pMutex) pthread_mutex_destroy(pMutex)

#ifndef PAGESIZE
#define PAGESIZE            4096
#endif

/* TODO:
 * Tomorrow: make pipeline can start successfully. Test one audio, one video, one a+v
 */

// make sure gst can be initialized only once
static gboolean gst_inited = FALSE;
// static int gfd = 0;

// ----------------------------------------------------------------------------
// helper functions to redirect gstreamer log to android's log system
// ----------------------------------------------------------------------------
#define GST_CONFIG_ENVIRONMENT_GROUP  "Environment"
#define NUL '\0'

// get_gst_env_from_conf()
// Read and export environment variables from /sdcard/gst.conf.
//
static int get_gst_env_from_conf()
{
    gboolean res = FALSE;
    GKeyFile* conf_file = NULL;
    gchar** key_list = NULL;
    gsize length = 0;
    GError* error = NULL;
 
    // open and load GST_CONFIG_FILE
    conf_file = g_key_file_new ();
    if(conf_file == NULL)
        return -1;

    GST_PLAYER_DEBUG("Load config file: "GST_CONFIG_FILE"\n");
    res = g_key_file_load_from_file(
        conf_file,
        GST_CONFIG_FILE,
        G_KEY_FILE_NONE,
        &error);
    if(res != TRUE)
    {
        if(error)
            GST_PLAYER_ERROR ("Load config file error: %d (%s)\n", 
                error->code, error->message);
        else
            GST_PLAYER_ERROR ("Load config file error.\n");
        goto EXIT;
    }

    // enumerate all environment viarables
    error = NULL;
    key_list = g_key_file_get_keys(
        conf_file,
        GST_CONFIG_ENVIRONMENT_GROUP,
        &length,
        &error);
    if(key_list == NULL)
    {
        if(error)
            GST_PLAYER_ERROR ("Fail to get environment vars: %d (%s)\n",
                error->code, error->message);
        else
            GST_PLAYER_ERROR ("Fail to get environment vars\n");
        res = FALSE;
        goto EXIT;
    }

    // export all environment viarables
    for(gsize i=0; i<length; i++)
    {
        if(key_list[i])
        {
            gchar* key = key_list[i];
            gchar* value = g_key_file_get_string(
                conf_file,
                GST_CONFIG_ENVIRONMENT_GROUP,
                key,
                NULL);
            if(value)
            {
                setenv(key, value, 1);
                GST_PLAYER_DEBUG("setenv:  %s=%s\n", key, value);
            }
            else
            {
                unsetenv(key);
                GST_PLAYER_DEBUG("unsetsev:  %s\n", key);
            }
        }
    }

EXIT:
    // release resource
    if(conf_file)
        g_key_file_free(conf_file);
    if(key_list)
        g_strfreev(key_list);

    return (res == TRUE) ? 0 : -1;
}

// android_gst_debug_log()
// Hook function to redirect gst log from stdout to android log system
//
static void android_gst_debug_log (GstDebugCategory * category, GstDebugLevel level,
    const gchar * file, const gchar * function, gint line,
    GObject * object, GstDebugMessage * message, gpointer unused)
{
    if (level > gst_debug_category_get_threshold (category))
    return;

    const gchar *sfile = NULL;
    // Remove the full path before file name. All android code build from top
    // folder, like .../external/gst-plugin-android/player/GstPlayer.cpp, which
    // make log too long. 
    sfile  = GST_PLAYER_GET_SHORT_FILENAME(file);

    // redirect gst log to android log
    switch (level)
    {
    case GST_LEVEL_ERROR:
        GST_ERROR_ANDROID ("[%d], %s   %s:%d:%s: %s\n", 
            gettid(),
            gst_debug_level_get_name (level), 
            sfile, line, function, 
            (char *) gst_debug_message_get (message));  
        break;
    case GST_LEVEL_WARNING:
        GST_WARNING_ANDROID ("[%d], %s   %s:%d:%s: %s\n", 
            gettid(),
            gst_debug_level_get_name (level), 
            sfile, line, function, 
            (char *) gst_debug_message_get (message));  
        break;
    case GST_LEVEL_INFO:
        GST_INFO_ANDROID ("[%d], %s   %s:%d:%s: %s\n", 
            gettid(),
            gst_debug_level_get_name (level), 
            sfile, line, function, 
            (char *) gst_debug_message_get (message));  
        break;
    case GST_LEVEL_DEBUG:
        GST_DEBUG_ANDROID ("[%d], %s   %s:%d:%s: %s\n",
            gettid(),
            gst_debug_level_get_name (level), 
            sfile, line, function, 
            (char *) gst_debug_message_get (message));  
        break;
    case GST_LEVEL_LOG:
        GST_LOG_ANDROID ("[%d], %s   %s:%d:%s: %s\n", 
            gettid(),
            gst_debug_level_get_name (level), 
            sfile, line, function, 
            (char *) gst_debug_message_get (message));  
        break;
    default:
        break;
    }
}

static gboolean init_gst()
{
    // print gstreamer environment
    GST_PLAYER_DEBUG ("Check gstreamer environment variables.\n");
    GST_PLAYER_DEBUG ("GST_PLUGIN_PATH=\"%s\"\n", getenv("GST_PLUGIN_PATH"));
    GST_PLAYER_DEBUG ("LD_LIBRARY_PATH=\"%s\"\n", getenv("LD_LIBRARY_PATH"));
    GST_PLAYER_DEBUG ("GST_REGISTRY=\"%s\"\n", getenv("GST_REGISTRY"));

    if (!gst_inited)
    {
        char * argv[2];
        char**argv2;
        int argc = 0;

        GST_PLAYER_DEBUG ("Initialize gst context\n");

        if (!g_thread_supported ())
            g_thread_init (NULL);

        // read & export gst environment from gst.conf
        get_gst_env_from_conf();

        // replace gst default log handler with android
        gst_debug_add_log_function (android_gst_debug_log, NULL);

        // initialize gst
        GST_PLAYER_DEBUG ("Initializing gst\n");
        argv2 = argv;
        argv[0] = "GstPlayer";
        argv[1] = NULL;
        argc++;

        gst_init(&argc, &argv2);
        gst_debug_remove_log_function(gst_debug_log_default );

        GST_PLAYER_DEBUG ("Gst is Initialized.\n");
        gst_inited = TRUE;    
    }
    else
    {
        GST_PLAYER_DEBUG ("Gst has been initialized\n");
    }
    return TRUE;
}

// ----------------------------------------------------------------------------
// GstPlayerPipeline
// ----------------------------------------------------------------------------
// Message Handler Function
gboolean GstPlayerPipeline::bus_callback (GstBus *bus, GstMessage *msg, 
        gpointer data)
{
    GstPlayerPipeline* pGstPlayerPipeline   = (GstPlayerPipeline*) data;
    GstObject *msgsrc=GST_MESSAGE_SRC(msg);
    GST_PLAYER_DEBUG("Get %s Message From %s\n", 
        GST_MESSAGE_TYPE_NAME(msg), GST_OBJECT_NAME(msgsrc));
       
    LOCK (&pGstPlayerPipeline->mActionMutex);

    switch (GST_MESSAGE_TYPE(msg))
    {
    case GST_MESSAGE_EOS:
        pGstPlayerPipeline->handleEos(msg);           
        break;
    case GST_MESSAGE_ERROR:
        pGstPlayerPipeline->handleError(msg);
        break;
    case GST_MESSAGE_TAG:
        pGstPlayerPipeline->handleTag(msg);
        break;
    case GST_MESSAGE_BUFFERING:
        pGstPlayerPipeline->handleBuffering(msg);
        break;
    case GST_MESSAGE_STATE_CHANGED:
        pGstPlayerPipeline->handleStateChanged(msg);
        break;
    case GST_MESSAGE_ASYNC_DONE:
        pGstPlayerPipeline->handleAsyncDone(msg);
        break;
    case GST_MESSAGE_ELEMENT:
        pGstPlayerPipeline->handleElement(msg);
        break;
    case GST_MESSAGE_APPLICATION:
        pGstPlayerPipeline->handleApplication(msg);
        break;       
    case GST_MESSAGE_SEGMENT_DONE:
        pGstPlayerPipeline->handleSegmentDone(msg);
        break;
    case GST_MESSAGE_DURATION:
        pGstPlayerPipeline->handleDuration(msg);
        break;
    default:
        break;
    }
    UNLOCK (&pGstPlayerPipeline->mActionMutex);
    return TRUE;
}

void GstPlayerPipeline::appsrc_need_data(GstAppSrc *src, guint length,
        gpointer user_data)
{
    GstPlayerPipeline* player_pipeline = 
        (GstPlayerPipeline*)user_data;
    GstBuffer *buffer = NULL;
    GstFlowReturn flow_ret;
    bool ret = false;
    off_t offset = 0;

    // GST_PLAYER_DEBUG ("player_pipeline=%p, Request length=%d, offset=%lu,
    // fd=%d", player_pipeline, length, (long unsigned
    // int)(player_pipeline->mOffset), player_pipeline->mFd);

    // check current offset is inside range
    if (player_pipeline->mOffset > player_pipeline->mLength)
    {
        GST_PLAYER_WARNING("Offset %lu is outside file %lu. Send EOS\n", 
                (unsigned long)(player_pipeline->mOffset), 
                (unsigned long)(player_pipeline->mLength));
        goto EXIT;
    }
    
    // read the amount of data, we are allowed to return less if we are EOS
    if (player_pipeline->mOffset + length > player_pipeline->mLength)
        length = player_pipeline->mLength - player_pipeline->mOffset;

    // create GstBuffer
    buffer = gst_buffer_new ();
    if(buffer == NULL)
    {
        GST_PLAYER_ERROR("Cannot create buffer! Send EOS\n");
        goto EXIT;
    }

    GST_BUFFER_DATA (buffer) = player_pipeline->mMemBase + player_pipeline->mOffset;
    GST_BUFFER_SIZE (buffer) = length;
    GST_BUFFER_OFFSET (buffer) = player_pipeline->mOffset;
    GST_BUFFER_OFFSET_END (buffer) =player_pipeline->mOffset + length;

    /*
    GST_PLAYER_DEBUG("offset=%lu, length=%u, data: %02x %02x %02x %02x %02x %02x %02x %02x",
        (unsigned long)player_pipeline->mOffset,
        length,
        *(GST_BUFFER_DATA (buffer) + 0),
        *(GST_BUFFER_DATA (buffer) + 1),
        *(GST_BUFFER_DATA (buffer) + 2),
        *(GST_BUFFER_DATA (buffer) + 3),
        *(GST_BUFFER_DATA (buffer) + 4),
        *(GST_BUFFER_DATA (buffer) + 5),
        *(GST_BUFFER_DATA (buffer) + 6),
        *(GST_BUFFER_DATA (buffer) + 7));
    */

    // send buffer to appsrc
    flow_ret = gst_app_src_push_buffer(player_pipeline->mAppSource, buffer);
    if(GST_FLOW_IS_FATAL(flow_ret)) 
        GST_PLAYER_DEBUG("Push error %d\n", flow_ret);

    player_pipeline->mOffset += length;
    ret = true;

EXIT:
    // gst_app_src_push_buffer() will steal the GstBuffer's reference, we need
    // not release it here.  
    // if (buffer) gst_buffer_unref (buffer);

    if (ret != true)
        gst_app_src_end_of_stream (src);
}

void GstPlayerPipeline::appsrc_enough_data(GstAppSrc *src, 
        gpointer user_data)
{
    GST_PLAYER_WARNING ("Shall not enter here\n");
}

gboolean GstPlayerPipeline::appsrc_seek_data(GstAppSrc *src, 
        guint64 offset, gpointer user_data)
{
    GstPlayerPipeline* player_pipeline = 
        (GstPlayerPipeline*)user_data;

    // GST_PLAYER_DEBUG ("Enter, offset=%lu\n", (long unsigned int)offset);
    player_pipeline->mOffset = offset;
    return TRUE;
}

// this callback is called when playbin2 has constructed a source object to
// read from. Since we provided the appsrc:// uri to playbin2, this will be the
// appsrc that we must handle. We set up a signals to push data into appsrc. 
void GstPlayerPipeline::playbin2_found_source(GObject * object, GObject * orig, 
    GParamSpec * pspec, gpointer user_data)
{
    GstPlayerPipeline* player_pipeline = 
        (GstPlayerPipeline*)user_data;



    // get a handle to the appsrc
    if (player_pipeline->mAppSource)
    {
        g_object_unref(player_pipeline->mAppSource);
        player_pipeline->mAppSource = NULL;
    }
    g_object_get (orig, pspec->name, &(player_pipeline->mAppSource), NULL);
    GST_PLAYER_DEBUG ("appsrc: %p", player_pipeline->mAppSource);

    // we can set the length in appsrc. This allows some elements to estimate
    // the total duration of the stream. It's a good idea to set the property
    // when you can but it's not required.  
    gst_app_src_set_size(player_pipeline->mAppSource, player_pipeline->mLength);

    // configure the appsrc to work in pull (random access) mode 
    gst_app_src_set_stream_type(player_pipeline->mAppSource, 
            GST_APP_STREAM_TYPE_RANDOM_ACCESS);
    

    // set appsrc callback 
    GstAppSrcCallbacks callback;
    callback.need_data = appsrc_need_data;
    callback.enough_data = appsrc_enough_data;
    callback.seek_data = appsrc_seek_data;
    gst_app_src_set_callbacks(player_pipeline->mAppSource, &callback, 
            player_pipeline, NULL);
}

void GstPlayerPipeline::taglist_foreach(const GstTagList *list, 
            const gchar *tag, gpointer user_data)
{
    // following code is copy from gst-plugins-bad/examples/gstplay/player.c
    gint i, count;

    count = gst_tag_list_get_tag_size (list, tag);

    for (i = 0; i < count; i++) {
        gchar *str;

        if (gst_tag_get_type (tag) == G_TYPE_STRING) 
        {
            if (!gst_tag_list_get_string_index (list, tag, i, &str))
            {
                GST_PLAYER_ERROR("Cannot get tag %s\n", tag);
            } 
        }
        else 
        {
            str = g_strdup_value_contents (
                    gst_tag_list_get_value_index (list, tag, i));
        }

        if (i == 0) 
        {
            GST_PLAYER_DEBUG("%15s: %s\n", gst_tag_get_nick (tag), str);
        } 
        else 
        {
            GST_PLAYER_DEBUG("               : %s\n", str);
        }
        g_free (str);
    }
}

GstPlayerPipeline::GstPlayerPipeline(GstPlayer* gstPlayer)
{
    GST_PLAYER_DEBUG ("Enter\n");
    INIT_LOCK(&mActionMutex);

    // initilize members
    LOCK(&mActionMutex);
    mGstPlayer = gstPlayer;

    // GstElement
    mPlayBin = NULL;
    mAudioSink = NULL;
    mVideoSink = NULL;
    mAppSource = NULL;

    // app source
    mFd = 0;
    mLength = 0;
    mOffset = 0;
    mMemBase = NULL;
    
    // mainloop
    mMainLoop = NULL;
    mBusMessageThread = NULL;

    // prepare
    mAsynchPreparePending = false;    

    // seek
    mSeeking = false;
    mSeekState = GST_STATE_VOID_PENDING;

    // others
    mIsLooping = false;
    
    // initialize gst framework
    init_gst();

    // create pipeline 
    create_pipeline();

    UNLOCK(&mActionMutex); 

    GST_PLAYER_DEBUG ("Leave\n");
    return;
}

GstPlayerPipeline::~GstPlayerPipeline()
{
    GST_PLAYER_DEBUG ("Enter\n");

    LOCK (&mActionMutex);

    delete_pipeline ();

    mGstPlayer = NULL;
    UNLOCK (&mActionMutex);
    DELETE_LOCK(&mActionMutex);
    GST_PLAYER_DEBUG ("Leave\n");
}


// TODO: delete this API
// Player will delete playbin2 everytime, try no do that in the future
bool GstPlayerPipeline::create_pipeline ()
{
    GstBus *bus = NULL;

    // create playbin2
    mPlayBin = gst_element_factory_make ("playbin2", NULL);
    if(mPlayBin == NULL)
    {
        GST_PLAYER_ERROR("Failed to create playbin2\n");
        goto ERROR;
    }

    // add watch message
    mMainLoop = g_main_loop_new (NULL, FALSE);
    if (mMainLoop == NULL) 
    {
        GST_PLAYER_ERROR ("Failed to create MainLoop.\n");
        goto ERROR;
    }
    bus = gst_pipeline_get_bus (GST_PIPELINE (mPlayBin));
    g_assert (bus);
    gst_bus_add_watch (bus, bus_callback, this);
    gst_object_unref (bus);
        
    // start a thread to run main loop
    mBusMessageThread = g_thread_create ((GThreadFunc)g_main_loop_run, mMainLoop, TRUE, NULL);
    if (mBusMessageThread == NULL)
    {
        GST_PLAYER_ERROR ("Failed to create Bus Message Thread.\n");
        goto ERROR;
    }

    // FIXME: after using fakesink, there is no unref() warning message, so
    // gstaudioflinger sink shall has some bugs
    //
    //
    // create audio & video sink elements for playbin2
    mAudioSink = gst_element_factory_make("audioflingersink", NULL);
    //mAudioSink = gst_element_factory_make("fakesink", NULL);
    if (mAudioSink == NULL)
    {
        GST_PLAYER_ERROR ("Failed to create audioflingersink\n");
        goto ERROR;
    }
    g_object_set (mPlayBin, "audio-sink", mAudioSink, NULL);

    mVideoSink = gst_element_factory_make("surfaceflingersink", NULL);
    if (mVideoSink == NULL)
    {
        GST_PLAYER_ERROR ("Failed to create surfaceflingersink\n");
        goto ERROR;
    }
    g_object_set (mPlayBin, "video-sink", mAudioSink, NULL);

    GST_PLAYER_DEBUG ("Pipeline is created successfully\n");

    return true;

ERROR:
    GST_PLAYER_ERROR ("Pipeline is created failed\n");
    // release resource
    return false;
}

void  GstPlayerPipeline::delete_pipeline ()
{
    // release pipeline & main loop
    if (mPlayBin) 
    {
        // send exit message to pipeline bus post an application specific 
        // message unlock here for bus_callback to process this message
        UNLOCK (&mActionMutex);
        if(mBusMessageThread)
        {
            GST_PLAYER_DEBUG ("Send QUIT_MAINLOOP message to bus\n");
            gst_element_post_message (
                GST_ELEMENT (mPlayBin),
                gst_message_new_application (
                    GST_OBJECT (mPlayBin),
                    gst_structure_new (MSG_QUIT_MAINLOOP,
                    "message", G_TYPE_STRING, "Deleting Pipeline", NULL)));

            /* Wait for main loop to quit*/
            GST_PLAYER_DEBUG ("Wait for main loop to quit ...\n");
            g_thread_join (mBusMessageThread);
            GST_PLAYER_DEBUG("BusMessageThread joins successfully\n");
            mBusMessageThread = NULL;
        } 
        else 
        {
            GST_PLAYER_DEBUG("BusMessageThread is NULL, no need to quit\n");
        }
        LOCK (&mActionMutex);

        GST_PLAYER_DEBUG ("One pipeline exist, now delete it .\n");
        gst_element_set_state (mPlayBin, GST_STATE_NULL);
    }

    // release audio & video sink
    if (mAudioSink)
    {
        GST_PLAYER_DEBUG ("Release audio sink\n");
        gst_object_unref (mAudioSink);
        mAudioSink = NULL;
    }
    if (mVideoSink)
    {
        GST_PLAYER_DEBUG ("Release video sink\n");
        gst_object_unref (mVideoSink);
        mVideoSink = NULL;
    }
    if (mAppSource)
    {
        GST_PLAYER_DEBUG ("Release app source\n");
        gst_object_unref (mAppSource);
        mAppSource = NULL;
    }
    if(mPlayBin)
    {   
        GST_PLAYER_DEBUG ("Delete playbin2\n");
        gst_object_unref (mPlayBin);
        mPlayBin = NULL;
    }
    if (mMainLoop)
    {
        GST_PLAYER_DEBUG ("Delete mainloop\n");
        g_main_loop_unref (mMainLoop);
        mMainLoop = NULL;
    }
    if (mMemBase)
    {
        GST_PLAYER_DEBUG ("Unmap fd\n");
        munmap((void*)mMemBase, (size_t)mLength);
    }

    // app source
    mFd = 0;
    mLength = 0;
    mOffset = 0;
    mMemBase = NULL;
    // seek
    mSeeking = false;
    mSeekState = GST_STATE_VOID_PENDING;
    // prepare
    mAsynchPreparePending = false;
    

    GST_PLAYER_DEBUG ("Leave\n");
}

bool GstPlayerPipeline::setDataSource(const char *url)
{
    GST_PLAYER_DEBUG("Enter, url=%s",url);

    LOCK (&mActionMutex);
    if (mPlayBin == NULL)
    {
        GST_PLAYER_ERROR ("Pipeline not initialized\n");
        return false;
    }
    
    // set uri to playbin2 
    gchar* full_url = NULL;
    if(url[0] == '/')
    {
        // url is an absolute path, add prefix "file://"
        int len = strlen(url) + 10;
        full_url = (gchar*)malloc(len);
        if(full_url == NULL)
            return false;
        g_strlcpy(full_url, "file://", len);
        g_strlcat(full_url, url, len);
    }
    else if (g_str_has_prefix(url, "file:///"))
    {
        full_url = g_strdup(url);
        if(full_url == NULL)
            return false;        
    }
    else
    {
        GST_PLAYER_ERROR("Invalide url.");
        return false;
    }
    GST_PLAYER_DEBUG("playbin2 uri: %s", full_url);
    g_object_set (mPlayBin, "uri", full_url, NULL);
    
    if(full_url)
        free (full_url);

    UNLOCK (&mActionMutex);
    return true;
}

bool GstPlayerPipeline::setDataSource(int fd, int64_t offset, int64_t length)
{
    // URI: appsrc://...  
    //
    // Example:
    // external/gst-plugins-base/tests/examples/app/appsrc-stream2.c
    // gst-plugins-base/tests/examples/app/appsrc-ra.c
    if (mPlayBin == NULL)
    {
        GST_PLAYER_ERROR ("Pipeline not initialized\n");
        return false;
    }  
    // TODO: reset player here
    if (fd == 0)
    {
        GST_PLAYER_ERROR ("Invalid fd: %d\n", fd);
        return false;
    }
    struct stat stat_buf;
    if( fstat(fd, &stat_buf) != 0)
    {
        GST_PLAYER_ERROR ("Cannot get file size\n");
        return false;
    }
    mFd = fd;
    mLength = stat_buf.st_size; 
    mOffset = 0;

    // map the file into memory
    mMemBase = (guint8*)mmap(0, (size_t)mLength, PROT_READ, MAP_PRIVATE, fd, 0);
    GST_PLAYER_DEBUG("playbin2 uri: appsrc://, fd: %d, length: %lu, mMemBase: %p", 
            mFd, (unsigned long int)mLength, mMemBase);

    // use appsrc in playbin2
    g_object_set (mPlayBin, "uri", "appsrc://", NULL);

    // get notification when the source is created so that we get a handle to
    // it and can configure it
    g_signal_connect (mPlayBin, "deep-notify::source", (GCallback)
            playbin2_found_source, this);

    return true;
}

bool GstPlayerPipeline::setAudioSink(sp<MediaPlayerInterface::AudioSink> audiosink)
{
    if(audiosink == 0)
    {
        GST_PLAYER_ERROR("Error audio sink %p", audiosink.get());
        return false;
    }

    if (!mPlayBin) 
    {
        GST_PLAYER_ERROR ("Pipeline not initialized\n");
        return false;
    }

    if(mAudioOut != 0)
    {
        mAudioOut.clear();
        mAudioOut = 0;
    }
    mAudioOut = audiosink;

    // set AudioSink
    GST_PLAYER_DEBUG("MediaPlayerInterface::AudioSink: %p\n", mAudioOut.get());
    g_object_set (mAudioSink, "audiosink", mAudioOut.get(), NULL);

    return true; 
}

bool GstPlayerPipeline::setVideoSurface(const sp<ISurface>& surface)
{
    GST_PLAYER_ERROR ("Not implemented yet!\n");
    LOCK (&mActionMutex);
      
    // TODO: add code here
    UNLOCK (&mActionMutex);

    return FALSE;
}

bool GstPlayerPipeline::prepare()
{
    bool ret = false;
    GstStateChangeReturn state_return;
       
    LOCK (&mActionMutex);
    if (!mPlayBin) 
    {
        GST_PLAYER_ERROR ("Pipeline not initialized\n");
        goto EXIT;
    }  
    GST_PLAYER_LOG("Enter\n"); 
    state_return = gst_element_set_state (mPlayBin, GST_STATE_PAUSED);
    GST_PLAYER_LOG("state_return = %d\n", state_return);
    if (state_return == GST_STATE_CHANGE_FAILURE) 
    {
        GST_PLAYER_ERROR ("Fail to set pipeline to PAUSED\n");
        goto EXIT;
    }
    else if (state_return == GST_STATE_CHANGE_ASYNC)
    {
        GstState state;

        // wait for state change complete
        GST_PLAYER_DEBUG("Wait for pipeline's state change to PAUSE...\n");
        state_return = gst_element_get_state (mPlayBin, &state, NULL, GST_CLOCK_TIME_NONE);
        GST_PLAYER_DEBUG("Pipeline's state change to PAUSE\n");
        if (state_return != GST_STATE_CHANGE_SUCCESS || state != GST_STATE_PAUSED ) 
        {
            GST_PLAYER_ERROR ("Fail to set pipeline to PAUSED\n");
            goto EXIT;
        }
    }

    // send prepare done message
    if (mGstPlayer)
        mGstPlayer->sendEvent(MEDIA_PREPARED);

    ret = true;

EXIT:
    UNLOCK (&mActionMutex);
    return ret;
}

bool GstPlayerPipeline::prepareAsync()
{
    bool ret = false;
    GstStateChangeReturn state_return;
       
    LOCK (&mActionMutex);
    if (!mPlayBin) 
    {
        GST_PLAYER_ERROR ("Pipeline not initialized\n");
        goto EXIT;
    }  
    
    GST_PLAYER_DEBUG("Enter\n"); 
    state_return = gst_element_set_state (mPlayBin, GST_STATE_PAUSED);
    GST_PLAYER_DEBUG("state_return = %d\n", state_return);
    if (state_return == GST_STATE_CHANGE_FAILURE) 
    {
        GST_PLAYER_ERROR ("Fail to set pipeline to PAUSED\n");
        goto EXIT;
    }

    // wait for state change complete in message loop
    mAsynchPreparePending = true;
    GST_PLAYER_DEBUG("Wait for pipeline's state change to PAUSE...\n");
    
    ret = true;
EXIT:
    UNLOCK (&mActionMutex);
    return ret;
}


bool GstPlayerPipeline::start()
{
    bool ret = false;
    GstStateChangeReturn state_return;
       
    LOCK (&mActionMutex);
    if (!mPlayBin) 
    { 
        GST_PLAYER_ERROR ("Pipeline not initialized\n");
        goto EXIT;
    }  
    
    GST_PLAYER_DEBUG("Enter\n"); 
    state_return = gst_element_set_state (mPlayBin, GST_STATE_PLAYING);
    GST_PLAYER_DEBUG("state_return = %d\n", state_return);

    if (state_return == GST_STATE_CHANGE_FAILURE) 
    {
        GST_PLAYER_ERROR ("Fail to set pipeline to PLAYING\n");
        goto EXIT;
    }
    else if (state_return == GST_STATE_CHANGE_ASYNC)
    {
        GstState state;

        // wait for state change complete
        GST_PLAYER_DEBUG("Wait for pipeline's state change to PLAYING...\n");
        state_return = gst_element_get_state (mPlayBin, &state, NULL, GST_CLOCK_TIME_NONE);
        GST_PLAYER_DEBUG("Pipeline's state change to PLAYING\n");
        if (state_return != GST_STATE_CHANGE_SUCCESS || state != GST_STATE_PLAYING ) 
        {
            GST_PLAYER_ERROR ("Fail to set pipeline to PLAYING\n");
            goto EXIT;
        }
    }    

    ret = true;

EXIT:
    UNLOCK (&mActionMutex);
    return ret;
}

bool GstPlayerPipeline::stop()
{
    bool ret = false;
    GstStateChangeReturn state_return;
       
    LOCK (&mActionMutex);
    if (!mPlayBin) 
    { 
        GST_PLAYER_ERROR ("Pipeline not initialized\n");
        goto EXIT;
    }  

    GST_PLAYER_DEBUG("Enter\n"); 
    state_return = gst_element_set_state (mPlayBin, GST_STATE_READY);
    GST_PLAYER_DEBUG("state_return = %d\n", state_return);

    if (state_return == GST_STATE_CHANGE_FAILURE) 
    {
        GST_PLAYER_ERROR ("Fail to set pipeline to PLAYING\n");
        goto EXIT;
    }
    else if (state_return == GST_STATE_CHANGE_ASYNC)
    {
        GstState state;

        // wait for state change complete
        GST_PLAYER_DEBUG("Wait for pipeline's state change to READY...\n");
        state_return = gst_element_get_state (mPlayBin, &state, NULL, GST_CLOCK_TIME_NONE);
        GST_PLAYER_DEBUG("Pipeline's state change to READY\n");
        if (state_return != GST_STATE_CHANGE_SUCCESS || state != GST_STATE_READY ) 
        {
            GST_PLAYER_ERROR ("Fail to set pipeline to READY\n");
            goto EXIT;
        }
    }    
    
    // seek
    if(mSeeking)
    {
         GST_PLAYER_DEBUG("Stop playback while seeking. Send MEDIA_SEEK_COMPLETE immediately");
         if(mGstPlayer)
             mGstPlayer->sendEvent(MEDIA_SEEK_COMPLETE);
    }
    mSeeking = false;
    mSeekState = GST_STATE_VOID_PENDING;
    
    // prepare
    if(mAsynchPreparePending)
    {
        GST_PLAYER_WARNING("mAsynchPreparePending shall not be true!");
    }
    mAsynchPreparePending = false;

    ret = true;

EXIT:
    UNLOCK (&mActionMutex);
    return ret;
}

bool  GstPlayerPipeline::pause()
{
    bool ret = false;
    GstStateChangeReturn state_return;
       
    LOCK (&mActionMutex);
    if (!mPlayBin) 
    {
        GST_PLAYER_ERROR ("Pipeline not initialized\n");
        goto EXIT;
    }  
    GST_PLAYER_DEBUG("Enter\n"); 

    if(mSeeking)
    {
        GST_PLAYER_DEBUG("Pause in seeking, switch seek pending state to pause\n"); 
        mSeekState = GST_STATE_PAUSED;
    }

    state_return = gst_element_set_state (mPlayBin, GST_STATE_PAUSED);
    GST_PLAYER_DEBUG("state_return = %d\n", state_return);
    if (state_return == GST_STATE_CHANGE_FAILURE) 
    {
        GST_PLAYER_ERROR ("Fail to set pipeline to PAUSED\n");
        goto EXIT;
    }
    else if (state_return == GST_STATE_CHANGE_ASYNC)
    {
        GstState state;

        // wait for state change complete
        GST_PLAYER_DEBUG("Wait for pipeline's state change to PAUSE...\n");
        state_return = gst_element_get_state (mPlayBin, &state, NULL, GST_CLOCK_TIME_NONE);
        GST_PLAYER_DEBUG("Pipeline's state change to PAUSE\n");
        if (state_return != GST_STATE_CHANGE_SUCCESS || state != GST_STATE_PAUSED ) 
        {
            GST_PLAYER_ERROR ("Fail to set pipeline to PAUSED\n");
            goto EXIT;
        }
    }
    ret = true;

EXIT:
    UNLOCK (&mActionMutex);
    return ret;
}

bool GstPlayerPipeline::isPlaying()
{
    GstState state, pending;
    bool ret = false;
       
    LOCK (&mActionMutex);
    if (!mPlayBin) 
    { 
        GST_PLAYER_ERROR ("Pipeline not initialized\n");
        goto EXIT;
    }

    gst_element_get_state (mPlayBin, &state, &pending, GST_CLOCK_TIME_NONE);
   
    GST_PLAYER_DEBUG("state: %d, pending: %d, seeking: %d", state, pending, mSeeking); 

    // we shall return true if, we are playing or we are seeking in playback
    if (state == GST_STATE_PLAYING || 
            ( mSeeking == true && mSeekState == GST_STATE_PLAYING))
        ret = true;

    GST_PLAYER_DEBUG("playing: %d", ret); 

EXIT:    
    UNLOCK (&mActionMutex);       
    return ret;
}

bool GstPlayerPipeline::getVideoSize(int *w, int *h)
{
    // TODO: implement it
    GST_PLAYER_ERROR ("Not implemented yet!\n");
    return  false;    
}


bool  GstPlayerPipeline::seekTo(int msec)
{
    GstState state, pending;
    bool ret = false;

    gint64 seek_pos = (gint64)msec * GST_MSECOND;
       
    LOCK (&mActionMutex);
    if (!mPlayBin) 
    { 
        GST_PLAYER_ERROR ("Pipeline not initialized\n");
        goto EXIT;
    }

    GST_PLAYER_DEBUG("Enter, seek to: %d\n", msec);

    // get current stable state
    gst_element_get_state (mPlayBin, &state, &pending, GST_CLOCK_TIME_NONE);
    GST_PLAYER_DEBUG("state: %d, pending: %d", state, pending); 

    if (gst_element_seek_simple(mPlayBin, GST_FORMAT_TIME, 
        (GstSeekFlags)(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT), seek_pos) != TRUE)
    {
        GST_PLAYER_ERROR ("Fail to seek to position %d\n", msec);
    }
    else
    {
        GST_PLAYER_DEBUG ("Seek to %d\n", msec);
        mSeeking = true;
        mSeekState = state;
        ret = true;
    }

EXIT:    
    UNLOCK (&mActionMutex);       
    return ret;
}

bool GstPlayerPipeline::getCurrentPosition(int *msec)
{
    GstFormat  fmt = GST_FORMAT_TIME;
    gint64  pos;
    bool ret = false;
    GstFormat format = GST_FORMAT_TIME;
    
    GST_PLAYER_DEBUG("enter"); 
    LOCK (&mActionMutex);

    *msec = 0;
    if (!mPlayBin || msec == NULL) 
    { 
        GST_PLAYER_ERROR ("Pipeline not initialized\n");
        goto EXIT;
    }
    if (gst_element_query_position(mPlayBin, &format, &pos) != TRUE 
            || format != GST_FORMAT_TIME)
        GST_PLAYER_ERROR ("Fail to get current position\n");
    else
    {
        *msec = (int)(pos / GST_MSECOND);
        GST_PLAYER_DEBUG ("Current position: %d\n", *msec);
        ret = true;
    }

EXIT:    
    UNLOCK (&mActionMutex);
    return ret;
}

bool GstPlayerPipeline::getDuration(int *msec)
{  
    GstFormat  fmt = GST_FORMAT_TIME;
    gint64  dur;
    bool ret = false;
    GstFormat format = GST_FORMAT_TIME;
    
    GST_PLAYER_DEBUG("enter"); 
    LOCK (&mActionMutex);

    *msec = 0;
    if (!mPlayBin || msec == NULL) 
    { 
        GST_PLAYER_ERROR ("Pipeline not initialized\n");
        goto EXIT;
    }
    if (gst_element_query_duration(mPlayBin, &format, &dur) != TRUE 
            || format != GST_FORMAT_TIME)
        GST_PLAYER_ERROR ("Fail to get duration\n");
    else
    {
        *msec = (int)(dur / GST_MSECOND);
        GST_PLAYER_DEBUG ("Duration: %d\n", *msec);
        ret = true;
    }

EXIT:    
    UNLOCK (&mActionMutex);
    return ret;
}

bool GstPlayerPipeline::reset()
{
    GST_PLAYER_DEBUG ("Enter\n");
    
    // stop playback
    stop();
    return true;
}

bool GstPlayerPipeline::setLooping(int loop)
{
    GST_PLAYER_DEBUG  ("setLooping (%s)\n", (loop!=0)?"TRUE":"FALSE");
    LOCK (&mActionMutex);
    mIsLooping = (loop != 0) ? true : false;
    UNLOCK (&mActionMutex);
    return true;
}

void GstPlayerPipeline::handleEos(GstMessage* p_msg)
{
    GST_PLAYER_DEBUG ("Recevied EOS.\n");

    if (mIsLooping) 
    {
        GST_PLAYER_DEBUG ("Loop is set, start playback again\n");
        start();
    } 
    else 
    {
        GST_PLAYER_DEBUG ("send MEDIA_PLAYBACK_COMPLETE event.\n");
        if(mGstPlayer)
            mGstPlayer->sendEvent(MEDIA_PLAYBACK_COMPLETE);
    }    
}

void GstPlayerPipeline::handleError(GstMessage* p_msg)
{
    gchar *debug=NULL;
    GError *err=NULL;

    gst_message_parse_error(p_msg, &err, &debug);
    GST_PLAYER_DEBUG("%s error(%d): %s debug: %s\n", g_quark_to_string(err->domain), err->code, err->message, debug);

    if (mGstPlayer)
        mGstPlayer->sendEvent(MEDIA_ERROR, err->code); 

    g_free(debug);
    g_error_free(err);
}

void GstPlayerPipeline::handleTag(GstMessage* p_msg)
{
    GstTagList* taglist=NULL;

    GST_PLAYER_DEBUG ("Enter\n");
    gst_message_parse_tag(p_msg, &taglist);
    // enumerate each tag
    gst_tag_list_foreach(taglist, taglist_foreach, this);

    gst_tag_list_free(taglist);
}

void GstPlayerPipeline::handleBuffering(GstMessage* p_msg)
{
    gint percent = 0;
    gst_message_parse_buffering (p_msg, &percent);

    GST_PLAYER_DEBUG("Buffering: %d", percent); 
    if (mGstPlayer)
        mGstPlayer->sendEvent(MEDIA_BUFFERING_UPDATE, (unsigned int)percent); 
}

void GstPlayerPipeline::handleStateChanged(GstMessage* p_msg)
{
    GstState newstate = GST_STATE_VOID_PENDING;
    GstState oldstate = GST_STATE_VOID_PENDING;
    GstState pending  = GST_STATE_VOID_PENDING;
    GstElement *msgsrc = (GstElement *)GST_MESSAGE_SRC(p_msg);

    gst_message_parse_state_changed(p_msg, &oldstate, &newstate, &pending);

    GST_PLAYER_DEBUG("State changed message: %s, old-%d, new-%d, pending-%d\n", GST_ELEMENT_NAME(msgsrc),
         (int)oldstate, (int)newstate, (int)pending);

    // prepareAsync
    if (mAsynchPreparePending == true && msgsrc == mPlayBin && 
            newstate == GST_STATE_PAUSED && 
            (pending == GST_STATE_VOID_PENDING || pending == GST_STATE_PLAYING) )
    {
        GST_PLAYER_DEBUG("prepareAsynch() done, send MEDIA_PREPARED event\n");
        mAsynchPreparePending = false;
        if (mGstPlayer)
            mGstPlayer->sendEvent(MEDIA_PREPARED);
    }   

    // seekTo
    if (mSeeking == true && msgsrc == mPlayBin && newstate == mSeekState &&
            (pending == GST_STATE_VOID_PENDING))
    {
        GST_PLAYER_DEBUG("seekTo() done, send MEDIA_SEEK_COMPLETE event\n");
        mSeeking = false;
        mSeekState = GST_STATE_VOID_PENDING;
        if (mGstPlayer)
            mGstPlayer->sendEvent(MEDIA_SEEK_COMPLETE);
    }
}

void GstPlayerPipeline::handleAsyncDone(GstMessage* p_msg)
{
    GST_PLAYER_DEBUG("Enter");
}

void GstPlayerPipeline::handleSegmentDone(GstMessage* p_msg)
{
    gint64 position = 0;
    GstFormat format = GST_FORMAT_TIME;
    gst_message_parse_segment_done(p_msg, &format, &position);
    GST_PLAYER_DEBUG ("Position: %d\n", (int)(position / GST_MSECOND));
}

void GstPlayerPipeline::handleDuration(GstMessage* p_msg)
{
    gint64 duration = 0;
    GstFormat format = GST_FORMAT_TIME;
    gst_message_parse_duration(p_msg, &format, &duration);
    GST_PLAYER_DEBUG ("Duration: %d\n", (int)(duration / GST_MSECOND));
}

void GstPlayerPipeline::handleElement(GstMessage* p_msg)
{
    const GstStructure* pStru = gst_message_get_structure(p_msg);
    const gchar* name = gst_structure_get_name(pStru);
    GstElement *msgsrc = (GstElement *)GST_MESSAGE_SRC(p_msg);

    GST_PLAYER_DEBUG("message string: %s\n", name);
    if (strcmp(name, "application/x-contacting") == 0)
    {
        //onContacting();
    }
    else if (strcmp(name, "application/x-contacted") == 0)
    {
        //onContacted();
    }
    else if (strcmp(name, "application/x-link-attempting") == 0)
    {
        //onLinkAttempting(msgsrc);
    }
    else if (strcmp(name, "application/x-link-attempted") == 0)
    {
        //onLinkAttempted(msgsrc);
    }
    else if (strcmp(name, "application/x-download-completed") == 0)
    {
        // onDownloadCompleted();
    }
}

void GstPlayerPipeline::handleApplication(GstMessage* p_msg)
{
    const GstStructure* pStru = gst_message_get_structure(p_msg);
    const gchar* name = gst_structure_get_name(pStru);
    GstElement *msgsrc = (GstElement *)GST_MESSAGE_SRC(p_msg);

    GST_PLAYER_DEBUG("message string: %s\n", name);
    if (strcmp(name,  MSG_QUIT_MAINLOOP) == 0)
    {
        GST_PLAYER_DEBUG ("Received QUIT_MAINLOOP message.\n");
        g_main_loop_quit (mMainLoop);
    }
}
