cmake_minimum_required(VERSION 3.5)

project(yabause-glfw)

yab_port_start()

### Load External Project
include(ExternalProject)

find_package(GLFW 3.2 QUIET)

if(GLFW_FOUND)
    message(STATUS "Found GLFW")
else()
    message(STATUS "GLFW not found - will build from source")

    ExternalProject_Add(glfw PREFIX glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.2

        UPDATE_COMMAND ""

        CMAKE_ARGS
            "-DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>"
            "-DCMAKE_BUILD_TYPE=Release"
            "-DGLFW_BUILD_EXAMPLES=OFF"
            "-DGLFW_BUILD_TESTS=OFF"
            "-DGLFW_BUILD_DOCS=OFF"

        CMAKE_CACHE_ARGS
            "-DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}"
            "-DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}"

        LOG_DOWNLOAD 1 LOG_UPDATE 1 LOG_CONFIGURE 1 LOG_BUILD 1 LOG_INSTALL 1
    )

    ExternalProject_Get_Property(glfw INSTALL_DIR)
    set(GLFW_INCLUDE_DIR ${INSTALL_DIR}/include)
    set(GLFW_LIBRARIES
        ${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}glfw3${CMAKE_STATIC_LIBRARY_SUFFIX})
				message(STATUS "GLFW_LIBRARIES is " ${GLFW_LIBRARIES} )
    if(UNIX)
        find_package(Threads REQUIRED)
        find_package(X11 REQUIRED)

        if(NOT X11_Xrandr_FOUND)
            message(FATAL_ERROR "Xrandr library not found - required for GLFW")
        endif()

        if(NOT X11_xf86vmode_FOUND)
            message(FATAL_ERROR "xf86vmode library not found - required for GLFW")
        endif()

        if(NOT X11_Xcursor_FOUND)
            message(FATAL_ERROR "Xcursor library not found - required for GLFW")
        endif()

        if(NOT X11_Xinerama_FOUND)
            message(FATAL_ERROR "Xinerama library not found - required for GLFW")
        endif()

        if(NOT X11_Xinput_FOUND)
            message(FATAL_ERROR "Xinput library not found - required for GLFW")
        endif()

        list(APPEND GLFW_LIBRARIES
            "${X11_Xrandr_LIB}" "${X11_Xxf86vm_LIB}" "${X11_Xcursor_LIB}"
            "${X11_Xinerama_LIB}" "${X11_Xinput_LIB}"
            "${CMAKE_THREAD_LIBS_INIT}" -lrt -ldl)
    endif()
endif()

include( external_zlib.cmake )
include( external_libpng.cmake )


set(YABAUSE_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../")
set(PORT_INCLUDE_DIRS ${GLFW_INCLUDE_DIR} ${png_INSTALL}/include/ ${ZLIB_INSTALL}/include/ )
set(PORT_LIBRARIES ${GLFW_LIBRARIES} ${png_STATIC_LIBRARIES} ${zlib_STATIC_LIBRARIES} ${CURL_LIBRARIES} )
include_directories( ${YABAUSE_INCLUDE_DIR}  ${PORT_INCLUDE_DIRS})
set(yabause_glfw_SOURCES
  main.cpp
  perf.c
)

add_executable(yabause-glfw ${yabause_glfw_SOURCES})
target_link_libraries(yabause-glfw yabause  ${YABAUSE_LIBRARIES} ${PORT_LIBRARIES}  )

yab_port_success(yabause-glfw)
configure_file(yabause.desktop.in ${YAB_PORT_NAME}.desktop)

install(TARGETS yabause-glfw DESTINATION "bin")
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${YAB_PORT_NAME}.desktop DESTINATION "share/applications")
install(FILES "doc/yabause.1" DESTINATION "${YAB_MAN_DIR}/man1" RENAME "${YAB_PORT_NAME}.1")
install(FILES "yabause.png" DESTINATION "share/pixmaps")
