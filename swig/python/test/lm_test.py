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
config.set_string('-dict', path.join(DATADIR, 'defective.dic'))
config.set_boolean('-mmap', False)
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
print ('Decoding with default settings:', decoder.hyp().hypstr)

# Load "turtle" language model and decode again.
lm = NGramModel(config, decoder.get_logmath(), path.join(DATADIR, 'turtle.lm.bin'))
print (lm.prob(['you']))
print (lm.prob(['are','you']))
print (lm.prob(['you', 'are', 'what']))
print (lm.prob(['lost', 'are', 'you']))

decoder.set_lm('turtle', lm)
decoder.set_search('turtle')
decoder.start_utt()
stream = open(path.join(DATADIR, 'goforward.raw'), 'rb')
while True:
    buf = stream.read(1024)
    if buf:
         decoder.process_raw(buf, False, False)
    else:
         break
decoder.end_utt()

print ('Decoding with "turtle" language:', decoder.hyp().hypstr)

## The word 'meters' isn't in the loaded dictionary.
## Let's add it manually.
decoder.add_word('foobie', 'F UW B IY', False)
decoder.add_word('meters', 'M IY T ER Z', True)
decoder.start_utt()
stream = open(path.join(DATADIR, 'goforward.raw'), 'rb')
while True:
    buf = stream.read(1024)
    if buf:
         decoder.process_raw(buf, False, False)
    else:
         break
decoder.end_utt()
print ('Decoding with customized language:', decoder.hyp().hypstr)
