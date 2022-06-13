#!/usr/bin/python

import os
from pocketsphinx5 import Decoder
import unittest

MODELDIR = os.path.join(os.path.dirname(__file__),
                        "../../../model")
DATADIR = os.path.join(os.path.dirname(__file__),
                       "../../../test/data")

class TestContinuous(unittest.TestCase):
    def test_continuous(self):
        config = Decoder.default_config()
        config.set_string('-hmm', os.path.join(MODELDIR, 'en-us/en-us'))
        config.set_string('-lm', os.path.join(MODELDIR, 'en-us/en-us.lm.bin'))
        config.set_string('-dict', os.path.join(MODELDIR, 'en-us/cmudict-en-us.dict'))
        decoder = Decoder(config)

        stream = open(os.path.join(DATADIR, 'goforward.raw'), 'rb')
        #stream = open('10001-90210-01803.wav', 'rb')

        decoder.start_utt()
        while True:
            buf = stream.read(1024)
            if buf:
                decoder.process_raw(buf, False, False)
            else:
                break
        decoder.end_utt()
        print('Result:', decoder.hyp().hypstr)
        self.assertEqual("go forward ten meters", decoder.hyp().hypstr)


if __name__ == "__main__":
    unittest.main()
