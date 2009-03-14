LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

GST_MAJORMINOR:= 0.10

LOCAL_SRC_FILES:= \
	audioflinger_wrapper.cpp \
	gstaudioflingersink.c         
	
LOCAL_SHARED_LIBRARIES := \
    libgstreamer-0.10       \
    libglib-2.0             \
    libgthread-2.0          \
    libgmodule-2.0          \
    libgobject-2.0          \
    libcutils               \
    libutils                \
    libaudioflinger         \
    libmediaplayerservice   \
    libmedia                \
    libgstbase-0.10         \
	libgstaudio-0.10

LOCAL_MODULE:= libgstaudioflingersink

LOCAL_TOP_PATH := $(LOCAL_PATH)/../../..
ANDROID_BASE := $(LOCAL_PATH)/../../../../..

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)   \
    $(LOCAL_TOP_PATH)/gstreamer       \
    $(LOCAL_TOP_PATH)/gstreamer/android      \
    $(LOCAL_TOP_PATH)/gstreamer/gst	\
    $(LOCAL_TOP_PATH)/gstreamer/gst/android	\
    $(LOCAL_TOP_PATH)/gstreamer/libs \
    $(LOCAL_TOP_PATH)/glib   \
    $(LOCAL_TOP_PATH)/glib/android   \
    $(LOCAL_TOP_PATH)/glib/glib   \
    $(LOCAL_TOP_PATH)/glib/gmodule   \
    $(LOCAL_TOP_PATH)/glib/gobject  \
    $(LOCAL_TOP_PATH)/glib/gthread  \
    $(LOCAL_TOP_PATH)/gstreamer/libs/gst/base  \
    $(LOCAL_TOP_PATH)/gst-plugins-android/servicegluelayer/gstaudioflinger \
    $(LOCAL_TOP_PATH)/gst-plugins-base/gst-libs/gst/audio  \
	$(LOCAL_TOP_PATH)/gst-plugins-base/gst-libs \
    $(ANDROID_BASE)/frameworks/base/libs/audioflinger \
    $(ANDROID_BASE)/frameworks/base/media/libmediaplayerservice \
    $(ANDROID_BASE)/frameworks/base/media/libmedia


LOCAL_CFLAGS := \
	-DHAVE_CONFIG_H			

include $(BUILD_SHARED_LIBRARY)
