PocketSphinx Examples
=====================

This directory contains some examples of basic PocketSphinx library
usage in C and Python.  To compile the C examples, you can build the
target `examples`.  If you want to see how it works manually, either
use the library directly in-place, for example, with `simple.c`:

    cmake -DBUILD_SHARED_LIBS=OFF .. && make
    cc -o simple simple.c -I../include -Iinclude -L. -lpocketsphinx -lm
    
Or if PocketSphinx is installed:

    cc -o simple simple.c $(pkg-config --static --libs --cflags pocketsphinx)

If PocketSphinx has not been installed, you will need to set the
`POCKETSPHINX_PATH` environment variable to run the examples:

    POCKETSPHINX_PATH=../model ./simple

The Python scripts, assuming you have installed the `pocketsphinx`
module (see [the top-leve README](../README.md) for instructions), can
just be run as-is:

    python simple.py spam.wav
    
Simplest possible example
-------------------------

The examples `simple.c` and `simple.py` read an entire audio file
(only WAV files are supported) and recognize it as a single, possibly
long, utterance.

Segmentation
------------

The example `segment.py` uses voice activity detection to *segment*
the input stream into speech-like regions.

Live recognition
----------------

Finally, the examples `live.c` and `live.py` do online segmentation
and recognition.
