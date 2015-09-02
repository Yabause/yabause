project(yabause-runner)

yab_port_start()

if ((NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lodepng/lodepng.h") OR (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lodepng/lodepng.cpp"))
	return()
endif()

include_directories(${PORT_INCLUDE_DIRS})

add_executable(yabause-runner yui.cpp)
target_link_libraries(yabause-runner yabause ${YABAUSE_LIBRARIES} ${PORT_LIBRARIES})

yab_port_success(yabause-runner)