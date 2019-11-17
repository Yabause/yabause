project(kronos)

#to compile sdl port, use "cmake -DYAB_PORTS=sdl -DYAB_ASYNC_RENDERING ../kronos"

include (CheckCSourceCompiles)
include(CheckFunctionExists)
include(CheckIncludeFile)

cmake_minimum_required(VERSION 2.8)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/CMakeTests)

set(kronos_HEADERS
	bios.h
	cdbase.h cheat.h coffelf.h core.h cs0.h cs1.h cs2.h
	debug.h
	error.h
	gameinfo.h
	japmodem.h
	m68kcore.h m68kd.h memory.h movie.h
	netlink.h
	osdcore.h
	peripheral.h profile.h
	scsp.h scspdsp.h scu.h sh2core.h sh2d.h sh2iasm.h sh2int.h smpc.h sock.h
	threads.h titan/titan.h
	vdp1.h vdp2.h vdp2debug.h vidogl.h vidshared.h vidsoft.h
	yabause.h ygl.h yui.h
	frameprofile.h
        eeprom.h
)

option(YAB_MULTIBUILD "Choose wether to build all ports or only a single one")
set(YAB_PORT_BUILT FALSE)
if (WIN32)
set(YAB_PORTS "qt" CACHE STRING "List of ports to build")
else()
set(YAB_PORTS "qt;linux" CACHE STRING "List of ports to build")
endif(WIN32)

set(kronos_SOURCES
	bios.c
	cdbase.c cheat.c coffelf.c cs0.c cs1.c cs2.c
	debug.c
	error.c
	gameinfo.c
	japmodem.c
	m68kcore.c m68kd.c memory.c movie.c
	netlink.c
	osdcore.c
	peripheral.c profile.c
	frameprofile.cpp
	scspdsp.c scu.c sh2core.c sh2d.c sh2iasm.c sh2int.c smpc.c snddummy.c
	titan/titan.c
	vdp1.c vdp2.c vdp2debug.c vidshared.c vidsoft.c
	yabause.c
        eeprom.c
        decrypt.c
        stv.c
        junzip.c
	)
add_definitions(-DIMPROVED_SAVESTATES)

add_subdirectory(libchdr)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libchdr/src)
set(KRONOS_LIBRARIES ${KRONOS_LIBRARIES} chdr-static)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libchdr/deps/zlib-1.2.11)
set(KRONOS_LIBRARIES ${KRONOS_LIBRARIES} zlibstatic)

include_directories(nanovg)
set(kronos_SOURCES ${kronos_SOURCES} nanovg/nanovg.c)

option(YAB_USE_CXX "Allow C++ in the emulation core.")
if (YAB_USE_CXX)
	set(kronos_SOURCES ${kronos_SOURCES} yglcache.cpp)
else()
	set(kronos_SOURCES ${kronos_SOURCES} yglcache.c)
endif()

#-------------------------------------------------------

option(YAB_WANT_SSH2_ASYNC "Execute slave SH2 emulation in a thread" OFF)
if (YAB_WANT_SSH2_ASYNC)
  add_definitions(-DSSH2_ASYNC=1)
endif()

option(YAB_WANT_DYNAREC_KRONOS "Enable Sh2 dynarec by Kronos" ON)
if (YAB_WANT_DYNAREC_KRONOS)
	add_definitions(-DDYNAREC_KRONOS=1)
  set(kronos_SOURCES
    ${kronos_SOURCES}
    sh2_kronos/sh2int.c
    sh2_kronos/sh2_opcodes.c
  )
if (MSVC)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /bigobj")
        set(CMAKE_CPP_FLAGS "${CMAKE_CPP_FLAGS} /bigobj")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
endif(MSVC)
endif()

option(YAB_WANT_ASYNC_CELL "Enable Threaded rendering of nbgx cells" ON)
if (YAB_WANT_ASYNC_CELL)
	add_definitions(-DCELL_ASYNC=1)
endif()
option(YAB_WANT_ASYNC_RGB "Enable Threaded rendering of rgb0" ON)
if (YAB_WANT_ASYNC_RGB)
	add_definitions(-DRGB_ASYNC=1)
endif()

option(YAB_WANT_ASYNC_VDP1_TEXTURE "Enable Threaded rendering of vdp1 texture" ON)
if (YAB_WANT_ASYNC_VDP1_TEXTURE)
	add_definitions(-DVDP1_TEXTURE_ASYNC=1)
endif()

option(YAB_WANT_VDP2_BLIT_CS "Enable last blitting of vdp2 in compute shader" OFF)
if (YAB_WANT_VDP2_BLIT_CS)
	add_definitions(-DVDP2_BLIT_CS=1)
endif()


option(YAB_WANT_ASAN_DEBUG "Enable ASAN compilation sanitizer" OFF)
if (YAB_WANT_ASAN_DEBUG)
   set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
   set(KRONOS_LIBRARIES ${KRONOS_LIBRARIES} "-L/lib/x86_64-linux-gnu -lpthread -lasan")
endif()

option(YAB_WANT_UBSAN_DEBUG "Enable UBSAN compilation sanitizer" OFF)
if (YAB_WANT_UBSAN_DEBUG)
   set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=undefined")
   set(KRONOS_LIBRARIES ${KRONOS_LIBRARIES} "-lubsan")
endif()

option(YAB_WANT_DYNAREC_DEVMIYAX "Enable Sh2 denyarec by devMiyax" OFF)
if (YAB_WANT_DYNAREC_DEVMIYAX)
	add_definitions(-DDYNAREC_DEVMIYAX=1)


  set(kronos_SOURCES
    ${kronos_SOURCES}
    sh2_dynarec_devmiyax/DynarecSh2CInterface.cpp
	  sh2_dynarec_devmiyax/DynarecSh2.cpp)

	set(kronos_HEADERS
		${kronos_HEADERS}
	  sh2_dynarec_devmiyax/DynarecSh2.h
	  sh2_dynarec_devmiyax/opcodes.h)

		if (MSVC) # NASM
	  	set(KRONOS_LIBRARIES
			  ${KRONOS_LIBRARIES}
			  ${CMAKE_CURRENT_SOURCE_DIR}/sh2_dynarec_devmiyax/dynalib_w32.obj )
                add_definitions(-DDYNAREC_X64=1)
		else() # UNIX(GAS)
			if("${CMAKE_SYSTEM_PROCESSOR}" MATCHES  "arm")
				enable_language(ASM-ATT)
				set(kronos_SOURCES
				  ${kronos_SOURCES}
					sh2_dynarec_devmiyax/dynalib_arm.s)
                        else ()
 				add_definitions(-DARCH_IS_LINUX=1)
 				add_definitions(-DDYNAREC_X64=1)
 				add_definitions(-DYAB_ASYNC_RENDERING)
				enable_language(ASM_NASM)
				set(CMAKE_ASM_NASM_FLAGS "${CMAKE_ASM_NASM_FLAGS} -O3 -l func.lst")
                        	if("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64")
 				  set(kronos_SOURCES
 				    ${kronos_SOURCES}
 				    sh2_dynarec_devmiyax/dynalib_x64.nasm)
                                else ()
 				  set(kronos_SOURCES
 				    ${kronos_SOURCES}
 				    sh2_dynarec_devmiyax/dynalib_i386.nasm)
				endif ()
			endif ()
		endif ()

endif()

option(YAB_USE_SSF "Include SSF player." ON)
if (YAB_USE_SSF)
  add_definitions(-DYAB_WANT_SSF=1)
  set(kronos_SOURCES ${kronos_SOURCES}
      aosdk/corlett.c
      aosdk/eng_ssf.c
      aosdk/ssf.c)

  set(kronos_HEADERS ${kronos_HEADERS}
      aosdk/ao.h
      aosdk/corlett.h
      aosdk/ssf.h)
endif()
    set(kronos_SOURCES ${kronos_SOURCES} scsp.c)

# Enable SCSP MIDI hooks in sound interface
option(YAB_USE_SCSPMIDI "Enable SCSP Midi support")
if (YAB_USE_SCSPMIDI)
	add_definitions(-DUSE_SCSPMIDI=1)
endif()

# disable strdup warning in MSVC
if (MSVC)
	add_definitions(/wd4996)
endif ()

# math library
if (UNIX)
    set(KRONOS_LIBRARIES ${KRONOS_LIBRARIES} "m")
endif()

# Bigendian
#include(TestBigEndian)
#test_big_endian(WORDS_BIGENDIAN)
#if (WORDS_BIGENDIAN)
#	add_definitions(-DWORDS_BIGENDIAN=1)
#endif (WORDS_BIGENDIAN)

include(CheckCSourceCompiles)

# variadic macros
check_c_source_compiles("#define MACRO(...) puts(__VA_ARGS__)
	int main(int argc, char ** argv) { MACRO(\"foo\"); }"
	VARIADIC_MACROS_OK)
if (VARIADIC_MACROS_OK)
	add_definitions(-DHAVE_C99_VARIADIC_MACROS=1)
endif (VARIADIC_MACROS_OK)

# gettimeofday
check_function_exists(gettimeofday GETTIMEOFDAY_OK)
if (GETTIMEOFDAY_OK)
    add_definitions(-DHAVE_GETTIMEOFDAY=1)
endif ()

# floorf
set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} "-lm")
check_function_exists(floorf FLOORF_OK)
if (FLOORF_OK)
	add_definitions(-DHAVE_FLOORF=1)
endif ()

# _wfopen
check_function_exists(_wfopen WFOPEN_OK)
if (WFOPEN_OK)
	add_definitions(-DHAVE_WFOPEN=1)
endif ()

# stricmp/strcasecmp
check_function_exists(strcasecmp STRCASECMP_OK)
if (STRCASECMP_OK)
	add_definitions(-DHAVE_STRCASECMP=1)
endif ()
check_function_exists(stricmp STRICMP_OK)
if (STRICMP_OK)
	add_definitions(-DHAVE_STRICMP=1)
endif ()

# __builtin_bswap16
check_c_source_compiles (
	"
	int main(void) {
		return !__builtin_bswap16(0xabcd) == 0xcdab;
	}
	"
	BSWAP16_OK)
if (BSWAP16_OK)
	add_definitions(-DHAVE_BUILTIN_BSWAP16=1)
endif()

# __builtin_bswap32
check_c_source_compiles (
	"
	int main(void) {
		return !__builtin_bswap32(0xdeadbeef) == 0xefbeadde;
	}
	"
	BSWAP32_OK)
if (BSWAP32_OK)
	add_definitions(-DHAVE_BUILTIN_BSWAP32=1)
endif()

# sys/time.h
check_include_file("sys/time.h" SYSTIME_OK)
if (SYSTIME_OK)
	add_definitions(-DHAVE_SYS_TIME_H=1)
endif()

# Find stdint.h
check_include_file("stdint.h" STDINT_H_FOUND)
if (STDINT_H_FOUND)
	add_definitions(-DHAVE_STDINT_H=1)
endif()

find_package(Threads)
set(KRONOS_LIBRARIES ${KRONOS_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

# 16BPP
set(YAB_RGB "" CACHE STRING "Bit configuration of pixels in the display buffer.")
if (YAB_RGB STREQUAL "555")
    add_definitions(-DUSE_16BPP=1 -DUSE_RGB_555=1)
elseif (YAB_RGB STREQUAL "565")
    add_definitions(-DUSE_16BPP=1 -DUSE_RGB_565=1)
endif ()

# OpenGL
option(YAB_WANT_OPENGL "use OpenGL for video output (most ports require it)" ON)
if (YAB_WANT_OPENGL AND (YAB_RGB STREQUAL ""))
	include(FindOpenGL)
	if (OPENGL_FOUND)
		add_definitions(-DHAVE_LIBGL=1)
		set(KRONOS_LIBRARIES ${KRONOS_LIBRARIES} ${OPENGL_LIBRARIES})

		include_directories(./ ./nanovg)
		set(kronos_SOURCES ${kronos_SOURCES}
			nanovg/nanovg.c
			nanovg/nanovg_osdcore.h
			nanovg/nanovg_osdcore.c
		)
		add_definitions(-DYAB_PORT_OSD)

		include(FindGLUT)
		if (GLUT_FOUND)
			message (WARNING "Deprecated Glut found. Support will be removed in the future. ")
			include_directories(${GLUT_INCLUDE_DIR})
			add_definitions(-DHAVE_LIBGLUT=1)
			set(KRONOS_LIBRARIES ${KRONOS_LIBRARIES} ${GLUT_LIBRARIES})
		endif()

		# glXGetProcAddress
		set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} ${OPENGL_LIBRARIES})
		check_function_exists(glXGetProcAddress GLXGETPROCADDRESS_OK)
		if (GLXGETPROCADDRESS_OK)
			add_definitions(-DHAVE_GLXGETPROCADDRESS=1)
		endif()
		add_definitions(-D_OGL3_)
    include(FindGLEW)
    add_definitions(-D_USEGLEW_)
    add_definitions(-DGLEW_STATIC)
    include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/gllibs/include/ )
		set(kronos_SOURCES
			${kronos_SOURCES}
			gllibs/glew/glew.c
			yglcache.c
			ygles.c
			vidogl.c
			yglshaderes.c
			rbg_compute.cpp
			vdp2_compute.cpp
			common_glshader.c
			upscale_shader.c
			yglcs.c
			vidcs.c
			vdp1/vdp1_compute.c
		)
		set(kronos_HEADERS
			${kronos_HEADERS}
			common_glshader.h
			vdp1/vdp1_compute.h
			vdp1/vdp1_prog_compute.h
		)
	endif(OPENGL_FOUND)
endif ()
option(SPRITE_CACHE "Vdp1 sprite cache" ON)
if (SPRITE_CACHE)
    add_definitions(-DSPRITE_CACHE=1)
    set(kronos_SOURCES ${kronos_SOURCES} patternManager.c )
    set(kronos_HEADERS ${kronos_HEADERS} patternManager.h )
endif ()

# SDL
option(YAB_WANT_SDL "use SDL cores if available" ON)
if (YAB_WANT_SDL)
	include(FindSDL2 OPTIONAL)

	if (SDL2_FOUND)
		add_definitions(-DHAVE_LIBSDL2=1)
		include_directories(${SDL2_INCLUDE_DIR})
		set(KRONOS_LIBRARIES ${KRONOS_LIBRARIES} ${SDL2_LIBRARY})
	else()
		include(FindSDL)
		if (SDL_FOUND)
			message (WARNING "Deprecated version of SDL found. Support will be removed in the future. ")
			include_directories(${SDL_INCLUDE_DIR})
			set(KRONOS_LIBRARIES ${KRONOS_LIBRARIES} ${SDL_LIBRARY})
		endif()
	endif()

	if (SDL_FOUND OR SDL2_FOUND)
		add_definitions(-DHAVE_LIBSDL=1)
		set(kronos_SOURCES ${kronos_SOURCES} persdljoy.c sndsdl.c)
		set(kronos_HEADERS ${kronos_HEADERS} persdljoy.h sndsdl.h)
	endif()
endif()

# OpenAL
option(YAB_WANT_OPENAL "use OpenAL sound core if available" ON)
if (YAB_WANT_OPENAL)
	include(FindOpenAL)
	if (OPENAL_FOUND)
		add_definitions(-DHAVE_LIBAL=1)
		include_directories(${OPENAL_INCLUDE_DIR})
		set(kronos_SOURCES ${kronos_SOURCES} sndal.c)
		set(kronos_HEADERS ${kronos_HEADERS} sndal.h)
		set(KRONOS_LIBRARIES ${KRONOS_LIBRARIES} ${OPENAL_LIBRARY})
	endif (OPENAL_FOUND)
endif (YAB_WANT_OPENAL)

# OpenSL (Android)
option(YAB_WANT_OPENSL "use OpenSL sound core (for Android port)" ON)

# mini18n
find_path(MINI18N_INCLUDE_DIR mini18n.h)
find_library(MINI18N_LIBRARY mini18n)
if (NOT MINI18N_INCLUDE_DIR STREQUAL "MINI18N_INCLUDE_DIR-NOTFOUND" AND NOT MINI18N_LIBRARY STREQUAL "MINI18N_LIBRARY-NOTFOUND")
	set(MINI18N_FOUND TRUE)
	include_directories(${MINI18N_INCLUDE_DIR})
	add_definitions(-DHAVE_LIBMINI18N=1)
	set(KRONOS_LIBRARIES ${KRONOS_LIBRARIES} ${MINI18N_LIBRARY})
endif (NOT MINI18N_INCLUDE_DIR STREQUAL "MINI18N_INCLUDE_DIR-NOTFOUND" AND NOT MINI18N_LIBRARY STREQUAL "MINI18N_LIBRARY-NOTFOUND")

	find_library(XRANDR_LIBRARY Xrandr)
	if(XRANDR_LIBRARY)
		add_definitions(-DHAVE_LIBXRANDR=1)
		set(kronos_SOURCES ${kronos_SOURCES} scr-x.c)
		set(kronos_HEADERS ${kronos_HEADERS} screen.h)
		set(KRONOS_LIBRARIES ${KRONOS_LIBRARIES} Xrandr X11)
	endif()

if (MINI18N_FOUND)
	if (UNIX)
		add_definitions(-DYTSDIR=\"${CMAKE_INSTALL_PREFIX}/share/${YAB_PACKAGE}/yts\")
	elseif (WIN32)
		add_definitions(-DYTSDIR=\"trans\")
	endif()
endif()


# Visual Studio
if (MSVC)
        # Find DDK
        if (EXISTS "$ENV{SYSTEMDRIVE}/WINDDK/3790.1830/")
		set(DDK_DIR "$ENV{SYSTEMDRIVE}/WINDDK/3790.1830/")
        elseif (EXISTS "$ENV{SYSTEMDRIVE}/WINDDK/6000/")
		set(DDK_DIR "$ENV{SYSTEMDRIVE}/WINDDK/6000/")
        elseif (EXISTS "$ENV{SYSTEMDRIVE}/WINDDK/7600.16385.0/")
		set(DDK_DIR "$ENV{SYSTEMDRIVE}/WINDDK/7600.16385.0/")
        endif (EXISTS "$ENV{SYSTEMDRIVE}/WINDDK/3790.1830/")

	add_definitions(-DHAVE_C99_VARIADIC_MACROS -D_CRT_SECURE_NO_WARNINGS -DC68K_NO_JUMP_TABLE
			-D_UNICODE -DUNICODE)
endif (MSVC)

if ( ANDROID_NDK_HOST_SYSTEM_NAME2 MATCHES  "windows" )
 add_definitions(-DC68K_NO_JUMP_TABLE)
endif (  )

if (WIN32)
	# Windows ddk
	option(YAB_WANT_DDK "Use the real DDK instead of the built-in one")
	if(YAB_WANT_DDK)
		message (WARNING "Windows DDK/WDK use is deprecated. Support will be removed in the future. ")

		# Find ntddcdrm.h
		find_path(ntddcdrm_INCLUDE_DIR ntddcdrm.h
			PATHS "${DDK_DIR}" "${DDK_DIR}/inc" PATH_SUFFIXES ddk api)

		if (ntddcdrm_INCLUDE_DIR)
			include_directories(${ntddcdrm_INCLUDE_DIR})
			message(STATUS "Found ntddcdrm.h: ${ntddcdrm_INCLUDE_DIR}")
			add_definitions(-DHAVE_NTDDCDRM=1)
		else (ntddcdrm_INCLUDE_DIR)
			message(STATUS "Could not find ntddcdrm.h")
		endif (ntddcdrm_INCLUDE_DIR)
	endif(YAB_WANT_DDK)

	set(kronos_SOURCES ${kronos_SOURCES} cd-windows.c)

	option(YAB_WANT_DIRECTSOUND "use DirectX sound core if available")
	option(YAB_WANT_DIRECTINPUT "use DirectX input core if available")

	# Direct X
	if (YAB_WANT_DIRECTSOUND OR YAB_WANT_DIRECTINPUT)
		find_path(DirectX_INCLUDE_DIR dxerr9.h "$ENV{DXSDK_DIR}/Include")
		if (NOT DirectX_INCLUDE_DIR)
			find_path(DirectX_INCLUDE_DIR "dxerr.h" "$ENV{DXSDK_DIR}/Include")
			if (DirectX_INCLUDE_DIR)
				set(DXERRH_IS_BROKEN 1 CACHE INTERNAL "dxerr is broken")
        		endif (DirectX_INCLUDE_DIR)
		endif(NOT DirectX_INCLUDE_DIR)

                message (STATUS "system processor = ${CMAKE_SYSTEM_PROCESSOR}")
		if("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64" OR "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "AMD64")
			set (DIRECTX_SEARCH_PATH "$ENV{DXSDK_DIR}/Lib/x64")
		else()
			set (DIRECTX_SEARCH_PATH "$ENV{DXSDK_DIR}/Lib/x86")
		endif()

		find_library(DirectX_GUID_LIBRARY dxguid "${DIRECTX_SEARCH_PATH}" "$ENV{DXSDK_DIR}/Lib")
		if (YAB_WANT_DIRECTINPUT)
			find_library(DirectX_INPUT8_LIBRARY dinput8 "${DIRECTX_SEARCH_PATH}" "$ENV{DXSDK_DIR}/Lib")
			find_library(DirectX_XINPUT_LIBRARY xinput "${DIRECTX_SEARCH_PATH}" "$ENV{DXSDK_DIR}/Lib")
		endif(YAB_WANT_DIRECTINPUT)
		if (YAB_WANT_DIRECTSOUND)
			find_library(DirectX_SOUND_LIBRARY dsound "${DIRECTX_SEARCH_PATH}" "$ENV{DXSDK_DIR}/Lib")
		endif(YAB_WANT_DIRECTSOUND)

		if (DXERRH_IS_BROKEN)
			find_library(DirectX_ERR_LIBRARY dxerr "${DIRECTX_SEARCH_PATH}" "$ENV{DXSDK_DIR}/Lib")
		elseif(MINGW)
			find_library(DirectX_ERR_LIBRARY dxerr8 "${DIRECTX_SEARCH_PATH}" "$ENV{DXSDK_DIR}/Lib")
		else()
			find_library(DirectX_ERR_LIBRARY dxerr9 "${DIRECTX_SEARCH_PATH}" "$ENV{DXSDK_DIR}/Lib")
		endif()

		if (DirectX_INCLUDE_DIR AND DirectX_GUID_LIBRARY AND DirectX_ERR_LIBRARY)
			include_directories(${DirectX_INCLUDE_DIR})
			set(KRONOS_LIBRARIES ${KRONOS_LIBRARIES} ${DirectX_GUID_LIBRARY} ${DirectX_ERR_LIBRARY})

			if (YAB_WANT_DIRECTINPUT AND DirectX_INPUT8_LIBRARY)
				add_definitions(-DHAVE_DIRECTINPUT)
				set(kronos_SOURCES ${kronos_SOURCES} perdx.c)
				set(kronos_HEADERS ${kronos_HEADERS} perdx.h)
				set(KRONOS_LIBRARIES ${KRONOS_LIBRARIES} ${DirectX_INPUT8_LIBRARY})
				if (DirectX_XINPUT_LIBRARY)
					add_definitions(-DHAVE_XINPUT)
					set(KRONOS_LIBRARIES ${KRONOS_LIBRARIES} ${DirectX_XINPUT_LIBRARY} wbemuuid)
				endif()
			endif ()
        	if (YAB_WANT_DIRECTSOUND AND DirectX_SOUND_LIBRARY)
				add_definitions(-DHAVE_DIRECTSOUND)
				set(kronos_SOURCES ${kronos_SOURCES} snddx.c)
				set(kronos_HEADERS ${kronos_HEADERS} snddx.h)
				set(KRONOS_LIBRARIES ${KRONOS_LIBRARIES} ${DirectX_SOUND_LIBRARY})
			endif ()

			if (DXERRH_IS_BROKEN)
				add_definitions(-DDXERRH_IS_BROKEN)
				message(STATUS "Using work-around for dxerr.h")
				set(kronos_HEADERS ${kronos_HEADERS} dx.h)
			endif(DXERRH_IS_BROKEN)
		endif (DirectX_INCLUDE_DIR AND DirectX_GUID_LIBRARY AND DirectX_ERR_LIBRARY)
	endif (YAB_WANT_DIRECTSOUND OR YAB_WANT_DIRECTINPUT)
	if (YAB_NETWORK OR YAB_WANT_GDBSTUB)
    	# Add Winsock if necessary
		set(KRONOS_LIBRARIES ${KRONOS_LIBRARIES} "wsock32")
		set(KRONOS_LIBRARIES ${KRONOS_LIBRARIES} "ws2_32")
	endif()
	if (YAB_USE_SCSPMIDI)
		set(KRONOS_LIBRARIES ${KRONOS_LIBRARIES} winmm)
	endif()

endif (WIN32)

option(YAB_WANT_ARM7 "Build a binary with arm7 support")

# SH2 dynamic recompiler
message(STATUS "CMAKE_SYSTEM_NAME ${CMAKE_SYSTEM_NAME}")
message(STATUS "CMAKE_SYSTEM_PROCESSOR ${CMAKE_SYSTEM_PROCESSOR}")

# c68k
option(YAB_WANT_C68K "enable c68k compilation" OFF)
if (YAB_WANT_C68K)
	include(ExternalProject)
	ExternalProject_Add(c68kinc
		DOWNLOAD_COMMAND ""
		SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/c68k
		CMAKE_GENERATOR "${CMAKE_GENERATOR}"
		#CMAKE_GENERATOR "Visual Studio 12 2013"
		INSTALL_COMMAND ""
		BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/c68k
	)

	add_definitions(-DHAVE_C68K=1)
	include_directories(${CMAKE_CURRENT_BINARY_DIR}/c68k)
	set(kronos_SOURCES ${kronos_SOURCES} c68k/c68kexec.c c68k/c68k.c m68kc68k.c)
	set(kronos_HEADERS ${kronos_HEADERS} c68k/c68k.h m68kc68k.h)
	if (MSVC)
		set_source_files_properties(c68k/c68kexec.c PROPERTIES COMPILE_FLAGS "/Od /wd4146")
	else()
		#set_source_files_properties(c68k/c68kexec.c PROPERTIES COMPILE_FLAGS "-O0")
		set_source_files_properties(c68k/c68kexec.c PROPERTIES COMPILE_FLAGS "-O3")
	endif()
endif(YAB_WANT_C68K)

option(YAB_WANT_MUSASHI "Enable musashi 68k" ON)
#TODO cross compile builds m68kmake for the target platform instead
#of the native one, making it impossible to generate the core
if (YAB_WANT_MUSASHI)
    set (MUSASHI_GENERATED_SOURCES
        ${CMAKE_CURRENT_BINARY_DIR}/musashi/m68kopac.c
        ${CMAKE_CURRENT_BINARY_DIR}/musashi/m68kopdm.c
        ${CMAKE_CURRENT_BINARY_DIR}/musashi/m68kopnz.c
        ${CMAKE_CURRENT_BINARY_DIR}/musashi/m68kops.c)

    set (MUSASHI_GENERATED_HEADERS
        ${CMAKE_CURRENT_BINARY_DIR}/musashi/m68kops.h)

    include(ExternalProject)
    ExternalProject_Add(m68kmake
        DOWNLOAD_COMMAND ""
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/musashi
        CMAKE_GENERATOR "${CMAKE_GENERATOR}"
        INSTALL_COMMAND ""
        BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/musashi
    )

    add_definitions(-DHAVE_MUSASHI=1)

	include_directories(
      ${CMAKE_CURRENT_BINARY_DIR}/musashi
      ${CMAKE_CURRENT_SOURCE_DIR}/musashi
    )

	set(kronos_SOURCES ${kronos_SOURCES}
      ${MUSASHI_GENERATED_SOURCES}
      musashi/m68kdasm.c
      musashi/m68kcpu.c
      m68kmusashi.c
      )

	set(kronos_HEADERS
      ${kronos_HEADERS}
      musashi/m68k.h
      musashi/m68kcpu.h
      ${MUSASHI_GENERATED_HEADERS}
      m68kmusashi.h
      )

    set_property(SOURCE ${MUSASHI_GENERATED_SOURCES} ${MUSASHI_GENERATED_HEADERS} PROPERTY GENERATED ON)
endif(YAB_WANT_MUSASHI)

# q68
option(YAB_WANT_Q68 "enable q68 compilation" OFF)
if (YAB_WANT_Q68)
	add_definitions(-DHAVE_Q68=1)
	set(kronos_SOURCES ${kronos_SOURCES}
		m68kq68.c q68/q68.c q68/q68-core.c q68/q68-disasm.c)
	set(kronos_HEADERS ${kronos_HEADERS}
		q68/q68-const.h q68/q68.h q68/q68-internal.h q68/q68-jit.h q68/q68-jit-psp.h q68/q68-jit-x86.h)
endif()

# gdb stub
option(YAB_WANT_GDBSTUB "enable gdb stub" OFF)
if (YAB_WANT_GDBSTUB)
    add_definitions(-DHAVE_GDBSTUB=1)
    set(kronos_SOURCES ${kronos_SOURCES} gdb/stub.c gdb/client.c gdb/packet.c)
	set(kronos_HEADERS ${kronos_HEADERS} gdb/stub.h gdb/client.h gdb/packet.h)
endif()

# *DEBUG
set(YAB_DEBUG "" CACHE STRING "List of enabled debug information")
foreach(DEBUG IN LISTS YAB_DEBUG)
	if (${DEBUG} STREQUAL "main")
		add_definitions(-DDEBUG=1)
	elseif (${DEBUG} STREQUAL "cd")
		add_definitions(-DCDDEBUG=1)
	elseif (${DEBUG} STREQUAL "idle")
		add_definitions(-DIDLE_DETECT_VERBOSE=1)
	else (${DEBUG} STREQUAL "main")
		string(TOUPPER ${DEBUG} UPDEBUG)
		add_definitions(-D${UPDEBUG}_DEBUG=1)
	endif (${DEBUG} STREQUAL "main")
endforeach(DEBUG)

# Network
option(YAB_NETWORK "Enable network")
if (YAB_NETWORK)
	add_definitions(-DUSESOCKET=1)
endif()

option(YAB_PORT_OSD "Let ports provides their own OSD core list" OFF)
if (YAB_PORT_OSD)
	add_definitions(-DYAB_PORT_OSD=1)
endif()

# Exec from cache
option(YAB_USE_CACHE "Allow external bus access to use cache" OFF)
if (YAB_USE_CACHE)
	add_definitions(-DUSE_CACHE=1)
endif()

# SH2 UBC
option(SH2_UBC "Enable SH2 User Break Controller" OFF)
if (SH2_UBC)
	add_definitions(-DSH2_UBC=1)
endif()

# Yabause Arch
if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	add_definitions(-DARCH_IS_MACOSX=1)
	set(kronos_SOURCES ${kronos_SOURCES} sock-dummy.c thr-linux.c pthread_barrier.c )
elseif (${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
	add_definitions(-DARCH_IS_FREEBSD=1)
	set(kronos_SOURCES ${kronos_SOURCES} sock-dummy.c thr-dummy.c cd-freebsd.c)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux" OR ${CMAKE_SYSTEM_NAME} MATCHES "Android" )
	add_definitions(-DARCH_IS_LINUX=1)
	#add_definitions(-DYAB_ASYNC_RENDERING)
	set(kronos_SOURCES ${kronos_SOURCES} sock-linux.c thr-linux.c cd-linux.c)

	if (${CMAKE_SYSTEM_NAME} MATCHES "Linux" )
		check_include_file("linux/joystick.h" LINUX_HAS_JOYSTICK)
		if (LINUX_HAS_JOYSTICK)
			set(kronos_SOURCES ${kronos_SOURCES} perlinuxjoy.c)
			set(kronos_HEADERS ${kronos_HEADERS} perlinuxjoy.h)
		endif()

		check_c_source_compiles("
			#include <linux/cdrom.h>
			int main(int argc, char ** argv) { int i = CDSL_CURRENT; }
		" LINUX_CDROM_H_OK)
		if (NOT LINUX_CDROM_H_OK)
				add_definitions(-DLINUX_CDROM_H_IS_BROKEN)
		endif (NOT LINUX_CDROM_H_OK)
	endif()
elseif (${CMAKE_SYSTEM_NAME} MATCHES "NetBSD")
	add_definitions(-DARCH_IS_NETBSD=1)
	set(kronos_SOURCES ${kronos_SOURCES} sock-dummy.c thr-dummy.c cd-netbsd.c)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "OpenBSD")
	add_definitions(-DARCH_IS_NETBSD=1)
	set(kronos_SOURCES ${kronos_SOURCES} sock-dummy.c thr-dummy.c cd-netbsd.c)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	add_definitions(-DARCH_IS_WINDOWS=1)
	set(kronos_SOURCES ${kronos_SOURCES} sock-windows.c thr-windows.c)
else ()
	add_definitions(-DUNKNOWN_ARCH=1)
	set(kronos_SOURCES ${kronos_SOURCES} sock-dummy.c thr-dummy.c)
endif ()

if (CMAKE_COMPILER_IS_GNUCC)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfpmath=sse -m64 -march=native -funroll-loops")
        set(CMAKE_CPP_FLAGS "${CMAKE_CPP_FLAGS} -mfpmath=sse -m64 -march=native -funroll-loops")
endif()

if (CMAKE_COMPILER_IS_GNUCC AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 6.0)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-PIE -no-pie")
        set(CMAKE_CPP_FLAGS "${CMAKE_CPP_FLAGS} -fno-PIE -no-pie")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fno-PIE -no-pie" )
endif()


set(YAB_OPTIMIZATION "-O3" CACHE STRING "Override optimization level")
if (MSVC)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /Ox")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Ox")
endif ()

if("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "i686")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${YAB_OPTIMIZATION} -march=i686 -msse")
endif("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "i686")
if("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${YAB_OPTIMIZATION}")
endif("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64")
if("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "armv5tel")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${YAB_OPTIMIZATION}")
endif("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "armv5tel")
if("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "armv7l")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -mcpu=cortex-a8 -mfpu=vfp -mfloat-abi=hard")
    set(CMAKE_CPP_FLAGS "${CMAKE_CPP_FLAGS} -O3 -mcpu=cortex-a8 -mfpu=vfp -mfloat-abi=hard")
endif("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "armv7l")
if("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "armv7-a")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -march=armv7ve -mcpu=cortex-a15 -mfpu=neon -ffast-math")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv7ve -std=c++11 -mcpu=cortex-a15 -mfpu=neon -ffast-math -fno-exceptions -fno-rtti")
endif()
if("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "aarch64")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv8-a -ffast-math")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv8-a -std=c++11  -ffast-math -fno-exceptions -fno-rtti  ")
endif()

# Warnings defined to know when we're breaking compilation with MSVC
if (CMAKE_COMPILER_IS_GNUCC)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wdeclaration-after-statement")
endif ()

if (MSVC)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /wd4018 /wd4244")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4244")
endif ()

# Turn string format security warnings into errors. Some Linux distros enable this by default and travis-ci, etc. needs a way to catch them
option(YAB_FORCE_SECURE_STRINGS "Turns all string format warnings into errors." OFF)
if (CMAKE_COMPILER_IS_GNUCC AND YAB_FORCE_SECURE_STRINGS)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wformat -Werror=format-security")
endif()

option(YAB_WERROR "Treat all warnings as errors." OFF)
if (YAB_WERROR)
    if(MSVC)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /WX")
    else()
	    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")
    endif()
endif()

add_definitions(-DPACKAGE=\"${YAB_PACKAGE}\")
add_definitions(-DVERSION=\"${YAB_VERSION}\")

if( IOS )
	add_library(kronos OBJECT ${kronos_SOURCES} ${kronos_HEADERS})
else()
	add_library(kronos ${kronos_SOURCES} ${kronos_HEADERS})
endif()

if (YAB_WANT_C68K)
	add_dependencies(kronos c68kinc)
endif(YAB_WANT_C68K)

if (YAB_WANT_MUSASHI)
	add_dependencies(kronos m68kmake)
endif(YAB_WANT_MUSASHI)

macro(yab_port_start)
	if (YAB_PORT_BUILT AND NOT YAB_MULTIBUILD)
		return()
	endif ()
endmacro(yab_port_start)

macro(yab_port_stop)
	set(YAB_PORT_BUILT TRUE PARENT_SCOPE)
endmacro(yab_port_stop)

macro(yab_port_success YAB_TARGET)
	if (NOT YAB_MULTIBUILD)
		set_target_properties(${YAB_TARGET} PROPERTIES OUTPUT_NAME kronos)
		set(YAB_PORT_NAME "kronos")
	else ()
		set(YAB_PORT_NAME ${YAB_TARGET})
	endif ()
	set(YAB_PORT_BUILT TRUE PARENT_SCOPE)
endmacro(yab_port_success)

set(YAB_MAN_DIR "share/man")
if (NOT $ENV{PKGMANDIR} STREQUAL "")
    set(YAB_MAN_DIR $ENV{PKGMANDIR})
endif ()


foreach(PORT IN LISTS YAB_PORTS)
	add_subdirectory(${PORT})
endforeach(PORT)

# this is stupid, but CMake automatic definitions are based on variables...
if (YAB_WANT_C68K)
	set(HAVE_C68K ON)
endif()
if (YAB_WANT_MUSASHI)
	set(HAVE_MUSASHI ON)
endif()
if (YAB_WANT_Q68)
	set(HAVE_Q68 ON)
endif()
if (YAB_WANT_OPENSL)
    set(HAVE_OPENSL ON)
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

if (YAB_NETWORK AND UNIX)
    set(KRONOS_LIBRARIES ${KRONOS_LIBRARIES} "socket")
endif()

option(YAB_TESTS "Build test programs for cores" OFF)
if (YAB_TESTS)
	add_subdirectory("tools")
endif()
