include(ExternalProject)
message(STATUS "Could not find libchdr.  This dependency will be downloaded.")

#find_package(Git)
#message("Git found: ${GIT_EXECUTABLE}")

if(ANDROID)
get_filename_component(TOOL_CHAIN_ABSOLUTE_PATH "${CMAKE_TOOLCHAIN_FILE}"
                       REALPATH BASE_DIR "${CMAKE_BINARY_DIR}")

set( ADDITIONAL_CMAKE_ARGS 
 -DANDROID_ABI=${ANDROID_ABI}
 -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM} 
 -DANDROID_NATIVE_API_LEVEL=${ANDROID_NATIVE_API_LEVEL}
 -DCMAKE_TOOLCHAIN_FILE=${TOOL_CHAIN_ABSOLUTE_PATH}
)

elseif(IOS)

  get_filename_component(TOOL_CHAIN_ABSOLUTE_PATH "${CMAKE_TOOLCHAIN_FILE}"
                       REALPATH BASE_DIR "${CMAKE_BINARY_DIR}")
  set( ADDITIONAL_CMAKE_ARGS 
    -DCMAKE_TOOLCHAIN_FILE=${TOOL_CHAIN_ABSOLUTE_PATH}
  )

else()
  set(ADDITIONAL_CMAKE_ARGS "")
endif()

ExternalProject_Add(
  libchdr
  GIT_REPOSITORY "https://github.com/devmiyax/libchdr.git"
  #GIT_TAG "96a2ce8a5bf59a900e4e786a9f7e42b910f7c6e0"
  #PATCH_COMMAND  git apply "${CMAKE_SOURCE_DIR}/CMake/Packages/libchdr.patch"
  CMAKE_ARGS  -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/libchdr 
              -DCMAKE_BUILD_TYPE:STRING=Release 
              -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
              -DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}
              ${ADDITIONAL_CMAKE_ARGS}
)

ExternalProject_Get_Property(libchdr SOURCE_DIR)
ExternalProject_Get_Property(libchdr BINARY_DIR)
set(LIBCHDR_INCLUDE_DIR ${SOURCE_DIR}/src )

if(WIN32)
set(LIBCHDR_LIBRARIES  
 ${BINARY_DIR}/$<CONFIGURATION>/${CMAKE_STATIC_LIBRARY_PREFIX}chdr-static${CMAKE_STATIC_LIBRARY_SUFFIX}
 ${BINARY_DIR}/$<CONFIGURATION>/${CMAKE_STATIC_LIBRARY_PREFIX}flac-static${CMAKE_STATIC_LIBRARY_SUFFIX}
 ${BINARY_DIR}/$<CONFIGURATION>/${CMAKE_STATIC_LIBRARY_PREFIX}crypto-static${CMAKE_STATIC_LIBRARY_SUFFIX}
 ${BINARY_DIR}/$<CONFIGURATION>/${CMAKE_STATIC_LIBRARY_PREFIX}lzma-static${CMAKE_STATIC_LIBRARY_SUFFIX}
)
elseif(ANDROID)
set(LIBCHDR_LIBRARIES  
 ${CMAKE_STATIC_LIBRARY_PREFIX}chdr-static${CMAKE_STATIC_LIBRARY_SUFFIX}
 ${CMAKE_STATIC_LIBRARY_PREFIX}flac-static${CMAKE_STATIC_LIBRARY_SUFFIX}
 ${CMAKE_STATIC_LIBRARY_PREFIX}crypto-static${CMAKE_STATIC_LIBRARY_SUFFIX}
 ${CMAKE_STATIC_LIBRARY_PREFIX}lzma-static${CMAKE_STATIC_LIBRARY_SUFFIX}
 )
else()
set(LIBCHDR_LIBRARIES  
 ${BINARY_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}chdr-static${CMAKE_STATIC_LIBRARY_SUFFIX}
 ${BINARY_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}flac-static${CMAKE_STATIC_LIBRARY_SUFFIX}
 ${BINARY_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}crypto-static${CMAKE_STATIC_LIBRARY_SUFFIX}
 ${BINARY_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}lzma-static${CMAKE_STATIC_LIBRARY_SUFFIX}
 )
endif()

set( LIBCHDR_LIB_DIR ${BINARY_DIR} )
message(STATUS "LIBCHDR_LIBRARIES is " ${LIBCHDR_LIBRARIES} )
message(STATUS "LIBCHDR_INCLUDE_DIR is " ${LIBCHDR_INCLUDE_DIR} )

include_directories( ${LIBCHDR_INCLUDE_DIR} )
