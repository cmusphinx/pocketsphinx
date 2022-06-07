bin_PROGRAMS = sphinx_pitch sphinx_cont_seg

sphinx_cont_seg_SOURCES = cont_seg.c

LDADD = $(top_builddir)/src/libsphinxbase/libsphinxbase.la \
	 $(top_builddir)/src/libsphinxad/libsphinxad.la

AM_CFLAGS = -I$(top_srcdir)/include -I$(top_builddir)/include
