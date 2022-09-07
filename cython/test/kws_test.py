#!/usr/bin/python

import unittest
import sys, os
from pocketsphinx import Decoder

DATADIR = os.path.join(os.path.dirname(__file__), "../../test/data")


class TestKWS(unittest.TestCase):
    def test_kws(self):
        # Open file to read the data
        stream = open(os.path.join(DATADIR, "goforward.raw"), "rb")

        # Process audio chunk by chunk. On keyphrase detected perform action and restart search
        decoder = Decoder(keyphrase="forward",
                          kws_threshold=1e20)
        decoder.start_utt()
        while True:
            buf = stream.read(1024)
            if buf:
                decoder.process_raw(buf, False, False)
            else:
                break
            if decoder.hyp() != None:
                print(
                    [
                        (seg.word, seg.prob, seg.start_frame, seg.end_frame)
                        for seg in decoder.seg()
                    ]
                )
                print("Detected keyphrase, restarting search")
                for seg in decoder.seg():
                    self.assertTrue(seg.end_frame > seg.start_frame)
                    self.assertEqual(seg.word, "forward")
                decoder.end_utt()
                decoder.start_utt()
        stream.close()


if __name__ == "__main__":
    unittest.main()
