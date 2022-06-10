#!/usr/bin/python

import unittest
import sys, os
from pocketsphinx5 import Decoder

MODELDIR = os.path.join(os.path.dirname(__file__),
                        "../../../model")
DATADIR = os.path.join(os.path.dirname(__file__),
                       "../../../test/data")

class TestKWS(unittest.TestCase):
    def test_kws(self):
        # Create a decoder with certain model
        config = Decoder.default_config()
        config.set_string('-hmm', os.path.join(MODELDIR, 'en-us/en-us'))
        config.set_string('-dict', os.path.join(MODELDIR, 'en-us/cmudict-en-us.dict'))
        config.set_string('-keyphrase', 'forward')
        config.set_float('-kws_threshold', 1e+20)

        # Open file to read the data
        stream = open(os.path.join(DATADIR, "goforward.raw"), "rb")

        # Process audio chunk by chunk. On keyphrase detected perform action and restart search
        decoder = Decoder(config)
        decoder.start_utt()
        while True:
            buf = stream.read(1024)
            if buf:
                 decoder.process_raw(buf, False, False)
            else:
                 break
            if decoder.hyp() != None:
                print ([(seg.word, seg.prob, seg.start_frame, seg.end_frame) for seg in decoder.seg()])
                print ("Detected keyphrase, restarting search")
                for seg in decoder.seg():
                    self.assertTrue(seg.end_frame > seg.start_frame)
                    self.assertEqual(seg.word, "forward")
                decoder.end_utt()
                decoder.start_utt()
        stream.close()


if __name__ == "__main__":
    unittest.main()
