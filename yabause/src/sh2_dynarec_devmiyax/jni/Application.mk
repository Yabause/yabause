APP_STL := gnustl_shared
APP_ABI := arm64-v8a
NDK_TOOLCHAIN_VERSION := 4.9
APP_OPTIM := debug
ifeq ($(APP_OPTIM),debug)
    cmd-strip := 
endif