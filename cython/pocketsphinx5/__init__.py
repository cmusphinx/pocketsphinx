"""Main module for the PocketSphinx speech recognizer.
"""

import collections
import os

from . import _pocketsphinx

from ._pocketsphinx import LogMath  # noqa: F401
from ._pocketsphinx import Config  # noqa: F401
from ._pocketsphinx import Decoder  # noqa: F401
from ._pocketsphinx import Jsgf  # noqa: F401
from ._pocketsphinx import JsgfRule  # noqa: F401
from ._pocketsphinx import NGramModel  # noqa: F401
from ._pocketsphinx import FsgModel  # noqa: F401
from ._pocketsphinx import Segment  # noqa: F401
from ._pocketsphinx import Hypothesis  # noqa: F401
from ._pocketsphinx import Vad  # noqa: F401
from ._pocketsphinx import Endpointer  # noqa: F401
from ._pocketsphinx import set_loglevel  # noqa: F401
from .segmenter import Segmenter  # noqa: F401

Arg = collections.namedtuple("Arg", ["name", "default", "doc",
                                     "type", "required"])
Arg.__doc__ = "Description of a configuration parameter."
Arg.name.__doc__ = "Parameter name (without leading dash)."
Arg.default.__doc__ = "Default value of parameter."
Arg.doc.__doc__ = "Description of parameter."
Arg.type.__doc__ = "Type (as a Python type object) of parameter value."
Arg.required.__doc__ = "Is this parameter required?"


def get_model_path(subpath=None):
    """Return path to the model directory, or optionally, a specific file
    or directory within it.

    Args:
        subpath: An optional path to add to the model directory.

    Returns:
        The requested path within the model directory."""
    model_path = _pocketsphinx._ps_default_modeldir()
    if model_path is None:
        model_path = os.path.join(os.path.dirname(__file__), "model")
    if subpath is not None:
        return os.path.join(model_path, subpath)
    else:
        return model_path
