gst_programs =
gst_tests =
if BUILD_GST
gst_programs += test_gst
gst_tests += test_gst
endif

check_PROGRAMS = \
	test_ps_init \
	test_ps_reinit \
	test_ps_fwdtree \
	test_ps_fwdtree_fwdflat \
	test_ps_fwdflat \
	test_ps_fwdflat_bestpath \
	test_ps_fwdtree_bestpath \
	test_ps_simple \
	test_ps_nbest \
	test_ps_lattice \
	test_acmod \
	test_acmod_grow \
	test_fwdtree \
	test_fwdflat \
	test_fwdtree_fwdflat \
	test_fwdtree_bestpath \
	test_fwdtree_nbest \
	test_pl_fwdtree \
	test_posterior \
	test_fsg \
	test_fsg2 \
	test_fsg3 \
	test_jsgf \
	test_lm_read \
	test_dict \
	$(gst_programs)

TESTS = $(check_PROGRAMS)

EXTRA_DIST = ps_test.c

noinst_HEADERS = test_macros.h

test_gst_LDADD = $(GST_LIBS)

INCLUDES = $(GST_CFLAGS) \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/src/libpocketsphinx \
	-I$(top_builddir)/include \
	-I$(srcdir) \
	-DMODELDIR=\"${top_srcdir}/model\" \
	-DDATADIR=\"${top_srcdir}/test/data\"

LDADD = \
	$(top_builddir)/src/libpocketsphinx/libpocketsphinx.la \
	-lsphinxbase

CLEANFILES = *.log *.out *.lat *.mfc *.raw *.dic
