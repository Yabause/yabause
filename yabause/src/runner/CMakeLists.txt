project(yabause-runner)

yab_port_start()

include_directories(${PORT_INCLUDE_DIRS})

add_executable(yabause-runner yui.c)
target_link_libraries(yabause-runner yabause ${YABAUSE_LIBRARIES} ${PORT_LIBRARIES})

yab_port_success(yabause-runner)