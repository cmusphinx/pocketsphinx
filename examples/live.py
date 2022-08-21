#!/usr/bin/env python3
"""
Recognize live speech from the default audio device.
"""

from pocketsphinx5 import Endpointer, Decoder
import subprocess
import sys
import os

MODELDIR = os.path.join(os.path.dirname(__file__), os.path.pardir, "model")


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
                print("Speech start at %.2f" % (ep.speech_start), file=sys.stderr)
                decoder.start_utt()
            decoder.process_raw(speech)
            hyp = decoder.hyp()
            if hyp is not None:
                print("PARTIAL RESULT:", hyp.hypstr, file=sys.stderr)
            if not ep.in_speech:
                print("Speech end at %.2f" % (ep.speech_end), file=sys.stderr)
                decoder.end_utt()
                print(decoder.hyp().hypstr)


try:
    main()
except KeyboardInterrupt:
    pass
