project(yabause-xu4)

add_definitions( -D__RETORO_ARENA__ )

yab_port_start()


find_package(SDL2 COMPONENTS sdl)
find_package(SDL2 REQUIRED)
find_package(OpenGLES)
find_package(PkgConfig REQUIRED)

find_package(Boost REQUIRED COMPONENTS system filesystem date_time locale)

if (NOT OPENGLES2_FOUND)
	return()
endif (NOT OPENGLES2_FOUND)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_subdirectory(pugixml)
add_subdirectory(nanogui-sdl)

include( ../glfw/external_zlib.cmake )
include( ../glfw/external_libpng.cmake )

#------------------------------------------------------------------------------
# nlohmann/json
#  * sets EXTERNAL_JSON_INCLUDE_DIR 

ExternalProject_Add(
     Json
     PREFIX ${CMAKE_CURRENT_BINARY_DIR}/Json
     GIT_REPOSITORY https://github.com/nlohmann/json.git
     # For shallow git clone (without downloading whole history)
     #GIT_SHALLOW 1
     # For point at certain tag
     GIT_TAG v3.2.0
     #disables auto update on every build
     UPDATE_DISCONNECTED 1
     #disable following
     CONFIGURE_COMMAND "" BUILD_COMMAND "" INSTALL_DIR "" INSTALL_COMMAND ""
     )
 # Update json target
 add_custom_target(external-Json-update
     COMMENT "Updated Nlohmann/Json"
     WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Json/src/Json
     COMMAND ${GIT_EXECUTABLE} pull
     DEPENDS Json)
# set include directory
set(EXTERNAL_JSON_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/Json/src/Json/single_include")

set(PORT_INCLUDE_DIRS ${EXTERNAL_JSON_INCLUDE_DIR} ${SDL2_INCLUDE_DIR} ${png_INCLUDE_DIR} ${zlib_INCLUDE_DIR} )
set(PORT_LIBRARIES ${SDL2_LIBRARY} ${OPENGLES2_LIBRARIES} ${png_STATIC_LIBRARIES} ${zlib_STATIC_LIBRARIES} )

message("Nlohmann/json dir =" ${EXTERNAL_JSON_INCLUDE_DIR})

set(yabause_xu4_SOURCES
	main.cpp
	InputManager.h
	InputManager.cpp
	InputConfig.h
	InputConfig.cpp
	MenuScreen.h
	MenuScreen.cpp
) 

include_directories( ${PORT_INCLUDE_DIRS} ${Boost_INCLUDE_DIR} "nanogui-sdl/include" "nanogui-sdl/ext/eigen" )
add_executable(yabause-xu4 ${yabause_xu4_SOURCES})
target_link_libraries(yabause-xu4 yabause  ${Boost_LIBRARIES} pugixml ${YABAUSE_LIBRARIES} ${PORT_LIBRARIES} nanogui )
add_dependencies(yabause-xu4 Json png zlib)
yab_port_success(yabause-xu4)

#install(TARGETS yabause-xu4 DESTINATION "/opt/retropie/emulators/yabasanshiro/")
#install(FILES "README.md" DESTINATION "/opt/retropie/emulators/yabasanshiro/" )

install(TARGETS yabause-xu4 DESTINATION "bin")
install(FILES "README.md" DESTINATION "." )
