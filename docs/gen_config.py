#!/usr/bin/env python3

"""Generate the config.rst source from the currently installed version
of PocketSphinx.

FIXME: This should be integrated into the Sphinx build but I haven't
figured out how to do that yet.
"""

from pocketsphinx import Decoder

PREAMBLE = """Configuration parameters
========================

These are the parameters currently recognized by
`pocketsphinx.Config` and `pocketsphinx.Decoder` along with their
default values.

.. method:: Config(*args, **kwargs)

   Create a PocketSphinx configuration.  This constructor can be
   called with a list of arguments corresponding to a command-line, in
   which case the parameter names should be prefixed with a '-'.
   Otherwise, pass the keyword arguments described below.  For
   example, the following invocations are equivalent::

        config = Config("-hmm", "path/to/things", "-dict", "my.dict")
        config = Config(hmm="path/to/things", dict="my.dict")

   The same keyword arguments can also be passed directly to the
   constructor for `pocketsphinx.Decoder`.

"""

config = Decoder.default_config()

# Sort them into required and other
required = []
other = []
for arg in config.describe():
    if arg.required:
        required.append(arg)
    else:
        other.append(arg)
required.sort(key=lambda a: a.name)
kwargs = required + other

print(PREAMBLE)
for arg in kwargs:
    arg_text = f"   :keyword {arg.type.__name__} {arg.name}: "
    if arg.doc is not None:
        arg_text += arg.doc
    if arg.default is not None:
        if arg.type == bool:
            default = arg.default == "yes"
        else:
            default = arg.type(arg.default)
        arg_text += f", defaults to ``{default}``"
    print(arg_text)
