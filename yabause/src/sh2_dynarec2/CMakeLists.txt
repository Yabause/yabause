cmake_minimum_required (VERSION 2.8.11)
project (dynalib_test)

#include_directories(dmy)
include_directories(../)

set(SOURCES
	DynarecSh2.cpp
	DynarecSh2.h
	DynarecSh2CInterface.cpp
)

if (MSVC)    
  add_definitions(-DHAVE_C99_VARIADIC_MACROS)
  link_directories(${CMAKE_CURRENT_SOURCE_DIR})
  set (LIBRARIES  ${CMAKE_CURRENT_SOURCE_DIR}/dynalib.obj )
  set (CMAKE_EXE_LINKER_FLAGS /SAFESEH:NO )
else (MSVC)
  enable_language(ASM-ATT)
  set(SOURCES ${SOURCES} dynalib_arm.s )
  set(CMAKE_ASM-ATT_FLAGS  "${CMAKE_ASM-ATT_FLAGS} -march=armv7-a -alm")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g -march=armv7-a -std=gnu++11")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -march=armv7-a -g")
endif (MSVC)

add_executable (dynalib_test main.cpp ${SOURCES})
target_link_libraries(dynalib_test ${LIBRARIES} )
