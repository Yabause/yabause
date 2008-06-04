EXTRA_DIST = CommonDialogs.cpp CommonDialogs.h \
	main.cpp PerQt.c PerQt.h \
	QtYabause.cpp QtYabause.h Settings.cpp Settings.h yabause.desktop \
	YabauseGL.cpp YabauseGL.h yabause.pro.in YabauseThread.cpp YabauseThread.h \
	resources/resources.qrc \
	resources/icons/made.png resources/icons/yabause.icns resources/icons/yabause.png resources/icons/yabause.rc \
	resources/icons/actions/about.png resources/icons/actions/cd.png resources/icons/actions/fps.png \
	resources/icons/actions/layer.png resources/icons/actions/no_fullscreen.png resources/icons/actions/quit.png \
	resources/icons/actions/settings.png resources/icons/actions/backup_ram.png resources/icons/actions/cheats.png \
	resources/icons/actions/frame_skipping.png resources/icons/actions/load_state.png resources/icons/actions/open_file.png \
	resources/icons/actions/reset.png resources/icons/actions/sound.png resources/icons/actions/button_cancel.png \
	resources/icons/actions/fullscreen.png resources/icons/actions/log.png resources/icons/actions/pause.png \
	resources/icons/actions/save_state.png resources/icons/actions/transfert.png resources/icons/actions/button_ok.png \
	resources/icons/actions/emu-compatibility.png resources/icons/actions/iso.png resources/icons/actions/mute.png \
	resources/icons/actions/play.png resources/icons/actions/screenshot.png resources/icons/actions/video.png \
	ui/UIAbout.cpp ui/UIAbout.h ui/UIAbout.ui \
	ui/UIBackupRam.cpp ui/UIBackupRam.h ui/UIBackupRam.ui \
	ui/UICheatAR.cpp ui/UICheatAR.h ui/UICheatAR.ui \
	ui/UICheatRaw.cpp ui/UICheatRaw.h ui/UICheatRaw.ui \
	ui/UICheats.cpp ui/UICheats.h ui/UICheats.ui \
	ui/UISettings.cpp ui/UISettings.h ui/UISettings.ui \
	ui/UIWaitInput.cpp ui/UIWaitInput.h ui/UIWaitInput.ui \
	ui/UIYabause.cpp ui/UIYabause.h ui/UIYabause.ui

if YUI_IS_QT
all-local: Makefile.qmake
	$(MAKE) -f Makefile.qmake release

clean-local: Makefile.qmake
	$(MAKE) -f Makefile.qmake clean

distclean-local: Makefile.qmake.Release Makefile.qmake.Debug
	$(MAKE) -f Makefile.qmake.Release distclean
	$(MAKE) -f Makefile.qmake.Debug distclean
endif
