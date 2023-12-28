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
import importlib.util
import os
import signal
from contextlib import contextmanager

from . import _pocketsphinx as pocketsphinx  # noqa: F401
from ._pocketsphinx import LogMath  # noqa: F401
from ._pocketsphinx import Config  # noqa: F401
from ._pocketsphinx import Decoder  # noqa: F401
from ._pocketsphinx import Jsgf  # noqa: F401
from ._pocketsphinx import JsgfRule  # noqa: F401
from ._pocketsphinx import NGramModel  # noqa: F401
from ._pocketsphinx import FsgModel  # noqa: F401
from ._pocketsphinx import Segment  # noqa: F401
from ._pocketsphinx import Hypothesis  # noqa: F401
from ._pocketsphinx import Lattice  # noqa: F401
from ._pocketsphinx import Vad  # noqa: F401
from ._pocketsphinx import Endpointer  # noqa: F401
from ._pocketsphinx import Alignment  # noqa: F401
from ._pocketsphinx import AlignmentEntry  # noqa: F401
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

    If the POCKETSPHINX_PATH environment variable is set, it will be
    returned here, otherwise the default is determined by your
    PocketSphinx installation, and may or may not be writable by you.

    Args:
        subpath: An optional path to add to the model directory.

    Returns:
        The requested path within the model directory.

    """
    model_path = pocketsphinx._ps_default_modeldir()
    if model_path is None:
        # Use importlib to find things (so editable installs work)
        model_path = importlib.util.find_spec(
            "pocketsphinx.model"
        ).submodule_search_locations[0]
    if subpath is not None:
        return os.path.join(model_path, subpath)
    else:
        return model_path


class Pocketsphinx(Decoder):
    """Compatibility wrapper class.

    This class is deprecated, as most of its functionality is now
    available in the main `Decoder` class, but it is here in case you
    had code that used the old external pocketsphinx-python module.
    """

    def __init__(self, **kwargs):
        if kwargs.get("dic") is not None and kwargs.get("dict") is None:
            kwargs["dict"] = kwargs.pop("dic")
        if kwargs.pop("verbose", False) is True:
            kwargs["loglevel"] = "INFO"
        self.start_frame = 0
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
            return [
                (
                    s.word,
                    lmath.log(s.prob),
                    self.start_frame + s.start_frame,
                    self.start_frame + s.end_frame,
                )
                for s in self.seg()
            ]
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
        return [
            (h.hypstr, lmath.log(h.score)) for h, i in zip(self.nbest(), range(count))
        ]

    def confidence(self):
        hyp = self.hyp()
        if hyp:
            return hyp.prob


class AudioFile(Pocketsphinx):
    """Simple audio file segmentation and speech recognition.

    It is recommended to use the `Segmenter` and `Decoder` classes
    directly, but this is here in case you had code that used the old
    external pocketsphinx-python module, or need something very
    simple.

    """

    def __init__(self, audio_file=None, **kwargs):
        signal.signal(signal.SIGINT, self.stop)

        self.audio_file = audio_file
        self.segmenter = Segmenter()

        # You would never actually set these!
        kwargs.pop("no_search", False)
        kwargs.pop("full_utt", False)
        kwargs.pop("buffer_size", False)
        self.keyphrase = kwargs.get("keyphrase")

        super(AudioFile, self).__init__(**kwargs)
        self.f = open(self.audio_file, "rb")

    def __iter__(self):
        with self.f:
            for speech in self.segmenter.segment(self.f):
                self.start_frame = int(speech.start_time * self.config["frate"] + 0.5)
                self.start_utt()
                self.process_raw(speech.pcm, full_utt=True)
                if self.keyphrase and self.hyp():
                    self.end_utt()
                    yield self
                else:
                    self.end_utt()
                    yield self

    def stop(self, *args, **kwargs):
        raise StopIteration


class LiveSpeech(Pocketsphinx):
    """Simple endpointing and live speech recognition.

    This class is not very useful for an actual application.  It is
    recommended to use the `Endpointer` and `Decoder` classes
    directly, but it is here in case you had code that used the old
    external pocketsphinx-python module, or need something incredibly
    simple.

    """

    def __init__(self, **kwargs):
        self.audio_device = kwargs.pop("audio_device", None)
        self.sampling_rate = kwargs.pop("sampling_rate", 16000)
        self.ep = Endpointer(sample_rate=self.sampling_rate)
        self.buffer_size = self.ep.frame_bytes

        # Setting these will not do anything good!
        kwargs.pop("no_search", False)
        kwargs.pop("full_utt", False)
        kwargs.pop("buffer_size", False)

        self.keyphrase = kwargs.get("keyphrase")

        try:
            import sounddevice

            assert sounddevice
        except Exception as e:
            # In case PortAudio is not present, for instance
            raise RuntimeError("LiveSpeech not supported: %s" % e)
        self.ad = sounddevice.RawInputStream(
            samplerate=self.sampling_rate,
            # WE DO NOT CARE ABOUT LATENCY!
            blocksize=self.buffer_size // 2,
            dtype="int16",
            channels=1,
            device=self.audio_device,
        )
        super(LiveSpeech, self).__init__(**kwargs)

    def __iter__(self):
        with self.ad:
            not_done = True
            while not_done:
                try:
                    self.buf, _ = self.ad.read(self.buffer_size // 2)
                    if len(self.buf) == self.buffer_size:
                        speech = self.ep.process(self.buf)
                    else:
                        speech = self.ep.end_stream(self.buf)
                        not_done = False
                    if speech is not None:
                        if not self.in_speech:
                            self.start_utt()
                        self.process_raw(speech)
                        if self.keyphrase and self.hyp():
                            with self.end_utterance():
                                yield self
                        elif not self.ep.in_speech:
                            self.end_utt()
                            if self.hyp():
                                yield self
                except KeyboardInterrupt:
                    break

    @property
    def in_speech(self):
        return self.get_in_speech()
