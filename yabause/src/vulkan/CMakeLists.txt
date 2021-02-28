project(yabause-vulkan)

yab_port_start()

#include(external_glfw.cmake)
#include(external_glm.cmake)
#include(external_stb.cmake)
#include(external_shaderc.cmake)

include( ${CMAKE_SOURCE_DIR}/CMake/Packages/external_libpng.cmake )
find_package(SDL2 COMPONENTS sdl)
find_package(SDL2 REQUIRED)


set(YABAUSE_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../")
set(PORT_INCLUDE_DIRS  ${SDL2_INCLUDE_DIR} ${GLM_INCLUDE_DIRS} ${GLFW_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include  ${SHADERC_INCLUDE_DIR} ${png_INCLUDE_DIR} ${zlib_INCLUDE_DIR} )
set(PORT_LIBRARIES ${GLFW_LIBRARIES} ${CMAKE_CURRENT_SOURCE_DIR}/lib/x86/vulkan-1.lib ${SDL2_LIBRARY} ${LIBCHDR_LIBRARIES}  ${zlib_STATIC_LIBRARIES} ${SHADERC_LIBRARIES} ${png_STATIC_LIBRARIES} )

include_directories( ${YABAUSE_INCLUDE_DIR}  ${PORT_INCLUDE_DIRS})
set(yabause_vulkan_SOURCES
	yui.cpp
	Window.h
	Window_glfw.cpp
)

link_directories( ${link_directories} ${LIBCHDR_LIB_DIR} )
add_executable(yabause-vulkan ${yabause_vulkan_SOURCES})
target_link_libraries(yabause-vulkan yabause ${YABAUSE_LIBRARIES} ${PORT_LIBRARIES}  )

if (${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "AMD64")
  set(GLSL_VALIDATOR "$ENV{VULKAN_SDK}/Bin/glslangValidator.exe")
else()
  set(GLSL_VALIDATOR "$ENV{VULKAN_SDK}/Bin32/glslangValidator.exe")
endif()

file(GLOB_RECURSE GLSL_SOURCE_FILES
    "shaders/*.frag"
    "shaders/*.vert"
    )

foreach(GLSL ${GLSL_SOURCE_FILES})
  get_filename_component(FILE_NAME ${GLSL} NAME)
  set(SPIRV "${PROJECT_BINARY_DIR}/shaders/${FILE_NAME}.spv")
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/shaders/"
    COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
    DEPENDS ${GLSL})
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(
    Shaders 
    DEPENDS ${SPIRV_BINARY_FILES}
    )

add_dependencies(yabause-vulkan Shaders png)
add_dependencies(yabause-vulkan shaderc)

add_custom_command(TARGET yabause-vulkan POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:yabause-vulkan>/shaders/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_BINARY_DIR}/shaders"
        "$<TARGET_FILE_DIR:yabause-vulkan>/shaders"
        )
        
#set_property(TARGET yabause-vulkan PROPERTY
#  MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")        
				
yab_port_success(yabause-vulkan)

