DISTCLEANFILES = asm.h core.h funcs.h fp.h

LIGHTNING_FILES = funcs-common.h core-common.h fp-common.h \
	asm-common.h \
	i386/asm.h i386/core.h i386/funcs.h i386/fp.h \
	sparc/asm.h sparc/core.h sparc/funcs.h sparc/fp.h \
	ppc/asm.h ppc/core.h ppc/funcs.h ppc/fp.h

if LIGHTNING_MAIN
lightningdir = $(includedir)/lightning
dist_pkgdata_DATA = Makefile.am
nobase_dist_lightning_HEADERS = $(LIGHTNING_FILES)
nodist_lightning_HEADERS = asm.h core.h funcs.h fp.h 
else
dist_noinst_HEADERS = $(LIGHTNING_FILES)
endif
