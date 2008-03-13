#!/usr/bin/env python

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
