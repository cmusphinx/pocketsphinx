"""VAD-based segmentation.
"""

from ._pocketsphinx import Endpointer
from collections import namedtuple

SpeechSegment = namedtuple("SpeechSegment", ["start_time", "end_time", "pcm"])


class Segmenter(Endpointer):
    """
    VAD-based speech segmentation
    """
    def __init__(self, *args, **kwargs):
        super(Segmenter, self).__init__(*args, **kwargs)
        self.speech_frames = []

    def segment(self, stream):
        """Split a stream of data into speech segments.

        Generates a `SpeechSegment` for each detected by the
        `Endpointer`.

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
