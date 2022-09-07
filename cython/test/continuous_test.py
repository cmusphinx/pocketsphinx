#!/usr/bin/python

import os
from pocketsphinx import Decoder
import unittest

DATADIR = os.path.join(os.path.dirname(__file__), "../../test/data")


class TestContinuous(unittest.TestCase):
    def test_continuous(self):
        prev_cmn = (
            "41,-5.29,-0.12,5.09,2.48,-4.07,-1.37,-1.78,-5.08,-2.05,-6.45,-1.42,1.17"
        )
        decoder = Decoder(cmninit=prev_cmn)
        self.assertEqual(prev_cmn, decoder.get_cmn(False))

        with open(os.path.join(DATADIR, "goforward.raw"), "rb") as stream:
            decoder.start_utt()
            while True:
                buf = stream.read(1024)
                if buf:
                    decoder.process_raw(buf, False, False)
                    cmn = decoder.get_cmn(True)
                    self.assertNotEqual(prev_cmn, cmn)
                    prev_cmn = cmn
                else:
                    break
            decoder.end_utt()
            print("Result:", decoder.hyp().hypstr)
            self.assertEqual("go forward ten meters", decoder.hyp().hypstr)
            cmn = decoder.get_cmn(False)
            self.assertNotEqual(prev_cmn, cmn)
            prev_cmn = cmn


if __name__ == "__main__":
    unittest.main()
