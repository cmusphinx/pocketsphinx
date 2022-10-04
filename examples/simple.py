#!/usr/bin/env python3
"""
Recognize a single utterance from a WAV file.

Supporting other file types is left as an exercise to the reader.
"""

# MIT license (c) 2022, see LICENSE for more information.
# Author: David Huggins-Daines <dhdaines@gmail.com>

from pocketsphinx import Decoder
import argparse
import wave

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("audio", help="Audio file to recognize")
args = parser.parse_args()
with wave.open(args.audio, "rb") as audio:
    decoder = Decoder(samprate=audio.getframerate())
    decoder.start_utt()
    decoder.process_raw(audio.getfp().read(), full_utt=True)
    decoder.end_utt()
    print(decoder.hyp().hypstr)
