project(YabaSanshiro)

cmake_minimum_required(VERSION 2.8)

#-------------------------------------------------------------------------------
#add local find scripts to CMAKE path
LIST(APPEND CMAKE_MODULE_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/CMake/Utils
    ${CMAKE_CURRENT_SOURCE_DIR}/CMake/Packages
)

option(YAB_FORCE_GLES20 "force to use OpenGLES 2.0" ON)
if (YAB_FORCE_GLES20)
set(GLSystem "OpenGL ES 2.0")
else(YAB_FORCE_GLES20)
set(GLSystem "Desktop OpenGL")
endif(YAB_FORCE_GLES20)

set(YAB_PACKAGE yabause)
set(YAB_VERSION_MAJOR 3)
set(YAB_VERSION_MINOR 3)
set(YAB_VERSION_PATCH 1)
set(YAB_VERSION "${YAB_VERSION_MAJOR}.${YAB_VERSION_MINOR}.${YAB_VERSION_PATCH}")

include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)
string(SUBSTRING ${GIT_SHA1} 0 6 GIT_SHA1_MINI )

set(CPACK_SOURCE_GENERATOR TGZ)
set(CPACK_PACKAGE_VERSION_MAJOR ${YAB_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${YAB_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${YAB_VERSION_PATCH})
set(CPACK_PACKAGE_VENDOR "devMiyax")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/EULA.txt")
set(CPACK_PACKAGE_FILE_NAME "yabasanshiro-${YAB_VERSION}-${GIT_SHA1_MINI}")
set(CPACK_PACKAGE_CONTACT "smiyaxdev@gmail.com" )


if (APPLE)
    set(CPACK_GENERATOR DragNDrop)
    set(CPACK_PACKAGE_FILE_NAME yabause-${YAB_VERSION}-mac)
endif ()

if (WIN32)
	SET(CPACK_NSIS_INSTALLED_ICON_NAME yabasanshiro.exe)
	set(CPACK_NSIS_MENU_LINKS yabasanshiro.exe;"Yaba Sanshiro")
	set(CPACK_NSIS_URL_INFO_ABOUT "http://www.uoyabause.org")
	set(CPACK_NSIS_COMPRESSOR "/SOLID lzma")
	if(${CMAKE_SIZEOF_VOID_P} EQUAL 8)
		set(CPACK_SYSTEM_NAME "win64")
		set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
		set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "${CPACK_PACKAGE_NAME}${CPACK_PACKAGE_VERSION} (Win64)")
	endif ()
	set(CPACK_WIX_UPGRADE_GUID "A25AD461-7B7E-40A3-BCB5-390EAAEF10AE" )
	set(CPACK_WIX_PRODUCT_ICON "${CMAKE_CURRENT_SOURCE_DIR}/src/android/app/src/main/res/mipmap-xxxhdpi/ic_launcher_round.png" )
	set(CPACK_WIX_UI_BANNER "${CMAKE_CURRENT_SOURCE_DIR}/res/banner.png")
	set(CPACK_WIX_UI_DIALOG "${CMAKE_CURRENT_SOURCE_DIR}/res/dialog.png")
	set(CPACK_WIX_CULTURES en-US ja-jp pt-BR fr-FR)
	set(CPACK_WIX_PROPERTY_ARPHELPLINK "http://www.uoyabause.org/")
endif ()

include(CPack)

add_subdirectory(doc)
add_subdirectory(l10n)
add_subdirectory(src)
