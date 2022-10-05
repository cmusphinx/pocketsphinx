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

You should be able to install this with pip for recent platforms and
versions of Python:

    pip3 install pocketsphinx

Alternately, you can also compile it from the source tree.  I highly
suggest doing this in a virtual environment (replace
`~/ve_pocketsphinx` with the virtual environment you wish to create),
from the top level directory:

    python3 -m venv ~/ve_pocketsphinx
    . ~/ve_pocketsphinx/bin/activate
    pip3 install .

On GNU/Linux and maybe other platforms, you must have
[PortAudio](http://www.portaudio.com/) installed for the `LiveSpeech`
class to work (we may add a fall-back to `sox` in the near future).
On Debian-like systems this can be achieved by installing the
`libportaudio2` package:

    sudo apt-get install libportaudio2

Usage
-----

See the [examples directory](../examples/) for a number of examples of
using the library from Python.  You can also read the [documentation
for the Python API](https://pocketsphinx.readthedocs.io) or [the C
API](https://cmusphinx.github.io/doc/pocketsphinx/).

It also mostly supports the same APIs as the previous
[pocketsphinx-python](https://github.com/bambocher/pocketsphinx-python)
module, as described below.

### LiveSpeech

An iterator class for continuous recognition or keyword search from a
microphone.  For example, to do speech-to-text with the default (some
kind of US English) model:

```python
from pocketsphinx import LiveSpeech
for phrase in LiveSpeech(): print(phrase)
```

Or to do keyword search:

```python
from pocketsphinx import LiveSpeech

speech = LiveSpeech(keyphrase='forward', kws_threshold=1e-20)
for phrase in speech:
    print(phrase.segments(detailed=True))
```

With your model and dictionary:

```python
import os
from pocketsphinx import LiveSpeech, get_model_path

speech = LiveSpeech(
    sampling_rate=16000,  # optional
    hmm=get_model_path('en-us'),
    lm=get_model_path('en-us.lm.bin'),
    dic=get_model_path('cmudict-en-us.dict')
)

for phrase in speech:
    print(phrase)
```

### AudioFile

This is an iterator class for continuous recognition or keyword search
from a file.  Currently it supports only raw, single-channel, 16-bit
PCM data in native byte order.

```python
from pocketsphinx import AudioFile
for phrase in AudioFile("goforward.raw"): print(phrase) # => "go forward ten meters"
```

An example of a keyword search:

```python
from pocketsphinx import AudioFile

audio = AudioFile("goforward.raw", keyphrase='forward', kws_threshold=1e-20)
for phrase in audio:
    print(phrase.segments(detailed=True)) # => "[('forward', -617, 63, 121)]"
```

With your model and dictionary:

```python
import os
from pocketsphinx import AudioFile, get_model_path

model_path = get_model_path()

config = {
    'verbose': False,
    'audio_file': 'goforward.raw',
    'hmm': get_model_path('en-us'),
    'lm': get_model_path('en-us.lm.bin'),
    'dict': get_model_path('cmudict-en-us.dict')
}

audio = AudioFile(**config)
for phrase in audio:
    print(phrase)
```

Convert frame into time coordinates:

```python
from pocketsphinx import AudioFile

# Frames per Second
fps = 100

for phrase in AudioFile(frate=fps):  # frate (default=100)
    print('-' * 28)
    print('| %5s |  %3s  |   %4s   |' % ('start', 'end', 'word'))
    print('-' * 28)
    for s in phrase.seg():
        print('| %4ss | %4ss | %8s |' % (s.start_frame / fps, s.end_frame / fps, s.word))
    print('-' * 28)

# ----------------------------
# | start |  end  |   word   |
# ----------------------------
# |  0.0s | 0.24s | <s>      |
# | 0.25s | 0.45s | <sil>    |
# | 0.46s | 0.63s | go       |
# | 0.64s | 1.16s | forward  |
# | 1.17s | 1.52s | ten      |
# | 1.53s | 2.11s | meters   |
# | 2.12s |  2.6s | </s>     |
# ----------------------------
```

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
Vyacheslav Klimkov, and others.  The
[pocketsphinx-python](https://github.com/bambocher/pocketsphinx-python)
module was originally written by Dmitry Prazdnichnov.

Currently this is maintained by David Huggins-Daines again.
