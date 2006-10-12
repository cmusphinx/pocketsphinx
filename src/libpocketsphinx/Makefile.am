ARCH   = `uname -m | sed s/i.86/i386/`_`uname -s | tr A-Z a-z`

lib_LTLIBRARIES = libpocketsphinx.la

libpocketsphinx_la_LDFLAGS = -version-info 0:0:0

libpocketsphinx_la_SOURCES = \
	add-table.c				\
	agc_emax.c				\
	allphone.c				\
	areadint.c				\
	awriteshort.c				\
	bin_mdef.c				\
	bisearch.c				\
	blkarray_list.c				\
	cache_lm.c				\
	cep_rw.c				\
	dict.c					\
	eht_quit.c				\
	fbs_main.c				\
	fsg_history.c				\
	fsg_lextree.c				\
	fsg_psubtree.c				\
	fsg_search.c				\
	get_a_word.c				\
	hmm_tied_r.c				\
	kb_main.c				\
	kdtree.c				\
	lab.c					\
	list.c					\
	lm_3g.c					\
	lm.c					\
	lmclass.c				\
	log_add.c				\
	logmsg.c				\
	logs3.c					\
	longio.c				\
	mdef.c					\
	nxtarg.c				\
	pconf.c					\
	phone.c					\
	s2_mmap.c				\
	s2_semi_mgau.c				\
	salloc.c				\
	search.c				\
	searchlat.c				\
	senscr.c				\
	skipto.c				\
	strcasecmp.c				\
	tmat.c					\
	time_align.c				\
	unlimit.c				\
	util.c					\
	uttproc.c				\
	vector.c				\
	word_fsg.c

INCLUDES = -I$(top_srcdir)/include \
           -I$(top_builddir)/include 
