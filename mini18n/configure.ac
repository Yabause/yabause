AC_INIT(mini18n, 0.1)

AC_CONFIG_MACRO_DIR([m4])

AC_CANONICAL_HOST

AM_INIT_AUTOMAKE(mini18n, 0.1)

AC_PROG_LIBTOOL
AC_PROG_CC

if test `expr x$CC : x$host_alias` -eq 0 ; then
	AC_MSG_ERROR([$CC is not a cross compiler and we're cross-compiling.])
fi

AC_PROG_INSTALL

AC_LANG(C)

AC_CHECK_HEADER([windows.h], [
	AC_DEFINE([HAVE_UTF16])
	want_windows=yes], [])

AC_ARG_ENABLE(log, AC_HELP_STRING(--enable-log, log non translated strings) , AC_DEFINE(MINI18N_LOG))

AC_CONFIG_FILES([Makefile
	doc/Doxyfile
	src/Makefile
	tools/Makefile
	example/Makefile
])

if test x${want_windows} = "xyes" ; then
	AC_DEFINE([HAVE_UTF16])
fi

AM_CONDITIONAL([WANT_WINDOWS], [test x${want_windows} = "xyes"])

AC_OUTPUT
