#!/usr/bin/python

import unittest
import os
from pocketsphinx import Decoder, NGramModel

DATADIR = os.path.join(os.path.dirname(__file__), "../../test/data")


class TestLM(unittest.TestCase):
    def test_lm(self):
        # Create a decoder with a broken dictionary
        decoder = Decoder(dict=os.path.join(DATADIR, "defective.dic"))

        decoder.start_utt()
        with open(os.path.join(DATADIR, "goforward.raw"), "rb") as stream:
            while True:
                buf = stream.read(1024)
                if buf:
                    decoder.process_raw(buf, False, False)
                else:
                    break
        decoder.end_utt()
        print("Decoding with default settings:", decoder.hyp().hypstr)
        self.assertEqual("", decoder.hyp().hypstr)

        # Load "turtle" language model and decode again.
        lm = NGramModel(
            decoder.config,
            decoder.logmath,
            os.path.join(DATADIR, "turtle.lm.bin"),
        )
        print(lm.prob(["you"]))
        print(lm.prob(["are", "you"]))
        print(lm.prob(["you", "are", "what"]))
        print(lm.prob(["lost", "are", "you"]))

        decoder.add_lm("turtle", lm)
        self.assertNotEqual(decoder.current_search(), "turtle")
        decoder.activate_search("turtle")
        self.assertEqual(decoder.current_search(), "turtle")
        decoder.start_utt()
        with open(os.path.join(DATADIR, "goforward.raw"), "rb") as stream:
            while True:
                buf = stream.read(1024)
                if buf:
                    decoder.process_raw(buf, False, False)
                else:
                    break
        decoder.end_utt()

        print('Decoding with "turtle" language:', decoder.hyp().hypstr)
        self.assertEqual("", decoder.hyp().hypstr)

        # The word 'meters' isn't in the loaded dictionary.
        # Let's add it manually.
        decoder.add_word("foobie", "F UW B IY", False)
        decoder.add_word("meters", "M IY T ER Z", True)
        decoder.start_utt()
        with open(os.path.join(DATADIR, "goforward.raw"), "rb") as stream:
            while True:
                buf = stream.read(1024)
                if buf:
                    decoder.process_raw(buf, False, False)
                else:
                    break
        decoder.end_utt()
        print("Decoding with customized language:", decoder.hyp().hypstr)
        self.assertEqual("foobie meters meters", decoder.hyp().hypstr)


if __name__ == "__main__":
    unittest.main()
