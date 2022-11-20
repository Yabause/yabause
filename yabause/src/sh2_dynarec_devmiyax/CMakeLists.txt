cmake_minimum_required (VERSION 2.8.11)
project (dynalib_test C CXX)

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
	dmy.c
  ../sh2d.c
)

if (MSVC)    
  add_definitions(-DHAVE_C99_VARIADIC_MACROS)
  link_directories(${CMAKE_CURRENT_SOURCE_DIR})
  set (CMAKE_EXE_LINKER_FLAGS /SAFESEH:NO )

  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    # 64 bits
    set (LIBRARIES  ${CMAKE_CURRENT_SOURCE_DIR}/dynalib_x86_64.obj )
  elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    # 32 bits
    set (LIBRARIES  ${CMAKE_CURRENT_SOURCE_DIR}/dynalib.obj )
  endif()

else (MSVC)
 set (LIBRARIES  ${CMAKE_CURRENT_SOURCE_DIR}/dynalib.obj )

	#enable_language(ASM-ATT)
  #add_definitions(-DAARCH64)
  #set(SOURCES ${SOURCES} dynalib_arm64.s )
  set(CMAKE_ASM-ATT_FLAGS  "${CMAKE_ASM-ATT_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g --std=gnu++11 -DGTEST --coverage -fprofile-arcs -ftest-coverage")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g -DGTEST --coverage -fprofile-arcs -ftest-coverage")
endif (MSVC)

add_executable (dynalib_test main.cpp ${SOURCES})
target_link_libraries(dynalib_test ${LIBRARIES} )
set_target_properties( dynalib_test PROPERTIES LINK_FLAGS "/LARGEADDRESSAWARE:NO")

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
    GIT_TAG v1.12.0
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
set(GTEST_LIBS_DIR ${binary_dir}/googletest/DebugLibs/ )

#"H:\GitHub\yabasanshiro_master\yabause\src\sh2_dynarec_devmiyax\buildwin64\src\googletest-build\googletest\DebugLibs\gtest_main.lib"

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

target_link_libraries( ${PROJECT_TEST_NAME} ${LIBRARIES} )
set_target_properties( ${PROJECT_TEST_NAME} PROPERTIES LINK_FLAGS "/LARGEADDRESSAWARE:NO")

target_link_libraries(${PROJECT_TEST_NAME}
   ${GTEST_LIBS_DIR}/gtest_main.lib
   ${GTEST_LIBS_DIR}/gtest.lib
   #googletest
   #pthread
   #gcov
   #-fprofile-arcs -ftest-coverage 
)
        
target_link_libraries(${PROJECT_TEST_NAME} ${CMAKE_THREAD_LIBS_INIT})



add_test(test1 ${PROJECT_TEST_NAME})
