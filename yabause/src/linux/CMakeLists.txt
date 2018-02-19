project(kronos-linux)

yab_port_start()

include(FindSDL2 OPTIONAL)
find_package(OpenGLES)
find_package(PkgConfig REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)

if (NOT OPENGLES2_FOUND)
	return()
endif (NOT OPENGLES2_FOUND)


if (SDL2_FOUND)
		add_definitions(-DHAVE_LIBSDL2=1)
		set(PORT_INCLUDE_DIRS ${PORT_INCLUDE_DIRS} ${SDL2_INCLUDE_DIR})
		set(PORT_LIBRARIES ${PORT_LIBRARIES} ${SDL2_LIBRARY})
endif (SDL2_FOUND)

set(PORT_INCLUDE_DIRS ${PORT_INCLUDE_DIRS} ${OPENGLES_INCLUDE_DIR} )
set(PORT_LIBRARIES ${PORT_LIBRARIES} ${OPENGLES2_LIBRARIES} )

if (GLFW_FOUND)
set(PORT_INCLUDE_DIRS ${GLFW_INCLUDE_DIRS})
set(PORT_LIBRARIES ${GLFW_LIBRARIES})
set(kronos_linux_SOURCES ${kronos_linux_SOURCES}
	platform/glfw/platform.c
)
set(kronos_linux_HEADERS ${kronos_linux_HEADERS}
	platform/glfw/platform.h
)
include_directories(platform/glfw)
endif (GLFW_FOUND)

add_executable(kronos-linux ${kronos_linux_SOURCES} ${kronos_linux_HEADERS} main.c )
target_link_libraries(kronos-linux kronos ${KRONOS_LIBRARIES} ${PORT_LIBRARIES})

yab_port_success(kronos-linux)

install(TARGETS kronos-linux DESTINATION "bin")
install(FILES "doc/kronos.1" DESTINATION "${YAB_MAN_DIR}/man1" RENAME "${YAB_PORT_NAME}.1")
install(FILES "kronos.png" DESTINATION "share/pixmaps")

option(INTERPRETER_TEST "Build the interpreter test suite" OFF)
if (INTERPRETER_TEST)

#--------------------------------------------------------------
# Google Unit Test Library
#--------------------------------------------------------------
cmake_minimum_required(VERSION 2.8.8)
project(gtest_builder C CXX)
include(ExternalProject)

#include_directories(dmy)
include_directories(${COMMON_INCLUDES})

set(GTEST_FORCE_SHARED_CRT ON)
set(GTEST_DISABLE_PTHREADS ON)

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
#find_package( Threads )
set(PROJECT_TEST_NAME Sh2IntCompatibility_test)
include_directories(${GTEST_INCLUDE_DIRS} ${COMMON_INCLUDES})

file(GLOB TEST_SRC_FILES ${PROJECT_SOURCE_DIR}/test/*.cpp ${kronos_linux_SOURCES} main_test.c)
add_executable(${PROJECT_TEST_NAME} ${TEST_SRC_FILES})
add_dependencies(${PROJECT_TEST_NAME} googletest)

target_link_libraries(${PROJECT_TEST_NAME}
    ${GTEST_LIBS_DIR}/libgtest.a
   ${GTEST_LIBS_DIR}/libgtest_main.a
   kronos ${KRONOS_LIBRARIES} ${PORT_LIBRARIES}
)
        
target_link_libraries(${PROJECT_TEST_NAME} ${CMAKE_THREAD_LIBS_INIT})

add_test(test1 ${PROJECT_TEST_NAME})
endif (INTERPRETER_TEST)
