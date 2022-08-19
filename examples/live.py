#!/usr/bin/env python3
"""
Segment live speech from the default audio device.
"""

from pocketsphinx5 import Vad, Decoder
from collections import deque
import subprocess
import sys
import os

MODELDIR = os.path.join(os.path.dirname(__file__), os.path.pardir, "model")


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
        self.buf.append((self.is_speech(frame), self.timestamp, frame))
        self.timestamp += self.frame_length
        speech_count = sum(f[0] for f in self.buf)
        # Handle state transitions
        if self.in_speech:
            if speech_count < (1.0 - self.ratio) * self.buf.maxlen:
                self.end_time = self.buf[-1][1]
                self.in_speech = False
                # Return all trailing frames to match webrtc example
                return b"".join(frame for _, _, frame in self.buf)
        else:
            if speech_count > self.ratio * self.buf.maxlen:
                self.start_time = self.buf[0][1]
                self.end_time = None
                self.in_speech = True
        # Return a buffer if we are in a speech region
        if self.in_speech:
            in_speech, timestamp, frame = self.buf.popleft()
            return frame
        return None


def main():
    ep = Endpointer()
    decoder = Decoder(
        hmm=os.path.join(MODELDIR, "en-us/en-us"),
        lm=os.path.join(MODELDIR, "en-us/en-us.lm.bin"),
        dict=os.path.join(MODELDIR, "en-us/cmudict-en-us.dict"),
        samprate=float(ep.sample_rate),
    )
    soxcmd = f"sox -q -r {ep.sample_rate} -c 1 -b 16 -e signed-integer -d -t raw -"
    sox = subprocess.Popen(soxcmd.split(), stdout=subprocess.PIPE)
    while True:
        frame = sox.stdout.read(ep.frame_bytes)
        prev_in_speech = ep.in_speech
        speech = ep.process(frame)
        if speech is not None:
            if not prev_in_speech:
                print("Speech start at %.2f" % (ep.start_time), file=sys.stderr)
                decoder.start_utt()
            decoder.process_raw(speech)
            hyp = decoder.hyp()
            if hyp is not None:
                print("PARTIAL RESULT:", hyp.hypstr, file=sys.stderr)
            if not ep.in_speech:
                print("Speech end at %.2f" % (ep.end_time), file=sys.stderr)
                decoder.end_utt()
                print(decoder.hyp().hypstr)


try:
    main()
except KeyboardInterrupt:
    pass
