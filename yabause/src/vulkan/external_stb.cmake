include(ExternalProject)

# Download `stb` and extract source path.
ExternalProject_Add(stb_EXTERNAL
    GIT_REPOSITORY "https://github.com/nothings/stb.git"
    GIT_TAG "master"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    LOG_DOWNLOAD 1
)

# Recover project paths for additional settings.
ExternalProject_Get_Property(stb_EXTERNAL SOURCE_DIR)
set(stb_INCLUDE_DIR "${SOURCE_DIR}")

# Workaround for https://cmake.org/Bug/view.php?id=15052
file(MAKE_DIRECTORY "${stb_INCLUDE_DIR}")

# Tell CMake that the external project generated a library so we
# can add dependencies to the library here.
add_library(stb INTERFACE IMPORTED)
add_dependencies(stb stb_EXTERNAL)
set_property(TARGET stb
    PROPERTY INTERFACE_INCLUDE_DIRECTORIES
    "${stb_INCLUDE_DIR}"
)