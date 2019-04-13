project(yabause-nx)

yab_port_start()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

#add_subdirectory(pugixml)
#add_subdirectory(nanogui-sdl)

include( ../glfw/external_zlib.cmake )
include( ../glfw/external_libpng.cmake )

#------------------------------------------------------------------------------
# nlohmann/json
#  * sets EXTERNAL_JSON_INCLUDE_DIR 

set(PORT_INCLUDE_DIRS "../" )
set(PORT_LIBRARIES EGL GLESv2 glapi drm_nouveau nx m SDL2  )

#message("Nlohmann/json dir =" ${EXTERNAL_JSON_INCLUDE_DIR})

set(yabause_nx_SOURCES
	main.cpp
	#InputManager.h
	#InputManager.cpp
	#InputConfig.h
	#InputConfig.cpp
	#MenuScreen.h
	#MenuScreen.cpp
) 


include_directories( ${YABAUSE_INCLUDE_DIR} ${PORT_INCLUDE_DIRS} ${Boost_INCLUDE_DIR} "nanogui-sdl/include" "nanogui-sdl/ext/eigen" )
link_directories( /opt/devkitpro/devkitA64/lib/ /opt/devkitpro/libnx/lib/ /opt/devkitpro/portlibs/switch/lib/  ) 
add_executable(yabause-nx ${yabause_nx_SOURCES})
target_link_libraries(yabause-nx yabause ${YABAUSE_LIBRARIES} ${PORT_LIBRARIES}  )

set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -specs=/opt/devkitpro/libnx/switch.specs -g -Wl,-Map,memory.map" )

#add_dependencies(yabause-nx Json png zlib)
yab_port_success(yabause-nx)

#install(TARGETS yabause-xu4 DESTINATION "/opt/retropie/emulators/yabasanshiro/")
#install(FILES "README.md" DESTINATION "/opt/retropie/emulators/yabasanshiro/" )

add_custom_command(TARGET yabause-nx
        POST_BUILD
        COMMAND /bin/bash ${CMAKE_CURRENT_SOURCE_DIR}/genSwitchExecutables.sh 
				)
				
install(TARGETS yabause-nx DESTINATION "bin")
install(FILES "README.md" DESTINATION "." )
