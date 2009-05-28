LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_TOP_PATH := $(LOCAL_PATH)/../..

LOCAL_SRC_FILES:= \
    GstPlayer.cpp \
    GstPlayerPipeline.cpp 
 
LOCAL_SHARED_LIBRARIES := \
    libgstapp-0.10		\
    libgstreamer-0.10  	\
    libglib-2.0     \
    libgthread-2.0  \
    libgmodule-2.0  \
    libgobject-2.0  \
    libutils

LOCAL_MODULE:= libgst_player

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)   \
    $(LOCAL_PATH)/..   \
    $(LOCAL_PATH)/../log   \
    $(LOCAL_TOP_PATH)/gstreamer       \
    $(LOCAL_TOP_PATH)/gstreamer/android  \
    $(LOCAL_TOP_PATH)/gstreamer/gst	\
    $(LOCAL_TOP_PATH)/gstreamer/gst/android	\
    $(LOCAL_TOP_PATH)/gstreamer/libs \
    $(LOCAL_TOP_PATH)/glib   \
    $(LOCAL_TOP_PATH)/glib/android   \
    $(LOCAL_TOP_PATH)/glib/glib   \
    $(LOCAL_TOP_PATH)/glib/glib/android   \
    $(LOCAL_TOP_PATH)/glib/gmodule   \
    $(LOCAL_TOP_PATH)/glib/gobject  \
    $(LOCAL_TOP_PATH)/glib/gthread  \
    $(LOCAL_TOP_PATH)/gst-plugins-base/gst-libs/gst/app \
    $(call include-path-for, graphics corecg)

LOCAL_CFLAGS :=  \
        -DENABLE_GST_PLAYER_LOG \
		-DBUILD_WITH_GST

include $(BUILD_SHARED_LIBRARY)


# build pipeline test application
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    GstPlayer.cpp \
    GstPlayerPipeline.cpp \
    pipeline_test.cpp
	
LOCAL_SHARED_LIBRARIES := \
    libgstapp-0.10		\
    libgstreamer-0.10       \
    libglib-2.0             \
    libgthread-2.0          \
    libgmodule-2.0          \
    libgobject-2.0			\
    libutils 

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)   \
    $(LOCAL_PATH)/..   \
    $(LOCAL_PATH)/../log   \
    $(LOCAL_TOP_PATH)/gstreamer       \
    $(LOCAL_TOP_PATH)/gstreamer/android  \
    $(LOCAL_TOP_PATH)/gstreamer/gst	\
    $(LOCAL_TOP_PATH)/gstreamer/gst/android	\
    $(LOCAL_TOP_PATH)/gstreamer/libs \
    $(LOCAL_TOP_PATH)/glib   \
    $(LOCAL_TOP_PATH)/glib/android   \
    $(LOCAL_TOP_PATH)/glib/glib   \
    $(LOCAL_TOP_PATH)/glib/glib/android   \
    $(LOCAL_TOP_PATH)/glib/gmodule   \
    $(LOCAL_TOP_PATH)/glib/gobject  \
    $(LOCAL_TOP_PATH)/glib/gthread  \
    $(LOCAL_TOP_PATH)/gst-plugins-base/gst-libs/gst/app \
    $(call include-path-for, graphics corecg)

LOCAL_CFLAGS :=  \
        -DENABLE_GST_PLAYER_LOG \
		-DBUILD_WITH_GST

LOCAL_MODULE:= pipelinetest

include $(BUILD_EXECUTABLE)

