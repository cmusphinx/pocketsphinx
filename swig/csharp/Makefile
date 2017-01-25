# Example how to create C# bindings

all: run

pocketsphinx.c: ../pocketsphinx.i
	mkdir -p gen
	swig -I.. -I../../../sphinxbase/swig -I../include -I../../sphinxbase/include \
	    -csharp \
	    -dllimport "libpocketsphinxwrap.so" \
	    -namespace "Pocketsphinx" -o sphinxbase.c \
	    -outdir gen ../../../sphinxbase/swig/sphinxbase.i
	swig -I.. -I../../../sphinxbase/swig -I../include -I../../sphinxbase/include \
	    -csharp \
	    -dllimport "libpocketsphinxwrap.so" \
	    -namespace "Pocketsphinx" -o pocketsphinx.c \
	    -outdir gen ../pocketsphinx.i

libpocketsphinxwrap.so: pocketsphinx.c
	gcc -fPIC `pkg-config --libs --cflags pocketsphinx` pocketsphinx.c sphinxbase.c -shared -o $@

test.exe: libpocketsphinxwrap.so
	mcs test.cs gen/*.cs

run: test.exe
	mono test.exe

clean:
	rm -f gen/*.cs
	rm -f libpocketsphinxwrap.so
	rm -f pocketsphinx.c
	rm -f sphinxbase.c
	rm -f test.exe

.PHONY: run clean


