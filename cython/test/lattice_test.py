#!/usr/bin/python

import unittest
import os
from pocketsphinx5 import Decoder

MODELDIR = os.path.join(os.path.dirname(__file__),
                        "../../../model")
DATADIR = os.path.join(os.path.dirname(__file__),
                       "../../../test/data")

class LatticeTest(unittest.TestCase):
    def test_lattice(self):
        # Create a decoder with certain model
        config = Decoder.default_config()
        config.set_string('-hmm', os.path.join(MODELDIR, 'en-us/en-us'))
        config.set_string('-lm', os.path.join(MODELDIR, 'en-us/en-us.lm.bin'))
        config.set_string('-dict', os.path.join(MODELDIR, 'en-us/cmudict-en-us.dict'))
        decoder = Decoder(config)

        decoder.start_utt()
        stream = open(os.path.join(DATADIR, 'goforward.raw'), 'rb')
        while True:
            buf = stream.read(1024)
            if buf:
                 decoder.process_raw(buf, False, False)
            else:
                 break
        stream.close()
        decoder.end_utt()

        decoder.get_lattice().write('goforward.lat')
        decoder.get_lattice().write_htk('goforward.htk')
        os.unlink("goforward.lat")
        os.unlink("goforward.htk")


if __name__ == "__main__":
    unittest.main()
