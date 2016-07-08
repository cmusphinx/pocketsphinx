check_PROGRAMS = \
	test_acmod \
	test_acmod_grow \
	test_alignment \
	test_allphone \
	test_dict2pid \
	test_dict \
	test_fsg \
	test_fwdflat \
	test_fwdtree_bestpath \
	test_fwdtree \
	test_init \
	test_jsgf \
	test_keyphrase \
	test_lattice \
	test_lm_read \
	test_mllr \
	test_nbest \
	test_posterior \
	test_ptm_mgau \
	test_reinit \
	test_senfh \
	test_set_search \
	test_simple \
	test_state_align

TESTS = $(check_PROGRAMS)

EXTRA_DIST = test_ps.c

noinst_HEADERS = test_macros.h

AM_CFLAGS =-I$(top_srcdir)/include \
	-I$(top_srcdir)/src/libpocketsphinx \
	-I$(top_builddir)/include \
	-I$(srcdir) \
	-DMODELDIR=\"${top_srcdir}/model\" \
	-DDATADIR=\"${top_srcdir}/test/data\"

LDADD = \
	$(top_builddir)/src/libpocketsphinx/libpocketsphinx.la \
	-lsphinxbase

CLEANFILES = *.log *.out *.lat *.mfc *.raw *.dic *.sen

valgrind-check:
	for testf in .libs/lt-*; do valgrind --leak-check=full --show-reachable=yes \
	    $$testf; done >& valgrind.log

