find_package(CURL)

if(CURL_FOUND)
  message(STATUS "Found CURL")
else()
    set(CURL_FLAGS -DBUILD_CURL_EXE=OFF -DBUILD_CURL_TESTS=OFF -DCURL_STATICLIB=ON -DCMAKE_USE_OPENSSL=OFF -DCURL_ZLIB=OFF -DHTTP_ONLY=ON)
    include(ExternalProject)
    message(STATUS "Could not find libcURL.  This dependency will be downloaded.")
    ExternalProject_Add(
      curl
      GIT_REPOSITORY "git://github.com/bagder/curl.git"
      GIT_TAG "1b6bc02fb926403f04061721f9159e9887202a96"
      PREFIX curl
      PATCH_COMMAND ${CURL_PATCH_COMMAND}
      CMAKE_ARGS -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/curl ${CURL_FLAGS}
    )

    ExternalProject_Get_Property(curl INSTALL_DIR)
    set(CURL_INCLUDE_DIR ${INSTALL_DIR}/include)
    set(CURL_LIBRARIES
        ${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}libcurl${CMAKE_STATIC_LIBRARY_SUFFIX})
        message(STATUS "GLFW_LIBRARIES is " ${CURL_LIBRARIES} )

    include_directories( ${CURL_INCLUDE_DIR} )

endif()
