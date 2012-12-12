#!/usr/bin/env python
# -*- py-indent-offset: 2; indent-tabs-mode: nil; coding: utf-8 -*-

import sys
import sphinxbase as sb
import pocketsphinx as ps

RAW_FILE = "../test/data/goforward.raw"

LM1_PATH = "../model/lm/en_US/wsj0vp.5000.DMP"
LM1_NAME = "lm1"
LM1 = sb.NGramModel(LM1_PATH)

LM2_PATH = "../model/lm/en_US/hub4.5000.DMP"
LM2_NAME = "lm2"
LM2 = sb.NGramModel(LM2_PATH)

def addLM(decoder, lm, name):
  lmSet = decoder.get_lmset()
  lmSet.set_add(lm, name, 1.0, 0)
  decoder.update_lmset(lmSet)
  return

def decodeAudio(decoder, uttID, audioFile, lm):
  lmSet = decoder.get_lmset()
  lmSet.set_select(lm)
  decoder.update_lmset(lmSet)

  decoder.start_utt(uttID)

  fh = open(audioFile, "r")
  decoder.decode_raw(fh, uttID)
  decoder.end_utt()
  fh.close()

  (txt, uttid, score) = decoder.get_hyp()
  prob = decoder.get_prob()
  print "txt(%s) uttid(%s) score(%d) prob(%f)" % (txt, uttid, score, prob)

def main():

  d = ps.Decoder()
    
  addLM(d, LM1, LM1_NAME)
  addLM(d, LM2, LM2_NAME)

  decodeAudio(d, "utt01", RAW_FILE, LM1_NAME)
  decodeAudio(d, "utt02", RAW_FILE, LM2_NAME)
  decodeAudio(d, "utt03", RAW_FILE, LM1_NAME)
  decodeAudio(d, "utt04", RAW_FILE, LM2_NAME)


if __name__ == "__main__":
  main()
