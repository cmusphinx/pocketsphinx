#!/usr/bin/python

import os
from pocketsphinx5 import Decoder
import unittest

MODELDIR = os.path.join(os.path.dirname(__file__),
                        "../../../model")
DATADIR = os.path.join(os.path.dirname(__file__),
                       "../../../test/data")

class TestDecoder(unittest.TestCase):
    def test_decoder(self):
      # Create a decoder with certain model
      config = Decoder.default_config()
      config.set_string('-hmm', os.path.join(MODELDIR, 'en-us/en-us'))
      config.set_string('-lm', os.path.join(MODELDIR, 'en-us/en-us.lm.bin'))
      config.set_string('-dict', os.path.join(MODELDIR, 'en-us/cmudict-en-us.dict'))

      # Decode streaming data.
      decoder = Decoder(config)

      print ("Pronunciation for word 'hello' is ", decoder.lookup_word("hello"))
      self.assertEqual("HH AH L OW", decoder.lookup_word("hello"))
      print ("Pronunciation for word 'abcdf' is ", decoder.lookup_word("abcdf"))
      self.assertEqual(None, decoder.lookup_word("abcdf"))

      decoder.start_utt()
      stream = open(os.path.join(DATADIR, 'goforward.raw'), 'rb')
      while True:
        buf = stream.read(1024)
        if buf:
          decoder.process_raw(buf, False, False)
        else:
          break
      decoder.end_utt()

      hypothesis = decoder.hyp()
      logmath = decoder.get_logmath()
      print ('Best hypothesis: ', hypothesis.hypstr, " model score: ", hypothesis.best_score, " confidence: ", logmath.exp(hypothesis.prob))

      print ('Best hypothesis segments: ', [seg.word for seg in decoder.seg()])

      # Access N best decodings.
      print ('Best 10 hypothesis: ')
      for best, i in zip(decoder.nbest(), range(10)):
          print (best.hypstr, best.score)

      stream = open(os.path.join(DATADIR, 'goforward.mfc'), 'rb')
      stream.read(4)
      buf = stream.read(13780)
      decoder.start_utt()
      decoder.process_cep(buf, False, True)
      decoder.end_utt()
      hypothesis = decoder.hyp()
      print ('Best hypothesis: ', hypothesis.hypstr, " model score: ", hypothesis.best_score, " confidence: ", hypothesis.prob)
      self.assertEqual("go forward ten meters", decoder.hyp().hypstr)


if __name__ == "__main__":
    unittest.main()
