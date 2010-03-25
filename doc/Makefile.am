man_MANS = \
	pocketsphinx_batch.1 \
	pocketsphinx_continuous.1 \
	pocketsphinx_mdef_convert.1

EXTRA_DIST = \
	pocketsphinx_batch.1 \
	pocketsphinx_continuous.1 \
	pocketsphinx_mdef_convert.1 \
	args2man.pl

# pocketsphinx_batch.1: pocketsphinx_batch.1.in
# 	$(srcdir)/args2man.pl $(top_builddir)/src/programs/pocketsphinx_batch \
# 		< $< > $@

# pocketsphinx_continuous.1: pocketsphinx_continuous.1.in
# 	$(srcdir)/args2man.pl $(top_builddir)/src/programs/pocketsphinx_continuous \
# 		< $< > $@

if BUILD_DOXYGEN
all-local: html/index.html
endif

headers = $(top_srcdir)/include/pocketsphinx.h \
	$(top_srcdir)/include/fsg_set.h \
	$(top_srcdir)/include/ps_lattice.h

latex/refman.pdf: doxyfile $(headers)
	doxygen
	$(MAKE) -C latex refman.pdf

html/index.html: doxyfile $(headers)
	doxygen

clean-local:
	-rm -rf html latex doxytags

# Totally CMU-specific rule for uploading documentation
upload: html/index.html
	rsync -av html/ fife:/usr12/apache2/htdocs/sphinx/doc/doxygen/pocketsphinx/
