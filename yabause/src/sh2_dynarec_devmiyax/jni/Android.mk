  LOCAL_PATH := $(call my-dir)

  ARCH := $(APP_API)
  
  include $(CLEAR_VARS)
  LOCAL_C_INCLUDES := ../
  LOCAL_MODULE := sh2d
  LOCAL_ARM_MODE := arm
  #LOCAL_LDFLAGS += -fPIE -pie
  LOCAL_CXXFLAGS:= -march=armv7-a -DARCH_IS_LINUX -DANDROID
  LOCAL_SRC_FILES := ../memory_for_test.cpp \
	../DynarecSh2.cpp \
	../DynarecSh2CInterface.cpp \
	../dynalib_arm.s 
	
  include $(BUILD_SHARED_LIBRARY)

  include $(CLEAR_VARS)
  LOCAL_MODULE := sh2d_unittest
  LOCAL_ARM_MODE := arm
  LOCAL_LDFLAGS += -fPIE -pie
  LOCAL_C_INCLUDES := ../
  LOCAL_C_INCLUDES += ../sh2_dynarec2/
  LOCAL_SRC_FILES :=  ../test/rte_test.cpp \
    ../test/xtract_test.cpp \
    ../test/rotcl_test.cpp \
    ../test/macl_test.cpp \
    ../test/shlr_test.cpp
   
  LOCAL_SHARED_LIBRARIES := sh2d
  LOCAL_STATIC_LIBRARIES := googletest_main
  include $(BUILD_EXECUTABLE)

  $(call import-module,third_party/googletest)
