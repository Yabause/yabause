project(yabause-windows)

yab_port_start()

if (NOT WIN32)
	return()
endif(NOT WIN32)

if ( NOT ntddcdrm_INCLUDE_DIR )
	return()
endif ( NOT ntddcdrm_INCLUDE_DIR )

add_definitions(-D_WIN32_IE=0x0500 -D_WIN32_WINNT=0x0500 -DUNICODE -D_UNICODE)

# Find DirectX SDK
find_path(DirectX_INCLUDE_DIR dxerr9.h "$ENV{DXSDK_DIR}/Include")
if (NOT DirectX_INCLUDE_DIR)
	find_path(DirectX_INCLUDE_DIR "dxerr.h" "$ENV{DXSDK_DIR}/Include")
	if (DirectX_INCLUDE_DIR)
		set(DXERRH_IS_BROKEN 1 CACHE INTERNAL "dxerr is broken")
        endif (DirectX_INCLUDE_DIR)
endif(NOT DirectX_INCLUDE_DIR)

find_library(DirectX_GUID_LIBRARY dxguid "$ENV{DXSDK_DIR}/Lib/x86" "$ENV{DXSDK_DIR}/Lib")
find_library(DirectX_INPUT8_LIBRARY dinput8 "$ENV{DXSDK_DIR}/Lib/x86" "$ENV{DXSDK_DIR}/Lib")
find_library(DirectX_SOUND_LIBRARY dsound "$ENV{DXSDK_DIR}/Lib/x86" "$ENV{DXSDK_DIR}/Lib")

if (DXERRH_IS_BROKEN)
	find_library(DirectX_ERR_LIBRARY dxerr "$ENV{DXSDK_DIR}/Lib/x86" "$ENV{DXSDK_DIR}/Lib")
elseif(MINGW)
	find_library(DirectX_ERR_LIBRARY dxerr8 "$ENV{DXSDK_DIR}/Lib/x86" "$ENV{DXSDK_DIR}/Lib")
else()
	find_library(DirectX_ERR_LIBRARY dxerr9 "$ENV{DXSDK_DIR}/Lib/x86" "$ENV{DXSDK_DIR}/Lib")
endif()

if (DirectX_INCLUDE_DIR AND DirectX_GUID_LIBRARY AND DirectX_INPUT8_LIBRARY AND DirectX_SOUND_LIBRARY AND DirectX_ERR_LIBRARY)
	set(DIRECTX_FOUND "found")
	set(DIRECTX_LIBRARIES ${DirectX_GUID_LIBRARY} ${DirectX_INPUT8_LIBRARY} ${DirectX_SOUND_LIBRARY} ${DirectX_ERR_LIBRARY})
	include_directories(${DirectX_INCLUDE_DIR})
endif ()

# DirectX
if (DIRECTX_FOUND)
	message(STATUS "Found DirectX SDK 9.0 or greater")
else (DIRECTX_FOUND)
	message(FATAL_ERROR "Could not find DirectX SDK 9.0 or greater")
endif (DIRECTX_FOUND)

if (DXERRH_IS_BROKEN)
	add_definitions(-DDXERRH_IS_BROKEN)
	message(STATUS "Using work-around for dxerr.h")
endif(DXERRH_IS_BROKEN)

set(yabause_windows_HEADERS
	aviout.h
	cd.h
	cheats.h
	custctl.h
	disasm.h
	dx.h
	hexedit.h
	hotkey.h
	perdx.h
	ramwatch.h
	ram_search.h
	resource.h
	snddx.h
	yuiwin.h
	cpudebug/yuidebug.h
	settings/settings.h)

set(yabause_windows_RESOURCES
	resource.rc)

set(yabause_windows_SOURCES
	aviout.c
	bup-manager.c
	cheats.c
	custctl.c
	disasm.c
	hexedit.c
	hotkey.cpp
	perdx.c
	ramwatch.cpp
	ram_search.cpp
	snddx.c
	yui.cpp
	cpudebug/debug-68k.c
	cpudebug/debug-scsp.c
	cpudebug/debug-scu.c
	cpudebug/debug-sh2.c
	cpudebug/debug-smpc.c
	cpudebug/debug-vdp1.c
	cpudebug/debug-vdp2.c
	cpudebug/yuidebug.c
	settings/settings.c
	settings/settings-basic.c
	settings/settings-input.c
	settings/settings-log.c
	settings/settings-netlink.c
	settings/settings-sound.c
	settings/settings-video.c)

if(MINGW)
	set(CMAKE_RC_COMPILE_OBJECT
		"<CMAKE_RC_COMPILER> <FLAGS> -O coff <DEFINES> -i <SOURCE> -o <OBJECT>")
endif(MINGW)

add_executable(yabause-windows WIN32 ${yabause_windows_SOURCES} ${yabause_windows_RESOURCES} ${yabause_windows_HEADERS})
target_link_libraries(yabause-windows yabause ${YABAUSE_LIBRARIES} ${PORT_LIBRARIES} comctl32 ${DIRECTX_LIBRARIES} vfw32)

if(MINGW AND CMAKE_CROSSCOMPILING)
	set_target_properties(yabause-windows PROPERTIES LINK_FLAGS "-static-libgcc -static-libstdc++")
endif()

yab_port_success(yabause-windows)
