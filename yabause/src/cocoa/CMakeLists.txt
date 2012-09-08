project(yabause-cocoa)

find_library(AUDIO_LIBRARY AudioUnit)

set(yabause_cocoa_SOURCES
	main.m
	PerCocoa.m
	YabauseButtonFormatter.m
	YabauseController.m
	YabauseGLView.m
	YabausePrefsController.m
	vidgcd.c
	vidgcd.h
)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/..")

add_executable(yabause-cocoa ${yabause_cocoa_SOURCES})
message(STATUS ${YABAUSE_LIBRARIES})
target_link_libraries(yabause-cocoa yabause ${YABAUSE_LIBRARIES}
	${AUDIO_LIBRARY})
