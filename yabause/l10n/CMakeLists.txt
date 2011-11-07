project(yabause-l10n)

set(LANGS de es fr it lt pt pt_BR sv)

foreach(LANG ${LANGS})
	install(FILES "yabause_${LANG}.yts" DESTINATION "share/yabause/yts" RENAME "${LANG}.yts")
endforeach()
