#!/usr/bin/env python3
"""
Detect speech endlessly from the default audio device.
"""

from pocketsphinx5 import Vad
import subprocess

VAD_FRAMES = 10  # Number of frames used to make speech/non-speech decision

vader = Vad()
soxcmd = f"sox -q -r {vader.sample_rate} -c 1 -b 16 -e signed-integer -d -t raw -"
with subprocess.Popen(soxcmd.split(), stdout=subprocess.PIPE) as sox:
    prev_is_speech = False
    try:
        while True:
            frame = sox.stdout.read(vader.frame_bytes)
            if len(frame) == 0:
                break
            is_speech = vader.is_speech(frame)
            if is_speech != prev_is_speech:
                print("speech" if is_speech else "not speech")
            prev_is_speech = is_speech
    except KeyboardInterrupt:
        pass
