#!/usr/bin/python

import os
from pocketsphinx import Decoder
import unittest

DATADIR = os.path.join(os.path.dirname(__file__), "../../test/data")


class TestAlignment(unittest.TestCase):
    def _run_decode(self, decoder, expect_fail=False):
        with open(os.path.join(DATADIR, "goforward.raw"), "rb") as fh:
            buf = fh.read()
            decoder.start_utt()
            decoder.process_raw(buf, no_search=False, full_utt=True)
            decoder.end_utt()

    def test_alignment(self):
        decoder = Decoder(lm=None)
        decoder.set_align_text("go forward ten meters")
        self._run_decode(decoder)
        words = []
        for seg in decoder.seg():
            if seg.word not in ("<s>", "</s>", "<sil>", "(NULL)"):
                words.append((seg.word, seg.start_frame, seg.end_frame))
        print(words)
        decoder.set_alignment()
        self._run_decode(decoder)
        for word in decoder.get_alignment():
            print(word.start, word.duration, word.score, word.name)
            for phone in word:
                print("\t", phone.start, phone.duration, phone.score, phone.name)
                for state in phone:
                    print("\t\t", state.start, state.duration, state.score, state.name)


if __name__ == "__main__":
    unittest.main()
