LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := simplegif
LOCAL_SRC_FILES := GifBridge.cpp\
                   GifDecoder.cpp
LOCAL_LDLIBS    := -llog
LOCAL_CFLAGS	:= -std=c++11 -fpermissive -DDEBUG -O0
LOCAL_LDFLAGS += -ljnigraphics
include $(BUILD_SHARED_LIBRARY)
