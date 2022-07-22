#!/usr/bin/python

import os
import unittest
from pocketsphinx5 import Decoder

MODELDIR = os.path.join(os.path.dirname(__file__),
                        "../../model")
DATADIR = os.path.join(os.path.dirname(__file__),
                       "../../test/data")


class PhonemeTest(unittest.TestCase):
  def test_phoneme(self):
    # Create a decoder with certain model
    config = Decoder.default_config()
    config.set_string('-hmm', os.path.join(MODELDIR, 'en-us/en-us'))
    config.set_string('-allphone', os.path.join(MODELDIR, 'en-us/en-us-phone.lm.bin'))
    config.set_float('-lw', 2.0)
    config.set_float('-pip', 0.3)
    config.set_float('-beam', 1e-200)
    config.set_float('-pbeam', 1e-20)
    config.set_boolean('-mmap', False)

    # Decode streaming data.
    with open(os.path.join(DATADIR, 'goforward.raw'), 'rb') as stream:
      decoder = Decoder(config)
      decoder.start_utt()
      while True:
        buf = stream.read(1024)
        if buf:
          decoder.process_raw(buf, False, False)
        else:
          break
      decoder.end_utt()

      hypothesis = decoder.hyp()
      print ('Best phonemes: ', [seg.word for seg in decoder.seg()])


if __name__ == "__main__":
  unittest.main()

