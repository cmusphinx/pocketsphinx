try:
    from setuptools import setup, Extension
except:
    from distutils.core import setup, Extension

import os

sb_includes = ['../../sphinxbase/include', '../../sphinxbase/win32/include']
sb_libdirs = ['../../sphinxbase/lib/debug']

setup(name = 'PocketSphinx',
      version = '0.6',
      author = 'David Huggins-Daines',
      author_email = 'dhuggins@cs.cmu.edu',
      description = 'Python interface to CMU PocketSphinx speech recognition',
      license = 'BSD',
      url = 'http://cmusphinx.sourceforge.net',
      ext_modules = [
        Extension('pocketsphinx',
                   sources=['pocketsphinx.c'],
                   libraries=['pocketsphinx', 'sphinxbase'],
                   include_dirs=['../include',
                                 '../python'] + sb_includes,
                   library_dirs=['../bin/debug'] + sb_libdirs)
        ],
     ) 
