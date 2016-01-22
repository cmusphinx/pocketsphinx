#!/usr/bin/python

import sys

from sphinxbase.sphinxbase import LogMath
from sphinxbase.sphinxbase import FsgModel

lmath = LogMath()
fsg = FsgModel("simple_grammar", lmath, 1.0, 10)
fsg.word_add("hello")
fsg.word_add("world")
print (fsg.word_id("world"))

fsg.add_silence("<sil>", 1, 0.5)

