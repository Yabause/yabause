project(yabause-glfw)

yab_port_start()

find_package(PkgConfig REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)
find_package(GLEW REQUIRED)
set(YABAUSE_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../")
set(PORT_INCLUDE_DIRS ${GLFW_INCLUDE_DIRS} ${GLEW_INCLUDE_DIRS} )
set(PORT_LIBRARIES ${GLFW_LIBRARIES} ${GLEW_LIBRARIES} )
include_directories( ${YABAUSE_INCLUDE_DIR}  ${PORT_INCLUDE_DIRS})
set(yabause_glfw_SOURCES
    main.cpp
    nanovg.c
perf.c
)



add_executable(yabause-glfw ${yabause_glfw_SOURCES})
target_link_libraries(yabause-glfw yabause ${YABAUSE_LIBRARIES} ${PORT_LIBRARIES})

yab_port_success(yabause-glfw)
configure_file(yabause.desktop.in ${YAB_PORT_NAME}.desktop)

install(TARGETS yabause-glfw DESTINATION "bin")
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${YAB_PORT_NAME}.desktop DESTINATION "share/applications")
install(FILES "doc/yabause.1" DESTINATION "${YAB_MAN_DIR}/man1" RENAME "${YAB_PORT_NAME}.1")
install(FILES "yabause.png" DESTINATION "share/pixmaps")
