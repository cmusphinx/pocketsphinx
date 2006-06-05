SUBDIRS = src \
	  model \
	  include \
	  scripts

EXTRA_DIST = autogen.sh \
	pocketsphinx.dsw \
	win32/PocketSphinx.dsw \
	win32/msdev/pocketsphinx/pocketsphinx.dsp \
	win32/msdev/pocketsphinx_allphone/pocketsphinx_allphone.dsp \
	win32/msdev/pocketsphinx_batch/pocketsphinx_batch.dsp \
	win32/msdev/pocketsphinx_continuous/pocketsphinx_continuous.dsp \
	win32/msdev/pocketsphinx_ptt/pocketsphinx_ptt.dsp

test:
	$(LIBTOOL) --mode=execute src/programs/pocketsphinx_batch \
		-verbose 9 -adcin TRUE -adcext 16k -samp 16000 \
		-ctlfn $(top_srcdir)/model/lm/turtle/turtle.ctl \
		-ctloffset 0 -ctlcount 100000000 \
		-datadir $(top_srcdir)/model/lm/turtle \
		-langwt 6.5 -fwdflatlw 8.5 -rescorelw 9.5 \
		-ugwt 0.5 -fillpen 1e-10 -silpen 0.005 -inspen 0.65 \
		-top 1 -topsenfrm 3 -topsenthresh -70000 -beam 2e-06 \
		-npbeam 2e-06 -lpbeam 2e-05 -lponlybeam 0.0005 \
		-nwbeam 0.0005 -fwdflat TRUE -fwdflatbeam 1e-08 \
		-fwdflatnwbeam 0.0003 -bestpath TRUE \
		-kbdumpdir $(top_srcdir)/model/lm/turtle \
		-lmfn $(top_srcdir)/model/lm/turtle/turtle.lm \
		-dictfn $(top_srcdir)/model/lm/turtle/turtle.dic \
		-mdeffn $(top_srcdir)/model/hmm/wsj0/mdef \
		-meanfn $(top_srcdir)/model/hmm/wsj0/means \
		-varfn $(top_srcdir)/model/hmm/wsj0/variances \
		-tmatfn $(top_srcdir)/model/hmm/wsj0/transition_matrices \
		-8bsen TRUE -sendumpfn $(top_srcdir)/model/hmm/wsj0/sendump \
		-mmap TRUE

test-debug:
	$(LIBTOOL) --mode=execute gdb --args src/programs/pocketsphinx_batch \
		-verbose 9 -adcin TRUE -adcext 16k -samp 16000 \
		-ctlfn $(top_srcdir)/model/lm/turtle/turtle.ctl \
		-ctloffset 0 -ctlcount 100000000 \
		-datadir $(top_srcdir)/model/lm/turtle \
		-langwt 6.5 -fwdflatlw 8.5 -rescorelw 9.5 \
		-ugwt 0.5 -fillpen 1e-10 -silpen 0.005 -inspen 0.65 \
		-top 1 -topsenfrm 3 -topsenthresh -70000 -beam 2e-06 \
		-npbeam 2e-06 -lpbeam 2e-05 -lponlybeam 0.0005 \
		-nwbeam 0.0005 -fwdflat TRUE -fwdflatbeam 1e-08 \
		-fwdflatnwbeam 0.0003 -bestpath TRUE \
		-kbdumpdir $(top_srcdir)/model/lm/turtle \
		-lmfn $(top_srcdir)/model/lm/turtle/turtle.lm \
		-dictfn $(top_srcdir)/model/lm/turtle/turtle.dic \
		-ndictfn $(top_srcdir)/model/hmm/6k/noisedict \
		-mdeffn $(top_srcdir)/model/hmm/6k/Communicator.6000.bmdef \
		-meanfn $(top_srcdir)/model/hmm/6k/means \
		-varfn $(top_srcdir)/model/hmm/6k/variances \
		-tmatfn $(top_srcdir)/model/hmm/6k/transition_matrices \
		-8bsen TRUE -sendumpfn $(top_srcdir)/model/hmm/6k/sendump \
		-cbdir $(top_srcdir)/model/hmm/6k -mmap TRUE
