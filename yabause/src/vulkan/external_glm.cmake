include(ExternalProject)
find_package(GLM QUIET)

if(GLM_FOUND)
    message(STATUS "Found GLM")
else()
    message(STATUS "GLM not found - will build from source")

    ExternalProject_Add(glm PREFIX glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG 0.9.9.0

        UPDATE_COMMAND "" BUILD_COMMAND "" INSTALL_COMMAND ""

        LOG_DOWNLOAD 1 LOG_UPDATE 1 LOG_CONFIGURE 1 LOG_BUILD 1 LOG_INSTALL 1
    )

    ExternalProject_Get_Property(glm SOURCE_DIR)
    set(GLM_INCLUDE_DIRS ${SOURCE_DIR})
endif()

set(GLM_INCLUDE_DIRS ${GLM_INCLUDE_DIRS} CACHE STRING "")