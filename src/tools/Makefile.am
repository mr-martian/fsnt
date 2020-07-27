
AM_CPPFLAGS = -I${top_srcdir}/src
LDADD = $(top_builddir)/src/lib/libfsnt.la

# uncomment for debugging
AM_LDFLAGS = -no-install

bin_PROGRAMS = fsnt-compose fsnt-expand fsnt-fst2txt fsnt-optimize-flags \
	fsnt-reverse fsnt-strip fsnt-txt2fst

fsnt_compose_SOURCES = compose.cc
fsnt_expand_SOURCES = expand.cc
fsnt_fst2txt_SOURCES = fst2txt.cc
fsnt_optimize_flags_SOURCES = optimize-flags.cc
fsnt_reverse_SOURCES = reverse.cc
fsnt_strip_SOURCES = strip.cc
fsnt_txt2fst_SOURCES = txt2fst.cc
