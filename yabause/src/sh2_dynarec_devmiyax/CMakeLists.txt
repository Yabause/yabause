
option(YAB_FORCE_32BITS "Compile in 32 bit compatible mode" OFF)

if (YAB_FORCE_32BITS)
set (CC_ARCH_FLAGS -m32)
endif (YAB_FORCE_32BITS)


cmake_minimum_required (VERSION 2.8.11)
project (dynalib_test)

set (COMMON_INCLUDES
  ../
  ${CMAKE_CURRENT_SOURCE_DIR}
)

#include_directories(dmy)
include_directories(${COMMON_INCLUDES})

set(SOURCES
  memory_for_test.cpp
	DynarecSh2.cpp
	DynarecSh2.h
	DynarecSh2CInterface.cpp
)

if (MSVC)    
  add_definitions(-DHAVE_C99_VARIADIC_MACROS)
  link_directories(${CMAKE_CURRENT_SOURCE_DIR})
  set (LIBRARIES  ${CMAKE_CURRENT_SOURCE_DIR}/dynalib_w32.obj )
    add_definitions(-DDYNAREC_X64=1)
  set (CMAKE_EXE_LINKER_FLAGS /SAFESEH:NO )
else (MSVC)
if("${CMAKE_SYSTEM_PROCESSOR}" MATCHES  "arm")
  enable_language(ASM-ATT)
  set(SOURCES ${SOURCES} dynalib_arm.s )
  set(CMAKE_ASM-ATT_FLAGS  "${CMAKE_ASM-ATT_FLAGS} -march=armv7-a")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g -march=armv7-a -std=gnu++11")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -march=armv7-a -g")
else ()
if("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64")
if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
    enable_language(ASM_NASM)
    SET(CMAKE_ASM_NASM_COMPILE_OBJECT "<CMAKE_ASM_NASM_COMPILER> -o <OBJECT> <SOURCE>")
    if (YAB_FORCE_32BITS)
       SET(CMAKE_ASM_NASM_COMPILER_ARG1 "-f elf32")
       set(SOURCES ${SOURCES} dynalib_i386.nasm)
    else ()
       SET(CMAKE_ASM_NASM_COMPILER_ARG1 "-f elf64")
       set(SOURCES ${SOURCES} dynalib_x64.nasm)
    endif()
    set(CMAKE_ASM_NASM_FLAGS "-O3 -l func.lst")
else ()
    set (YABAUSE_LIB ${CMAKE_CURRENT_SOURCE_DIR}/dynalib_w32.obj)
    add_definitions(-DDYNAREC_X64=1)
endif()
    set(CMAKE_ASM_NASM_FLAGS "${CMAKE_ASM_NASM_FLAGS} -O3 -l func.lst")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 ${CC_ARCH_FLAGS} -g -std=gnu++11")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 ${CC_ARCH_FLAGS} -g")
    add_definitions(-DARCH_IS_LINUX=1)
    add_definitions(-DDYNAREC_X64=1)
    add_definitions(-DTEST_MODE=1)
    find_package(Threads)
    set(LIBRARIES ${LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
    set(SOURCES ${SOURCES}
	../thr-linux.c
    )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -std=gnu++11")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")
  endif ()
endif ()
endif (MSVC)

add_executable (dynalib_test main.cpp ${SOURCES})
target_link_libraries(dynalib_test ${LIBRARIES} )

#--------------------------------------------------------------
# Google Unit Test Library
#--------------------------------------------------------------
cmake_minimum_required(VERSION 2.8.8)
project(gtest_builder C CXX)
include(ExternalProject)

set(GTEST_FORCE_SHARED_CRT ON)
set(GTEST_DISABLE_PTHREADS OFF)

if(MINGW)
    set(GTEST_DISABLE_PTHREADS ON)
endif()

ExternalProject_Add(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    CMAKE_ARGS -DCMAKE_CXX_FLAGS=${CC_ARCH_FLAGS} -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG:PATH=DebugLibs
    -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE:PATH=ReleaseLibs
    -Dgtest_force_shared_crt=${GTEST_FORCE_SHARED_CRT}
    -Dgtest_disable_pthreads=${GTEST_DISABLE_PTHREADS}
    -DBUILD_GTEST=ON
    PREFIX "${CMAKE_CURRENT_BINARY_DIR}"
    # Disable install step
    INSTALL_COMMAND ""
    )

# Specify include dir
ExternalProject_Get_Property(googletest source_dir)
set(GTEST_INCLUDE_DIRS ${source_dir}/googletest/include )

# Specify MainTest's link libraries
ExternalProject_Get_Property(googletest binary_dir)
set(GTEST_LIBS_DIR ${binary_dir}/googlemock/gtest )

# set runtime directory
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

#--------------------------------------------------------------
# Unit test
#--------------------------------------------------------------
enable_testing()
find_package( Threads )
set(PROJECT_TEST_NAME Compatibility_test)
include_directories(${GTEST_INCLUDE_DIRS} ${COMMON_INCLUDES})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CC_ARCH_FLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CC_ARCH_FLAGS}")

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
    enable_language(ASM_NASM)
    SET(CMAKE_ASM_NASM_COMPILE_OBJECT "<CMAKE_ASM_NASM_COMPILER> -o <OBJECT> <SOURCE>")
    if (YAB_FORCE_32BITS)
       SET(CMAKE_ASM_NASM_COMPILER_ARG1 "-f elf32")
       set(SOURCES ${SOURCES} dynalib_i386.nasm)
    else ()
       SET(CMAKE_ASM_NASM_COMPILER_ARG1 "-f elf64")
       set(SOURCES ${SOURCES} dynalib_x64.nasm)
    endif()
    set(CMAKE_ASM_NASM_FLAGS "-O3 -l func.lst")
else ()
    set (YABAUSE_LIB ${CMAKE_CURRENT_SOURCE_DIR}/dynalib_w32.obj)
    add_definitions(-DDYNAREC_X64=1)
endif()

file(GLOB TEST_SRC_FILES ${PROJECT_SOURCE_DIR}/test/*.cpp)
add_executable(${PROJECT_TEST_NAME} ${TEST_SRC_FILES} ${SOURCES})
add_dependencies(${PROJECT_TEST_NAME} googletest)


target_link_libraries(${PROJECT_TEST_NAME}
    ${GTEST_LIBS_DIR}/libgtest.a
   ${GTEST_LIBS_DIR}/libgtest_main.a
   ${YABAUSE_LIB}
)
        
target_link_libraries(${PROJECT_TEST_NAME} ${CMAKE_THREAD_LIBS_INIT})

add_test(test1 ${PROJECT_TEST_NAME})
