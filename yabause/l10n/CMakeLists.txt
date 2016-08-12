project(yabause-l10n)

set(LANGS ar da de el es fr it ja ko lt nl pl_PL pt_BR pt ru sv tr zh_CN zh_TW)

if (UNIX AND NOT APPLE)
    foreach(LANG ${LANGS})
	    install(FILES "yabause_${LANG}.yts" DESTINATION "share/yabause/yts" RENAME "${LANG}.yts")
    endforeach()
elseif (WIN32)
    foreach(LANG ${LANGS})
	    install(FILES "yabause_${LANG}.yts" DESTINATION "trans" RENAME "${LANG}.yts")
    endforeach()
endif ()
