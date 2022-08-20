#!/usr/bin/env python3
"""
Segment speech endlessly from the default audio device.
"""

from pocketsphinx5 import Vad
from collections import deque, namedtuple
import subprocess


Speech = namedtuple("Speech", ["idx", "start_time", "end_time", "pcm"])


class Segmenter(Vad):
    def __init__(
        self,
        vad_frames=10,
        vad_ratio=0.9,
        vad_mode=Vad.LOOSE,
        sample_rate=Vad.DEFAULT_SAMPLE_RATE,
        frame_length=Vad.DEFAULT_FRAME_LENGTH,
    ):
        super(Segmenter, self).__init__(vad_mode, sample_rate, frame_length)
        self.buf = deque(maxlen=vad_frames)
        self.ratio = vad_ratio
        self.timestamp = 0.0
        self.start_time = self.end_time = None
        self.speech_frames = None

    def segment(self, stream):
        idx = 0
        while True:
            if self.speech_frames and len(self.buf) == self.buf.maxlen:
                raise IndexError("VAD queue overflow (should not happen)")
            frame = stream.read(self.frame_bytes)
            if len(frame) == 0:
                break
            self.buf.append((self.is_speech(frame), self.timestamp, frame))
            self.timestamp += self.frame_length
            speech_count = sum(f[0] for f in self.buf)
            if self.speech_frames:
                self.speech_frames.append(self.buf.popleft()[-1])
                if speech_count < (1.0 - self.ratio) * self.buf.maxlen:
                    self.end_time = self.buf[-1][1] + self.frame_length
                    self.speech_frames.extend(frame for _, _, frame in self.buf)
                    yield Speech(
                        idx=idx,
                        start_time=self.start_time,
                        end_time=self.end_time,
                        pcm=b"".join(self.speech_frames),
                    )
                    self.buf.clear()
                    self.speech_frames = None
                    self.start_time = None
                    idx += 1
            else:
                if speech_count > self.ratio * self.buf.maxlen:
                    self.start_time = self.buf[0][1]
                    self.end_time = None
                    self.speech_frames = [self.buf.popleft()[-1]]


seg = Segmenter()
incmd = f"sox -q -r {seg.sample_rate} -c 1 -b 16 -e signed-integer -d -t raw -".split()
outcmd = f"sox -q -t raw -r {seg.sample_rate} -c 1 -b 16 -e signed-integer -".split()
with subprocess.Popen(incmd, stdout=subprocess.PIPE) as sox:
    try:
        for speech in seg.segment(sox.stdout):
            outfile = "%03d_%.2f-%.2f.wav" % (
                speech.idx,
                speech.start_time,
                speech.end_time,
            )
            with subprocess.Popen(outcmd + [outfile], stdin=subprocess.PIPE) as soxout:
                soxout.stdin.write(speech.pcm)
            print("Wrote %s" % outfile)
    except KeyboardInterrupt:
        pass
