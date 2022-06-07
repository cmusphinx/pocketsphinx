bin_PROGRAMS = sphinx_lm_eval sphinx_lm_convert

LDADD = \
	$(top_builddir)/src/libsphinxbase/libsphinxbase.la \
	$(top_builddir)/src/libsphinxbase/lm/libsphinxlm.la

AM_CFLAGS =-I$(top_srcdir)/include -I$(top_builddir)/include \
	-I$(top_srcdir)/src/libsphinxlm

