#!/usr/bin/env python3
"""
Detect speech endlessly from the default audio device.
"""

from pocketsphinx5 import Vad
from collections import deque
import subprocess

VAD_FRAMES = 10  # Number of frames used to make speech/non-speech decision

vader = Vad()
soxcmd = f"sox -q -r {vader.sample_rate} -c 1 -b 16 -e signed-integer -d -t raw -"
with subprocess.Popen(soxcmd.split(), stdout=subprocess.PIPE) as sox:
    buf = deque(maxlen=VAD_FRAMES)
    in_speech = False
    timestamp = 0.0
    try:
        while True:
            frame = sox.stdout.read(vader.frame_bytes)
            if len(frame) == 0:
                break
            is_speech = vader.is_speech(frame)
            buf.append((frame, is_speech, timestamp))
            timestamp += vader.frame_length
            n_is_speech = sum(is_speech for _, is_speech, _ in buf)
            if in_speech:
                if n_is_speech == 0:
                    print("speech end at %.3f" % buf[-1][2])
                    in_speech = False
            else:
                if n_is_speech == buf.maxlen:
                    print("speech start at %.3f" % buf[0][2])
                    in_speech = True
    except KeyboardInterrupt:
        pass
