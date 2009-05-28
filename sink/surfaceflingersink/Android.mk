LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	surfaceflinger_wrap.cpp \
	gstsurfaceflingersink.c  
	
LOCAL_SHARED_LIBRARIES := \
    libgstreamer-0.10       \
    libglib-2.0             \
    libgthread-2.0          \
    libgmodule-2.0          \
    libgobject-2.0          \
    libgstbase-0.10         \
	libgstvideo-0.10		\
    libcutils               \
    libutils                \
	libui					\
	libsurfaceflinger

LOCAL_MODULE:= libgstsurfaceflingersink

LOCAL_TOP_PATH := $(LOCAL_PATH)/../../../

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)   \
    $(LOCAL_PATH)/../..   \
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
    $(LOCAL_TOP_PATH)/gstreamer/libs/gst/base  \
    $(LOCAL_TOP_PATH)/gst-plugins-base/gst-libs/gst/video  \
    $(LOCAL_TOP_PATH)/gst-plugins-base/gst-libs/gst/video/android  \
    $(LOCAL_TOP_PATH)/gst-plugins-base/gst-libs \
    $(LOCAL_TOP_PATH)/../../frameworks/base/media/libmediaplayerservice	\
    $(LOCAL_TOP_PATH)/../../frameworks/base/media/libmedia


LOCAL_CFLAGS := \
	-DHAVE_CONFIG_H			
ifeq ($(TARGET_OS)-$(TARGET_SIMULATOR),linux-true)
LOCAL_LDLIBS += -ldl
endif

ifneq ($(TARGET_SIMULATOR),true)
LOCAL_SHARED_LIBRARIES += libdl
endif

include $(BUILD_PLUGIN_LIBRARY)

# build surfaceflinger test application
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	surfaceflinger_wrap.cpp \
	wrap_test.cpp  
	
LOCAL_SHARED_LIBRARIES := \
    libcutils               \
    libutils                \
	libui					\
	libsurfaceflinger

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)   \
    $(LOCAL_PATH)/../..   \
    $(LOCAL_TOP_PATH)/../../frameworks/base/media/libmediaplayerservice	\
    $(LOCAL_TOP_PATH)/../../frameworks/base/media/libmedia

LOCAL_MODULE:= surfacetest

include $(BUILD_EXECUTABLE)


