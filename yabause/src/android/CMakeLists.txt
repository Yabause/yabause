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
set( SOURCES 
 jni/yui.c
 jni/sndaudiotrack.c
 jni/sndopensl.c
 jni/osdcore.c
)

set (PORT_LIBRARIES
 log
 jnigraphics
 GLESv3
 EGL
 OpenSLES
 android
)

add_library(${LIBRARY_NAME} SHARED ${SOURCES}  )
include_directories( ${YABAUSE_INCLUDE_DIR} ${PROJECT_BINARY_DIR}/../ ${PROJECT_SOURCE_DIR}/../ )
target_link_libraries(${LIBRARY_NAME} yabause ${YABAUSE_LIBRARIES} ${PORT_LIBRARIES})

# copy library to the android java directoy
add_custom_command(
 TARGET 
 ${LIBRARY_NAME} 
 POST_BUILD 
 COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_SOURCE_DIR}/libs/${ANDROID_NDK_ABI_NAME}
 COMMAND ${CMAKE_COMMAND} -E copy  $<TARGET_FILE:${LIBRARY_NAME}> ${PROJECT_SOURCE_DIR}/libs/${ANDROID_NDK_ABI_NAME}
)

