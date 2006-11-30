bin_PROGRAMS = yabause
yabause_SOURCES = main.c settings.c settings.h cpustatus.c cpustatus.h
yabause_CFLAGS = $(YAB_CFLAGS)
yabause_LDADD = $(YAB_LIBS) ../libyabause.a

all-local:
	if [ ! -e Yabause.app/Contents/MacOS ]; then mkdir Yabause.app/Contents/MacOS; fi
	cp yabause Yabause.app/Contents/MacOS
