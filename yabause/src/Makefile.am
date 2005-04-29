bin_PROGRAMS = yabause
if DYNAREC
yabause_SOURCES = cdbase.cc cs0.cc cs1.cc cs2.cc exception.cc intc.cc memory.cc scu.cc smpc.cc superh.cc vdp1.cc vdp2.cc cs1.hh intc.hh vdp1.hh cs2.hh memory.hh saturn_memory.hh saturn.hh smpc.hh vdp2.hh exception.hh registres.hh scu.hh superh.hh yabause.cc scsp.hh scsp.cc $(yabause_arch)/cd.cc $(yabause_yui)/yui.cc c68k/c68kexec.c c68k/c68k.c c68k/gen68k.c c68k/c68k_ini.inc ygl.cc ygl.hh jit/hash.hh jit/hash.cc jit/lru2.hh jit/lru2.cc
else
yabause_SOURCES = cdbase.cc cs0.cc cs1.cc cs2.cc exception.cc intc.cc memory.cc scu.cc smpc.cc superh.cc vdp1.cc vdp2.cc cs1.hh intc.hh vdp1.hh cs2.hh memory.hh saturn_memory.hh saturn.hh smpc.hh vdp2.hh exception.hh registres.hh scu.hh superh.hh yabause.cc scsp.hh scsp.cc $(yabause_arch)/cd.cc $(yabause_yui)/yui.cc c68k/c68kexec.c c68k/c68k.c c68k/gen68k.c c68k/c68k_ini.inc ygl.cc ygl.hh
endif
BUILT_SOURCES = c68k/c68k_ini.inc
c68k/c68k_ini.inc: c68k/gen68k
	cd c68k && ./gen68k
c68k/gen68k: c68k/c68kexec.c c68k/c68k.c c68k/gen68k.c
	$(COMPILE) -DC68K_GEN c68k/c68kexec.c c68k/c68k.c c68k/gen68k.c -o c68k/gen68k
EXTRA_DIST = logo.png logo.bmp Makefile.dc tree.h tree.c
