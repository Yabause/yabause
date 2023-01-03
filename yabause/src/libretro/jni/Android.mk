LOCAL_PATH := $(call my-dir)

CORE_DIR := $(LOCAL_PATH)/..

FLAGS        :=
DYNAREC      := 0
HAVE_THREADS := 1
HAVE_MUSASHI := 1
USE_RGB_565  := 1
# compression libraries
ENABLE_ZLIB  := 1
ENABLE_CHD   := 1
# The following is broken upstream ?
USE_PLAY_JIT := 0
USE_SCSP2    := 0

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
  #DYNAREC := 3
else ifeq ($(TARGET_ARCH_ABI),x86_64)
  #DYNAREC := 2
endif

include $(CORE_DIR)/Makefile.common

COREFLAGS := $(FLAGS)

GIT_VERSION := " $(shell git rev-parse --short HEAD || echo unknown)"
ifneq ($(GIT_VERSION)," unknown")
  COREFLAGS += -DGIT_VERSION=\"$(GIT_VERSION)\"
endif

include $(CLEAR_VARS)
LOCAL_MODULE       := retro
LOCAL_SRC_FILES    := $(SOURCES_CXX) $(SOURCES_C) $(OBJECTS_S)
LOCAL_C_INCLUDES   := $(INCLUDE_DIRS)
LOCAL_CFLAGS       := -std=gnu99 $(COREFLAGS)
LOCAL_LDFLAGS      := -Wl,-version-script=$(CORE_DIR)/link.T
include $(BUILD_SHARED_LIBRARY)
