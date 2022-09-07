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

`simple.c` and `live.c` will accept the same set of command-line
arguments as `pocketsphinx`, so for example, if you have not installed
PocketSphinx, you will need to specify the acoustic and language
models, e.g.:

    ./simple -hmm ../model/en-us/en-us \
        -lm ../model/en-us/en-us.lm.bin \
        -dict ../model/en-us/cmudict-en-us.dict

The Python scripts, assuming you have installed the `pocketsphinx`
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
