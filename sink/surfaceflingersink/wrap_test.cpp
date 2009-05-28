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
 * This is a surface flinger wrapper test application
 */

#include <stdio.h>
#include "surfaceflinger_wrap.h"
#include <cutils/log.h>
#include <ui/PixelFormat.h>
using namespace android;

#define LOG_TAG "surfacewrapper_test"

#define WIDTH 320
#define HEIGHT 240
#define MAX_STEP 200


int main(int argc, char** argv)
{
    VideoFlingerDeviceHandle videodev = NULL;
    short framebuf[WIDTH*HEIGHT];
    
    LOGE("surface_test, enter\n");

    // create surface flinger
    videodev = videoflinger_device_create(NULL);
    
    // register buffer
    videoflinger_device_register_framebuffers(videodev, WIDTH, HEIGHT, VIDEO_FLINGER_RGB_565);
    
    for (int step=0; step<MAX_STEP; step++)
    {
        for (int i=0; i< WIDTH*HEIGHT; i++)
        {
            framebuf[i] = (short)(i + step*16) ;
        }
        // post buffer
        videoflinger_device_post(videodev, framebuf, WIDTH*HEIGHT*sizeof(short));
        usleep(5*1000*10);
    }
    
    // unregister buffer
    videoflinger_device_unregister_framebuffers(videodev);
    
    // free buffer
    videoflinger_device_release(videodev);
    
    LOGE("surface_test, leave\n");

    return 0;
}


