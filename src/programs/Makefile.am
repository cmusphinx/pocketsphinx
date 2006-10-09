bin_PROGRAMS = \
	pocketsphinx_batch \
	pocketsphinx_continuous \
	pocketsphinx_ptt \
	pocketsphinx_mdef_convert

noinst_PROGRAMS = \
	cont_adseg \
	cont_fileseg


cont_adseg_SOURCES = cont_adseg.c

cont_fileseg_SOURCES = cont_fileseg.c

pocketsphinx_mdef_convert_SOURCES = mdef_convert.c

pocketsphinx_batch_SOURCES = batch.c

pocketsphinx_continuous_SOURCES =  tty-continuous.c

pocketsphinx_ptt_SOURCES = tty-ptt.c

LDADD = $(top_builddir)/src/libpocketsphinx/libpocketsphinx.la \
	-lsphinxutil -lsphinxfe -lsphinxad -lm

INCLUDES = -I$(top_srcdir)/include \
       -I$(top_builddir)/include
