project(yabause-sdl)

yab_port_start()

find_package(SDL2 COMPONENTS sdl)
find_package(SDL2 REQUIRED)
find_package(OpenGLES)

if (NOT SDL2_FOUND)
	return()
endif (NOT SDL2_FOUND)

if (NOT OPENGLES2_FOUND)
	return()
endif (NOT OPENGLES2_FOUND)

set(PORT_INCLUDE_DIRS ${SDL2_INCLUDE_DIR} ${OPENGLES_INCLUDE_DIR})
set(PORT_LIBRARIES ${SDL2_LIBRARY} ${OPENGLES2_LIBRARIES})

set(yabause_sdl_SOURCES
	main.c
)

add_executable(yabause-sdl ${yabause_sdl_SOURCES})
target_link_libraries(yabause-sdl yabause ${YABAUSE_LIBRARIES} ${PORT_LIBRARIES})

yab_port_success(yabause-sdl)

install(TARGETS yabause-sdl DESTINATION "bin")
install(FILES "doc/yabause.1" DESTINATION "${YAB_MAN_DIR}/man1" RENAME "${YAB_PORT_NAME}.1")
install(FILES "yabause.png" DESTINATION "share/pixmaps")
