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
CONFIG	+= debug_and_release
QT	+= opengl
LIBS	+= -L../ -lyabause
win32:LIBS	+= -lopengl32
!mac:LIBS	+= -lSDL -lglut
else:LIBS	+= -framework SDL -framework IOKit -framework glut
RESOURCES	+= resources/resources.qrc
mac:ICON	+= resources/icons/yabause.icns

# program defines
DEFINES	+= "PACKAGE_NAME=\"\\\"yabause\\\"\"" \
	"PACKAGE_TARNAME=\"\\\"yabause\\\"\"" \
	"PACKAGE_VERSION=\"\\\"0.9.3\\\"\"" \
	"PACKAGE_BUGREPORT=\"\\\"\\\"\"" \
	"PACKAGE=\"\\\"yabause\\\"\"" \
	"VERSION=\"\\\"0.9.3\\\"\"" \
	"PACKAGE_STRING=\"\\\"yabause 0.9.3\\\"\""

# include defines
DEFINES	+= STDC_HEADERS=1 \
	HAVE_SYS_TYPES_H=1 \
	HAVE_SYS_STAT_H=1 \
	DHAVE_STDLIB_H=1 \
	HAVE_STRING_H=1 \
	HAVE_MEMORY_H=1 \
	HAVE_STRINGS_H=1 \
	HAVE_INTTYPES_H=1 \
	HAVE_STDINT_H=1 \
	HAVE_UNISTD_H=1 \
	HAVE_LIBSDL=1 \
	HAVE_LIBGL=1 \
	HAVE_C99_VARIADIC_MACROS=1 \
	HAVE_C68K=1 \
	USENEWPERINTERFACE=1 \
	DEBUG=1

win32:DEFINES	+= _WIN32_IE=0x0400

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
include( "3rdparty/libjsw.pri" )

FORMS	+= ui/UIYabause.ui \
	ui/UISettings.ui \
	ui/UIAbout.ui \
	ui/UICheats.ui \
	ui/UICheatAR.ui \
	ui/UICheatRaw.ui \
	ui/UIWaitInput.ui

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
	JSWHelper.h \
	PerJSW.h \
	PerQtSDL.h

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
	JSWHelper.cpp \
	PerJSW.cpp \
	PerQtSDL.c
