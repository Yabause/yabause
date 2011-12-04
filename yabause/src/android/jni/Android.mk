LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := yabause
LOCAL_SRC_FILES := yui.c
LOCAL_STATIC_LIBRARIES := yabause-prebuilt
LOCAL_LDLIBS := -llog -ljnigraphics
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := yabause-prebuilt
LOCAL_SRC_FILES := libyabause.a
include $(PREBUILT_STATIC_LIBRARY)
