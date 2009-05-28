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
 
#define LOG_TAG "surfacewrapper"
#include <ui/ISurface.h>
#include <ui/Surface.h>
#include <ui/SurfaceComposerClient.h>
#include <utils/MemoryBase.h>
#include <utils/MemoryHeapBase.h>
#include <cutils/log.h>
#include "surfaceflinger_wrap.h"

using namespace android;

typedef struct 
{
    sp<MemoryHeapBase> frame_heap;
    sp<ISurface> isurface;
    sp<Surface> surface;
    int32_t hor_stride;
    int32_t ver_stride;
    uint32_t width;
    uint32_t height;
    PixelFormat format;
    int frame_offset[2];
    int buf_index;
} VideoFlingerDevice;

/* max frame buffers */
#define  MAX_FRAME_BUFFERS     2

/* map wrapper layer's log to android's log system */
static const char* short_file_name = "surfaceflinger_wrap.cpp";
  

#define SF_ERROR(fmt, args...) \
    LOGE("%s, %s, (%d): "fmt, short_file_name, __FUNCTION__,  __LINE__, ##args)
#define SF_WARNING(fmt, args...) \
    LOGW("%s, %s, (%d): "fmt, short_file_name, __FUNCTION__,  __LINE__, ##args)
#define  SF_INFO(fmt, args...) \
    LOGI("%s, %s, (%d): "fmt, short_file_name, __FUNCTION__,  __LINE__, ##args)
#define  SF_DEBUG(fmt, args...) \
    LOGD("%s, %s, (%d): "fmt, short_file_name, __FUNCTION__,  __LINE__, ##args)
#define  SF_LOG(fmt, args...)  \
    LOGV("%s, %s, (%d): "fmt, short_file_name, __FUNCTION__,  __LINE__, ##args)


static int videoflinger_device_create_new_surface(VideoFlingerDevice* videodev);

/* 
 * The only purpose of class "MediaPlayer" is to call Surface::getISurface()
 * in frameworks/base/include/ui/Surface.h, which is private function and accessed
 * by friend class MediaPlayer.
 *
 * We define a fake one to cheat compiler
 */
namespace android {
class MediaPlayer
{
public:
    static sp<ISurface> getSurface(const Surface* surface)
    {
        return surface->getISurface();
    };
};
};


VideoFlingerDeviceHandle videoflinger_device_create(void* isurface)
{
    VideoFlingerDevice *videodev = NULL;

    SF_INFO("Enter\n");
    videodev = new VideoFlingerDevice;
    if (videodev == NULL)
    {
        return NULL;
    }
    videodev->frame_heap.clear();
    videodev->isurface = (ISurface*)isurface;
    videodev->surface.clear();
    videodev->format = -1;
    videodev->width = 0;
    videodev->height = 0;
    videodev->hor_stride = 0;
    videodev->ver_stride = 0;
    videodev->buf_index = 0;
    for ( int i = 0; i<MAX_FRAME_BUFFERS; i++)
    {
        videodev->frame_offset[i] = 0;
    }

    SF_INFO("Leave\n");
    return (VideoFlingerDeviceHandle)videodev;    
}



int videoflinger_device_create_new_surface(VideoFlingerDevice* videodev)
{
    status_t state;
    int pid = getpid();

    SF_INFO("Enter\n");

    /* Create a new Surface object with 320x240
     * TODO: Create surface according to device's screen size and rotate it
     * 90, but not a pre-defined value.
     */
    sp<SurfaceComposerClient> videoClient = new SurfaceComposerClient;
    if (videoClient.get() == NULL)
    {
        SF_ERROR("Fail to create SurfaceComposerClient\n");
        return -1;
    }

    /* release privious surface */
    videodev->surface.clear();
    videodev->isurface.clear();

    int width = (videodev->width) > 320 ? 320 : videodev->width;
    int height = (videodev->height) > 320 ? 320 : videodev->height;

    videodev->surface = videoClient->createSurface(
            pid, 
            0, 
            width, 
            height, 
            PIXEL_FORMAT_RGB_565, 
            ISurfaceComposer::eFXSurfaceNormal|ISurfaceComposer::ePushBuffers);
    if (videodev->surface.get() == NULL)
    {
        SF_ERROR("Fail to create Surface\n");
        return -1;
    }

    videoClient->openTransaction();

    /* set Surface toppest z-order, this will bypass all isurface created 
     * in java side and make sure this surface displaied in toppest */
    state =  videodev->surface->setLayer(INT_MAX);
    if (state != NO_ERROR)
    {
        SF_INFO("videoSurface->setLayer(), state = %d", state);
        videodev->surface.clear();
        return -1;
    }

    /* show surface */
    state =  videodev->surface->show();
    state =  videodev->surface->setLayer(INT_MAX);
    if (state != NO_ERROR)
    {
        SF_INFO("videoSurface->show(), state = %d", state);
        videodev->surface.clear();
        return -1;
    }

    /* get ISurface interface */
    videodev->isurface = MediaPlayer::getSurface(videodev->surface.get());

    videoClient->closeTransaction();

    /* Smart pointer videoClient shall be deleted automatically
     * when function exists
     */
    SF_INFO("Leave\n");
    return 0;
}

int videoflinger_device_release(VideoFlingerDeviceHandle handle)
{
    SF_INFO("Enter");
    
    if (handle == NULL)
    {
        return -1;
    }
    
    /* unregister frame buffer */
    videoflinger_device_unregister_framebuffers(handle); 

    /* release ISurface & Surface */
    VideoFlingerDevice *videodev = (VideoFlingerDevice*)handle;
    videodev->isurface.clear();
    videodev->surface.clear();

    /* delete device */
    delete videodev;

    SF_INFO("Leave");
    return 0;
}

int videoflinger_device_register_framebuffers(VideoFlingerDeviceHandle handle, 
    int w, int h, VIDEO_FLINGER_PIXEL_FORMAT format)
{
    int surface_format = 0;

    SF_INFO("Enter");
    if (handle == NULL)
    {
        SF_ERROR("videodev is NULL");
        return -1;
    }
   
    /* TODO: Now, only PIXEL_FORMAT_RGB_565 is supported. Change here to support
     * more pixel type
     */
    if (format !=  VIDEO_FLINGER_RGB_565 )
    {
        SF_ERROR("Unsupport format: %d", format);
        return -1;
    }
    surface_format = PIXEL_FORMAT_RGB_565;
   
    VideoFlingerDevice *videodev = (VideoFlingerDevice*)handle;
    /* unregister previous buffers */
    if (videodev->frame_heap.get())
    {
        videoflinger_device_unregister_framebuffers(handle);
    }

    /* reset framebuffers */
    videodev->format = surface_format;
    videodev->width = (w + 1) & -2;
    videodev->height = (h + 1) & -2;
    videodev->hor_stride = videodev->width;
    videodev->ver_stride =  videodev->height;
    
    /* create isurface internally, if no ISurface interface input */
    if (videodev->isurface.get() == NULL)
    {
        videoflinger_device_create_new_surface(videodev);
    }

    /* use double buffer in post */
    int frameSize = videodev->width * videodev->height * 2;
    SF_INFO( 
        "format=%d, width=%d, height=%d, hor_stride=%d, ver_stride=%d, frameSize=%d",
        videodev->format,
        videodev->width,
        videodev->height,
        videodev->hor_stride,
        videodev->ver_stride,
        frameSize);

    /* create frame buffer heap base */
    videodev->frame_heap = new MemoryHeapBase(frameSize * MAX_FRAME_BUFFERS);
    if (videodev->frame_heap->heapID() < 0) 
    {
        SF_ERROR("Error creating frame buffer heap!");
        return -1;
    }

    /* create frame buffer heap and register with surfaceflinger */
    ISurface::BufferHeap buffers(
        videodev->width,
        videodev->height,
        videodev->hor_stride,
        videodev->ver_stride,
        videodev->format,
        videodev->frame_heap);

    if (videodev->isurface->registerBuffers(buffers) < 0 )
    {
        SF_ERROR("Cannot register frame buffer!");
        videodev->frame_heap.clear();
        return -1;
    }

    for ( int i = 0; i<MAX_FRAME_BUFFERS; i++)
    {
        videodev->frame_offset[i] = i*frameSize;
    }
    videodev->buf_index = 0;
    SF_INFO("Leave");

    return 0;
}

void videoflinger_device_unregister_framebuffers(VideoFlingerDeviceHandle handle)
{
    SF_INFO("Enter");

    if (handle == NULL)
    {
        return;
    }

    VideoFlingerDevice* videodev = (VideoFlingerDevice*)handle;
    if (videodev->frame_heap.get())
    {
        SF_INFO("Unregister frame buffers.  videodev->isurface = %p", videodev->isurface.get());
        
        /* release ISurface */
        SF_INFO("Unregister frame buffer");
        videodev->isurface->unregisterBuffers();

        /* release MemoryHeapBase */
        SF_INFO("Clear frame buffers.");
        videodev->frame_heap.clear();

        /* reset offset */
        for (int i = 0; i<MAX_FRAME_BUFFERS; i++)
        {
            videodev->frame_offset[i] = 0;
        }
            
        videodev->format = -1;
        videodev->width = 0;
        videodev->height = 0;
        videodev->hor_stride = 0;
        videodev->ver_stride = 0;
        videodev->buf_index = 0;
    }

    SF_INFO("Leave");
}

void videoflinger_device_post(VideoFlingerDeviceHandle handle, void * buf, int bufsize)
{
    SF_INFO("Enter");
    
    if (handle == NULL)
    {
        return;
    }

    VideoFlingerDevice* videodev = (VideoFlingerDevice*)handle;
    
    if (++videodev->buf_index == MAX_FRAME_BUFFERS) 
        videodev->buf_index = 0;
   
    memcpy (static_cast<unsigned char *>(videodev->frame_heap->base()) + videodev->frame_offset[videodev->buf_index],  buf, bufsize);

    SF_INFO ("Post buffer[%d].\n", videodev->buf_index);
    videodev->isurface->postBuffer(videodev->frame_offset[videodev->buf_index]);

    SF_INFO("Leave");
}
