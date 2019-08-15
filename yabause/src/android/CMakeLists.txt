#----------------------------------------------------------------------------------------
# Android build.
#----------------------------------------------------------------------------------------
# How to build on Windows host. 
#  Requirements:
#    1) android-ndk-r10d or above.
#    2) ninja (https://github.com/martine/ninja)
#    3) Visual C++ ( for c68k code generation )
#  Build step:
#    1) edit ANDROID_NDK PATH on "yabause/src/android/CMakeLists.txt"
#    2) cmake -GNinja -DCMAKE_TOOLCHAIN_FILE=yabause/src/android/androidtoolchain.cmake
#    3) edit settings listed below
#         ANDROID_ABI               armeabi-v7a with NEON
#         ANDROID_NATIVE_API_LEVEL  21 
#         YAB_PORTS                 android
#         YAB_WANT_ARM7             checked  
#    4) "Configure" and "Generate"
#    5) exec command prompt and exec "vcvarsall.bat x86" for building using cl.exe 
#    6) exec eclipse and import "yabause/src/android"
#----------------------------------------------------------------------------------------

set(ENV{ANDROID_NDK} "C:/NVPACK/android-ndk-r10e")

project(yabause-android)

yab_port_start()

set( LIBRARY_NAME yabause_native )

set( ImageLibrarySOURCES 
	jni/libpng/png.c
	jni/libpng/pngerror.c
	jni/libpng/pngget.c
	jni/libpng/pngmem.c
	jni/libpng/pngpread.c
	jni/libpng/pngread.c 
	jni/libpng/pngrio.c 
	jni/libpng/pngrtran.c 
	jni/libpng/pngrutil.c 
	jni/libpng/pngset.c 
	jni/libpng/pngtest.c 
	jni/libpng/pngtrans.c 
	jni/libpng/pngwio.c
	jni/libpng/pngwrite.c 
	jni/libpng/pngwtran.c 
	jni/libpng/pngwutil.c
)

set( SOURCES 
 ${ImageLibrarySOURCES}
 jni/yui.c
 jni/sndaudiotrack.c
 jni/sndopensl.c
 jni/BackupManager.cpp
 jni/jsoncpp.cpp
 jni/ChdFileInfo.cpp
 jni/ChdFileInfo.h
)


set (PORT_LIBRARIES
 log
 jnigraphics
 OpenSLES
 android
 GLESv3
 EGL
 z
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DJSONCPP_NO_LOCALE_SUPPORT -fexceptions " )


set( LIBRARY_OUTPUT_PATH  ${PROJECT_SOURCE_DIR}/app/src/main/jniLibs/${ANDROID_NDK_ABI_NAME} )
#set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})
#include(AndroidNdkGdb)
#android_ndk_gdb_enable()

link_directories( ${link_directories} ${LIBCHDR_LIB_DIR} )

add_library(${LIBRARY_NAME} SHARED ${SOURCES}  )
#android_ndk_gdb_debuggable(${LIBRARY_NAME})
add_dependencies(${LIBRARY_NAME} libchdr)


include_directories( ${YABAUSE_INCLUDE_DIR} ${PROJECT_BINARY_DIR}/../ ${PROJECT_SOURCE_DIR}/../ )
target_link_libraries(${LIBRARY_NAME} yabause ${YABAUSE_LIBRARIES} 
	${PORT_LIBRARIES}
  ${LIBCHDR_LIBRARIES}	
	#${zlib_STATIC_LIBRARIES}
	#    ${PROJECT_SOURCE_DIR}/debuglib/libNvidia_gfx_debugger_stub.a
)
 


