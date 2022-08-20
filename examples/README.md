PocketSphinx Examples
=====================

This directory contains some examples of basic PocketSphinx library
usage in C and Python.  To compile the C example, you can either use
the library directly in-place, for example, with `simple.c`:

    cmake -DBUILD_SHARED_LIBS=OFF .. && make
    cc -o simple simple.c -I../include -Iinclude -L. -lpocketsphinx -lm
    
Or if PocketSphinx is installed:

    cc -o simple simple.c $(pkg-config --static --libs --cflags pocketsphinx)

By default they assume you are running from this directory, otherwise
you could define `MODELDIR` when compiling, e.g.:

    cc -o simple simple.c -DMODELDIR=/my/models \
        $(pkg-config --static --libs --cflags pocketsphinx)

The Python scripts, assuming you have installed the `pocketsphinx5`
module (see [the top-leve README](../README.md) for instructions), can
just be run as-is:

    python simple.py
    
Simplest possible example
-------------------------

The examples `simple.c` and `simple.py` just recognize speech
endlessly, until you stop them with Control-C.  They, like all the
other examples below, use [SoX](http://sox.sourceforge.net/) to get
audio from the default input device.  Feel free to modify them to read
from standard input.

Voice activity detection
------------------------

The examples `vad.c` and `vad.py` just *detect* speech/non-speech
transitions endlessly, until you stop them with Control-C.  Note that
this does VAD at the frame level (30ms) only, which is probably not
what you want.

Segmentation
------------

The example `segment.py` uses voice activity detection to *segment*
the input stream into speech-like regions.

Live recognition
----------------

Finally, the examples `live.c` and `live.py` do online segmentation
and recognition.
