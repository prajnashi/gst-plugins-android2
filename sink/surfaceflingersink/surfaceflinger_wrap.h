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
 * This file defines APIs to convert C++ ISurface 
 * interface to C interface
 */
#ifndef __SURFACE_FLINGER_WRAP_H__
#define  __SURFACE_FLINGER_WRAP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <grp.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>

typedef void* VideoFlingerDeviceHandle;

typedef enum
{
    VIDEO_FLINGER_RGB_565 = 1,
    VIDEO_FLINGER_RGB_888 = 2,
} VIDEO_FLINGER_PIXEL_FORMAT;

VideoFlingerDeviceHandle videoflinger_device_create(void * isurface);
int videoflinger_device_release(VideoFlingerDeviceHandle handle);
int videoflinger_device_register_framebuffers(VideoFlingerDeviceHandle handle, int w, int h, VIDEO_FLINGER_PIXEL_FORMAT format);
void videoflinger_device_unregister_framebuffers(VideoFlingerDeviceHandle handle);
void videoflinger_device_post(VideoFlingerDeviceHandle handle, void * buf, int bufsize);

#ifdef __cplusplus
}
#endif

#endif /*__SURFACE_FLINGER_WRAP_H__*/
