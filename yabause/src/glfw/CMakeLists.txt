project(yabause-glfw)

yab_port_start()

if (WIN32)
	set( GLEW_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include/ )
	set( GLEW_LIBRARIES ${CMAKE_CURRENT_SOURCE_DIR}/lib/glew32.lib)
	set( GLFW_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include/ )
	set( GLFW_LIBRARIES  ${CMAKE_CURRENT_SOURCE_DIR}/lib/glfw3.lib )
else()
	find_package(PkgConfig REQUIRED)
	pkg_search_module(GLFW REQUIRED glfw3)
endif()


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

if (WIN32)
add_custom_command(
	TARGET yabause-glfw POST_BUILD 
	COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/bin/glew32.dll $<TARGET_FILE_DIR:yabause-glfw>
)
endif()

yab_port_success(yabause-glfw)
configure_file(yabause.desktop.in ${YAB_PORT_NAME}.desktop)

install(TARGETS yabause-glfw DESTINATION "bin")
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${YAB_PORT_NAME}.desktop DESTINATION "share/applications")
install(FILES "doc/yabause.1" DESTINATION "${YAB_MAN_DIR}/man1" RENAME "${YAB_PORT_NAME}.1")
install(FILES "yabause.png" DESTINATION "share/pixmaps")
