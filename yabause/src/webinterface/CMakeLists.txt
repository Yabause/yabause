set(yabause_SOURCES
    ${yabause_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/YabauseWebInterface.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CivetServer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/civetweb.c
    PARENT_SCOPE
    )
    
set(yabause_HEADERS
    ${yabause_HEADERS}
    ${CMAKE_CURRENT_SOURCE_DIR}/include/CivetServer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/civetweb.h 
    PARENT_SCOPE
    )
