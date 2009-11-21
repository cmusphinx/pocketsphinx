EXTRA_DIST = pocketsphinx.c \
	pocketsphinx.pyx \
	pocketsphinx.pxd

pkginclude_HEADERS = pocketsphinx.pxd

noinst_HEADERS = bogus_pygobject.h

if BUILD_PYTHON
all-local: pymod-build-stamp

install-exec-local: pymod-build-stamp
	$(PYTHON) setup.py install --prefix $(DESTDIR)$(prefix)

uninstall-local:
	$(PYTHON) setup.py bogus_uninstall --prefix $(DESTDIR)$(prefix)

clean-local:
	$(PYTHON) setup.py clean --all
# This is dumb, but distutils is completely incapable of VPATH building
	test -z "$(VPATH)" || $(RM) pocketsphinx.c
	$(RM) pymod-build-stamp

pymod-build-stamp: $(srcdir)/pocketsphinx.c
# This is dumb, but distutils is completely incapable of VPATH building
	test -z "$(VPATH)" || cp "$(srcdir)/pocketsphinx.c" pocketsphinx.c
	$(PYTHON) setup.py build
	touch $@
endif

if BUILD_CYTHON
$(srcdir)/pocketsphinx.c: $(srcdir)/pocketsphinx.pyx $(srcdir)/pocketsphinx.pxd
	cython -o $@ $< $(CPPFLAGS) -I$(sphinxbase)/python
endif
