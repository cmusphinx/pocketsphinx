EXTRA_DIST = pocketsphinx.c \
	pocketsphinx.pyx \
	pocketsphinx.pxd

pkginclude_HEADERS = pocketsphinx.pxd

noinst_HEADERS = bogus_pygobject.h

if BUILD_PYTHON
all-local: pymod-build-stamp

install-exec-local: pymod-build-stamp
# This is dumb, but setuptools screws everything up if you try to use --prefix
	if [ "$(DESTDIR)" ]; then \
		$(PYTHON) setup.py install --root $(DESTDIR); \
	else \
		$(PYTHON) setup.py install --root /; \
	fi

uninstall-local:
# This is dumb, but setuptools screws everything up if you try to use --prefix
	if [ "$(DESTDIR)" ]; then \
		$(PYTHON) setup.py bogus_uninstall --root $(DESTDIR); \
	else \
		$(PYTHON) setup.py bogus_uninstall --root /; \
	fi

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
