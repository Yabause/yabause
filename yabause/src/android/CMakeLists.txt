find_program(NDK_BUILD ndk-build)

if(NOT NDK_BUILD)
    message(FATAL_ERROR "ndk build not found, bye")
endif()

find_program(SDK_ANDROID android)

if(NOT SDK_ANDROID)
    message(FATAL_ERROR "sdk android tool not found, bye")
endif()

if (NOT CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)
	set(yabause_android_SHADOW
		AndroidManifest.xml
		build.gradle
		gradlew
		gradlew.bat
		gradle/wrapper/gradle-wrapper.jar
		gradle/wrapper/gradle-wrapper.properties
		jni/yui.c
		jni/miniegl.h
		jni/sndaudiotrack.c
		jni/sndaudiotrack.h
		jni/sndopensl.c
		jni/sndopensl.h
		src/org/yabause/android/Cartridge.java
		src/org/yabause/android/GameInfo.java
		src/org/yabause/android/GameInfoManager.java
		src/org/yabause/android/GameList.java
		src/org/yabause/android/GameListAdapter.java
		src/org/yabause/android/Home.java
		src/org/yabause/android/PadEvent.java
		src/org/yabause/android/PadManager.java
		src/org/yabause/android/PadManagerV8.java
		src/org/yabause/android/PadManagerV16.java
		src/org/yabause/android/SaveList.java
		src/org/yabause/android/SaveListAdapter.java
		src/org/yabause/android/SaveListModeListener.java
		src/org/yabause/android/Yabause.java
		src/org/yabause/android/YabauseAudio.java
		src/org/yabause/android/YabauseView.java
		src/org/yabause/android/YabausePad.java
		src/org/yabause/android/YabauseSettings.java
		src/org/yabause/android/YabauseStorage.java
		res/drawable-hdpi/icon.png
		res/drawable-ldpi/icon.png
		res/drawable-mdpi/icon.png
		res/drawable-xhdpi/banner.png
		res/layout/game_item.xml
		res/layout/home.xml
		res/layout/main.xml
		res/layout/save_item.xml
		res/layout-v11/save_item.xml
		res/layout/save_list.xml
		res/menu/save_list.xml
		res/values/strings.xml
		res/xml/preferences.xml
	)

	foreach(item IN LISTS yabause_android_SHADOW)
		message(STATUS ${item})
		add_custom_command(
			OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${item}"
			COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/${item}" "${CMAKE_CURRENT_BINARY_DIR}/${item}"
			DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${item}"
		)
	endforeach()
endif()

set(YABAUSE_JNI_SRC "sndaudiotrack.c")
if (YAB_WANT_OPENSL)
    set(YABAUSE_JNI_SRC "${YABAUSE_JNI_SRC} sndopensl.c")
    set(YABAUSE_JNI_LIB "-lOpenSLES")
endif()

if (YAB_USE_SSF)
    set(YABAUSE_JNI_LIB "${YABAUSE_JNI_LIB} -lz")
endif()

set(YABAUSE_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/..")
configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/jni/Android.mk.in
	${CMAKE_CURRENT_BINARY_DIR}/jni/Android.mk
	@ONLY
)

configure_file(
       ${CMAKE_CURRENT_SOURCE_DIR}/gradle.properties.in
       ${CMAKE_CURRENT_BINARY_DIR}/gradle.properties
       @ONLY
)

add_custom_command(
	OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/local.properties"
	COMMAND ${SDK_ANDROID} update project -p "${CMAKE_CURRENT_BINARY_DIR}" --target android-24
	DEPENDS
		"${CMAKE_CURRENT_BINARY_DIR}/AndroidManifest.xml"
		"${CMAKE_CURRENT_BINARY_DIR}/build.gradle"
		"${CMAKE_CURRENT_BINARY_DIR}/jni/Android.mk"
)

add_custom_command(
	OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/jni/libyabause.a"
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/../libyabause.a ${CMAKE_CURRENT_BINARY_DIR}/jni/libyabause.a
	DEPENDS yabause "${CMAKE_CURRENT_BINARY_DIR}/../config.h"
	DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/local.properties"
)

add_custom_command(
	OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/libs/armeabi/libyabause.so"
	COMMAND "${NDK_BUILD}"
	DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/jni/libyabause.a"
	DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/jni/yui.c"
	DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/jni/miniegl.h"
	DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/jni/sndaudiotrack.c"
	DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/jni/sndaudiotrack.h"
	DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/jni/sndopensl.c"
	DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/jni/sndopensl.h"
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

set(yabause_android_RES
	"${CMAKE_CURRENT_BINARY_DIR}/res/drawable-hdpi/icon.png"
	"${CMAKE_CURRENT_BINARY_DIR}/res/drawable-ldpi/icon.png"
	"${CMAKE_CURRENT_BINARY_DIR}/res/drawable-mdpi/icon.png"
	"${CMAKE_CURRENT_BINARY_DIR}/res/drawable-xhdpi/banner.png"
	"${CMAKE_CURRENT_BINARY_DIR}/res/layout/game_item.xml"
	"${CMAKE_CURRENT_BINARY_DIR}/res/layout/home.xml"
	"${CMAKE_CURRENT_BINARY_DIR}/res/layout/main.xml"
	"${CMAKE_CURRENT_BINARY_DIR}/res/layout/save_item.xml"
	"${CMAKE_CURRENT_BINARY_DIR}/res/layout-v11/save_item.xml"
	"${CMAKE_CURRENT_BINARY_DIR}/res/layout/save_list.xml"
	"${CMAKE_CURRENT_BINARY_DIR}/res/menu/save_list.xml"
	"${CMAKE_CURRENT_BINARY_DIR}/res/values/strings.xml"
	"${CMAKE_CURRENT_BINARY_DIR}/res/xml/preferences.xml"
)
set(yabause_android_SRC
    "${CMAKE_CURRENT_BINARY_DIR}/src/org/yabause/android/Cartridge.java"
    "${CMAKE_CURRENT_BINARY_DIR}/src/org/yabause/android/GameInfo.java"
    "${CMAKE_CURRENT_BINARY_DIR}/src/org/yabause/android/GameInfoManager.java"
    "${CMAKE_CURRENT_BINARY_DIR}/src/org/yabause/android/GameList.java"
    "${CMAKE_CURRENT_BINARY_DIR}/src/org/yabause/android/GameListAdapter.java"
    "${CMAKE_CURRENT_BINARY_DIR}/src/org/yabause/android/Home.java"
    "${CMAKE_CURRENT_BINARY_DIR}/src/org/yabause/android/PadEvent.java"
    "${CMAKE_CURRENT_BINARY_DIR}/src/org/yabause/android/PadManager.java"
    "${CMAKE_CURRENT_BINARY_DIR}/src/org/yabause/android/PadManagerV8.java"
    "${CMAKE_CURRENT_BINARY_DIR}/src/org/yabause/android/PadManagerV16.java"
    "${CMAKE_CURRENT_BINARY_DIR}/src/org/yabause/android/SaveList.java"
    "${CMAKE_CURRENT_BINARY_DIR}/src/org/yabause/android/SaveListAdapter.java"
    "${CMAKE_CURRENT_BINARY_DIR}/src/org/yabause/android/SaveListModeListener.java"
    "${CMAKE_CURRENT_BINARY_DIR}/src/org/yabause/android/Yabause.java"
    "${CMAKE_CURRENT_BINARY_DIR}/src/org/yabause/android/YabauseAudio.java"
    "${CMAKE_CURRENT_BINARY_DIR}/src/org/yabause/android/YabauseView.java"
    "${CMAKE_CURRENT_BINARY_DIR}/src/org/yabause/android/YabausePad.java"
    "${CMAKE_CURRENT_BINARY_DIR}/src/org/yabause/android/YabauseSettings.java"
    "${CMAKE_CURRENT_BINARY_DIR}/src/org/yabause/android/YabauseStorage.java"
)

set(yabause_android_GRADLEW
	"${CMAKE_CURRENT_BINARY_DIR}/gradlew"
	"${CMAKE_CURRENT_BINARY_DIR}/gradlew.bat"
	"${CMAKE_CURRENT_BINARY_DIR}/gradle/wrapper/gradle-wrapper.jar"
	"${CMAKE_CURRENT_BINARY_DIR}/gradle/wrapper/gradle-wrapper.properties"
)

add_custom_command(
	OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/build/outputs/apk/Yabause-release.apk"
	COMMAND "${CMAKE_CURRENT_BINARY_DIR}/gradlew" build
	DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/libs/armeabi/libyabause.so"
	DEPENDS ${yabause_android_SRC}
	DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/gradle.properties"
	DEPENDS ${yabause_android_RES}
	DEPENDS ${yabause_android_GRADLEW}
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_custom_target(yabause-android ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/build/outputs/apk/Yabause-release.apk")
