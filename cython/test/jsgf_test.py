#!/usr/bin/python

import unittest
import os
from pocketsphinx5 import Decoder, Jsgf

MODELDIR = os.path.join(os.path.dirname(__file__),
                        "../../model")
DATADIR = os.path.join(os.path.dirname(__file__),
                       "../../test/data")

class TestJsgf(unittest.TestCase):
    def test_jsgf(self):
        # Create a decoder with certain model
        config = Decoder.default_config()
        config.set_string('-hmm', os.path.join(MODELDIR, 'en-us/en-us'))
        config.set_string('-lm', os.path.join(DATADIR, 'turtle.lm.bin'))
        config.set_string('-dict', os.path.join(DATADIR, 'turtle.dic'))
        decoder = Decoder(config)

        # Decode with lm
        decoder.start_utt()
        with open(os.path.join(DATADIR, 'goforward.raw'), 'rb') as stream:
            while True:
                buf = stream.read(1024)
                if buf:
                     decoder.process_raw(buf, False, False)
                else:
                     break
        decoder.end_utt()
        print ('Decoding with "turtle" language:', decoder.hyp().hypstr)
        self.assertEqual("go forward ten meters", decoder.hyp().hypstr)

        # Switch to JSGF grammar
        jsgf = Jsgf(os.path.join(DATADIR, 'goforward.gram'))
        rule = jsgf.get_rule('goforward.move2')
        fsg = jsgf.build_fsg(rule, decoder.get_logmath(), 7.5)
        fsg.writefile('goforward.fsg')

        decoder.set_fsg("goforward", fsg)
        decoder.set_search("goforward")

        decoder.start_utt()
        with open(os.path.join(DATADIR, 'goforward.raw'), 'rb') as stream:
            while True:
                buf = stream.read(1024)
                if buf:
                     decoder.process_raw(buf, False, False)
                else:
                     break
        decoder.end_utt()
        print ('Decoding with "goforward" grammar:', decoder.hyp().hypstr)
        self.assertEqual("go forward ten meters", decoder.hyp().hypstr)


if __name__ == "__main__":
    unittest.main()
