project(kronos)

#to compile sdl port, use "cmake -DYAB_PORTS=sdl -DYAB_ASYNC_RENDERING ../kronos"

include (CheckCSourceCompiles)
include(CheckFunctionExists)
include(CheckIncludeFile)

include_directories(sys/bios/include)
include_directories(sys/vdp1/include)
include_directories(sys/vdp2/include)
include_directories(sys/sh2/include)
include_directories(sys/scu/include)
include_directories(sys/scsp/include)
include_directories(sys/smpc/include)
include_directories(sys/memory/include)
include_directories(utils/include)
include_directories(core/video/opengl/core/include)
include_directories(core/video/opengl/compute_shader/include)
include_directories(core/video/opengl/common/include)
include_directories(core/video/soft)
include_directories(core/sh2/sh2_simple/include)
include_directories(core/sh2/sh2_kronos/include)
include_directories(core/peripheral/mac)
include_directories(core/peripheral/linux)
include_directories(core/peripheral/sdl)
include_directories(core/peripheral/common)
include_directories(core/net)
include_directories(core/net/linux)
include_directories(core/net/windows)
include_directories(core/net/dummy)
include_directories(core/sound/al)
include_directories(core/sound/sdl)
include_directories(ctrl/include)
include_directories(ctrl/threads)
include_directories(tools/junzip)
include_directories(core/osd/include)
include_directories(core/m68k)

cmake_minimum_required(VERSION 3.12)

cmake_policy(VERSION 3.12)

option(YAB_FORCE_GLES31 "force to use OpenGLES 3.1" OFF)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/CMakeTests)

set(kronos_HEADERS
	sys/bios/include/bios.h
	sys/vdp1/include/vdp1.h
	sys/vdp2/include/vdp2.h
  sys/vdp2/include/vdp2debug.h
	sys/sh2/include/sh2core.h
	sys/scu/include/scu.h
	sys/scsp/include/scsp.h
	sys/scsp/include/scspdsp.h
	sys/smpc/include/smpc.h
	sys/memory/include/memory.h
	sys/memory/include/cs0.h
	sys/memory/include/cs1.h
	sys/memory/include/cs2.h
	sys/memory/include/eeprom.h
	sys/memory/include/decrypt.h
  core/osd/include/osdcore.h
  core/osd/include/font.h
  core/m68k/m68kcore.h
	core/video/opengl/core/include/vidogl.h
	core/video/opengl/common/include/vidshared.h
	core/video/opengl/common/include/ygl.h
	core/video/soft/vidsoft.h
	core/video/soft/titan/titan.h
	core/sh2/sh2_simple/include/sh2int.h
	core/peripheral/common/peripheral.h
	core/net/sock.h
  ctrl/include/core.h
  ctrl/include/yabause.h
  ctrl/include/movie.h
  ctrl/include/yui.h
	ctrl/threads/threads.h
  utils/include/error.h
	utils/include/cdbase.h
  utils/include/profile.h
  utils/include/netlink.h
  utils/include/cheat.h
  utils/include/gameinfo.h
  utils/include/japmodem.h
	tools/junzip/junzip.h
)

option(YAB_MULTIBUILD "Choose wether to build all ports or only a single one")
set(YAB_PORT_BUILT FALSE)
if (WIN32)
set(YAB_PORTS "qt" CACHE STRING "List of ports to build")
else()
set(YAB_PORTS "qt;linux" CACHE STRING "List of ports to build")
endif(WIN32)

if (CMAKE_COMPILER_IS_GNUCC)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fomit-frame-pointer")
endif (CMAKE_COMPILER_IS_GNUCC)

set(kronos_SOURCES
  utils/src/cheat.c
	utils/src/gameinfo.c
	utils/src/japmodem.c
	core/m68k/m68kcore.c
	utils/src/netlink.c
	core/osd/src/osdcore.c
	utils/src/profile.c
	sys/vdp2/src/vdp2debug.c
	core/m68k/Counter.cpp
	sys/bios/src/bios.c
	sys/vdp1/src/vdp1.c
	sys/vdp2/src/vdp2.c
	sys/sh2/src/sh2core.c
	sys/scu/src/scu.c
	sys/scsp/src/scsp.c
	sys/scsp/src/scspdsp.c
	sys/smpc/src/smpc.c
	sys/memory/src/memory.c
	sys/memory/src/cs0.c
	sys/memory/src/cs1.c
	sys/memory/src/cs2.c
	sys/memory/src/eeprom.c
	sys/memory/src/decrypt.c
	core/sh2/sh2_simple/src/sh2int.c
	core/video/soft/vidsoft.c
	core/video/soft/titan/titan.c
	core/video/opengl/common/src/vidshared.c
	core/peripheral/common/peripheral.c
	core/sound/dummy/snddummy.c
	ctrl/src/yabause.c
  ctrl/src/stv.c
  ctrl/src/movie.c
	utils/src/error.c
	utils/src/cdbase.c
	tools/junzip/junzip.c
	)
add_definitions(-DIMPROVED_SAVESTATES)

add_subdirectory(tools/libchdr)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/tools/libchdr/src)
set(KRONOS_LIBRARIES ${KRONOS_LIBRARIES} chdr-static)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/tools/libchdr/deps/zlib-1.2.11)
set(KRONOS_LIBRARIES ${KRONOS_LIBRARIES} zlibstatic)

include_directories(tools/nanovg)
set(kronos_SOURCES ${kronos_SOURCES} tools/nanovg/nanovg.c)

#-------------------------------------------------------

option(YAB_WANT_SH2_ASYNC "Execute slave SH2 emulation in a thread" OFF)
if (YAB_WANT_SH2_ASYNC)
  add_definitions(-DxSH2_ASYNC=1)
endif()

option(YAB_WANT_DYNAREC_KRONOS "Enable Sh2 dynarec by Kronos" ON)
if (YAB_WANT_DYNAREC_KRONOS)
	add_definitions(-DDYNAREC_KRONOS=1)
  set(kronos_SOURCES
    ${kronos_SOURCES}
    core/sh2/sh2_kronos/src/sh2int.c
    core/sh2/sh2_kronos/src/sh2_opcodes.c
  )
if (MSVC)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /bigobj")
        set(CMAKE_CPP_FLAGS "${CMAKE_CPP_FLAGS} /bigobj")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
endif(MSVC)
endif()

option(YAB_WANT_VDP1RAM_CS_ASYNC "Enable Threaded update of VDP1 Ram for CS core" ON)
if (YAB_WANT_VDP1RAM_CS_ASYNC)
	add_definitions(-DVDP1RAM_CS_ASYNC=1)
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

option(YAB_WANT_SOFT_RENDERING "Enable software renderer support" OFF)
if (YAB_WANT_SOFT_RENDERING)
   add_definitions(-DUSE_SOFT_RENDER=1)
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
	if (NOT YAB_FORCE_GLES31)
		include(FindOpenGL)
	endif (NOT YAB_FORCE_GLES31)
  include(FindOpenGLES)
	include(FindEGL)

	if (OPENGL_FOUND)
	  message(STATUS "OPENGL Core supported")
		add_definitions(-D_OGL3_)
		SET( NEED_GLEW "YES")
		SET( OPENGL_CURRENT_LIBRARIES ${OPENGL_LIBRARIES} )
		SET( OPENGL_SUPPORTED "YES" )
	else (OPENGL_FOUND)
		if (OPENGLES31_FOUND)
		  if (NOT EGL_FOUND)
				message(ERROR "No EGL found")
			endif (NOT EGL_FOUND)
		  message(STATUS "OPENGL ES 3.1 supported")
			add_definitions(-D_OGLES31_)
			SET( OPENGL_CURRENT_LIBRARIES ${OPENGLES_LIBRARIES} ${EGL_LIBRARIES} )
			SET( OPENGL_SUPPORTED "YES" )
		else (OPENGLES31_FOUND)
			if (OPENGLES3_FOUND)
				if (NOT EGL_FOUND)
					message(ERROR "No EGL found")
				endif (NOT EGL_FOUND)
		  	message(STATUS "OPENGL ES 3.0 supported")
				SET( NEED_GLEW "YES")
				SET( OPENGL_CURRENT_LIBRARIES ${OPENGLES_LIBRARIES} ${EGL_LIBRARIES} )
				SET( OPENGL_SUPPORTED "YES" )
			else (OPENGLES3_FOUND)
				SET( OPENGL_SUPPORTED "NO" )
			endif (OPENGLES3_FOUND)
		endif (OPENGLES31_FOUND)
	endif (OPENGL_FOUND)


	if (OPENGL_SUPPORTED)
		add_definitions(-DHAVE_LIBGL=1)
		set(KRONOS_LIBRARIES ${KRONOS_LIBRARIES} ${OPENGL_CURRENT_LIBRARIES})

		include_directories(./ ./tools/nanovg)
		set(kronos_SOURCES ${kronos_SOURCES}
			tools/nanovg/nanovg.c
			tools/nanovg/nanovg_osdcore.h
			tools/nanovg/nanovg_osdcore.c
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
		set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} ${OPENGL_CURRENT_LIBRARIES})
		check_function_exists(glXGetProcAddress GLXGETPROCADDRESS_OK)
		if (GLXGETPROCADDRESS_OK)
			add_definitions(-DHAVE_GLXGETPROCADDRESS=1)
		endif()

		if (NEED_GLEW)
	    include(FindGLEW)
	    add_definitions(-D_USEGLEW_)
	    add_definitions(-DGLEW_STATIC)
	    include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/tools/gllibs/include/ )
			set(kronos_SOURCES
				${kronos_SOURCES}
				tools/gllibs/glew/glew.c
			)
		endif (NEED_GLEW)
		set(kronos_SOURCES
				${kronos_SOURCES}
			core/video/opengl/common/src/commongl.c
			core/video/opengl/core/src/yglcache.c
			core/video/opengl/core/src/vidogl.c
			core/video/opengl/common/src/ogl_shader.c
			core/video/opengl/common/src/rbg_compute.cpp
			core/video/opengl/common/src/common_glshader.c
			core/video/opengl/common/src/upscale_shader.c
			core/video/opengl/compute_shader/src/yglcs.c
			core/video/opengl/compute_shader/src/vidcs.c
			core/video/opengl/compute_shader/src/vdp1_compute.c
		)
		set(kronos_HEADERS
			${kronos_HEADERS}
			core/video/opengl/common/include/common_glshader.h
			core/video/opengl/compute_shader/include/vidcs.h
			core/video/opengl/compute_shader/include/vdp1_compute.h
			core/video/opengl/compute_shader/include/vdp1_prog_compute.h
		)
	endif(OPENGL_SUPPORTED)
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
		set(kronos_SOURCES ${kronos_SOURCES} core/peripheral/sdl/persdljoy.c core/sound/sdl/sndsdl.c)
		set(kronos_HEADERS ${kronos_HEADERS} core/peripheral/sdl/persdljoy.h core/sound/sdl/sndsdl.h)
	endif()
endif()

# OpenAL
option(YAB_WANT_OPENAL "use OpenAL sound core if available" ON)
if (YAB_WANT_OPENAL)
	include(FindOpenAL)
	if (OPENAL_FOUND)
		add_definitions(-DHAVE_LIBAL=1)
		include_directories(${OPENAL_INCLUDE_DIR})
		set(kronos_SOURCES ${kronos_SOURCES} core/sound/al/sndal.c)
		set(kronos_HEADERS ${kronos_HEADERS} core/sound/al/sndal.h)
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

	set(kronos_SOURCES ${kronos_SOURCES} core/cd/windows/cd-windows.c)

	if (YAB_USE_SCSPMIDI)
		set(KRONOS_LIBRARIES ${KRONOS_LIBRARIES} winmm)
	endif()

endif (WIN32)

option(YAB_WANT_ARM7 "Build a binary with arm7 support")

# SH2 dynamic recompiler
message(STATUS "CMAKE_SYSTEM_NAME ${CMAKE_SYSTEM_NAME}")
message(STATUS "CMAKE_SYSTEM_PROCESSOR ${CMAKE_SYSTEM_PROCESSOR}")

#MUSASHI GENERATION
set (MUSASHI_GENERATED_SOURCES
    ${CMAKE_CURRENT_BINARY_DIR}/core/m68k/musashi/m68kopac.c
    ${CMAKE_CURRENT_BINARY_DIR}/core/m68k/musashi/m68kopdm.c
    ${CMAKE_CURRENT_BINARY_DIR}/core/m68k/musashi/m68kopnz.c
    ${CMAKE_CURRENT_BINARY_DIR}/core/m68k/musashi/m68kops.c)

set (MUSASHI_GENERATED_HEADERS
    ${CMAKE_CURRENT_BINARY_DIR}/core/m68k/musashi/m68kops.h)

include(ExternalProject)
ExternalProject_Add(m68kmake
    DOWNLOAD_COMMAND ""
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/core/m68k/musashi
    CMAKE_GENERATOR "${CMAKE_GENERATOR}"
    INSTALL_COMMAND ""
    BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/core/m68k/musashi
)

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}/core/m68k/musashi
  ${CMAKE_CURRENT_SOURCE_DIR}/core/m68k/musashi
)

set(kronos_SOURCES ${kronos_SOURCES}
  ${MUSASHI_GENERATED_SOURCES}
  core/m68k/musashi/m68kcpu.c
  core/m68k/m68kmusashi.c
  )

set(kronos_HEADERS
  ${kronos_HEADERS}
  core/m68k/musashi/m68k.h
  core/m68k/musashi/m68kcpu.h
  ${MUSASHI_GENERATED_HEADERS}
  core/m68k/m68kmusashi.h
  )

set_property(SOURCE ${MUSASHI_GENERATED_SOURCES} ${MUSASHI_GENERATED_HEADERS} PROPERTY GENERATED ON)
#END MUSASHI


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
	set(kronos_SOURCES ${kronos_SOURCES} core/net/dummy/sock-dummy.c ctrl/threads/thr-linux.c  ctrl/threads/linux/pthread_barrier.c )
elseif (${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
	add_definitions(-DARCH_IS_FREEBSD=1)
	set(kronos_SOURCES ${kronos_SOURCES} core/net/dummy/sock-dummy.c ctrl/threads/thr-dummy.c core/cd/freebsd/cd-freebsd.c)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux" OR ${CMAKE_SYSTEM_NAME} MATCHES "Android" )
	add_definitions(-DARCH_IS_LINUX=1)
	#add_definitions(-DYAB_ASYNC_RENDERING)
	set(kronos_SOURCES ${kronos_SOURCES} core/net/linux/sock-linux.c ctrl/threads/thr-linux.c core/cd/linux/cd-linux.c)

	if (${CMAKE_SYSTEM_NAME} MATCHES "Linux" )
		check_include_file("linux/joystick.h" LINUX_HAS_JOYSTICK)
		if (LINUX_HAS_JOYSTICK)
			set(kronos_SOURCES ${kronos_SOURCES} core/peripheral/linux/perlinuxjoy.c)
			set(kronos_HEADERS ${kronos_HEADERS} core/peripheral/linux/perlinuxjoy.h)
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
	set(kronos_SOURCES ${kronos_SOURCES} core/net/dummy/sock-dummy.c ctrl/threads/thr-dummy.c core/cd/netbsd/cd-netbsd.c)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "OpenBSD")
	add_definitions(-DARCH_IS_NETBSD=1)
	set(kronos_SOURCES ${kronos_SOURCES} core/net/dummy/sock-dummy.c ctrl/threads/thr-dummy.c core/cd/netbsd/cd-netbsd.c)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	add_definitions(-DARCH_IS_WINDOWS=1)
	set(kronos_SOURCES ${kronos_SOURCES} core/net/windows/sock-windows.c ctrl/threads/thr-windows.c)
else ()
	add_definitions(-DUNKNOWN_ARCH=1)
	set(kronos_SOURCES ${kronos_SOURCES} core/net/dummy/sock-dummy.c ctrl/threads/thr-dummy.c)
endif ()

if (CMAKE_COMPILER_IS_GNUCC)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -D_DEFAULT_SOURCE -Wno-format -mfpmath=sse -m64 -march=native -funroll-loops")
        set(CMAKE_CPP_FLAGS "${CMAKE_CPP_FLAGS} -mfpmath=sse -m64 -march=native -funroll-loops")
endif()

if (CMAKE_COMPILER_IS_GNUCC AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 6.0)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-PIE -no-pie")
        set(CMAKE_CPP_FLAGS "${CMAKE_CPP_FLAGS} -fno-PIE -no-pie")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fno-PIE -no-pie" )
endif()


set(YAB_OPTIMIZATION "-O3 -s" CACHE STRING "Override optimization level")
if (MSVC)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /Ox")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Ox")
endif ()

if("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "i686")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${YAB_OPTIMIZATION} -march=i686 -msse")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${YAB_OPTIMIZATION} -std=c++11")
endif("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "i686")
if("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${YAB_OPTIMIZATION}")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${YAB_OPTIMIZATION} -std=c++11")
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
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
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

add_dependencies(kronos m68kmake)

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
	add_subdirectory(port/${PORT})
endforeach(PORT)

# this is stupid, but CMake automatic definitions are based on variables...
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
