#!/usr/bin/python

from os import environ, path

from pocketsphinx.pocketsphinx import *
from sphinxbase.sphinxbase import *

MODELDIR = "../../../model"
DATADIR = "../../../test/data"

# Create a decoder with certain model
config = Decoder.default_config()
config.set_string('-hmm', path.join(MODELDIR, 'en-us/en-us'))
config.set_string('-allphone', path.join(MODELDIR, 'en-us/en-us-phone.lm.bin'))
config.set_float('-lw', 2.0)
config.set_float('-pip', 0.3)
config.set_float('-beam', 1e-200)
config.set_float('-pbeam', 1e-20)
config.set_boolean('-mmap', False)

# Decode streaming data.
decoder = Decoder(config)

decoder.start_utt()
stream = open(path.join(DATADIR, 'goforward.raw'), 'rb')
while True:
  buf = stream.read(1024)
  if buf:
    decoder.process_raw(buf, False, False)
  else:
    break
decoder.end_utt()

hypothesis = decoder.hyp()
print ('Best phonemes: ', [seg.word for seg in decoder.seg()])
