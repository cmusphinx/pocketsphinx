TESTS = test_lm_init.sh test_lmctl_init.sh test_lm_read test_ps_init

check_PROGRAMS = test_lm_read test_ps_init

INCLUDES = \
	-I$(top_srcdir)/include \
	-I$(top_builddir)/include \
	-DMODELDIR=\"${top_srcdir}/model\" \
	-DDATADIR=\"${top_srcdir}/test/data\"

LDADD = \
	$(top_builddir)/src/libpocketsphinx/libpocketsphinx.la \
	-lsphinxbase

TESTDATA =

EXTRA_DIST = $(TESTS) $(TESTDATA)
