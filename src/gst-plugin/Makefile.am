my_plugins = 
if BUILD_GST
my_plugins += libgstpocketsphinx.la
endif

plugin_LTLIBRARIES = $(my_plugins)

libgstpocketsphinx_la_SOURCES = gstpocketsphinx.c
libgstpocketsphinx_la_CFLAGS = $(GST_CFLAGS) -DPOCKETSPHINX_PREFIX=\"$(prefix)\"
libgstpocketsphinx_la_LIBADD = $(GST_LIBS) -lgstaudio-$(GST_MAJORMINOR) \
	$(top_builddir)/src/libpocketsphinx/libpocketsphinx.la \
	-lsphinxbase -lsphinxad
libgstpocketsphinx_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)

noinst_HEADERS = gstpocketsphinx.h 


INCLUDES = -I$(top_srcdir)/include \
	-I$(top_builddir)/include
