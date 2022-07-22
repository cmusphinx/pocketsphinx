#!/usr/bin/python

import os
from pocketsphinx5 import Decoder
import unittest

MODELDIR = os.path.join(os.path.dirname(__file__), "../../model")
DATADIR = os.path.join(os.path.dirname(__file__), "../../test/data")


class TestContinuous(unittest.TestCase):
    def test_continuous(self):
        config = Decoder.default_config()
        config.set_string("-hmm", os.path.join(MODELDIR, "en-us/en-us"))
        config.set_string("-lm", os.path.join(MODELDIR, "en-us/en-us.lm.bin"))
        config.set_string("-dict", os.path.join(MODELDIR, "en-us/cmudict-en-us.dict"))
        config.set_string(
            "-cmninit",
            "41.00,-5.29,-0.12,5.09,2.48,-4.07,-1.37,-1.78,-5.08,-2.05,-6.45,-1.42,1.17",
        )
        decoder = Decoder(config)

        with open(os.path.join(DATADIR, "goforward.raw"), "rb") as stream:
            in_speech_bf = False
            decoder.start_utt()
            while True:
                buf = stream.read(1024)
                if buf:
                    decoder.process_raw(buf, False, False)
                    if decoder.get_in_speech() != in_speech_bf:
                        in_speech_bf = decoder.get_in_speech()
                        if not in_speech_bf:
                            decoder.end_utt()
                            print('Result:', decoder.hyp().hypstr)
                            decoder.start_utt()
                else:
                    break
            decoder.end_utt()
            print("Result:", decoder.hyp().hypstr)
            self.assertEqual("go forward ten meters", decoder.hyp().hypstr)


if __name__ == "__main__":
    unittest.main()
