QPRO_PWD   = $$PWD

HEADERS	+= $${QPRO_PWD}/include/cfgfmt.h \
	$${QPRO_PWD}/include/cs.h \
	$${QPRO_PWD}/include/disk.h \
	$${QPRO_PWD}/include/fio.h \
	$${QPRO_PWD}/include/forcefeedback.h \
	$${QPRO_PWD}/include/jsw.h \
	$${QPRO_PWD}/include/os.h \
	$${QPRO_PWD}/include/prochandle.h \
	$${QPRO_PWD}/include/string.h \
	$${QPRO_PWD}/include/xsw_ctype.h

SOURCES	+= $${QPRO_PWD}/libjsw/attributes.c \
	$${QPRO_PWD}/libjsw/axisio.c \
	$${QPRO_PWD}/libjsw/buttonio.c \
	$${QPRO_PWD}/libjsw/calibrationfio.c \
	$${QPRO_PWD}/libjsw/disk.cpp \
	$${QPRO_PWD}/libjsw/fio.cpp \
	$${QPRO_PWD}/libjsw/forcefeedback.c \
	$${QPRO_PWD}/libjsw/jsw.c \
	$${QPRO_PWD}/libjsw/string.cpp \
	$${QPRO_PWD}/libjsw/utils.c
