"""VAD-based segmentation.
"""

from ._pocketsphinx import Endpointer
from collections import namedtuple

SpeechSegment = namedtuple("SpeechSegment", ["start_time", "end_time", "pcm"])


class Segmenter(Endpointer):
    """VAD-based speech segmentation.

    This is a simple class that segments audio from an input stream,
    which is assumed to produce binary data as 16-bit signed integers
    when `read` is called on it.  It takes the same arguments as its
    parent `Endpointer` class.

    You could obviously use this on a raw audio file, but also on a
    `sounddevice.RawInputStream` or the output of `sox`.  You can even
    use it with the built-in `wave` module, for example::

        with wave.open("foo.wav", "r") as w:
            segmenter = Segmenter(sample_rate=w.getframerate())
            for seg in segmenter.segment(w.getfp()):
                with wave.open("%.2f-%.2f.wav"
                               % (seg.start_time, seg.end_time), "w") as wo:
                    wo.setframerate(w.getframerate())
                    wo.writeframesraw(seg.pcm)

    Args:
      window(float): Length in seconds of window for decision.
      ratio(float): Fraction of window that must be speech or
                    non-speech to make a transition.
      mode(int): Aggressiveness of voice activity detction (0-3)
      sample_rate(int): Sampling rate of input, default is 16000.
                        Rates other than 8000, 16000, 32000, 48000
                        are only approximately supported, see note
                        in `frame_length`.  Outlandish sampling
                        rates like 3924 and 115200 will raise a
                        `ValueError`.
      frame_length(float): Desired input frame length in seconds,
                           default is 0.03.  The *actual* frame
                           length may be different if an
                           approximately supported sampling rate is
                           requested.  You must *always* use the
                           `frame_bytes` and `frame_length`
                           attributes to determine the input size.

    Raises:
      ValueError: Invalid input parameter.  Also raised if the ratio
                  makes it impossible to do endpointing (i.e. it
                  is more than N-1 or less than 1 frame).
    """
    def __init__(self, *args, **kwargs):
        super(Segmenter, self).__init__(*args, **kwargs)
        self.speech_frames = []

    def segment(self, stream):
        """Split a stream of data into speech segments.

        Args:
            stream: File-like object returning binary data (assumed to
                    be single-channel, 16-bit integer PCM)

        Returns:
           Iterable[SpeechSegment]: Generator over `SpeechSegment` for
           each speech region detected by the `Endpointer`.

        """
        idx = 0
        while True:
            frame = stream.read(self.frame_bytes)
            if len(frame) == 0:
                break
            elif len(frame) < self.frame_bytes:
                speech = self.end_stream(frame)
            else:
                speech = self.process(frame)
            if speech is not None:
                self.speech_frames.append(speech)
                if not self.in_speech:
                    yield SpeechSegment(
                        start_time=self.speech_start,
                        end_time=self.speech_end,
                        pcm=b"".join(self.speech_frames),
                    )
                    del self.speech_frames[:]
                    idx += 1
