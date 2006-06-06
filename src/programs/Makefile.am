bin_PROGRAMS = \
	cdcn_test \
	cont_adseg \
	cont_fileseg \
	mdef_convert \
	pocketsphinx_allphone \
	pocketsphinx_batch \
	pocketsphinx_continuous \
	pocketsphinx_ptt

cdcn_test_SOURCES = cdcn_test.c

cont_adseg_SOURCES = cont_adseg.c

cont_fileseg_SOURCES = cont_fileseg.c

mdef_convert_SOURCES = mdef_convert.c

pocketsphinx_allphone_SOURCES = allphone-test.c

pocketsphinx_batch_SOURCES = batch.c

pocketsphinx_continuous_SOURCES =  tty-continuous.c

pocketsphinx_ptt_SOURCES = tty-ptt.c

LDADD = $(top_builddir)/src/libpocketsphinx/libpocketsphinx.la \
	-lsphinxutil -lsphinxfe -lsphinxad -lm

INCLUDES = -I$(top_srcdir)/include \
       -I$(top_builddir)/include
