SUBDIRS = doc
Applicationsdir = $(datadir)/applications
Applications_DATA = yabause.desktop
pixmapdir = $(datadir)/pixmaps
pixmap_DATA = yabause.png
bin_PROGRAMS = yabause
EXTRA_DIST = yabause.desktop yabause.png

yabause_SOURCES = gtkglwidget.c gtkglwidget.h main.c settings.c menu.c gtk-compat.c gtk-compat.h \
	yuifileentry.c yuifileentry.h yuirange.c yuirange.h yuiinputentry.c yuiinputentry.h yuipage.c yuipage.h \
	yuiresolution.c yuiresolution.h yuiwindow.c yuiwindow.h yuivdp1.c yuivdp1.h yuivdp2.c yuivdp2.h \
	yuiscsp.c yuiscsp.h pergtk.c pergtk.h yuiscreenshot.c yuiscreenshot.h \
	yuish.c yuish.h yuitransfer.c yuitransfer.h yuim68k.c yuim68k.h yuiscudsp.c yuiscudsp.h yuimem.c yuimem.h \
	yuiviewer.c yuiviewer.h settings.h \
	yuicheckbutton.c yuicheckbutton.h
yabause_CFLAGS = $(YAB_CFLAGS)
yabause_LDADD = ../libyabause.a $(YAB_LIBS)
yabause_CPPFLAGS = -DYTSDIR=\"$(datadir)/$(PACKAGE)/yts\"

install-data-hook:
	@for f in "$(DESTDIR)$(Applicationsdir)/yabause.desktop" ; do \
		fext=`echo $$f | sed 's,.\+\(\.[^.]\+\)$$,\1,'` ; \
		fbase=`basename "$$f" $$fext` ; \
		fdir=`dirname "$$f"` ; \
		fdest="$$fdir"/`echo "$$fbase" | sed '$(transform)'`$$fext ; \
		if test \( -e "$$f" \) -a \( ! "$$f" = "$$fdest" \) ; then \
			echo " mv $$f $$fdest" ; \
			mv "$$f" "$$fdest" ; \
		fi ; \
		nn=`echo yabause | sed '$(transform)'` ; \
		ftmp=/tmp/$$RANDOM ; \
		sed "s/Exec=yabause/Exec=$$nn/" $$fdest > $$ftmp ; \
		mv $$ftmp $$fdest ; \
	done

uninstall-hook:
	@for f in "$(DESTDIR)$(Applicationsdir)/yabause.desktop" ; do \
		fext=`echo $$f | sed 's,.\+\(\.[^.]\+\)$$,\1,'` ; \
		fbase=`basename "$$f" $$fext` ; \
		fdir=`dirname "$$f"` ; \
		fdest="$$fdir"/`echo "$$fbase" | sed '$(transform)'`$$fext ; \
		echo " rm -f $$fdest" ; \
		rm -f "$$fdest" ; \
	done
