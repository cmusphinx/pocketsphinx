#!/usr/bin/env python3
"""
Segment live speech from the default audio device.
"""

from pocketsphinx import Vad, Endpointer, set_loglevel
from contextlib import closing
import unittest
import subprocess
import wave
import sys
import os

DATADIR = os.path.join(os.path.dirname(__file__), "../../test/data/librivox")


class VadQ:
    def __init__(self, vad_frames=10, frame_length=0.03):
        self.frames = [None] * vad_frames
        self.is_speech = [0] * vad_frames
        self.n = self.pos = 0
        self.maxlen = vad_frames
        self.frame_length = frame_length
        self.start_time = 0.0

    def __len__(self):
        return self.n

    def empty(self):
        return self.n == 0

    def full(self):
        return self.n == self.maxlen

    def clear(self):
        self.n = 0

    def push(self, is_speech, pcm):
        i = (self.pos + self.n) % self.maxlen
        self.frames[i] = pcm
        self.is_speech[i] = is_speech
        if self.full():
            self.start_time += self.frame_length
            self.pos = (self.pos + 1) % self.maxlen
        else:
            self.n += 1

    def pop(self):
        if self.empty():
            raise IndexError("Queue is empty")
        self.start_time += self.frame_length
        rv = self.is_speech[self.pos], self.frames[self.pos]
        self.pos = (self.pos + 1) % self.maxlen
        self.n -= 1
        return rv

    def speech_count(self):
        if self.empty():
            return 0
        if self.full():
            return sum(self.is_speech)
        # Ideally we would let it equal self.maxlen
        end = (self.pos + self.n) % self.maxlen
        if end > self.pos:
            return sum(self.is_speech[self.pos: end])
        else:
            # Note second term is 0 if end is 0
            return sum(self.is_speech[self.pos:]) + sum(self.is_speech[:end])


class PyEndpointer(Vad):
    def __init__(
        self,
        window=0.3,
        ratio=0.9,
        vad_mode=Vad.LOOSE,
        sample_rate=Vad.DEFAULT_SAMPLE_RATE,
        frame_length=Vad.DEFAULT_FRAME_LENGTH,
    ):
        super(PyEndpointer, self).__init__(vad_mode, sample_rate, frame_length)
        maxlen = int(window / self.frame_length + 0.5)
        self.start_frames = int(ratio * maxlen)
        self.end_frames = int((1.0 - ratio) * maxlen + 0.5)
        print("Threshold %d%% of %.3fs window (>%d frames <%d frames of %d)" %
              (int(ratio * 100.0 + 0.5),
               maxlen * self.frame_length,
               self.start_frames, self.end_frames, maxlen))
        self.vadq = VadQ(maxlen, self.frame_length)
        self.timestamp = 0.0
        self.in_speech = False
        self.speech_start = self.speech_end = None

    def end_stream(self, frame):
        if len(frame) > self.frame_bytes:
            raise IndexError(
                "Last frame size must be %d bytes or less" % self.frame_bytes
            )
        speech_frames = []
        self.timestamp += len(frame) * 0.5 / self.sample_rate
        if not self.in_speech:
            return None
        self.in_speech = False
        self.speech_end = self.vadq.start_time
        while not self.vadq.empty():
            is_speech, pcm = self.vadq.pop()
            if is_speech:
                speech_frames.append(pcm)
                self.speech_end = self.vadq.start_time
            else:
                break
        # If we used all the VAD queue, add the trailing samples
        if self.vadq.empty() and self.speech_end == self.vadq.start_time:
            speech_frames.append(frame)
            self.speech_end = self.timestamp
        self.vadq.clear()
        return b"".join(speech_frames)

    def process(self, frame):
        if self.in_speech:
            assert not self.vadq.full(), "VAD queue overflow (should not happen)"
        if len(frame) != self.frame_bytes:
            raise IndexError("Frame size must be %d bytes" % self.frame_bytes)
        self.vadq.push(self.is_speech(frame), frame)
        self.timestamp += self.frame_length
        speech_count = self.vadq.speech_count()
        #print("%.2f %d %d %d" % (self.timestamp, speech_count, self.start_frames, self.end_frames))
        # Handle state transitions
        if self.in_speech:
            if speech_count < self.end_frames:
                # Return only the first frame.  Either way it's sort
                # of arbitrary, but this avoids having to drain the
                # queue to prevent overlapping segments.  It's also
                # closer to what human annotators will do.
                _, outframe = self.vadq.pop()
                self.speech_end = self.vadq.start_time
                self.in_speech = False
                return outframe
        else:
            if speech_count > self.start_frames:
                self.speech_start = self.vadq.start_time
                self.speech_end = None
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
    def srtest(self, sample_rate):
        ep = Endpointer(vad_mode=3, sample_rate=sample_rate)
        pyep = PyEndpointer(vad_mode=3, sample_rate=sample_rate)
        self.assertEqual(ep.frame_bytes, pyep.frame_bytes)
        soxcmd = ["sox"]
        files, labels = make_single_track()
        soxcmd.extend(files)
        soxcmd.extend("-c 1 -b 16 -e signed-integer -D -G -r".split())
        soxcmd.append("%d" % ep.sample_rate)
        soxcmd.extend("-t raw -".split())
        with subprocess.Popen(soxcmd, stdout=subprocess.PIPE) as sox:
            idx = 0
            while True:
                frame = sox.stdout.read(ep.frame_bytes)
                if len(frame) == 0:
                    break
                elif len(frame) < ep.frame_bytes:
                    speech = ep.end_stream(frame)
                    pyspeech = pyep.end_stream(frame)
                    self.assertEqual(speech, pyspeech)
                else:
                    speech = ep.process(frame)
                    pyspeech = pyep.process(frame)
                    self.assertEqual(speech, pyspeech)
                if speech is not None:
                    self.assertEqual(ep.in_speech, pyep.in_speech)
                    if not ep.in_speech:
                        self.assertFalse(pyep.in_speech)
                        start_time, end_time, _ = labels[idx]
                        start_diff = abs(start_time - ep.speech_start)
                        end_diff = abs(end_time - ep.speech_end)
                        print(
                            "%.2f:%.2f (py: %.2f:%.2f) (truth: %.2f:%.2f) (diff:%.2f:%.2f)"
                            % (
                                ep.speech_start,
                                ep.speech_end,
                                pyep.speech_start,
                                pyep.speech_end,
                                start_time,
                                end_time,
                                start_diff,
                                end_diff,
                            )
                        )
                        self.assertAlmostEqual(ep.speech_start, pyep.speech_start, 3)
                        self.assertAlmostEqual(ep.speech_end, pyep.speech_end, 3)
                        self.assertLess(start_diff, 0.06)
                        self.assertLess(end_diff, 0.21)
                        idx += 1

    def testEndpointer(self):
        set_loglevel("INFO")
        # 8000, 44100, 48000 give slightly different results unfortunately
        for sample_rate in 11025, 16000, 22050, 32000:
            print(sample_rate)
            self.srtest(sample_rate)


if __name__ == "__main__":
    unittest.main()
