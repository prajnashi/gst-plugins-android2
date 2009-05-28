LOCAL_PATH := $(call my-dir)

GST_PLUGIN_ANDROID_TOP := $(LOCAL_PATH)

include $(CLEAR_VARS)

include $(GST_PLUGIN_ANDROID_TOP)/sink/audioflingersink/Android.mk
include $(GST_PLUGIN_ANDROID_TOP)/sink/surfaceflingersink/Android.mk
include $(GST_PLUGIN_ANDROID_TOP)/player/Android.mk
