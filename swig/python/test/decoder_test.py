# ====================================================================
# Copyright (c) 2013 Carnegie Mellon University.  All rights
# reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer. 
#
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in
#    the documentation and/or other materials provided with the
#    distribution.
#
# This work was supported in part by funding from the Defense Advanced 
# Research Projects Agency and the National Science Foundation of the 
# United States of America, and the CMU Sphinx Speech Consortium.
#
# THIS SOFTWARE IS PROVIDED BY CARNEGIE MELLON UNIVERSITY ``AS IS'' AND 
# ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
# NOR ITS EMPLOYEES BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# ====================================================================


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
