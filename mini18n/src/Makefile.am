lib_LTLIBRARIES = libmini18n.la
include_HEADERS = mini18n.h
libmini18n_la_SOURCES = mini18n.c \
	mini18n_pv_data.c mini18n_pv_hash.c mini18n_pv_list.c \
	mini18n_pv_data.h mini18n_pv_hash.h mini18n_pv_list.h \
	mini18n_pv_conv.h \
	mini18n_pv_file.h \
	mini18n_pv_file_po.c mini18n_pv_file_po.h \
	mini18n_pv_file_yts.c mini18n_pv_file_yts.h

if WANT_WINDOWS
libmini18n_la_SOURCES += mini18n_pv_conv_windows.c mini18n_pv_conv_windows.h
endif
