#!/usr/bin/env python3
"""
Segment live speech from the default audio device.
"""

from pocketsphinx5 import Vad
from collections import deque
from contextlib import closing
import unittest
import subprocess
import wave
import sys
import os

DATADIR = os.path.join(os.path.dirname(__file__), "../../test/data/librivox")


class Endpointer(Vad):
    def __init__(
        self,
        vad_frames=10,
        vad_ratio=0.9,
        vad_mode=Vad.LOOSE,
        sample_rate=Vad.DEFAULT_SAMPLE_RATE,
        frame_length=Vad.DEFAULT_FRAME_LENGTH,
    ):
        super(Endpointer, self).__init__(vad_mode, sample_rate, frame_length)
        self.buf = deque(maxlen=vad_frames)
        self.ratio = vad_ratio
        self.timestamp = 0.0
        self.in_speech = False
        self.start_time = self.end_time = None

    def process(self, frame):
        if self.in_speech and len(self.buf) == self.buf.maxlen:
            raise IndexError("VAD queue overflow (should not happen)")
        # Handle end of file
        if len(frame) < self.frame_bytes:
            self.timestamp += len(frame) * 0.5 / self.sample_rate
            if self.in_speech:
                self.in_speech = False
                data = b""
                for in_speech, timestamp, outframe in self.buf:
                    if in_speech:
                        data += outframe
                    else:
                        self.end_time = timestamp + self.frame_length
                        return data
                data += frame
                self.end_time = self.timestamp
                return data
            else:
                return None
        self.buf.append((self.is_speech(frame), self.timestamp, frame))
        self.timestamp += self.frame_length
        speech_count = sum(f[0] for f in self.buf)
        # Handle state transitions
        if self.in_speech:
            if speech_count < (1.0 - self.ratio) * self.buf.maxlen:
                # Return only the first frame.  Either way it's sort
                # of arbitrary, but this avoids having to drain the
                # queue to prevent overlapping segments.  It's also
                # closer to what human annotators will do.
                _, timestamp, outframe = self.buf.popleft()
                self.end_time = timestamp + self.frame_length
                self.in_speech = False
                return outframe
        else:
            if speech_count > self.ratio * self.buf.maxlen:
                self.start_time = self.buf[0][1]
                self.end_time = None
                self.in_speech = True
        # Return a buffer if we are in a speech region
        if self.in_speech:
            in_speech, timestamp, outframe = self.buf.popleft()
            return outframe
        else:
            return None


def get_wavfile_length(path):
    with closing(wave.open(path)) as reader:
        nfr = reader.getnframes()
        frate = reader.getframerate()
        return nfr / frate


def get_labels(path, pos):
    with open(path, "rt") as infh:
        labels = [(pos, "silence")]
        for spam in infh:
            # The labels are a bit odd
            start, _, label = spam.strip().split()
            labels.append((pos + float(start), label))
    return labels


def make_single_track():
    labels = []
    infiles = []
    with open(os.path.join(DATADIR, "fileids"), "rt") as infh:
        pos = 0.0
        for spam in infh:
            fileid = spam.strip()
            path = os.path.join(DATADIR, fileid + ".wav")
            infiles.append(path)
            nsec = get_wavfile_length(path)
            path = os.path.join(DATADIR, fileid + ".lab")
            labels.extend(get_labels(path, pos))
            pos += nsec
    out_labels = []
    start_time, label = labels[0]
    for end_time, next_label in labels[1:]:
        if next_label != label:
            if label == "speech":
                out_labels.append((start_time, end_time, label))
            start_time = end_time
        label = next_label
    if label == "speech":
        out_labels.append((start_time, pos, label))
    return infiles, out_labels


class EndpointerTest(unittest.TestCase):
    def testEndpointer(self):
        ep = Endpointer(vad_mode=3)
        soxcmd = ["sox"]
        files, labels = make_single_track()
        soxcmd.extend(files)
        soxcmd.extend("-c 1 -b 16 -e signed-integer -r".split())
        soxcmd.append("%d" % ep.sample_rate)
        soxcmd.extend("-t raw -".split())
        with subprocess.Popen(soxcmd, stdout=subprocess.PIPE) as sox:
            idx = 0
            while True:
                prev_in_speech = ep.in_speech
                frame = sox.stdout.read(ep.frame_bytes)
                speech = ep.process(frame)
                if speech is not None:
                    if not ep.in_speech:
                        start_time, end_time, _ = labels[idx]
                        print(
                            "%.2f:%.2f (truth: %.2f:%.2f) (diff:%.2f:%.2f)"
                            % (
                                ep.start_time,
                                ep.end_time,
                                start_time,
                                end_time,
                                abs(start_time - ep.start_time),
                                abs(end_time - ep.end_time),
                            )
                        )
                        idx += 1
                if len(frame) == 0:
                    break


if __name__ == "__main__":
    unittest.main()
