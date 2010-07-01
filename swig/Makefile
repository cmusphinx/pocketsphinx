CFLAGS := -g -Wall -DPIC -fPIC
CPPFLAGS := -I/usr/lib/jvm/default-java/include `pkg-config --cflags sphinxbase pocketsphinx`

all: libpocketsphinx_jni.so

libpocketsphinx_jni.so: pocketsphinx_wrap.o
	$(CC) -shared -o $@ pocketsphinx_wrap.o `pkg-config --libs sphinxbase pocketsphinx`

pocketsphinx_wrap.c: pocketsphinx.i
	mkdir -p edu/cmu/pocketsphinx
	swig -I../include -I../../sphinxbase/include \
		-java -package edu.cmu.pocketsphinx \
		-outdir edu/cmu/pocketsphinx pocketsphinx.i

clean:
	$(RM) *.so *_wrap.c *_wrap.o *.class edu/cmu/pocketsphinx/*.class
