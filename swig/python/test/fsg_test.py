#!/usr/bin/python

import sys
import sphinxbase as sb

lmath = sb.LogMath()
fsg = sb.FsgModel("simple_grammar", lmath, 1.0, 10)
fsg.write(sys.stdout)
