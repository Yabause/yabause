CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

include(CMakeForceCompiler)
include(ExternalProject)

SET(CMAKE_SYSTEM_NAME SegaSaturn)

# Specify the cross compiler.
CMAKE_FORCE_C_COMPILER("C:/Program Files (x86)/KPIT Cummins/GNUSHv10.02-ELF/sh-elf/bin/sh-elf-gcc.exe" GNU)
CMAKE_FORCE_CXX_COMPILER("C:/Program Files (x86)/KPIT Cummins/GNUSHv10.02-ELF/sh-elf/bin/sh-elf-g++.exe" GNU)
SET(CMAKE_FIND_ROOT_PATH "C:/Program Files (x86)/KPIT Cummins/GNUSHv10.02-ELF/sh-elf/bin/" "C:/Program Files (x86)/KPIT Cummins/GNUSHv10.02-ELF/Other Utilities/")

ExternalProject_Add(iapetus
  GIT_REPOSITORY "https://github.com/cyberwarriorx/iapetus.git"
  LOG_DOWNLOAD 1
  LOG_CONFIGURE 1
  LOG_BUILD 1
  INSTALL_COMMAND ""
)

PROJECT(YabauseUT)

ExternalProject_Get_Property(iapetus source_dir)
include_directories(${source_dir}/src)

#CONFIGURE_FILE(config.h.cmake config.h)

set(YABAUSEUT_VERSION_MAJOR 0)
set(YABAUSEUT_VERSION_MINOR 1)
set(YABAUSEUT_VERSION_PATCH 0)
set(YABAUSEUT_VERSION "${YABAUSEUT_VERSION_MAJOR}.${YABAUSEUT_VERSION_MINOR}.${YABAUSEUT_VERSION_PATCH}")

set(CPACK_SOURCE_GENERATOR TGZ)
set(CPACK_PACKAGE_VERSION_MAJOR ${YABAUSEUT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${YABAUSEUT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${YABAUSEUT_VERSION_PATCH})
set(CPACK_PACKAGE_VENDOR "Yabause team")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "YABAUSEUT-${YABAUSEUT_VERSION}")
include(CPack)

set(CMAKE_CONFIGURATION_TYPES Release)

ExternalProject_Get_Property(iapetus binary_dir)
set(YabauseUT_LIBRARIES "${binary_dir}/src/${CMAKE_FIND_LIBRARY_PREFIXES}iapetus.a")

add_subdirectory(src)

add_dependencies(${PROJECT_NAME} iapetus)
