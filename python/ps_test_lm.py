#!/usr/bin/env python
# -*- py-indent-offset: 2; indent-tabs-mode: nil; coding: utf-8 -*-

import sys
import sphinxbase as sb
import pocketsphinx as ps

DEBUG = False

RAW_FILE = "../test/data/goforward.raw"

LM1_PATH = "../model/lm/en_US/wsj0vp.5000.DMP"
LM1_NAME = "lm1"
LM1 = sb.NGramModel(LM1_PATH)

LM2_PATH = "../model/lm/en_US/hub4.5000.DMP"
LM2_NAME = "lm2"
LM2 = sb.NGramModel(LM2_PATH)

def debugMsg(msg):
  sys.stdout.write("%s\n" % msg)

def addLM(decoder, lm, name):
  if DEBUG: debugMsg("ADDING LANGUAGE MODEL '%s'" % name)
  lmSet = decoder.get_lmset()
  lmSet = lmSet.set_add(lm, name, 1.0, 1)
  if lmSet is None:
    debugMsg("ERROR ADDING LANGUAGE MODEL '%s'" % name)
  else:
    lmSet = decoder.update_lmset(lmSet)
    if DEBUG: debugMsg("FINISHED ADDING LANGUAGE MODEL '%s'" % name)
  return lmSet

def decodeAudio(decoder, uttID, audioFile, lm):
  if DEBUG: 
    debugMsg("ABOUT TO DECODE FILE '%s' as '%s' USING LM '%s'" % (audioFile,
                                                                  uttID,
                                                                  lm))
  lmSet = decoder.get_lmset()
  if DEBUG: debugMsg("SWITCHING TO %s LM" % lm)
  lmSet = lmSet.set_select(lm)
  if lmSet is None:
    debugMsg("LM SWITCH FAILED")
    return
  else:
    lmSet = decoder.update_lmset(lmSet)
    if lmSet is None:
      debugMsg("UPDATE LMSET FAILED")
      return

  if DEBUG: debugMsg("STARTING TO DECODE")
  decoder.start_utt(uttID)

  fh = open(audioFile, "r")
  decoder.decode_raw(fh, uttID)
  decoder.ps_end_utt()
  fh.close()
  if DEBUG: debugMsg("DECODING FINISHED")

  (txt, uttid, score) = decoder.get_hyp()
  prob = decoder.get_prob()
  print "txt(%s) uttid(%s) score(%d) prob(%f)" % (txt, uttid, score, prob)

def main():

  if DEBUG: 
    debugMsg("SPEECH RECOGNIZER INITIALIZING")

  d = ps.Decoder()

  if DEBUG: 
    debugMsg("SPEECH RECOGNIZER FINISHED INITIALIZATION")

  addLM(d, LM1, LM1_NAME)
  addLM(d, LM2, LM2_NAME)
  
  decodeAudio(d, "utt01", RAW_FILE, LM1_NAME)
  decodeAudio(d, "utt02", RAW_FILE, LM2_NAME)
  decodeAudio(d, "utt03", RAW_FILE, LM1_NAME)
  decodeAudio(d, "utt04", RAW_FILE, LM2_NAME)


if __name__ == "__main__":
  main()
