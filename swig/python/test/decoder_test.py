#!/usr/bin/python

from os import environ, path


from pocketsphinx.pocketsphinx import *
from sphinxbase.sphinxbase import *

MODELDIR = "../../../model"
DATADIR = "../../../test/data"

# Create a decoder with certain model
config = Decoder.default_config()
config.set_string('-hmm', path.join(MODELDIR, 'en-us/en-us'))
config.set_string('-lm', path.join(MODELDIR, 'en-us/en-us.lm.bin'))
config.set_string('-dict', path.join(MODELDIR, 'en-us/cmudict-en-us.dict'))

# Decode streaming data.
decoder = Decoder(config)

print ("Pronunciation for word 'hello' is ", decoder.lookup_word("hello"))
print ("Pronunciation for word 'abcdf' is ", decoder.lookup_word("abcdf"))

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
logmath = decoder.get_logmath()
print ('Best hypothesis: ', hypothesis.hypstr, " model score: ", hypothesis.best_score, " confidence: ", logmath.exp(hypothesis.prob))

print ('Best hypothesis segments: ', [seg.word for seg in decoder.seg()])

# Access N best decodings.
print ('Best 10 hypothesis: ')
for best, i in zip(decoder.nbest(), range(10)):
    print (best.hypstr, best.score)

stream = open(path.join(DATADIR, 'goforward.mfc'), 'rb')
stream.read(4)
buf = stream.read(13780)
decoder.start_utt()
decoder.process_cep(buf, False, True)
decoder.end_utt()
hypothesis = decoder.hyp()
print ('Best hypothesis: ', hypothesis.hypstr, " model score: ", hypothesis.best_score, " confidence: ", hypothesis.prob)
