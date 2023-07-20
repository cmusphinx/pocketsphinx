#!/usr/bin/python

import os
from pocketsphinx import Decoder
import unittest
import wave

DATADIR = os.path.join(os.path.dirname(__file__), "../../test/data")


class TestAlignment(unittest.TestCase):
    def _run_decode(self, decoder):
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

    def test_default_lm(self):
        decoder = Decoder()
        self.assertEqual(decoder.current_search(), "_default")
        decoder.set_align_text("go forward then meters")
        self.assertEqual(decoder.current_search(), "_align")
        self._run_decode(decoder)
        self.assertEqual(decoder.hyp().hypstr, "go forward then meters")
        decoder.activate_search()
        self.assertEqual(decoder.current_search(), "_default")
        self._run_decode(decoder)
        self.assertEqual(decoder.hyp().hypstr, "go forward ten meters")

    def _run_phone_align(self, decoder, buf):
        decoder.start_utt()
        decoder.process_raw(buf, no_search=False, full_utt=True)
        decoder.end_utt()
        decoder.set_alignment()
        decoder.start_utt()
        decoder.process_raw(buf, no_search=False, full_utt=True)
        decoder.end_utt()

    def test_align_forever(self):
        decoder = Decoder(loglevel="INFO", backtrace=True, lm=None)
        decoder.set_align_text("feels like these days go on forever")
        with wave.open(
            os.path.join(DATADIR, "forever", "input_2_16k.wav"), "r"
        ) as infh:
            data = infh.readframes(infh.getnframes())
            self._run_phone_align(decoder, data)
            alignment = decoder.get_alignment()
            phones = [entry.name for entry in alignment.phones()]
            self.assertEqual(
                phones,
                [
                    "F",
                    "IY",
                    "L",
                    "Z",
                    "L",
                    "AY",
                    "K",
                    "DH",
                    "IY",
                    "Z",
                    "D",
                    "EY",
                    "Z",
                    "G",
                    "OW",
                    "AO",
                    "N",
                    "F",
                    "ER",
                    "EH",
                    "V",
                    "ER",
                    "SIL",
                ],
            )


if __name__ == "__main__":
    unittest.main()
