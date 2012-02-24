#!/usr/bin/env python
# -*- py-indent-offset: 2; indent-tabs-mode: nil; coding: utf-8 -*-


import pocketsphinx as ps

decoder = ps.Decoder(lm="../model/lm/en_US/hub4.5000.DMP")
nsamps  = decoder.decode_raw(file("../test/data/goforward.raw", "rb"))

(segments, score) = decoder.segments()

for seg in segments:
    word   = seg.word()
    sf, ef = seg.frames()
    print "%d %s %d" % (sf, word, ef)
