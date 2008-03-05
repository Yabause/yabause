AC_DEFUN([YAB_CHECK_HOST_TOOLS],
	[
		AC_CHECK_TOOLS([$1], [$2])
		if test `expr x$[$1] : x$host_alias` -eq 0 ; then
			[$1]=""
		fi
	])
