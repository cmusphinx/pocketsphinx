ARCH   = `uname -m | sed s/i.86/i386/`_`uname -s | tr A-Z a-z`

lib_LTLIBRARIES = libpocketsphinx.la

libpocketsphinx_la_LDFLAGS = -version-info 0:0:0

libpocketsphinx_la_SOURCES =			\
	allphone.c				\
	awriteshort.c				\
	bin_mdef.c				\
	blkarray_list.c				\
	dict.c					\
	fbs_main.c				\
	fsg_history.c				\
	fsg_lextree.c				\
	fsg_psubtree.c				\
	fsg_search.c				\
	hmm.c					\
	kb_main.c				\
	kdtree.c				\
	lm_3g.c					\
	lmclass.c				\
	mdef.c					\
	ms_gauden.c				\
	ms_mgau.c				\
	ms_senone.c				\
	phone.c					\
	s2_semi_mgau.c				\
	search.c				\
	searchlat.c				\
	senscr.c				\
	subvq_mgau.c				\
	tmat.c					\
	uttproc.c				\
	vector.c				\
	word_fsg.c

noinst_HEADERS = uttproc.h allphone.h

INCLUDES = -I$(top_srcdir)/include \
           -I$(top_builddir)/include 
