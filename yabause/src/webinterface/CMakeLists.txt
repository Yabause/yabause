set(yabause_SOURCES
    ${yabause_SOURCES}
    src/YabauseWebInterface.cpp
    src/CivetServer.cpp
    src/civetweb.c
    PARENT_SCOPE
    )
    
set(yabause_HEADERS
    ${yabause_HEADERS}
    include/CivetServer.h
    include/civetweb.h 
    PARENT_SCOPE
    )
