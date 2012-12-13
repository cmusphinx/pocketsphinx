SUBDIRS = src \
	  doc \
	  model \
	  include \
	  test \
	  python

EXTRA_DIST = autogen.sh							\
	pocketsphinx.sln						\
	pocketsphinx.pc.in						\
	win32/pocketsphinx/pocketsphinx.vcxproj.filters			\
	win32/pocketsphinx/pocketsphinx.vcxproj				\
	win32/pocketsphinx_batch/pocketsphinx_batch.vcxproj		\
	win32/pocketsphinx_continuous/pocketsphinx_continuous.vcxproj	\
	win32/pocketsphinx_mdef_convert/pocketsphinx_mdef_convert.vcxproj	\
	swig/Makefile							\
	swig/Android.mk							\
	swig/pocketsphinx.i						\
	m4/pkg.m4							\
	group/ABLD.BAT							\
	group/bld.inf							\
	group/pocketsphinx.mmp

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = pocketsphinx.pc
CLEANFILES = pocketsphinx.pc

test:
	$(LIBTOOL) --mode=execute src/programs/pocketsphinx_batch \
		-adcin TRUE -cepext .16k -samprate 16000 \
		-ctl $(top_srcdir)/model/lm/turtle/turtle.ctl \
		-cepdir $(top_srcdir)/model/lm/turtle \
		-lw 6.5 -fwdflatlw 8.5 -bestpathlw 9.5 \
		-uw 0.5 -fillpen 1e-10 -silpen 0.005 -wip 0.65 \
		-topn 1 -topsenfrm 3 -topsenthresh -70000 -beam 1e-50 \
		-pbeam 1e-50 -lpbeam 1e-40 -lponlybeam 1e-40 \
		-wbeam 1e-50 -fwdflat TRUE -fwdflatbeam 1e-70 \
		-fwdflatwbeam 1e-40 -bestpath TRUE \
		-lmdumpdir $(top_srcdir)/model/lm/turtle \
		-lm $(top_srcdir)/model/lm/turtle/turtle.lm \
		-dict $(top_srcdir)/model/lm/turtle/turtle.dic \
		-hmm $(top_srcdir)/model/hmm/wsj0 \
		-mmap TRUE

test-debug:
	$(LIBTOOL) --mode=execute gdb --args src/programs/pocketsphinx_batch \
		-adcin TRUE -cepext .16k -samprate 16000 \
		-ctl $(top_srcdir)/model/lm/turtle/turtle.ctl \
		-cepdir $(top_srcdir)/model/lm/turtle \
		-lw 6.5 -fwdflatlw 8.5 -bestpathlw 9.5 \
		-uw 0.5 -fillpen 1e-10 -silpen 0.005 -wip 0.65 \
		-topn 1 -topsenfrm 3 -topsenthresh -70000 -beam 1e-50 \
		-pbeam 1e-50 -lpbeam 1e-40 -lponlybeam 1e-40 \
		-wbeam 1e-50 -fwdflat TRUE -fwdflatbeam 1e-70 \
		-fwdflatwbeam 1e-40 -bestpath TRUE \
		-lmdumpdir $(top_srcdir)/model/lm/turtle \
		-lm $(top_srcdir)/model/lm/turtle/turtle.lm \
		-dict $(top_srcdir)/model/lm/turtle/turtle.dic \
		-hmm $(top_srcdir)/model/hmm/wsj0 \
		-mmap TRUE

ACLOCAL_AMFLAGS = -I m4
