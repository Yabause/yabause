EXTRA_DIST = Makefile.dc Makefile.mng logo.bmp logo.png Makefile.wii logo.svg
if COMPILE_C68K
SUBDIRS = c68k . $(YUI_SUBDIR)
else
SUBDIRS = . $(YUI_SUBDIR)
endif
if YUI_IS_CARBON
YUI_SUBDIR=carbon
endif
if YUI_IS_DREAMCAST
YUI_SUBDIR=dreamcast
endif
if YUI_IS_GTK
YUI_SUBDIR=gtk
endif
if YUI_IS_QT
YUI_SUBDIR=qt
endif
if YUI_IS_WII
YUI_SUBDIR=wii
endif
if YUI_IS_WINDOWS
YUI_SUBDIR=windows
endif
noinst_LIBRARIES = libyabause.a
libyabause_a_SOURCES = \
        bios.c bios.h cdbase.c cdbase.h cheat.c cheat.h coffelf.c coffelf.h \
        core.h cs0.c cs0.h cs1.c cs1.h cs2.c cs2.h \
	debug.c debug.h error.c error.h memory.c memory.h \
	m68kcore.c m68kcore.h \
        m68kd.c netlink.c netlink.h peripheral.h peripheral.c \
	persdljoy.c persdljoy.h profile.c profile.h \
	scu.c scu.h sh2core.c sh2core.h \
	sh2d.c sh2d.h sh2idle.c sh2idle.h sh2int.c sh2int.h \
	smpc.c smpc.h scsp.c scsp.h sndal.c sndal.h sndsdl.c sndsdl.h vdp1.c vdp1.h \
        vdp2.c vdp2.h vdp2debug.c vdp2debug.h vidogl.c vidogl.h vidshared.c vidshared.h vidsoft.c vidsoft.h \
	yabause.c yabause.h ygl.h ygl.c yui.h
if COMPILE_C68K
libyabause_a_SOURCES += c68k/c68kexec.c c68k/c68k.c c68k/gen68k.c m68kc68k.c m68kc68k.h
c68kexec.o: c68k/c68kexec.c
	$(COMPILE) -Ic68k -O0 $(srcdir)/c68k/c68kexec.c -c -o c68kexec.o
endif
if ARCH_IS_FREEBSD
libyabause_a_SOURCES += cd-freebsd.c
endif
if ARCH_IS_LINUX
libyabause_a_SOURCES += cd-linux.c perlinuxjoy.c perlinuxjoy.h
endif
if ARCH_IS_MACOSX
libyabause_a_SOURCES += cd-macosx.c macjoy.c macjoy.h permacjoy.c permacjoy.h
endif
if ARCH_IS_NETBSD
libyabause_a_SOURCES += cd-netbsd.c
endif
if ARCH_IS_WINDOWS
libyabause_a_SOURCES += cd-windows.c
endif
