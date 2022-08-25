PocketSphinx 5.0.0 release candidate 1
======================================

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

There is no longer any dependency on SphinxBase, because there is no
reason for SphinxBase to exist.  You can just link against the
PocketSphinx library, which now includes all of its functionality.

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

There is currently no command-line tool for PocketSphinx, aside from
`pocketsphinx_batch`, which has far too many options to describe here,
and is mostly just useful for evaluating on large test sets.  This
will be fixed before the actual release.

For now, see the [examples directory](./examples/) for a number of
examples of using the library from C and Python.
