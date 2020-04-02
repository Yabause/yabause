# Copyright 2017 The TensorFlow Authors. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ==============================================================================
include (ExternalProject)

message(STATUS "libpng not found - will build from source")

set(png_URL https://download.sourceforge.net/libpng/libpng-1.6.37.tar.gz)
set(png_HASH SHA256=daeb2620d829575513e35fecc83f0d3791a620b9b93d800b763542ece9390fb4)
set(png_BUILD ${CMAKE_CURRENT_BINARY_DIR}/png/src/png)
set(png_INSTALL ${CMAKE_CURRENT_BINARY_DIR}/png/install)
set(png_INCLUDE_DIR ${png_INSTALL}/include) 

if(WIN32)
  set(png_STATIC_LIBRARIES 
    debug ${CMAKE_CURRENT_BINARY_DIR}/png/install/lib/libpng16_staticd.lib
    optimized ${CMAKE_CURRENT_BINARY_DIR}/png/install/lib/libpng16_static.lib)
else()
  set(png_STATIC_LIBRARIES ${CMAKE_CURRENT_BINARY_DIR}/png/install/lib/libpng16.a)
endif()

set(png_HEADERS
    "${png_INSTALL}/include/libpng16/png.h"
    "${png_INSTALL}/include/libpng16/pngconf.h"
)

if(ANDROID)
get_filename_component(TOOL_CHAIN_ABSOLUTE_PATH "${CMAKE_TOOLCHAIN_FILE}"
                       REALPATH BASE_DIR "${CMAKE_BINARY_DIR}")

set( ADDITIONAL_CMAKE_ARGS 
 -DANDROID_ABI=${ANDROID_ABI}
 -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM} 
 -DANDROID_NATIVE_API_LEVEL=${ANDROID_NATIVE_API_LEVEL}
 -DCMAKE_TOOLCHAIN_FILE=${TOOL_CHAIN_ABSOLUTE_PATH}
)

else()
  set(ADDITIONAL_CMAKE_ARGS "")
endif()


ExternalProject_Add(
    png
    PREFIX png
    DEPENDS zlib
    URL ${png_URL}
    URL_HASH ${png_HASH}
    INSTALL_DIR ${png_INSTALL}
    DOWNLOAD_DIR "${DOWNLOAD_LOCATION}"
    CMAKE_ARGS
    	-DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON
        -DCMAKE_BUILD_TYPE:STRING=Release
        -DCMAKE_VERBOSE_MAKEFILE:BOOL=OFF
        -DCMAKE_INSTALL_PREFIX:STRING=${png_INSTALL}
        -DZLIB_ROOT:STRING=${ZLIB_INSTALL}
        -DPNG_NO_STDIO:BOOL=OFF
        -DPNG_SHARED:BOOL=OFF
        #-DCMAKE_C_FLAGS=-DPNG_SETJMP_SUPPORTED
        ${ADDITIONAL_CMAKE_ARGS} 
)
set( LIBPNG_LIB_DIR ${CMAKE_CURRENT_BINARY_DIR}/png/install/lib/ )


