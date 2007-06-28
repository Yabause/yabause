bin_PROGRAMS = yabause
yabause_SOURCES = main.c settings.c settings.h cpustatus.c cpustatus.h
yabause_CFLAGS = $(YAB_CFLAGS)
yabause_LDADD = $(YAB_LIBS) ../libyabause.a

all-local:
	if [ ! -e Yabause.app/Contents/MacOS ]; then mkdir Yabause.app/Contents/MacOS; fi
	cp yabause Yabause.app/Contents/MacOS
	if [ ! -e Yabause.app/Contents/Frameworks ]; then mkdir Yabause.app/Contents/Frameworks; fi
	if [ ! -e Yabause.app/Contents/Frameworks/SDL.framework ]; then \
		if [ -e /Library/Frameworks/SDL.framework ]; then \
			cp -R /Library/Frameworks/SDL.framework Yabause.app/Contents/Frameworks; \
		elif [ -e ~/Library/Frameworks/SDL.framework ]; then \
			cp -R ~/Library/Frameworks/SDL.framework Yabause.app/Contents/Frameworks; \
		fi; \
	fi
