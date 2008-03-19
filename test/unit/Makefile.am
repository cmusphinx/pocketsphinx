gst_programs =
gst_tests =
if BUILD_GST
gst_programs += test_gst
gst_tests += test_gst
endif

check_PROGRAMS = \
	test_lm_read \
	test_ps_init \
	test_ps_fwdtree \
	test_acmod \
	test_fwdtree \
	test_fwdtree_fbs \
	test_fwdflat \
	$(gst_programs)

TESTS = \
	test_lm_init.sh \
	test_lmctl_init.sh \
	test_lm_read \
	test_ps_init \
	test_ps_fwdtree \
	test_acmod \
	test_fwdtree \
	test_fwdtree_fbs \
	test_fwdflat \
	$(gst_tests)

noinst_HEADERS = test_macros.h

test_gst_LDADD = $(GST_LIBS)

INCLUDES = $(GST_CFLAGS) \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/src/libpocketsphinx \
	-I$(top_builddir)/include \
	-DMODELDIR=\"${top_srcdir}/model\" \
	-DDATADIR=\"${top_srcdir}/test/data\"

LDADD = \
	$(top_builddir)/src/libpocketsphinx/libpocketsphinx.la \
	-lsphinxbase

CLEANFILES = *.log

EXTRA_DIST = \
	test_lm_init.sh \
	test_lmctl_init.sh

