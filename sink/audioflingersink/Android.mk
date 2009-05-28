LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

GST_MAJORMINOR:= 0.10

LOCAL_SRC_FILES:= \
	audioflinger_wrapper.cpp \
	gstaudioflingersink.c         

LOCAL_SHARED_LIBRARIES := 	\
	libgstreamer-0.10	\
	libglib-2.0		\
	libgthread-2.0		\
	libgmodule-2.0		\
	libgobject-2.0		\
	libcutils		\
	libutils		\
	libaudioflinger		\
	libmediaplayerservice   \
	libmedia		\
	libgstbase-0.10		\
	libgstaudio-0.10

LOCAL_MODULE:= libgstaudioflingersink

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)   \
	$(LOCAL_PATH)/../..   \
	$(LOCAL_PATH)/../../log   \
	external/gstreamer	   \
	external/gstreamer/android	  \
	external/gstreamer/gst	\
	external/gstreamer/gst/android	\
	external/gstreamer/libs \
	external/glib   \
	external/glib/android   \
	external/glib/glib   \
	external/glib/gmodule   \
	external/glib/gobject  \
	external/glib/gthread  \
	external/gstreamer/libs/gst/base  \
	external/gst-plugins-android/servicegluelayer/gstaudioflinger \
	external/gst-plugins-base/gst-libs/gst/audio  \
	external/gst-plugins-base/gst-libs \
	frameworks/base/libs/audioflinger \
	frameworks/base/media/libmediaplayerservice \
	frameworks/base/media/libmedia	\
	frameworks/base/include/media


LOCAL_CFLAGS := \
	-DHAVE_CONFIG_H			

include $(BUILD_PLUGIN_LIBRARY)
