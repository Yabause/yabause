EXTRA_DIST = cd.c Makefile.mng logo.bmp logo.ico logo.png
SUBDIRS = c68k dreamcast freebsd linux macosx windows
bin_PROGRAMS = yabause
yabause_SOURCES = \
	c68k/c68kexec.c c68k/c68k.c c68k/gen68k.c \
	cdbase.c cdbase.h core.h cs0.c cs0.h cs1.c cs1.h cs2.c cs2.h \
	debug.c debug.h error.c error.h memory.c memory.h \
	peripheral.h peripheral.c m68kd.c \
	persdl.c persdl.h profile.c profile.h \
	scu.c scu.h sh2core.c sh2core.h \
	sh2d.c sh2d.h sh2idle.c sh2idle.h sh2int.c sh2int.h \
	smpc.c smpc.h scsp.c scsp.h sndsdl.c sndsdl.h vdp1.c vdp1.h \
	vdp2.c vdp2.h vidsdlgl.c vidsdlgl.h vidsdlsoft.c vidsdlsoft.h \
	yabause.c yabause.h ygl.h ygl.c \
	$(yabause_yui)/yui.c yui.h
nodist_yabause_SOURCES = c68k/c68k_ini.inc
yabause_CFLAGS = $(YAB_CFLAGS)
yabause_LDADD = $(YAB_LIBS) cd-$(yabause_arch).$(OBJEXT)
c68k/c68k_ini.inc:
	cd c68k && ./gen68k
