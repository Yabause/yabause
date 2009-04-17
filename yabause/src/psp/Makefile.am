EXTRA_DIST = sh2-core.i icache-funcs-2450.patch

# The ELF file is only an imtermediate target; the final target is EBOOT.PBP
# (but keep the ELF and PRX files around for debugging/testing purposes)
all-local: EBOOT.PBP
.SECONDARY: yabause.elf yabause.prx

noinst_PROGRAMS = yabause.elf
yabause_elf_SOURCES = display.c main.c profile.c psp-per.c psp-sh2.c \
                      psp-sound.c psp-video.c sys.c texcache.c \
                      common.h display.h mips.h profile.h psp-per.h \
                      psp-sh2.h psp-sound.h psp-video.h sys.h texcache.h
# Can't use -Wstrict-prototypes without cleaning up ../*.h
yabause_elf_CFLAGS = $(YAB_CFLAGS) -Wmissing-prototypes #-Wstrict-prototypes 
yabause_elf_LDADD = ../libyabause.a $(YAB_LIBS)

yabause.prx: yabause.elf
	psp-fixup-imports $<
	psp-prxgen $< $@

yabause.sfo:
#	mksfo "Yabause $(PACKAGE_VERSION)" $@
	mksfoex -d MEMSIZE=1 "Yabause $(PACKAGE_VERSION)" $@

EBOOT.PBP: yabause.prx yabause.sfo
	pack-pbp $@ yabause.sfo NULL NULL NULL NULL NULL yabause.prx NULL
#	pack-pbp $@ yabause.sfo icon0.png NULL NULL pic1.png NULL yabause.prx NULL
