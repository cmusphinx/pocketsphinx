check_PROGRAMS = \
	test_lm_read \
	test_lm_score \
	test_lm_add \
	test_lm_casefold \
	test_lm_class \
	test_lm_set \
	test_lm_write

TESTS = $(check_PROGRAMS)

AM_CFLAGS =\
	-I$(top_srcdir)/include/sphinxbase \
	-I$(top_srcdir)/include \
	-I$(top_builddir)/include \
	-DLMDIR=\"${srcdir}\"

LDADD = ${top_builddir}/src/libsphinxbase/libsphinxbase.la

noinst_HEADERS = test_macros.h

EXTRA_DIST = \
	100.lm.gz \
	100.lm.bz2 \
	100.lm.dmp \
	100.lm.bin \
	100.probdef \
	100.lmctl \
	102.lm.gz \
	102.lm.dmp \
	104.lm.gz \
	105.lm.gz \
	106.lm.gz \
	107.lm.gz \
	turtle.lm \
	turtle.lm.dmp \
	turtle.ug.lm \
	turtle.ug.lm.dmp

CLEANFILES = 100.tmp.lm.bin 100.tmp.lm turtle.ug.tmp.lm.bin
