man_MANS = \
	pocketsphinx_batch.1 \
	pocketsphinx_continuous.1 \
	pocketsphinx_mdef_convert.1 \
	pocketsphinx_swb.1 \
	pocketsphinx_tidigits.1

pocketsphinx_batch.1: pocketsphinx_batch.1.in
	$(srcdir)/args2man.pl $(top_builddir)/src/programs/pocketsphinx_batch \
		< $< > $@

pocketsphinx_continuous.1: pocketsphinx_continuous.1.in
	$(srcdir)/args2man.pl $(top_builddir)/src/programs/pocketsphinx_continuous \
		< $< > $@

if BUILD_DOXYGEN
all-local: latex/refman.pdf html/index.html

install-data-local:
	$(INSTALL) -d $(docdir)
	$(INSTALL) latex/refman.pdf $(docdir)
	(tar cf - html) | (cd $(docdir) && tar xvf -)

uninstall-local:
	$(RM) -r $(docdir)
endif

headers = $(top_srcdir)/include/pocketsphinx.h \
	 $(top_srcdir)/include/fbs.h

latex/refman.pdf: doxyfile $(headers)
	doxygen
	$(MAKE) -C latex refman.pdf

html/index.html: doxyfile $(headers)
	doxygen

clean-local:
	-rm -rf html latex
