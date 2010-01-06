#!/usr/bin/env python

import sys
import numpy
import struct
import s3mixw

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

def norm_floor_mixw(mixw, floor=1e-7):
    return (mixw.T / mixw.T.sum(0)).T.clip(floor, 1.0)

fmtdesc = \
"""BEGIN FILE FORMAT DESCRIPTION
(int32) <length(string)> (including trailing 0)
<string> (including trailing 0)
... preceding 2 items repeated any number of times
(int32) 0 (length(string)=0 terminates the header)
(int32) <#components>
(int32) <#gmms>
#gmms (unsigned char) quantized mixture weights for feature-0 gmm-0
preceding 2 items repeated feature_count times.
preceding 4 items repeated codebook_count times.
END FILE FORMAT DESCRIPTION
cluster_count 0
logbase 1.0001
codebook_count 1
feature_count %d"""

def write_sendump(mixw, outfile, floor=1e-7):
    n_sen, n_feat, n_gau = mixw.shape
    fh = open(outfile, "wb")
    # Write the header
    fmtdesc0 = fmtdesc % (n_feat)
    for line in fmtdesc0.split('\n'):
        fh.write(struct.pack('>I', len(line) + 1))
        fh.write(line)
        fh.write('\0')
    # Align to 4 bytes
    k = fh.tell() & 3
    if k > 0:
        k = 4 - k
        fh.write(struct.pack('>I', k))
        fh.write('!' * k)
    fh.write(struct.pack('>I', 0))
    # Align number of senones to 4 bytes
    aligned_n_sen = (n_sen + 3) & ~3
    fh.write(struct.pack('>I', n_gau))
    fh.write(struct.pack('>I', aligned_n_sen))
    # Write them out transposed and quantized (could be much faster)
    if floor == 0.0:
        # Assume they are already normalized and floored
        qmixw = (-numpy.log(mixw) / numpy.log(1.0001)).astype('i') >> 10
    else:
        qmixw = (-numpy.log(norm_floor_mixw(mixw, floor))
                 / numpy.log(1.0001)).astype('i') >> 10
    qmixw = qmixw.clip(0, 159).astype('uint8')
    for f in range(0, n_feat):
        for d in range(0, n_gau):
            qmixw[:,f,d].tofile(fh)
            # Align it to 4 byte boundary (why?)
            if aligned_n_sen > n_sen:
                fh.write('\0' * (aligned_n_sen - n_sen))
    fh.close()

if __name__ == '__main__':
    ifn, ofn, tmw, mmw = sys.argv[1:]
    tmw = int(tmw)
    mmw = int(mmw)
    mixw = norm_floor_mixw(s3mixw.open(ifn).getall())
    prune_mixw_entropy_min(mixw, tmw, mmw)
    write_sendump(mixw, ofn)
