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
  set (LIBRARIES  ${CMAKE_CURRENT_SOURCE_DIR}/dynalib.obj )
  set (CMAKE_EXE_LINKER_FLAGS /SAFESEH:NO )
else (MSVC)
  enable_language(ASM-ATT)
  set(SOURCES ${SOURCES} dynalib_arm.s )
  set(CMAKE_ASM-ATT_FLAGS  "${CMAKE_ASM-ATT_FLAGS} -march=armv7-a")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g -march=armv7-a -std=gnu++11")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -march=armv7-a -g")
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
    CMAKE_ARGS -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG:PATH=DebugLibs
    -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE:PATH=ReleaseLibs
    -DCMAKE_CXX_FLAGS=${MSVC_COMPILER_DEFS}
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
set(PROJECT_TEST_NAME ${PROJECT_NAME_STR}_test)
include_directories(${GTEST_INCLUDE_DIRS} ${COMMON_INCLUDES})

file(GLOB TEST_SRC_FILES ${PROJECT_SOURCE_DIR}/test/*.cpp)
add_executable(${PROJECT_TEST_NAME} ${TEST_SRC_FILES} ${SOURCES})
add_dependencies(${PROJECT_TEST_NAME} googletest)

target_link_libraries(${PROJECT_TEST_NAME}
    ${GTEST_LIBS_DIR}/libgtest.a
   ${GTEST_LIBS_DIR}/libgtest_main.a
)
        
target_link_libraries(${PROJECT_TEST_NAME} ${CMAKE_THREAD_LIBS_INIT})

add_test(test1 ${PROJECT_TEST_NAME})
