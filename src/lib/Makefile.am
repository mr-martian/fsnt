
SUBDIRS=utils
AUTOMAKE_OPTIONS=subdir-objects

lib_LTLIBRARIES = libfsnt.la

libfsnt_la_SOURCES = \
	transition.cc symbol_table.cc transducer.cc \
	io.cc to_lookup.cc tokenizer.cc \
	compose.cc optimize_flags.cc relabel.cc reverse.cc strip.cc

include_HEADERS = \
	transition.h symbol_table.h transducer.h \
	io.h to_lookup.h tokenizer.h \
	compose.h optimize_flags.h relabel.h reverse.h strip.h

libfsnt_la_LIBADD = \
	utils/libfsntutils.la
