EXTRA_DIST = cd.h
bin_PROGRAMS = yabause
yabause_SOURCES = \
	resource.h resource.rc settings.c settings.h yui.c \
        perdx.c perdx.h snddx.c snddx.h cheats.c cheats.h
yabause_LDFLAGS = -mwindows -mthreads
yabause_LDADD = ../libyabause.a resource.o -lmingw32 -lopengl32 -lglut32 -lkernel32 -lgdi32 -lcomctl32 -lcomdlg32 -lwinmm -ldxguid -ldinput8 -ldsound -ldxerr8
resource.o: resource.rc resource.h
	windres $< -o $@
