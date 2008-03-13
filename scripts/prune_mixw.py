#!/usr/bin/env python

# ====================================================================
# Copyright (c) 1999-2001 Carnegie Mellon University.  All rights
# reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer. 
#
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in
#    the documentation and/or other materials provided with the
#    distribution.
#
# This work was supported in part by funding from the Defense Advanced 
# Research Projects Agency and the National Science Foundation of the 
# United States of America, and the CMU Sphinx Speech Consortium.
#
# THIS SOFTWARE IS PROVIDED BY CARNEGIE MELLON UNIVERSITY ``AS IS'' AND 
# ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
# NOR ITS EMPLOYEES BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# ====================================================================

"""
Functions for pruning and roughening mixture weight distributions for
increased performance.  Paper submitted to ACL2008.
"""
__author__ = "David Huggins-Daines <dhuggins@cs.cmu.edu>"

import sys
import sphinx.s3mixw
import numpy

def perplexity(dist):
    return numpy.exp(-(dist * numpy.log(dist)).sum())

def prune_mixw_entropy(mixw, avgn):
    # Calculate average entropy
    avgp = 0
    count = 0
    for m in mixw:
        for f in m:
            pplx = perplexity(f)
            avgp += pplx
            count += 1
    avgp /= count
    scale = float(avgn) / avgp
    avgtop = 0
    mintop = 999
    maxtop = 0
    histo = numpy.zeros(len(mixw[0,0]),'i')
    for m in mixw:
        for f in m:
            pplx = perplexity(f)
            top = round(pplx * scale)
            if top < len(f): histo[top] += 1
            avgtop += top
            if top < mintop: mintop = top
            if top > maxtop: maxtop = top
            f.put(f.argsort()[:-top], 0)
    print "Average #mixw: %.2f" % (float(avgtop) / count)
    print "Min #mixw: %d Max #mixw: %d" % (mintop, maxtop)
    return histo

def prune_mixw_entropy_min(mixw, avgn, minn):
    # Calculate average entropy
    avgp = 0
    count = 0
    for m in mixw:
        for f in m:
            pplx = perplexity(f)
            avgp += pplx
            count += 1
    avgp /= count
    scale = float(avgn) / avgp
    avgtop = 0
    mintop = 999
    maxtop = 0
    histo = numpy.zeros(len(mixw[0,0]),'i')
    for m in mixw:
        for f in m:
            pplx = perplexity(f)
            top = round(pplx * scale)
            if top < minn: top = minn
            elif top >= len(f): top = len(f)-1
            else: histo[top] += 1
            avgtop += top
            if top < mintop: mintop = top
            if top > maxtop: maxtop = top
            f.put(f.argsort()[:-top], 0)
    print "Average #mixw: %.2f" % (float(avgtop) / count)
    print "Min #mixw: %d Max #mixw: %d" % (mintop, maxtop)
    return histo

def prune_mixw_pplx_hist(mixw):
    # Calculate perplexity histogram
    histo = numpy.zeros(len(mixw[0,0]),'i')
    for m in mixw:
        for f in m:
            pplx = perplexity(f)
            histo[round(pplx)] += 1
    # Floor number of mixture weights at the mode of perplexity
    minn = histo.argmax()
    avgtop = 0
    mintop = 999
    maxtop = 0
    for m in mixw:
        for f in m:
            top = round(perplexity(f))
            avgtop += top
            if top < minn: top = minn
            if top < mintop: mintop = top
            if top > maxtop: maxtop = top
            f.put(f.argsort()[:-top], 0)
    count = mixw.shape[0] * mixw.shape[1]
    print "Average #mixw: %.2f" % (float(avgtop) / count)
    print "Min #mixw: %d Max #mixw: %d" % (mintop, maxtop)
    return histo

def prune_mixw_topn(mixw, n):
    for m in mixw:
        for f in m:
            f.put(f.argsort()[:-n], 0)

def prune_mixw_thresh(mixw, thresh):
    avgtop = 0
    mintop = 999
    maxtop = 0
    histo = numpy.zeros(len(mixw[0,0]),'i')
    for m in mixw:
        for f in m:
            toprune = numpy.less(f, thresh).nonzero()[0]
            top = len(f) - len(toprune)
            histo[top] += 1
            avgtop += top
            if top < mintop: mintop = top
            if top > maxtop: maxtop = top
            f.put(toprune, 0)
    count = mixw.shape[0] * mixw.shape[1]
    print "Average #mixw: %.2f" % (float(avgtop) / count)
    print "Min #mixw: %d Max #mixw: %d" % (mintop, maxtop)
    return histo
