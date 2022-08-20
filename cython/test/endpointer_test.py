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


class VadQ:
    def __init__(self, vad_frames=10, frame_length=0.03):
        self.frames = deque(maxlen=vad_frames)
        self.is_speech = deque(maxlen=vad_frames)
        self.maxlen = vad_frames
        self.frame_length = frame_length
        self.start_time = 0.0

    def __len__(self):
        return len(self.frames)

    def empty(self):
        return len(self.frames) == 0

    def full(self):
        return len(self.frames) == self.maxlen

    def push(self, is_speech, pcm):
        if self.full():
            self.start_time += self.frame_length
        self.frames.append(pcm)
        self.is_speech.append(is_speech)

    def pop(self):
        self.start_time += self.frame_length
        return self.is_speech.popleft(), self.frames.popleft()

    def speech_count(self):
        return sum(self.is_speech)

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
        self.vadq = VadQ(vad_frames, frame_length)
        self.ratio = vad_ratio
        self.timestamp = 0.0
        self.in_speech = False
        self.start_time = self.end_time = None

    def eof_speech(self, frame):
        speech_frames = []
        while not self.vadq.empty():
            is_speech, pcm = self.vadq.pop()
            if is_speech:
                speech_frames.append(pcm)
                self.end_time = self.vadq.start_time
            else:
                break
        if self.vadq.empty():
            speech_frames.append(frame)
            self.end_time = self.timestamp
        return b"".join(speech_frames)

    def process(self, frame):
        if self.in_speech:
            assert not self.vadq.full(), "VAD queue overflow (should not happen)" 
        # Handle end of file
        if len(frame) < self.frame_bytes:
            self.timestamp += len(frame) * 0.5 / self.sample_rate
            if self.in_speech:
                self.in_speech = False
                return self.eof_speech(frame)
            else:
                return None
        self.vadq.push(self.is_speech(frame), frame)
        self.timestamp += self.frame_length
        speech_count = self.vadq.speech_count()
        # Handle state transitions
        if self.in_speech:
            if speech_count < (1.0 - self.ratio) * self.vadq.maxlen:
                # Return only the first frame.  Either way it's sort
                # of arbitrary, but this avoids having to drain the
                # queue to prevent overlapping segments.  It's also
                # closer to what human annotators will do.
                _, outframe = self.vadq.pop()
                self.end_time = self.vadq.start_time
                self.in_speech = False
                return outframe
        else:
            if speech_count > self.ratio * self.vadq.maxlen:
                self.start_time = self.vadq.start_time
                self.end_time = None
                self.in_speech = True
        # Return a buffer if we are in a speech region
        if self.in_speech:
            _, outframe = self.vadq.pop()
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
                frame = sox.stdout.read(ep.frame_bytes)
                speech = ep.process(frame)
                if speech is not None:
                    if not ep.in_speech:
                        start_time, end_time, _ = labels[idx]
                        start_diff = abs(start_time - ep.start_time)
                        end_diff = abs(end_time - ep.end_time)
                        print(
                            "%.2f:%.2f (truth: %.2f:%.2f) (diff:%.2f:%.2f)"
                            % (
                                ep.start_time,
                                ep.end_time,
                                start_time,
                                end_time,
                                start_diff,
                                end_diff,
                            )
                        )
                        self.assertLess(start_diff, 0.06)
                        self.assertLess(end_diff, 0.21)
                        idx += 1
                if len(frame) == 0:
                    break


if __name__ == "__main__":
    unittest.main()
