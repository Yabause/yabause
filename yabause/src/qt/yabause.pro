###########################################################################################
##		Created using Monkey Studio v1.8.1.0
##
##	Author    : Filipe AZEVEDO aka Nox P@sNox <pasnox@gmail.com>
##	Project   : yabause
##	FileName  : yabause.pro
##	Date      : 2008-01-28T23:22:47
##	License   : GPL
##	Comment   : Creating using Monkey Studio RAD
##	Home Page   : http://www.monkeystudio.org
##
##	This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
##	WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
##
###########################################################################################

TEMPLATE	= app
LANGUAGE	= Qt4/C++
TARGET	= yabause
mac:TARGET	= Yabause
CONFIG	+= debug_and_release
QT	+= opengl
LIBS	+= -L../ -lyabause  -L/usr/lib -lSDL  -lGL -lglut -Wl,--export-dynamic -lgtkglext-x11-1.0 -lgdkglext-x11-1.0 -lGLU -lGL -lXmu -lXt -lSM -lICE -lgtk-x11-2.0 -lpangox-1.0 -lgdk-x11-2.0 -latk-1.0 -lgdk_pixbuf-2.0 -lm -lpangocairo-1.0 -lfontconfig -lXext -lXrender -lXinerama -lXi -lXrandr -lXcursor -lXcomposite -lXdamage -lpango-1.0 -lcairo -lX11 -lXfixes -lgobject-2.0 -lgmodule-2.0 -ldl -lglib-2.0    -L/usr/lib -lSDL  -lGL -lglut
AC_DEFS = -DPACKAGE_NAME=\"yabause\" -DPACKAGE_TARNAME=\"yabause\" -DPACKAGE_VERSION=\"0.9.3\" -DPACKAGE_STRING=\"yabause\ 0.9.3\" -DPACKAGE_BUGREPORT=\"\" -DPACKAGE=\"yabause\" -DVERSION=\"0.9.3\" -DSTDC_HEADERS=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_MEMORY_H=1 -DHAVE_STRINGS_H=1 -DHAVE_INTTYPES_H=1 -DHAVE_STDINT_H=1 -DHAVE_UNISTD_H=1 -DHAVE_LIBSDL=1 -DHAVE_LIBGL=1 -DHAVE_LIBGLUT=1 -DHAVE_C99_VARIADIC_MACROS=1 -DHAVE_C68K=1
QMAKE_CXXFLAGS	+= $$replace( AC_DEFS, "\", "\\\"" )
QMAKE_CFLAGS	+= $$replace( AC_DEFS, "\", "\\\"" )
# i don't know why u limit the build to panther, but it's a bad idea for leopard users
QMAKE_CXXFLAGS	-= -DMAC_OS_X_VERSION_MAX_ALLOWED=MAC_OS_X_VERSION_10_3
QMAKE_CFLAGS	-= -DMAC_OS_X_VERSION_MAX_ALLOWED=MAC_OS_X_VERSION_10_3
RESOURCES	+= resources/resources.qrc
mac:ICON	+= resources/icons/yabause.icns

BUILD_PATH	= ./build
BUILDER	= GNUMake
COMPILER	= G++
EXECUTE_RELEASE	= yabause
EXECUTE_DEBUG	= yabause_debug

CONFIG(debug, debug|release) {
	#Debug
	CONFIG	+= console
	unix:TARGET	= $$join(TARGET,,,_debug)
	else:TARGET	= $$join(TARGET,,,d)
	unix:OBJECTS_DIR	= $${BUILD_PATH}/debug/.obj/unix
	win32:OBJECTS_DIR	= $${BUILD_PATH}/debug/.obj/win32
	mac:OBJECTS_DIR	= $${BUILD_PATH}/debug/.obj/mac
	UI_DIR	= $${BUILD_PATH}/debug/.ui
	MOC_DIR	= $${BUILD_PATH}/debug/.moc
	RCC_DIR	= $${BUILD_PATH}/debug/.rcc
} else {
	#Release
	unix:OBJECTS_DIR	= $${BUILD_PATH}/release/.obj/unix
	win32:OBJECTS_DIR	= $${BUILD_PATH}/release/.obj/win32
	mac:OBJECTS_DIR	= $${BUILD_PATH}/release/.obj/mac
	UI_DIR	= $${BUILD_PATH}/release/.ui
	MOC_DIR	= $${BUILD_PATH}/release/.moc
	RCC_DIR	= $${BUILD_PATH}/release/.rcc
}

# include jsw library files
unix:!mac {
	include( 3rdparty/libjsw.pri )
	
	DEFINES	+= HAVE_LIBJSW=1

	HEADERS	+= JSWHelper.h \
		PerJSW.h
		
	SOURCES	+= JSWHelper.cpp \
		PerJSW.cpp
}

FORMS	+= ui/UIYabause.ui \
	ui/UISettings.ui \
	ui/UIAbout.ui \
	ui/UICheats.ui \
	ui/UICheatAR.ui \
	ui/UICheatRaw.ui \
	ui/UIWaitInput.ui \
	ui/UIBackupRam.ui

HEADERS	+= ui/UIYabause.h \
	YabauseGL.h \
	ui/UISettings.h \
	Settings.h \
	YabauseThread.h \
	QtYabause.h \
	ui/UIAbout.h \
	ui/UICheats.h \
	ui/UICheatAR.h \
	ui/UICheatRaw.h \
	CommonDialogs.h \
	PerQt.h \
	ui/UIWaitInput.h \
	PerQtSDL.h \
	ui/UIBackupRam.h

SOURCES	+= main.cpp \
	ui/UIYabause.cpp \
	YabauseGL.cpp \
	ui/UISettings.cpp \
	Settings.cpp \
	YabauseThread.cpp \
	QtYabause.cpp \
	ui/UIAbout.cpp \
	ui/UICheats.cpp \
	ui/UICheatAR.cpp \
	ui/UICheatRaw.cpp \
	CommonDialogs.cpp \
	PerQt.c \
	ui/UIWaitInput.cpp \
	PerQtSDL.c \
	ui/UIBackupRam.cpp
