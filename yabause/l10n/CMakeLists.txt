project(yabause-l10n)

set(LANGS ar de es fr it lt nl pt pt_BR ru sv zh_CN zh_TW)

if (UNIX AND NOT APPLE)
    foreach(LANG ${LANGS})
	    install(FILES "kronos_${LANG}.yts" DESTINATION "share/kronos/yts" RENAME "${LANG}.yts")
    endforeach()
elseif (WIN32)
    foreach(LANG ${LANGS})
	    install(FILES "kronos_${LANG}.yts" DESTINATION "trans" RENAME "${LANG}.yts")
    endforeach()
endif ()
