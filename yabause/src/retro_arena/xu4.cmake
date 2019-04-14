set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

set (USE_EGL True)

set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_POSIX_C_SOURCE=199309L" )
add_definitions( -D__XU4__ )
add_definitions( -D__RETORO_ARENA__ )
