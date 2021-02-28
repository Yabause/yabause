include(ExternalProject)

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
    shaderc 
    PREFIX shaderc
    GIT_REPOSITORY https://github.com/google/shaderc.git
    GIT_TAG v2020.5
    CMAKE_ARGS
        -DSHADERC_SKIP_INSTALL=OFF
        -DSHADERC_SKIP_TESTS=ON
        -DSHADERC_SKIP_EXAMPLES=ON
        -DCMAKE_BUILD_TYPE=Release
        -DSHADERC_ENABLE_SHARED_CRT=TRUE
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        ${ADDITIONAL_CMAKE_ARGS}
)

ExternalProject_Get_Property(shaderc SOURCE_DIR)
ExternalProject_Get_Property(shaderc BINARY_DIR)

ExternalProject_Add_Step(
    shaderc
    copySource
    DEPENDEES download
    DEPENDERS configure
    WORKING_DIRECTORY ${SOURCE_DIR}
    COMMAND python3 utils/git-sync-deps
    COMMENT "utils/git-sync-deps"
)


ExternalProject_Get_Property(shaderc INSTALL_DIR)

set(SHADERC_INCLUDE_DIR ${SOURCE_DIR}/libshaderc/include)
set(SHADERC_LIBRARY_DIR ${BINARY_DIR}/libshaderc )
if(MSVC)
  set(SHADERC_LIBRARIES ${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}shaderc_combined${CMAKE_STATIC_LIBRARY_SUFFIX}  )
else()
  set(SHADERC_LIBRARIES -lshaderc_combined )
endif()


