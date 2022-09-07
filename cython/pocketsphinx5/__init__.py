"""Main module for the PocketSphinx speech recognizer.
"""

# Copyright (c) 1999-2016 Carnegie Mellon University. All rights
# reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in
#    the documentation and/or other materials provided with the
#    distribution.
#
# This work was supported in part by funding from the Defense Advanced
# Research Projects Agency and the National Science Foundation of the
# United States of America, and the CMU Sphinx Speech Consortium.
#
# THIS SOFTWARE IS PROVIDED BY CARNEGIE MELLON UNIVERSITY ``AS IS'' AND
# ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
# NOR ITS EMPLOYEES BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import collections
import os
import signal
from contextlib import contextmanager

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

Arg = collections.namedtuple("Arg", ["name", "default", "doc", "type", "required"])
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


class Pocketsphinx(Decoder):
    def __init__(self, **kwargs):
        if kwargs.get("dic") is not None and kwargs.get("dict") is None:
            kwargs["dict"] = kwargs.pop("dic")
        if kwargs.pop("verbose", False) is True:
            kwargs["loglevel"] = "INFO"

        super(Pocketsphinx, self).__init__(**kwargs)

    def __str__(self):
        return self.hypothesis()

    @contextmanager
    def start_utterance(self):
        self.start_utt()
        yield
        self.end_utt()

    @contextmanager
    def end_utterance(self):
        self.end_utt()
        yield
        self.start_utt()

    def decode(self, audio_file, buffer_size=2048, no_search=False, full_utt=False):
        buf = bytearray(buffer_size)

        with open(audio_file, "rb") as f:
            with self.start_utterance():
                while f.readinto(buf):
                    self.process_raw(buf, no_search, full_utt)
        return self

    def segments(self, detailed=False):
        if detailed:
            lmath = self.get_logmath()
            return [(s.word, lmath.log(s.prob),
                     s.start_frame, s.end_frame) for s in self.seg()]
        else:
            return [s.word for s in self.seg()]

    def hypothesis(self):
        hyp = self.hyp()
        if hyp:
            return hyp.hypstr
        else:
            return ""

    def probability(self):
        hyp = self.hyp()
        if hyp:
            return self.get_logmath().log(hyp.prob)

    def score(self):
        hyp = self.hyp()
        if hyp:
            return self.get_logmath().log(hyp.best_score)

    def best(self, count=10):
        lmath = self.get_logmath()
        return [(h.hypstr, lmath.log(h.score)) for h, i in zip(self.nbest(), range(count))]

    def confidence(self):
        hyp = self.hyp()
        if hyp:
            return hyp.prob


class AudioFile(Pocketsphinx):
    def __init__(self, **kwargs):
        signal.signal(signal.SIGINT, self.stop)

        self.audio_file = kwargs.pop("audio_file", None)
        self.buffer_size = kwargs.pop("buffer_size", 2048)
        self.no_search = kwargs.pop("no_search", False)
        self.full_utt = kwargs.pop("full_utt", False)

        self.keyphrase = kwargs.get("keyphrase")

        self.in_speech = False
        self.buf = bytearray(self.buffer_size)

        super(AudioFile, self).__init__(**kwargs)

        self.f = open(self.audio_file, "rb")

    def __iter__(self):
        with self.f:
            with self.start_utterance():
                while self.f.readinto(self.buf):
                    self.process_raw(self.buf, self.no_search, self.full_utt)
                    if self.keyphrase and self.hyp():
                        with self.end_utterance():
                            yield self
                    elif self.in_speech != self.get_in_speech():
                        self.in_speech = self.get_in_speech()
                        if not self.in_speech and self.hyp():
                            with self.end_utterance():
                                yield self

    def stop(self, *args, **kwargs):
        raise StopIteration


class LiveSpeech(Pocketsphinx):
    def __init__(self, **kwargs):
        signal.signal(signal.SIGINT, self.stop)

        self.audio_device = kwargs.pop("audio_device", None)
        self.sampling_rate = kwargs.pop("sampling_rate", 16000)
        self.buffer_size = kwargs.pop("buffer_size", 2048)
        self.no_search = kwargs.pop("no_search", False)
        self.full_utt = kwargs.pop("full_utt", False)

        self.keyphrase = kwargs.get("keyphrase")

        self.in_speech = False
        self.buf = bytearray(self.buffer_size)
        self.ad = Ad(self.audio_device, self.sampling_rate)

        super(LiveSpeech, self).__init__(**kwargs)

    def __iter__(self):
        with self.ad:
            with self.start_utterance():
                while self.ad.readinto(self.buf) >= 0:
                    self.process_raw(self.buf, self.no_search, self.full_utt)
                    if self.keyphrase and self.hyp():
                        with self.end_utterance():
                            yield self
                    elif self.in_speech != self.get_in_speech():
                        self.in_speech = self.get_in_speech()
                        if not self.in_speech and self.hyp():
                            with self.end_utterance():
                                yield self

    def stop(self, *args, **kwargs):
        raise StopIteration
