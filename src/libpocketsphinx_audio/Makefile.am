# libsphinx2ad: Audio Device interface for Sphinx-II

lib_LTLIBRARIES = libpocketsphinx_audio.la

libpocketsphinx_audio_la_LDFLAGS = -version-info 0:0:0

libpocketsphinx_audio_la_SOURCES = cont_ad_base.c

EXTRA_libpocketsphinx_audio_la_SOURCES = ad_base.c \
	ad_oss.c \
	ad_oss_bsd.c \
	ad_alsa.c \
	ad_osf.c \
	ad_irix.c \
	ad_sunos.c \
	audio_utils_sunos.c \
	play_win32.c \
	rec_win32.c \
	mulaw_base.c

noinst_HEADERS = audio_utils_sunos.h

libpocketsphinx_audio_la_DEPENDENCIES = @ad_files@

libpocketsphinx_audio_la_LIBADD = @ad_files@ @ad_libs@

INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include
