LOCAL_PATH := $(call my-dir)

CORE_DIR := $(LOCAL_PATH)/..

FLAGS        :=
DYNAREC      := 0
HAVE_THREADS := 1

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
  #DYNAREC := 3
else ifeq ($(TARGET_ARCH_ABI),x86_64)
  #DYNAREC := 2
endif

include $(CORE_DIR)/Makefile.common

COREFLAGS := -DINLINE=inline -D__LIBRETRO__ -DVERSION=\"0.9.12\" -DUSE_SCSP2=1 -DNO_CLI -DHAVE_GETTIMEOFDAY -DHAVE_STRCASECMP $(INCFLAGS) $(FLAGS)

GIT_VERSION := " $(shell git rev-parse --short HEAD || echo unknown)"
ifneq ($(GIT_VERSION)," unknown")
  COREFLAGS += -DGIT_VERSION=\"$(GIT_VERSION)\"
endif

include $(CLEAR_VARS)
LOCAL_MODULE    := retro
LOCAL_SRC_FILES := $(SOURCES_CXX) $(SOURCES_C) $(OBJECTS_S) $(C68KEXEC_SOURCE)
LOCAL_CFLAGS    := -std=gnu99 $(COREFLAGS)
LOCAL_LDFLAGS   := -Wl,-version-script=$(CORE_DIR)/link.T
include $(BUILD_SHARED_LIBRARY)
