PocketSphinx 5.0.0
==================

This is PocketSphinx, one of Carnegie Mellon University's open source large
vocabulary, speaker-independent continuous speech recognition engines.

Although this was at one point a research system, active development
has largely ceased and it has become very, very far from the state of
the art.  I am making a release, because people are nonetheless using
it, and there are a number of historical errors in the build system
and API which needed to be corrected.

The version number is strangely large because there was a "release"
that people are using called 5prealpha, and we will use proper
[semantic versioning](https://semver.org/) from now on.

**Please see the LICENSE file for terms of use.**

Installation
------------

We now use CMake for building, which should give reasonable results
across Linux and Windows.  Not certain about Mac OS X because I don't
have one of those.  In addition, the audio library, which never really
built or worked correctly on any platform at all, has simply been
removed.

There is no longer any dependency on SphinxBase.  There is no
SphinxBase anymore.  This is not the SphinxBase you're looking for.
All your SphinxBase are belong to us.

To install the Python module in a virtual environment (replace
`~/ve_pocketsphinx` with the virtual environment you wish to create),
from the top level directory:

```
python3 -m venv ~/ve_pocketsphinx
. ~/ve_pocketsphinx/bin/activate
pip install .
```

To install the C library and bindings (assuming you have access to
/usr/local - if not, use `-DCMAKE_INSTALL_PREFIX` to set a different
prefix in the first `cmake` command below):

```
cmake -S . -B build
cmake --build build
cmake --build build --target install
```

Usage
-----

The `pocketsphinx` command-line program reads single-channel 16-bit
PCM audio from standard input or one or more files, and attemps to
recognize speech in it using the default acoustic and language model.
It accepts a large number of options which you probably don't care
about, a *command* which defaults to `live`, and one or more inputs
(except in `align` mode), or `-` to read from standard input.

If you have a single-channel WAV file called "speech.wav" and you want
to recognize speech in it, you can try doing this (the results may not
be wonderful):

    pocketsphinx single speech.wav
    
If your input is in some other format I suggest converting it with
`sox` as described below.

The commands are as follows:

  - `help`: Print a long list of those options you don't care about.
  
  - `config`: Dump configuration as JSON to standard output (can be
    loaded with the `-config` option).

  - `live`: Detect speech segments in each input, run recognition
    on them (using those options you don't care about), and write the
    results to standard output in line-delimited JSON.  I realize this
    isn't the prettiest format, but it sure beats XML.  Each line
    contains a JSON object with these fields, which have short names
    to make the lines more readable:
    
    - `b`: Start time in seconds, from the beginning of the stream
    - `d`: Duration in seconds
    - `p`: Estimated probability of the recognition result, i.e. a
      number between 0 and 1 representing the likelihood of the input
      according to the model
    - `t`: Full text of recognition result
    - `w`: List of segments (usually words), each of which in turn
      contains the `b`, `d`, `p`, and `t` fields, for start, end,
      probability, and the text of the word.  If `-phone_align yes`
      has been passed, then a `w` field will be present containing
      phone segmentations, in the same format.

  - `single`: Recognize each input as a single utterance, and write a
    JSON object in the same format described above.
    
  - `align`: Align a single input file (or `-` for standard input) to
    a word sequence, and write a JSON object in the same format
    described above.  The first positional argument is the input, and
    all subsequent ones are concatenated to make the text, to avoid
    surprises if you forget to quote it.  You are responsible for
    normalizing the text to remove punctuation, uppercase, centipedes,
    etc. For example:
    
        pocketsphinx align goforward.wav "go forward ten meters"
        
    By default, only word-level alignment is done.  To get phone
    alignments, pass `-phone_align yes` in the flags, e.g.:
    
        pocketsphinx -phone_align yes align audio.wav $text
        
    This will make not particularly readable output, but you can use
    [jq](https://stedolan.github.io/jq/) to clean it up.  For example,
    you can get just the word names and start times like this:
    
        pocketsphinx align audio.wav $text | jq '.w[]|[.t,.b]'
        
    Or you could get the phone names and durations like this:
    
        pocketsphinx -phone_align yes align audio.wav $text | jq '.w[]|.w[]|[.t,.d]'
        
    There are many, many other possibilities, of course.

  - `soxflags`: Return arguments to `sox` which will create the
    appropriate input format.  Note that because the `sox`
    command-line is slightly quirky these must always come *after* the
    filename or `-d` (which tells `sox` to read from the microphone).
    You can run live recognition like this:
    
        sox -d $(pocketsphinx soxflags) | pocketsphinx -

    or decode from a file named "audio.mp3" like this:
    
        sox audio.mp3 $(pocketsphinx soxflags) | pocketsphinx -
        
By default only errors are printed to standard error, but if you want
more information you can pass `-loglevel INFO`.  Partial results are
not printed, maybe they will be in the future, but don't hold your
breath.

Programming
-----------

For programming, see the [examples directory](./examples/) for a
number of examples of using the library from C and Python.  You can
also read the [documentation for the Python
API](https://pocketsphinx.readthedocs.io) or [the C
API](https://cmusphinx.github.io/doc/pocketsphinx/)

Authors
-------

PocketSphinx is ultimately based on `Sphinx-II` which in turn was
based on some older systems at Carnegie Mellon University, which were
released as free software under a BSD-like license thanks to the
efforts of Kevin Lenzo.  Much of the decoder in particular was written
by Ravishankar Mosur (look for "rkm" in the comments), but various
other people contributed as well, see [the AUTHORS file](./AUTHORS)
for more details.

David Huggins-Daines (the author of this document) is
guilty^H^H^H^H^Hresponsible for creating `PocketSphinx` which added
various speed and memory optimizations, fixed-point computation, JSGF
support, portability to various platforms, and a somewhat coherent
API.  He then disappeared for a while.

Nickolay Shmyrev took over maintenance for quite a long time
afterwards, and a lot of code was contributed by Alexander Solovets,
Vyacheslav Klimkov, and others.

Currently this is maintained by David Huggins-Daines again.
