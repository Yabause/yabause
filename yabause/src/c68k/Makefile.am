EXTRA_DIST = c68k.h c68kmac.inc gen68k.h gen68k.inc types.h
bin_PROGRAMS = gen68k
gen68k_SOURCES = c68kexec.c c68k.c gen68k.c
gen68k_CFLAGS = -DC68K_GEN
gen68k_LDFLAGS = -DC68K_GEN
nodist_pkgdata_DATA = c68k_ini.inc
c68k_ini.inc: gen68k
	./gen68k
