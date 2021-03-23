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

#if( YAB_WANT_VULKAN )
#	set( LIBRARY_NAME yabause_native_vulkan )
#else()
	set( LIBRARY_NAME yabause_native )
#endif()

include( ${CMAKE_SOURCE_DIR}/CMake/Packages/external_libpng.cmake )
#include( ${CMAKE_SOURCE_DIR}/CMake/Packages/external_zlib.cmake )

set( SOURCES 
 jni/yui.cpp
 #jni/sndaudiotrack.c
 jni/sndopensl.c
 jni/jniBackupManager.cpp
 jni/jniBackupManager.h
 jni/jsoncpp.cpp
 jni/ChdFileInfo.cpp
 jni/ChdFileInfo.h
)

if( YAB_WANT_VULKAN )
set( SOURCES ${SOURCES} ../vulkan/Window_android.cpp )
endif()

set (PORT_LIBRARIES
 log
 jnigraphics
 OpenSLES
 android
 GLESv3
 EGL
 c++_static
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DJSONCPP_NO_LOCALE_SUPPORT -fexceptions " )


set( LIBRARY_OUTPUT_PATH  ${PROJECT_SOURCE_DIR}/app/src/main/jniLibs/${ANDROID_NDK_ABI_NAME} )
#set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})
#include(AndroidNdkGdb)
#android_ndk_gdb_enable()

link_directories( ${LIBCHDR_LIB_DIR} ${LIBPNG_LIB_DIR} ${LIBZ_LIB_DIR} ${SHADERC_LIBRARY_DIR} ${link_directories})

add_library(${LIBRARY_NAME} SHARED ${SOURCES}  )
#android_ndk_gdb_debuggable(${LIBRARY_NAME})

include_directories( 
	${YABAUSE_INCLUDE_DIR} 
	${PROJECT_BINARY_DIR}/../ 
	${PROJECT_SOURCE_DIR}/../ 
	${png_INCLUDE_DIR}
)
target_link_libraries(${LIBRARY_NAME} yabause ${YABAUSE_LIBRARIES} 
	${PORT_LIBRARIES}
	${LIBCHDR_LIBRARIES}	
	png16
	${ZLIB_LIBRARY_RELEASE}
	${VULKAN_LIBRARIES}
	#${zlib_STATIC_LIBRARIES}
	#    ${PROJECT_SOURCE_DIR}/debuglib/libNvidia_gfx_debugger_stub.a
)
 
add_dependencies(${LIBRARY_NAME} libchdr)
add_dependencies(${LIBRARY_NAME} png)


