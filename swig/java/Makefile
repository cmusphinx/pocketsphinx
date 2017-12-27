CFLAGS := -g -Wall -DPIC -fPIC
CPPFLAGS := -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/darwin -I$(JAVA_HOME)/include/linux -Wno-unused-function `pkg-config --cflags sphinxbase pocketsphinx`

# Dynamic libraries are named differently on Linux and MacOS.
# Set DYLIB to the one we want for this platform.
UNAME := $(shell uname)
ifeq ($(UNAME), Linux)
DYLIB=libpocketsphinx_jni.so
else
ifeq ($(UNAME), Darwin)
DYLIB=libpocketsphinx_jni.dylib
endif
endif


all: $(DYLIB) jar

jar: build/pocketsphinx.jar

build/pocketsphinx.jar: $(DYLIB)
	mkdir -p build/edu/cmu/pocketsphinx
	javac edu/cmu/pocketsphinx/*.java
	mv edu/cmu/pocketsphinx/*.class build/edu/cmu/pocketsphinx
	cd build; jar cf pocketsphinx.jar edu

libpocketsphinx_jni.dylib: pocketsphinx_wrap.o sphinxbase_wrap.o
	$(CC) -dynamiclib -o $@ pocketsphinx_wrap.o sphinxbase_wrap.o `pkg-config --libs sphinxbase pocketsphinx`


libpocketsphinx_jni.so: pocketsphinx_wrap.o sphinxbase_wrap.o
	$(CC) -shared -o $@ pocketsphinx_wrap.o sphinxbase_wrap.o `pkg-config --libs sphinxbase pocketsphinx`

pocketsphinx_wrap.c: ../pocketsphinx.i
	mkdir -p edu/cmu/pocketsphinx
	swig -I.. -I../../../sphinxbase/swig -I../include -I../../sphinxbase/include \
		-java -package edu.cmu.pocketsphinx \
		-outdir edu/cmu/pocketsphinx -o $@ $<

sphinxbase_wrap.c: ../../../sphinxbase/swig/sphinxbase.i
	mkdir -p edu/cmu/pocketsphinx
	swig -I.. -I../../../sphinxbase/swig -I../include -I../../sphinxbase/include \
		-java -package edu.cmu.pocketsphinx \
		-outdir edu/cmu/pocketsphinx -o $@ $<

clean:
	$(RM) *.so *.dylib *_wrap.c *_wrap.o test/*.class
	$(RM) -r edu
	$(RM) -r build

run:
	javac test/*.java edu/cmu/pocketsphinx/*.java
	java -Djava.library.path=. -cp . test.DecoderTest
