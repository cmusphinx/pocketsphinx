noinst_LTLIBRARIES = libsphinxlm.la

AM_YFLAGS = -d
BUILT_SOURCES = jsgf_parser.h jsgf_parser.c

libsphinxlm_la_SOURCES =			\
	ngrams_raw.c				\
	lm_trie.c					\
	lm_trie_quant.c				\
	ngram_model.c				\
	ngram_model_set.c			\
	ngram_model_trie.c			\
	fsg_model.c				\
	jsgf.c					\
	jsgf_scanner.c				\
	jsgf_parser.y

libsphinxlm_la_LIBADD = $(LIBICONV)

noinst_HEADERS = ngram_model_internal.h		\
	ngram_model_set.h			\
	ngram_model_trie.h			\
	ngrams_raw.h				\
	lm_trie.h					\
	lm_trie_quant.h				\
	jsgf_internal.h				\
	jsgf_scanner.h				\
	jsgf_parser.h

AM_CFLAGS =-I$(top_srcdir)/include/sphinxbase \
	   -I$(top_srcdir)/include \
	   -I$(top_builddir)/include

# Warning: you need flex 2.6.1 at least, command line is
# flex --nounistd -o jsgf_scanner.c _jsgf_scanner.l
EXTRA_DIST = _jsgf_scanner.l
