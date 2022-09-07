PocketSphinx Documentation
============================

Welcome to the documentation for the Python interface to the
PocketSphinx speech recognizer!

Quick Start
-----------

To install PocketSphinx on most recent versions of Python, you should
be able to simply use `pip`::

  pip install pocketsphinx

This is a (somewhat) "batteries-included" install, which comes with a
default model and dictionary.  Sadly, this model is specifically for
US (and, by extension Canadian) English, so it may not work well for
your dialect and certainly won't work for your native language.

On Unix-like platforms you may need to install `PortAudio
<https://portaudio.com>`_ for live audio input to work.  Now you can
try the simplest possible speech recognizer::

  from pocketsphinx import LiveSpeech
  for phrase in LiveSpeech():
      print(phrase)

This will open the default audio device and start listening, detecting
segments of speech and printing out the recognized text, which may or
may not resemble what you actually said.

There are of course many other things you can do with it.  See
the :ref:`apidoc` for more information.

.. _apidoc:

API Documentation
-----------------

.. toctree::
   pocketsphinx
   config_params


Indices and tables
==================

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`
