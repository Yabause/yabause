find_program(NDK_BUILD ndk-build)

if(NOT NDK_BUILD)
    message(FATAL_ERROR "ndk build not found, bye")
endif()

find_program(SDK_ANDROID android)

if(NOT SDK_ANDROID)
    message(FATAL_ERROR "sdk android tool not found, bye")
endif()

add_custom_command(
	OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/local.properties"
	COMMAND ${SDK_ANDROID} update project -p "${CMAKE_CURRENT_SOURCE_DIR}"
)

add_custom_command(
	OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/jni/libyabause.a"
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/../libyabause.a ${CMAKE_CURRENT_SOURCE_DIR}/jni/libyabause.a
	DEPENDS yabause
	DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/local.properties"
)

add_custom_command(
	OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/libs/armeabi/libyabause.so"
	COMMAND "${NDK_BUILD}"
	DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/jni/libyabause.a"
	DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/jni/yui.c"
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_custom_command(
	OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/bin/Yabause-debug.apk"
	COMMAND "ant" ARGS "debug"
	DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/libs/armeabi/libyabause.so"
	DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/org/yabause/android/Yabause.java"
	DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/AndroidManifest.xml"
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_custom_command(
	OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/bin/Yabause-release-unsigned.apk"
	COMMAND "ant" ARGS "release"
	DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/libs/armeabi/libyabause.so"
	DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/org/yabause/android/Yabause.java"
	DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/AndroidManifest.xml"
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_custom_target(yabause-android ALL DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/bin/Yabause-release-unsigned.apk")
else()
    add_custom_target(yabause-android ALL DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/bin/Yabause-debug.apk")
endif()
