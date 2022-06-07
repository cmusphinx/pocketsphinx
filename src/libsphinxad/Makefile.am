
# libsphinxad: Audio Device interface for Sphinx

lib_LTLIBRARIES = libsphinxad.la

# Confusing libtool version info means:
#
libsphinxad_la_LDFLAGS = -version-info 3:0:0

libsphinxad_la_SOURCES = 

EXTRA_libsphinxad_la_SOURCES = \
	ad_base.c \
	ad_oss.c \
	ad_alsa.c \
	ad_pulse.c \
	ad_openal.c \
	ad_win32.c

libsphinxad_la_DEPENDENCIES = @ad_files@

libsphinxad_la_LIBADD = \
	@ad_files@ \
	@ad_libs@ \
	$(top_builddir)/src/libsphinxbase/libsphinxbase.la

AM_CFLAGS =\
	@ad_cppflags@ \
	-I$(top_srcdir)/include/sphinxbase \
	-I$(top_srcdir)/include \
	-I$(top_builddir)/include
