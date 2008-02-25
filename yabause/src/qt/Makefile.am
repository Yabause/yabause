EXTRA_DIST = CommonDialogs.cpp CommonDialogs.h JSWHelper.cpp JSWHelper.h \
	main.cpp PerJSW.cpp PerJSW.h PerQt.c PerQt.h PerQtSDL.c PerQtSDL.h \
	QtYabause.cpp QtYabause.h Settings.cpp Settings.h yabause.desktop \
	YabauseGL.cpp YabauseGL.h yabause.pro.in YabauseThread.cpp YabauseThread.h \
	resources/resources.qrc \
	resources/icons/made.png resources/icons/yabause.icns resources/icons/yabause.png \
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
endif
