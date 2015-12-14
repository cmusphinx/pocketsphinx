#!/bin/sh

swig \
    -I`pkg-config --variable=datadir sphinxbase`/swig \
    -ruby \
    -o pocketsphinx/ps_wrap.c \
    `pkg-config --variable=datadir pocketsphinx`/swig/pocketsphinx.i


swig \
    -ruby \
    -o sphinxbase/sb_wrap.c \
    `pkg-config --variable=datadir sphinxbase`/swig/sphinxbase.i

cd pocketsphinx && ruby extconf.rb && cd ..
cd sphinxbase && ruby extconf.rb && cd ..
make -C pocketsphinx RUBYARCHDIR=`pwd` install
make -C sphinxbase RUBYARCHDIR=`pwd` install

ruby -I. test.rb

