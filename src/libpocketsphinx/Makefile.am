ARCH   = `uname -m | sed s/i.86/i386/`_`uname -s | tr A-Z a-z`

lib_LTLIBRARIES = libpocketsphinx.la

libpocketsphinx_la_LDFLAGS = -version-info 1:0:0

libpocketsphinx_la_SOURCES =			\
	acmod.c					\
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
	mdef.c					\
	ms_gauden.c				\
	ms_mgau.c				\
	ms_senone.c				\
	ngram_search.c				\
	ngram_search_fwdtree.c			\
	ngram_search_fwdflat.c			\
	ngram_search_dag.c			\
	phone.c					\
	s2_semi_mgau.c				\
	search.c				\
	searchlat.c				\
	senscr.c				\
	tmat.c					\
	uttproc.c				\
	vector.c				\
	word_fsg.c				\
	pocketsphinx.c

noinst_HEADERS =				\
	pocketsphinx_internal.h			\
	acmod.h					\
	ngram_search.h				\
	uttproc.h				\
	bin_mdef.h				\
	blkarray_list.h				\
	dict.h					\
	fsg_history.h				\
	fsg_lextree.h				\
	fsg_psubtree.h				\
	fsg_search.h				\
	hmm.h					\
	kb.h					\
	kdtree.h				\
	log.h					\
	mdef.h					\
	ms_gauden.h				\
	ms_mgau.h				\
	ms_senone.h				\
	ngram_search.h				\
	ngram_search_fwdtree.h			\
	ngram_search_fwdflat.h			\
	ngram_search_dag.h			\
	phone.h					\
	posixwin32.h				\
	s2_semi_mgau.h				\
	s2io.h					\
	search_const.h				\
	search.h				\
	senscr.h				\
	tmat.h					\
	vector.h				\
	word_fsg.h

INCLUDES = -I$(top_srcdir)/include \
           -I$(top_builddir)/include 
