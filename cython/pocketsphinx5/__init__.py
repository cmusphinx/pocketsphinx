"""Main module for the PocketSphinx speech recognizer.
"""

import collections

from ._pocketsphinx import Config
from ._pocketsphinx import Decoder
from ._pocketsphinx import FsgModel
from ._pocketsphinx import Segment
from ._pocketsphinx import Hypothesis

Arg = collections.namedtuple("Arg", ["name", "default", "doc", "type", "required"])
Arg.__doc__ = "Description of a configuration parameter."
Arg.name.__doc__ = "Parameter name (without leading dash)."
Arg.default.__doc__ = "Default value of parameter."
Arg.doc.__doc__ = "Description of parameter."
Arg.type.__doc__ = "Type (as a Python type object) of parameter value."
Arg.required.__doc__ = "Is this parameter required?"
