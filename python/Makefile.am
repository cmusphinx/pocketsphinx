EXTRA_DIST = pocketsphinx.c \
	pocketsphinx.pyx \
	pocketsphinx.pxd \
	ps_test.py \
	setup_win32.py

pkginclude_HEADERS = pocketsphinx.pxd

noinst_HEADERS = bogus_pygobject.h

if BUILD_PYTHON
# Setuptools isn't really compatible with distutils, unfortunately
SETUPTOOLS_CRAP = `python -c 'import setuptools' 2>/dev/null \
	&& echo --single-version-externally-managed --record filelist.txt`

all-local: pymod-build-stamp

install-exec-local: pymod-build-stamp
	$(PYTHON) setup.py install --prefix $(DESTDIR)$(prefix) $(SETUPTOOLS_CRAP)

uninstall-local:
	$(PYTHON) setup.py bogus_uninstall --prefix $(DESTDIR)$(prefix)

clean-local:
	$(PYTHON) setup.py clean --all
# This is dumb, but distutils is completely incapable of VPATH building
	test -z "$(VPATH)" || $(RM) pocketsphinx.c
	$(RM) -r *.egg-info
	$(RM) pymod-build-stamp filelist.txt

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
