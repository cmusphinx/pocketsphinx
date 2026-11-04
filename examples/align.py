#!/usr/bin/env python3
"""
Force-align a WAV file to a given text, at word, phone and state level.

This is the Python counterpart of the command-line
``pocketsphinx -state_align yes align`` mode, which is otherwise
not covered by any example.
"""

# MIT license (c) 2026, see LICENSE for more information.

import argparse
import wave

from pocketsphinx import Decoder

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("audio", help="Audio file to align")
parser.add_argument("text", help="Text to align to, as whitespace-separated words")
args = parser.parse_args()

with wave.open(args.audio, "rb") as audio:
    data = audio.readframes(audio.getnframes())
    decoder = Decoder(lm=None, samprate=audio.getframerate())

# First pass: word-level alignment.  The text has to be normalized
# (no punctuation, lower case) by the caller.
decoder.set_align_text(args.text)
decoder.start_utt()
decoder.process_raw(data, full_utt=True)
decoder.end_utt()

print("Word alignment:")
for seg in decoder.seg():
    if seg.word in ("<s>", "</s>", "<sil>", "(NULL)"):
        continue
    print("  %-20s %6d %6d" % (seg.word, seg.start_frame, seg.end_frame))

# Second pass: sub-word alignment.  Phone and state durations are
# only available after re-running the decode in alignment mode.
decoder.set_alignment()
decoder.start_utt()
decoder.process_raw(data, full_utt=True)
decoder.end_utt()

print("Phone alignment:")
for word in decoder.get_alignment():
    for phone in word:
        print("  %-20s %-20s %6d %6d" % (word.name, phone.name, phone.start, phone.duration))
