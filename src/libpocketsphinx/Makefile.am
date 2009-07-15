ARCH   = `uname -m | sed s/i.86/i386/`_`uname -s | tr A-Z a-z`

lib_LTLIBRARIES = libpocketsphinx.la

libpocketsphinx_la_LDFLAGS = -version-info 1:0:0

libpocketsphinx_la_SOURCES =			\
	acmod.c					\
	bin_mdef.c				\
	blkarray_list.c				\
	ctxt_table.c				\
	dict.c					\
	dict2pid.c				\
	fillpen.c				\
	fsg_history.c				\
	fsg_lextree.c				\
	fsg_search.c				\
	hmm.c					\
	kdtree.c				\
	lextree.c				\
	lts.c					\
	cmu6_lts_rules.c			\
	mdef.c					\
	ms_gauden.c				\
	ms_mgau.c				\
	ms_senone.c				\
	ngram_search.c				\
	ngram_search_fwdtree.c			\
	ngram_search_fwdflat.c			\
	phone_loop_search.c			\
	ps_lattice.c				\
	ps_mllr.c				\
	s2_semi_mgau.c				\
	s3_dict.c				\
	tmat.c					\
	tst_search.c				\
	vector.c				\
	vithist.c				\
	pocketsphinx.c

noinst_HEADERS =				\
	pocketsphinx_internal.h			\
	acmod.h					\
	ngram_search.h				\
	bin_mdef.h				\
	blkarray_list.h				\
	ctxt_table.h				\
	dict.h					\
	dict2pid.h				\
	fillpen.h				\
	fsg_history.h				\
	fsg_lextree.h				\
	fsg_search_internal.h			\
	hmm.h					\
	kdtree.h				\
	lextree.h				\
	lts.h					\
	cmu6_lts_rules.h			\
	mdef.h					\
	ms_gauden.h				\
	ms_mgau.h				\
	ms_senone.h				\
	ngram_search.h				\
	ngram_search_fwdtree.h			\
	ngram_search_fwdflat.h			\
	phone_loop_search.h			\
	ps_lattice_internal.h			\
	posixwin32.h				\
	s2_semi_mgau.h				\
	s3_dict.h				\
	tmat.h					\
	tst_search.h				\
	vector.h				\
	vithist.h

INCLUDES = -I$(top_srcdir)/include \
           -I$(top_builddir)/include 
