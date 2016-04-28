#!/usr/bin/python

from os import environ, path

from pocketsphinx.pocketsphinx import *
from sphinxbase.sphinxbase import *

#some dumb test for checking during developent

MODELDIR = "../../../model"

config = Decoder.default_config()

intval = 256
floatval = 8000.0
stringval = "~/pocketsphinx"
boolval = True

# Check values that was previously set.
s = config.get_float("-samprate")
print ("Float: ",floatval ," ", s)
config.set_float("-samprate", floatval)
s = config.get_float("-samprate")
print ("Float: ",floatval ," ", s)

s = config.get_int("-nfft")
print ("Int:",intval, " ", s)
config.set_int("-nfft", intval)
s = config.get_int("-nfft")
print ("Int:",intval, " ", s)

s = config.get_string("-rawlogdir")
print ("String:",stringval, " ", s)
config.set_string("-rawlogdir", stringval)
s = config.get_string("-rawlogdir")
print ("String:",stringval, " ", s)

s = config.get_boolean("-backtrace")
print ("Boolean:", boolval, " ", s)
config.set_boolean("-backtrace", boolval);
s = config.get_boolean("-backtrace")
print ("Boolean:", boolval, " ", s)

config.set_string_extra("-something12321", "abc")
print config.get_string("-something12321")

