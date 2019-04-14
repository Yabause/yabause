set (CMAKE_SYSTEM_NAME Switch)
set (CMAKE_SYSTEM_VERSION 1)
set (CMAKE_SYSTEM_PROCESSOR arm64-v8a)

if (NOT DEFINED DEVKITPRO)
	set (DEVKITPRO "/opt/devkitpro/")
endif (NOT DEFINED DEVKITPRO)

#set(CMAKE_SYSROOT "${DEVKITPRO}/libnx/" )
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE NEVER)

set( DEVKITPRO_BIN "${DEVKITPRO}/devkitA64/bin/" )
set( PREFIX "aarch64-none-elf-" )

# Tools
set(CMAKE_C_COMPILER  ${DEVKITPRO_BIN}${PREFIX}gcc )
set(CMAKE_CXX_COMPILER ${DEVKITPRO_BIN}${PREFIX}g++ )
set(CMAKE_ASM_COMPILER ${DEVKITPRO_BIN}${PREFIX}as )
#set(CMAKE_AR ${DEVKITPRO_BIN}${PREFIX}gcc-ar )
set(CMAKE_ASM_COMPILER_AR ${DEVKITPRO_BIN}${PREFIX}gcc-ar CACHE FILEPATH "" FORCE)
set(CMAKE_ASM_COMPILER_RANLIB ${DEVKITPRO_BIN}${PREFIX}gcc-ranlib )

set(CMAKE_OBJCOPY ${DEVKITPRO_BIN}${PREFIX}objcopy )
set(CMAKE_STRIP ${DEVKITPRO_BIN}${PREFIX}strip )
set(CMAKE_NM ${DEVKITPRO_BIN}${PREFIX}nm )
set(CMAKE_AR ${DEVKITPRO_BIN}${PREFIX}gcc-ar CACHE FILEPATH "" FORCE)
set(CMAKE_RANLIB ${DEVKITPRO_BIN}${PREFIX}gcc-ranlib )
set(CMAKE_LINKER ${DEVKITPRO_BIN}${PREFIX}ld )

set( ARCH "-march=armv8-a -mtune=cortex-a57 -mtp=soft -fPIE -D__SWITCH__" )

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wall -O2 -ffunction-sections ${ARCH}")
set (CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_CXX_FLAGS} -fno-rtti -fno-exceptions")

set (CMAKE_C_LINK_FLAGS "-specs=${DEVKITPRO}/libnx/switch.specs ${CMAKE_C_LINK_FLAGS}")
#set (CMAKE_CXX_LINK_FLAGS "-specs=${DEVKITPRO}/libnx/switch.specs ${CMAKE_CXX_LINK_FLAGS}")

# Headers
include_directories( SYSTEM "${DEVKITPRO}/devkitA64/include" "${DEVKITPRO}/libnx/include" "${DEVKITPRO}/portlibs/switch/include" )

# Libs
#set(CMAKE_FIND_LIBRARY_PREFIXES lib )
#set(CMAKE_FIND_LIBRARY_SUFFIXES .a )
link_directories( ${link_directories} "${DEVKITPRO}/devkitA64/lib" )

#("${DEVKITPRO}/libnx/lib" "${DEVKITPRO}/portlibs/switch/lib" )

add_definitions( -DNX )