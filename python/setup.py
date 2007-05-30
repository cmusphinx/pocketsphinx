from distutils.core import setup, Extension

module = Extension('_pocketsphinx',
                   include_dirs = ['../../../sphinxbase/include',
                                   '../include',
                                   '/usr/local/include/sphinxbase/',
                                   '/usr/local/include/pocketsphinx',
                                   ],
                   libraries = ['sphinxutil', 'sphinxfe', 'sphinxfeat',
                                'sphinxad', 'pocketsphinx'],
                   sources = ['_pocketsphinxmodule.c'])

setup(name = 'PocketSphinx',
      version = '0.1',
      description = 'Python interface to PocketSphinx speech recognition',
      ext_modules = [module])
