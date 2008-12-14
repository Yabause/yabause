AC_INIT(mini18n, 0.1)

AC_CANONICAL_HOST

case $host in
	*mingw32*)
		want_windows=yes
		AC_DEFINE([HAVE_UTF16])
		;;
esac

AM_INIT_AUTOMAKE(mini18n, 0.1)

AC_PROG_LIBTOOL

AC_PROG_CC
AC_PROG_INSTALL

AC_LANG(C)

AC_ARG_ENABLE(log, AC_HELP_STRING(--enable-log, log non translated strings) , AC_DEFINE(MINI18N_LOG))

AC_CONFIG_FILES([Makefile
	src/Makefile
	example/Makefile
])

AM_CONDITIONAL([WANT_WINDOWS], [test x${want_windows} = "xyes"])

AC_OUTPUT
