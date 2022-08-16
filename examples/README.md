PocketSphinx Examples
=====================

This directory contains some examples of basic PocketSphinx library
usage in C and Python.  To compile the C example, you can either use
the library directly in-place:

    cmake -DBUILD_SHARED_LIBS=OFF .. && make
    cc -o simple simple.c -I../include -Iinclude -L. -lpocketsphinx -lm
    
Or if PocketSphinx is installed:

    cc -o simple simple.c $(pkg-config --static --libs --cflags pocketsphinx)

By default it assumes you are running it from this directory,
otherwise you could define `MODELDIR` when compiling, e.g.:

    cc -o simple simple.c -DMODELDIR=/my/models \
        $(pkg-config --static --libs --cflags pocketsphinx)

The Python script, assuming you have installed the `pocketsphinx5`
module (see [the top-leve README](../README.md) for instructions), can
just be run as-is:

    python simple.py

Now speak.  It should start printing partial results, and not stop,
until you interrupt it with Control-C, whereupon it will print one
last final result.
