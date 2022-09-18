#!/usr/bin/python

import os
from pocketsphinx import Decoder
import unittest

DATADIR = os.path.join(os.path.dirname(__file__), "../../test/data")


class TestDecoder(unittest.TestCase):
    def _run_decode(self, decoder, expect_fail=False):
        with open(os.path.join(DATADIR, "goforward.raw"), "rb") as fh:
            buf = fh.read()
            decoder.start_utt()
            decoder.process_raw(buf, no_search=False, full_utt=True)
            decoder.end_utt()
            self._check_hyp(decoder.hyp().hypstr, decoder.seg(), expect_fail)

    def _check_hyp(self, hyp, hypseg, expect_fail=False):
        if expect_fail:
            self.assertNotEqual(hyp, "go forward ten meters")
        else:
            self.assertEqual(hyp, "go forward ten meters")
        words = []
        try:
            for seg in hypseg:
                if seg.word not in ("<s>", "</s>", "<sil>", "(NULL)"):
                    words.append(seg.word)
        except AttributeError:
            for word, start, end in hypseg:
                if word not in ("<s>", "</s>", "<sil>", "(NULL)"):
                    words.append(word)
        if expect_fail:
            self.assertNotEqual(words, "go forward ten meters".split())
        else:
            self.assertEqual(words, "go forward ten meters".split())

    def test_decoder(self):
        decoder = Decoder()
        print("log(1e-150) = ", decoder.logmath.log(1e-150))
        print("Pronunciation for word 'hello' is ", decoder.lookup_word("hello"))
        self.assertEqual("HH AH L OW", decoder.lookup_word("hello"))
        print("Pronunciation for word 'abcdf' is ", decoder.lookup_word("abcdf"))
        self.assertEqual(None, decoder.lookup_word("abcdf"))

        self._run_decode(decoder)

        # Access N best decodings.
        print("Best 10 hypothesis: ")
        for best, i in zip(decoder.nbest(), range(10)):
            print(best.hypstr, best.score)

        with open(os.path.join(DATADIR, "goforward.mfc"), "rb") as stream:
            stream.read(4)
            buf = stream.read(13780)
            decoder.start_utt()
            decoder.process_cep(buf, False, True)
            decoder.end_utt()
            hypothesis = decoder.hyp()
            print(
                "Best hypothesis: ",
                hypothesis.hypstr,
                " model score: ",
                hypothesis.best_score,
                " confidence: ",
                hypothesis.prob,
            )
            self.assertEqual("go forward ten meters", decoder.hyp().hypstr)

    def test_reinit(self):
        decoder = Decoder()
        decoder.add_word("_forward", "F AO R W ER D", True)
        self._run_decode(decoder)
        # should preserve dict words, but make decoding fail
        decoder.config["samprate"] = 48000
        decoder.reinit_feat()
        self.assertEqual("F AO R W ER D", decoder.lookup_word("_forward"))
        self._run_decode(decoder, expect_fail=True)
        decoder.config["samprate"] = 16000
        # should erase dict words
        decoder.reinit()
        self.assertEqual(None, decoder.lookup_word("_forward"))
        self._run_decode(decoder)


if __name__ == "__main__":
    unittest.main()
