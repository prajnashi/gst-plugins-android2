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

/* Remove the full path before file name. All android code build from top
 * folder, like .../external/gst-plugin-android/player/GstPlayer.cpp, which
 * make log too long. 
 */
#define  GST_PLAYER_GET_SHORT_FILENAME(longfilename)   \
    ( (strrchr(longfilename, '/')==NULL) ? \
        longfilename: ((char*)(strrchr(longfilename, '/')+1)))

#ifdef ENABLE_GST_PLAYER_LOG

/* TODO: remove GST_PLAYER_TEMP_LOG later */
#ifdef GST_PLAYER_TEMP_LOG

/* redirect log to stdio */
#define TRACE(fmt, args... )  \
    printf("TRACE: %s:%d, %s(): "fmt, \
        GST_PLAYER_GET_SHORT_FILENAME(__FILE__), __LINE__, __FUNCTION__, ##args)

#define GST_PLAYER_ERROR(fmt, args...)      TRACE(fmt,##args)
#define GST_PLAYER_WARNING(fmt, args...)    TRACE(fmt,##args)
#define GST_PLAYER_INFO(fmt, args...)       TRACE(fmt,##args)
#define GST_PLAYER_DEBUG(fmt, args...)      TRACE(fmt,##args)
#define GST_PLAYER_LOG(fmt, args...)        TRACE(fmt,##args)

#define GST_ERROR_ANDROID(fmt, args...)     printf(fmt,##args)                         
#define GST_WARNING_ANDROID(fmt, args...)   printf(fmt,##args)                         
#define GST_INFO_ANDROID(fmt, args...)      printf(fmt,##args)                         
#define GST_DEBUG_ANDROID(fmt, args...)     printf(fmt,##args)                         
#define GST_LOG_ANDROID(fmt, args...)       printf(fmt,##args)                         

#else /* GST_PLAYER_TEMP_LOG */
#define LOG_TAG "GstPlayer"

#define GST_PLAYER_ERROR(fmt, args...)      \
    LOGE("[%d], ERROR   %s:%d, %s(): "fmt,           \
        gettid(), GST_PLAYER_GET_SHORT_FILENAME(__FILE__), __LINE__, __FUNCTION__, ##args)                         
#define GST_PLAYER_WARNING(fmt, args...)    \
    LOGW("[%d], WARNING   %s:%d, %s(): "fmt,           \
        gettid(), GST_PLAYER_GET_SHORT_FILENAME(__FILE__), __LINE__, __FUNCTION__, ##args)                             
#define GST_PLAYER_INFO(fmt, args...)       \
    LOGI("[%d], INFO   %s:%d, %s(): "fmt,           \
        gettid(), GST_PLAYER_GET_SHORT_FILENAME(__FILE__), __LINE__, __FUNCTION__, ##args)                              
#define GST_PLAYER_DEBUG(fmt, args...)      \
    LOGD("[%d], DEBUG   %s:%d, %s(): "fmt,           \
        gettid(), GST_PLAYER_GET_SHORT_FILENAME(__FILE__), __LINE__, __FUNCTION__, ##args)                              
#define GST_PLAYER_LOG(fmt, args...)        \
    LOGV("[%d], LOG   %s:%d, %s(): "fmt,           \
        gettid(), GST_PLAYER_GET_SHORT_FILENAME(__FILE__), __LINE__, __FUNCTION__, ##args)                         

#define GST_ERROR_ANDROID(fmt, args...)      LOGE(fmt,##args)                         
#define GST_WARNING_ANDROID(fmt, args...)    LOGW(fmt,##args)                         
#define GST_INFO_ANDROID(fmt, args...)       LOGI(fmt,##args)                         
#define GST_DEBUG_ANDROID(fmt, args...)      LOGD(fmt,##args)                         
#define GST_LOG_ANDROID(fmt, args...)        LOGV(fmt,##args) 
#endif

#else /* ENABLE_GST_PLAYER_LOG */

#define GST_PLAYER_ERROR(fmt, args...) 
#define GST_PLAYER_WARNING(fmt, args...) 
#define GST_PLAYER_INFO(fmt, args...)
#define GST_PLAYER_DEBUG(fmt, args...) 
#define GST_PLAYER_LOG(fmt, args...) 

#endif /* ENABLE_GST_PLAYER_LOG */


