#!/usr/bin/env python

__author__ = "David Huggins-Daines <dhuggins@cs.cmu.edu>"

import numpy
import sys
import os
import struct
import s3mixw
import sphinxbase

def mixw_kmeans_iter(lmw, cb):
    cbacc = numpy.zeros(len(cb))
    cbcnt = numpy.zeros(len(cb))
    tdist = 0
    for m in lmw:
        dist = (cb - m)
        dist *= dist
        cw = dist.argmin()
        tdist += dist.min()
        cbacc[cw] += m
        cbcnt[cw] += 1
    cb[:] = cbacc / cbcnt
    return tdist

def map_mixw_cb(mixw, cb, zero=0.0):
    n_sen, n_feat, n_gau = mixw.shape
    lmw = numpy.log(mixw)
    mwmap = numpy.zeros(mixw.shape, 'uint8')
    for s in range(0, n_sen):
        for f in range(0, n_feat):
            for g in range(0, n_gau):
                x = mixw[s,f,g]
                if x <= zero:
                    mwmap[s,f,g] = len(cb)
                else:
                    dist = (cb - lmw[s,f,g])
                    dist *= dist
                    mwmap[s,f,g] = dist.argmin()
    return mwmap

def mixw_freq(mixwmap):
    hist = numpy.zeros(mixwmap.max() + 1, 'i')
    for cw in mixwmap.ravel():
        hist[cw] += 1
    return hist

try:
    from qmwx import mixw_kmeans_iter, map_mixw_cb, mixw_freq
except:
    pass
    
def quantize_mixw_kmeans(mixw, k, zero=0.0):
    mw = mixw.ravel()
    lmw = numpy.log(mw.take(numpy.greater(mw, zero).nonzero()[0]))
    mmw = lmw.min()
    xmw = lmw.max()
    rmw = xmw - mmw
    print "min log mixw: %f range: %f" % (mmw, rmw)
    cb = numpy.random.random(k) * rmw + mmw
    pdist = 1e+50
    for i in range(0,10):
        tdist = mixw_kmeans_iter(lmw, cb)
        conv = (pdist - tdist) / pdist
        print "Total distortion: %e convergence ratio: %e" % (tdist, conv)
        if conv < 0.01:
            print "Training has converged, stopping"
            break
        pdist = tdist
    return cb

def hb_encode(mixw):
    comp = []
    for i in range(0, len(mixw)-1, 2):
        comp.append((mixw[i+1] << 4) | mixw[i])
    if len(mixw) % 2:
        comp.append(mixw[-1])
    return comp

fmtdesc3 = \
"""BEGIN FILE FORMAT DESCRIPTION
(int32) <length(string)> (including trailing 0)
<string> (including trailing 0)
... preceding 2 items repeated any number of times
(int32) 0 (length(string)=0 terminates the header)
cluster_count centroids
cluster index array (feature_count x mixture_count x model_count)
... preceding 2 items repeated codebook_count times
END FILE FORMAT DESCRIPTION
feature_count %d
codebook_count 1
mixture_count %d
model_count %d
cluster_count %d
cluster_bits 4
logbase 1.0001
mixw_shift 10"""

def write_sendump_hb(mixwmap, cb, outfile):
    n_sen, n_feat, n_gau = mixwmap.shape
    fh = open(outfile, "wb")
    # Write the header
    fmtdesc0 = fmtdesc3 % (n_feat, n_gau, n_sen, len(cb))
    for line in fmtdesc0.split('\n'):
        fh.write(struct.pack('>I', len(line) + 1))
        fh.write(line)
        fh.write('\0')
    fh.write(struct.pack('>I', 0))
    # Add one extra index to the end to hold the "zero" value
    qcb = numpy.resize(-(cb / numpy.log(1.0001)).astype('i') >> 10, len(cb)+1)
    qcb[-1] = 159
    qcb.astype('uint8').tofile(fh)
    for f in range(0, n_feat):
        for g in range(0, n_gau):
            mm = numpy.array(hb_encode(mixwmap[:,f,g]), 'uint8')
            mm.tofile(fh)
    fh.close()

fmtdesc4 = \
"""BEGIN FILE FORMAT DESCRIPTION
(int32) <length(string)> (including trailing 0)
<string> (including trailing 0)
... preceding 2 items repeated any number of times
(int32) 0 (length(string)=0 terminates the header)
codebook_count <codebook_count>
mixture_count <mixture_count>
model_count <model_count>
cluster_count <cluster_count>
huffman_coded 1
logbase <logarithm_base>
mixw_shift <log_bits_downshifted>
<huffman codebook>
<compressed arrays of mixture weights>
END FILE FORMAT DESCRIPTION
feature_count %d
codebook_count 1
mixture_count %d
model_count %d
cluster_count %d
huffman_coded 1
logbase 1.0001
mixw_shift 10"""

def write_sendump_huff(mixwmap, cb, outfile):
    n_sen, n_feat, n_gau = mixwmap.shape
    fh = open(outfile, "wb")
    # Write the header
    fmtdesc0 = fmtdesc3 % (n_feat, n_gau, n_sen, len(cb))
    for line in fmtdesc0.split('\n'):
        fh.write(struct.pack('>I', len(line) + 1))
        fh.write(line)
        fh.write('\0')
    # Terminate it with a null entry
    fh.write(struct.pack('>I', 0))
    # If there's an extra "floor" value then add it to the codebook
    if mixwmap.max() == len(cb):
        qcb = numpy.resize(-(cb / numpy.log(1.0001)).astype('i') >> 10, len(cb)+1)
        qcb[-1] = 159
    else:
        qcb = numpy.resize(-(cb / numpy.log(1.0001)).astype('i') >> 10, len(cb))
    # Histogram the mixture weight map and build a Huffman codebook
    hist = mixw_freq(mixwmap)
    # Write the codebook (we code directly to quantized mixw values)
    huff = sphinxbase.HuffCode(zip(qcb, hist))
    huff.write(fh)
    # Now Huffman code the mixture weights (not their codebook indices
    # mind you!) to the output file.
    huff.attach(fh, "wb")
    for f in range(0, n_feat):
        for g in range(0, n_gau):
            syms = [qcb[x] for x in mixwmap[:,f,g]]
            huff.encode_to_file(syms)
    huff.detach()
    fh.close()

def read_sendump(infile, n_cb=4):
    def readstr(fh):
        nbytes = struct.unpack('I', fh.read(4))[0]
        if nbytes == 0:
            return None
        else:
            return fh.read(nbytes)

    sendump = file(infile, "rb")
    title = readstr(sendump)
    while True:
        header = readstr(sendump)
        if header == None:
            break

    # Number of codewords and pdfs
    r, c = struct.unpack('II', sendump.read(8))
    print r,c

    # Now read the stuff
    opdf_8b = numpy.empty((670,n_cb,r))
    for i in range(0,n_cb):
        for j in range(0,r):
            # Read bytes, expand to ints, shift them up
            mixw = numpy.fromfile(sendump, dtype='int8', count=670).astype('i') << 10
            sendump.read(2)
            # Negate, exponentiate, and untranspose
            opdf_8b[:,i,j] = numpy.power(1.0001, -mixw)

    return opdf_8b

def norm_floor_mixw(mixw, floor=1e-7):
    return (mixw.T / mixw.T.sum(0)).T.clip(floor, 1.0)

if __name__ == '__main__':
    ifn, ofn = sys.argv[1:]
    if os.path.basename(ifn).startswith('sendump'):
        mixw = read_sendump(ifn)
    else:
        mixw = norm_floor_mixw(s3mixw.open(ifn).getall(), 1e-7)
    cb = quantize_mixw_kmeans(mixw, 15, 1e-7)
    print cb
    mwmap = map_mixw_cb(mixw, cb, 1e-7)
    write_sendump_hb(mwmap, cb, ofn)
