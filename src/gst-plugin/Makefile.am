my_plugins = 
if BUILD_GST
my_plugins += libgstpocketsphinx.la
endif

plugin_LTLIBRARIES = $(my_plugins)

libgstpocketsphinx_la_SOURCES = gstpocketsphinx.c gstvader.c psmarshal.c
libgstpocketsphinx_la_CFLAGS = $(GST_CFLAGS) -DMODELDIR=\"$(pkgdatadir)/model\"
libgstpocketsphinx_la_LIBADD = $(GST_LIBS) -lgstaudio-$(GST_MAJORMINOR) \
	$(top_builddir)/src/libpocketsphinx/libpocketsphinx.la -lsphinxbase
libgstpocketsphinx_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)

noinst_HEADERS = gstpocketsphinx.h gstvader.h psmarshal.h

INCLUDES = -I$(top_srcdir)/include \
	-I$(top_builddir)/include

EXTRA_DIST = psmarshal.list
BUILT_SOURCES = psmarshal.h psmarshal.c
CLEANFILES = psmarshal.h psmarshal.c

psmarshal.h: psmarshal.list
	glib-genmarshal --header --prefix=ps_marshal $(srcdir)/psmarshal.list > psmarshal.h.tmp
	mv psmarshal.h.tmp psmarshal.h

psmarshal.c: psmarshal.list
	echo "#include \"glib-object.h\"" >> psmarshal.c.tmp
	echo "#include \"psmarshal.h\"" >> psmarshal.c.tmp
	glib-genmarshal --body --prefix=ps_marshal $(srcdir)/psmarshal.list >> psmarshal.c.tmp
	mv psmarshal.c.tmp psmarshal.c
