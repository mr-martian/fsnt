
SUBDIRS=utils
AUTOMAKE_OPTIONS=subdir-objects

lib_LTLIBRARIES = libfsnt.la

libfsnt_la_SOURCES = \
	transition.cc symbol_table.cc string_interner.cc transducer.cc \
	io.cc \
	compose.cc reverse.cc

include_HEADERS = \
	transition.h symbol_table.h string_interner.h transducer.h \
	io.h \
	compose.h reverse.h

libfsnt_la_LIBADD = \
	utils/libfsntutils.la
