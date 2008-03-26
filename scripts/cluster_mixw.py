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
Functions for bottom-up clustering of mixture weights in a
semi-continuous acoustic model (or, really, any set of multinomials,
if you want).
"""
__author__ = "David Huggins-Daines <dhuggins@cs.cmu.edu>"

import numpy
import sys
import prune_mixw as pm
from sphinx import s3mixw

def kl_divergence(p, q):
    """Kullback-Liebler divergence from multinomial p to multinomial q,
    expressed in nats."""
    if (len(q.shape) == 2):
        axis = 1
    else:
        axis = 0
    return (p * (numpy.log(p) - numpy.log(q))).sum(axis)

def js_divergence(p, q):
    """Jensen-Shannon divergence (symmetric) between two multinomials,
    expressed in nats."""
    if (len(q.shape) == 2):
        axis = 1
    else:
        axis = 0
    # D_{JS}(P\|Q) = (D_{KL}(P\|Q) + D_{KL}(Q\|P)) / 2
    return 0.5 * ((q * (numpy.log(q) - numpy.log(p))).sum(axis)
                      + (p * (numpy.log(p) - numpy.log(q))).sum(axis))

def sqdiff(p, q):
    """Squared difference (Euclidean distance) between two multinomials."""
    if (len(q.shape) == 2):
        axis = 1
    else:
        axis = 0
    diff = p - q
    return (diff * diff).sum(axis)

def leaves(tree):
    subtree, centroid = tree
    if isinstance(subtree, int):
        return (subtree,)
    else:
        return leaves(subtree[0]) + leaves(subtree[1])

def nodes(tree):
    active_nodes = [tree]
    while len(active_nodes):
        new_active_nodes = []
        for i, l in enumerate(active_nodes):
            subtree, centroid = l
            if isinstance(subtree, int):
                # Terminal node
                yield ((subtree,), centroid, -1, -1)
            else:
                # Non-terminals: calculate offsets to left and right
                # branches (which are added to the new active list
                # below)
                left = len(active_nodes) - i + len(new_active_nodes)
                right = left + 1
                yield (leaves(l), # FIXME: slow!
                       centroid, left, right)
                for branch in subtree:
                    subtree2, centroid2 = branch
                    new_active_nodes.append((subtree2, centroid2))
        active_nodes = new_active_nodes

def prunetree(tree, nleaves):
    # Traverse the tree breadth-first until the desired number of
    # leaves is obtained, keeping track of centroids
    leafnodes = [tree]
    while len(leafnodes) < nleaves:
        newleafnodes = []
        for l in leafnodes:
            subtree, centroid = l
            if isinstance(subtree, int):
                # Terminal node - nothing to do
                newleafnodes.append(l)
            else:
                # Expand non-terminals
                for branch in subtree:
                    subtree2, centroid2 = branch
                    newleafnodes.append((subtree2, centroid2))
        print "Number of leafnodes", len(newleafnodes)
        leafnodes = newleafnodes
    # Now flatten out the leafnodes to their component distributions
    for i, leaf in enumerate(leafnodes):
        subtree, centroid = leaf
        senones = list(leaves(leaf))
        # Sort senones for each leafnode
        senones.sort()
        leafnodes[i] = (senones, centroid)
    # Sort leafnodes by senone ID
    leafnodes.sort(lambda x,y: cmp(x[0][0],y[0][0]))
    return leafnodes

def apply_cluster(tree, mixw):
    """
    Apply tree topology from one set of mixture weights to another,
    producing a tree with the same topology but adjusted centroids.
    """
    # Need to do this recursively:
    #
    # Centroid for tree = (left + right) / 2
    # Subtree for tree = (left + right)
    subtree, centroid = tree
    if isinstance(subtree, int):
        # Terminal node, just fetch the mixw distribution
        return (subtree, mixw[subtree])
    else:
        left, right = subtree
        lsub, lcentroid = apply_cluster(left, mixw)
        rsub, rcentroid = apply_cluster(right, mixw)
        return ((lsub, lcentroid), (rsub, rcentroid)), (lcentroid + rcentroid) / 2.

def make_bitmap(leafnodes, nbits=None):
    if nbits == None:
        nbits = max(leafnodes) + 1
    nwords = (nbits + 31) / 32
    bits = numpy.zeros(nwords, 'int32')
    for l in leafnodes:
        w = l / 32
        b = l % 32
        bits[w] |= (1<<b)
    # Now strip out all the zeros
    start = min(leafnodes) /  32
    end = (max(leafnodes) + 1 + 31) / 32
    return start, end-start, bits[start:end]

def quantize_mixw(mixw, logbase=1.0001, floor=1e-8):
    mixw = mixw.clip(floor, 1.0)
    logmixw = -(numpy.log(mixw) / numpy.log(logbase) / (1<<10))
    return logmixw.clip(0,160).astype('uint8')

def writetree(outfile, tree):
    if not isinstance(outfile, file):
        outfile = open(outfile, 'wb')
    leafnodes = leaves(tree)
    n_mixw = max(leafnodes) + 1
    # Traverse the tree breadth-first writing out nodes
    for leafnodes, centroid, left, right in nodes(tree):
        # Create a compressed bitmap of the leafnodes
        startword, nwords, bits = make_bitmap(leafnodes, n_mixw)
        # Quantize the mixture weight distribution
        mixw = quantize_mixw(centroid, 1.0001)
        # Now write all this out in binary form
        subtree = numpy.array((left, right), 'int16')
        subtree.tofile(outfile)
        bitpos = numpy.array((startword, nwords), 'int16')
        bitpos.tofile(outfile)
        bits.tofile(outfile)
        mixw.tofile(outfile)
    # Write a "sentinel" node to mark the end of the tree
    subtree = numpy.array((-1, -1), 'int16')
    bitpos = numpy.array((0, 0), 'int16')
    subtree.tofile(outfile)
    bitpos.tofile(outfile)

def writetrees(outfile, trees):
    if not isinstance(outfile, file):
        outfile = open(outfile, 'wb')
    leafnodes = leaves(trees[0])
    n_mixw = max(leafnodes) + 1
    n_density = len(trees[0][1])
    outfile.write("s3\nversion 0.1\nn_feat %d\nn_mixw %d\nn_density %d\nlogbase 1.0001\n"
                  % (len(trees), n_mixw, n_density))
    pos = outfile.tell() + len("endhdr\n")
    # Align to 4 byte boundary with spaces
    align = 4 - (pos & 3)
    outfile.write(" " * align)
    outfile.write("endhdr\n")
    byteorder = numpy.array((0x11223344,), 'int32')
    byteorder.tofile(outfile)
    for tree in trees:
        writetree(outfile, tree)

def unmake_bitmap(bits, startword, nwords):
    startbit = startword * 32
    leafnodes = []
    for i in range(0, len(bits)):
        for j in range(0, 32):
            if bits[i] & (1<<j):
               leafnodes.append(startbit + i * 32 + j)
    return tuple(leafnodes)

def readtrees(infile):
    def readtree():
        nodes = []
        # Read in all the nodes
        while True:
            subtree = numpy.fromfile(infile, 'int16', 2)
            if len(subtree) == 0:
                return None
            bitpos = numpy.fromfile(infile, 'int16', 2)
            if bitpos[1] == 0:
                break;
            bits = numpy.fromfile(infile, 'int32', bitpos[1])
            mixw = numpy.fromfile(infile, 'uint8', n_density)
            mixw = (logbase ** -(mixw.astype('i') << 10))
            # Replace subtree with senone ID for leafnodes
            if subtree[0] == -1:
                leafnodes = unmake_bitmap(bits, *bitpos)
                subtree = leafnodes[0]
            nodes.append([subtree, mixw])
        # Now snap all the links to produce a tree
        for i, n in enumerate(nodes):
            subtree, mixw = n
            if isinstance(subtree, int):
                # Do nothing
                pass
            else:
                left, right = subtree
                n[0] = (nodes[i + left], nodes[i + right])
        return nodes[0]
    if not isinstance(infile, file):
        infile = open(infile, 'rb')
    while True:
        spam = infile.readline().strip()
        if spam.endswith('endhdr'):
            break
        if spam == 's3':
            continue
        key, val = spam.split()
        if key == 'n_mixw':
            n_mixw = int(val)
        elif key == 'n_density':
            n_density = int(val)
        elif key == 'logbase':
            logbase = float(val)
    byteorder = numpy.fromfile(infile, 'int32', 1)
    trees = []
    while True:
        tree = readtree()
        if tree == None:
            break
        trees.append(tree)
    return trees

def cluster(mixw, dfunc=js_divergence):
    # Start with each distribution in its own cluster
    centroids = list(mixw)
    trees = range(0, len(centroids))
    # Keep merging until we only have one cluster
    while len(centroids) > 1:
        i = 0
        while i < len(centroids):
            p = centroids[i]
            # Find the lowest cost merge (interpolation)
            merged = (p + numpy.array(centroids)) * 0.5
            kl = dfunc(p, merged)
            nbest = kl.argsort()
            for k, best in enumerate(nbest):
                if kl[best] > 0:
                    break
            q = centroids[best]
            # Merge these two
            newcentroid = (p + q) * 0.5
            print "Merging", i, best, kl[best], len(centroids)
            newtree = ((trees[i], p), (trees[best], q))
            centroids[i] = newcentroid
            trees[i] = newtree
            # Remove the other one
            del centroids[best]
            del trees[best]
            i = i + 1
    return trees[0], centroids[0]

def writetree_merged(outfile, tree, mixw):
    if not isinstance(outfile, file):
        outfile = open(outfile, 'wb')
    n_mixw, n_feat, n_density = mixw.shape
    outfile.write("s3\nversion 0.1\nn_mixw %d\nn_feat %d\nn_density %d\nlogbase 1.0001\n"
                  % mixw.shape)
    pos = outfile.tell() + len("endhdr\n")
    # Align to 4 byte boundary with spaces
    align = 4 - (pos & 3)
    outfile.write(" " * align)
    outfile.write("endhdr\n")
    # Write byte order marker
    byteorder = numpy.array((0x11223344,), 'int32')
    byteorder.tofile(outfile)
    # Now write out a merged tree to the file
    # Traverse the tree breadth-first writing out nodes
    for leafnodes, centroid, left, right in nodes(tree):
        # Write out subtree pointers
        subtree = numpy.array((left, right), 'int16')
        subtree.tofile(outfile)
        # Create a compressed bitmap of the leafnodes
        startword, nwords, bits = make_bitmap(leafnodes, n_mixw)
        bitpos = numpy.array((startword, nwords), 'int16')
        bitpos.tofile(outfile)
        bits.tofile(outfile)
        # Quantize the mixture weight distributions
        for feat in centroid:
            mixw = quantize_mixw(feat, 1.0001)
            mixw.tofile(outfile)
    # Write a "sentinel" node to mark the end of the tree
    subtree = numpy.array((-1, -1), 'int16')
    bitpos = numpy.array((0, 0), 'int16')
    subtree.tofile(outfile)
    bitpos.tofile(outfile)

def cluster_merged(mixw, dfunc=js_divergence):
    # Start with each distribution in its own cluster
    # Each of these is a 4x256 array (or whatever)
    centroids = [mixw[i,:] for i in range(0,mixw.shape[0])]
    trees = range(0, len(centroids))
    # Keep merging until we only have one cluster
    while len(centroids) > 1:
        i = 0
        while i < len(centroids):
            p = centroids[i]
            # Evaluate distances for all features
            dist = numpy.empty((len(centroids), len(p)))
            for j in range(0, len(p)):
                qs = numpy.array([x[j] for x in centroids])
                dist[:,j] = dfunc(p[j], (p[j] + qs) * 0.5)
            dist = dist.mean(1)
            # Find the lowest mean distance
            nbest = dist.argsort()
            for k, best in enumerate(nbest):
                if dist[best] > 0:
                    break
            q = centroids[best]
            # Merge these two
            newcentroid = (p + q) * 0.5
            print "Merging", i, best, dist[best], len(centroids)
            newtree = ((trees[i], p), (trees[best], q))
            centroids[i] = newcentroid
            trees[i] = newtree
            # Remove the other one
            del centroids[best]
            del trees[best]
            i = i + 1
    return trees[0], centroids[0]

def readtree_merged(infile):
    if not isinstance(infile, file):
        infile = open(infile, 'rb')
    while True:
        spam = infile.readline().strip()
        if spam.endswith('endhdr'):
            break
        if spam == 's3':
            continue
        key, val = spam.split()
        if key == 'n_mixw':
            n_mixw = int(val)
        elif key == 'n_feat':
            n_feat = int(val)
        elif key == 'n_density':
            n_density = int(val)
        elif key == 'logbase':
            logbase = float(val)
    byteorder = numpy.fromfile(infile, 'int32', 1)
    nodes = []
    # Read in all the nodes
    while True:
        subtree = numpy.fromfile(infile, 'int16', 2)
        if len(subtree) == 0:
            return None
        bitpos = numpy.fromfile(infile, 'int16', 2)
        if bitpos[1] == 0:
            break;
        bits = numpy.fromfile(infile, 'int32', bitpos[1])
        mixw = numpy.fromfile(infile, 'uint8', n_feat * n_density).reshape(n_feat, n_density)
        mixw = (logbase ** -(mixw.astype('i') << 10))
        # Replace subtree with senone ID for leafnodes
        if subtree[0] == -1:
            leafnodes = unmake_bitmap(bits, *bitpos)
            subtree = leafnodes[0]
        nodes.append([subtree, mixw])
    # Now snap all the links to produce a tree
    for i, n in enumerate(nodes):
        subtree, mixw = n
        if isinstance(subtree, int):
            # Do nothing
            pass
        else:
            left, right = subtree
            n[0] = (nodes[i + left], nodes[i + right])
    return nodes[0]

def cluster_nary(mixw, dfunc=js_divergence):
    # Start with each distribution in its own cluster
    centroids = list(mixw)
    trees = range(0, len(centroids))
    # Keep merging until we only have one cluster
    while len(centroids) > 1:
        i = 0
        while i < len(centroids):
            p = centroids[i]
            # Find the lowest cost merge (interpolation)
            merged = (p + numpy.array(centroids)) * 0.5
            kl = dfunc(p, merged)
            nbest = kl.argsort()
            for k, best in enumerate(nbest):
                if kl[best] > 0:
                    break
            q = centroids[best]
            print "Merging", i, best, kl[best], len(centroids)
            # Merge these two
            newcentroid = (p + q) * 0.5
            # Concatenate ints to existing clusters
            if isinstance(trees[i], int) and isinstance(trees[best], tuple):
                newtree = ((trees[i],) + trees[best], newcentroid)
            else:
                newtree = ((trees[i], p), (trees[best], q))
            centroids[i] = newcentroid
            trees[i] = newtree
            # Remove the other one
            del centroids[best]
            del trees[best]
            i = i + 1
    return trees[0], centroids[0]

def cluster_balanced(mixw, dfunc=js_divergence):
    # Start with each distribution in its own cluster
    centroids = mixw
    trees = range(0, len(centroids))
    while len(centroids) > 1:
        newcentroids = []
        newtrees = []
        j = 0
        done = numpy.zeros(len(centroids), 'b')
        # Find lowest cost merge among not yet seen clusters
        for i in range(0, len(centroids)):
            if done[i]:
                continue
            done[i] = 1
            merged = (centroids[i] + centroids) * 0.5
            kl = dfunc(centroids[i], centroids)
            nbest = kl.argsort()
            for k, best in enumerate(nbest):
                if done[best] == 0:
                    done[best] = 1
                    break
            print "Merging", i, best, kl[best]
            newclust = (centroids[i] + centroids[best]) * 0.5
            newtree = ((trees[i], centroids[i]), (trees[best], centroids[best]))
            newcentroids.append(newclust)
            newtrees.append(newtree)
            j = j + 1
        centroids = numpy.array(newcentroids)
        trees = newtrees
    return trees[0], centroids[0]

def prune_mixw(mixw, trees, nclust):
    for feat, tree in enumerate(trees):
        leafnodes = prunetree(tree, nclust)
        for cluster, centroid in leafnodes:
            for senone in cluster:
                mixw[senone,feat,:] = centroid

def write_pruned(outfile, mixw, trees, nclust, target=0, minn=0):
    if not isinstance(outfile, file):
        outfile = open(outfile, 'wb')
    # Write header
    outfile.write("s3\nversion 0.1\nn_mixw %d\nn_feat %d\nn_density %d\nlogbase 1.0001\n"
                  % mixw.shape)
    pos = outfile.tell() + len("endhdr\n")
    # Align to 4 byte boundary with spaces
    align = 4 - (pos & 3)
    outfile.write(" " * align)
    outfile.write("endhdr\n")
    byteorder = numpy.array((0x11223344,), 'int32')
    byteorder.tofile(outfile)
    # Construct clusters and senone to cluster mapping
    n_mixw, n_feat, n_density = mixw.shape
    senmap = numpy.empty((n_feat, n_mixw), 'uint16')
    qmixw = []
    for feat, tree in enumerate(trees):
        leafnodes = prunetree(tree, nclust)
        # Add each leafnode's senone set to the mapping
        for cid, node in enumerate(leafnodes):
            senones, centroid = node
            senmap[feat].put(senones, cid)
        # Create an array from the centroids and append it to qmixw
        qmixw.append(numpy.array([x[1] for x in leafnodes]))
        # Prune it if requested
        if target != 0:
            if minn != 0:
                pm.prune_mixw_entropy_min(qmixw[feat][numpy.newaxis], target, minn)
            else:
                pm.prune_mixw_entropy(qmixw[feat][numpy.newaxis], target)
    # Now write the senone map
    senmap.tofile(outfile)
    # Write the number of clusters for each feature
    nclust = numpy.array([len(x) for x in qmixw], 'uint16')
    nclust.tofile(outfile)
    for feat, centroids in enumerate(qmixw):
        print "feature %d nclust %d" % (feat, len(centroids))
        # Quantize and transpose
        cmixw = quantize_mixw(centroids, 1.0001).T
        cmixw.tofile(outfile)

def write_pruned2(outfile, mixw, trees, nclust, target=0, minn=0):
    if not isinstance(outfile, file):
        outfile = open(outfile, 'wb')
    # Write header
    outfile.write("s3\nversion 0.2\nn_sen %d\nn_feat %d\nn_density %d\nlogbase 1.0001\n"
                  % mixw.shape)
    pos = outfile.tell() + len("endhdr\n")
    # Align to 4 byte boundary with spaces
    align = 4 - (pos & 3)
    outfile.write(" " * align)
    outfile.write("endhdr\n")
    byteorder = numpy.array((0x11223344,), 'int32')
    byteorder.tofile(outfile)
    # Construct clusters and senone to cluster mapping
    n_mixw, n_feat, n_density = mixw.shape
    senmap = []
    qmixw = []
    for feat, tree in enumerate(trees):
        leafnodes = prunetree(tree, nclust)
        # Add each leafnode's senone set to the mapping
        senmap.append([x[0] for x in leafnodes])
        # Create an array from the centroids and append it to qmixw
        qmixw.append(numpy.array([x[1] for x in leafnodes]))
        # Prune it if requested
        if target != 0:
            if minn != 0:
                pm.prune_mixw_entropy_min(qmixw[feat][numpy.newaxis], target, minn)
            else:
                pm.prune_mixw_entropy(qmixw[feat][numpy.newaxis], target)
    # Write the number of clusters for each feature
    nclust = numpy.array([len(x) for x in qmixw], 'uint16')
    nclust.tofile(outfile)
    # Now write the senone map
    for feat in senmap:
        for senones in feat:
            spam = [len(senones)] + senones
            smap = numpy.array(spam, 'uint16')
            smap.tofile(outfile)
    # And, finally, the clusters themselves
    for feat, centroids in enumerate(qmixw):
        print "feature %d nclust %d" % (feat, len(centroids))
        # Quantize and transpose
        cmixw = quantize_mixw(centroids, 1.0001).T
        cmixw.tofile(outfile)

def write_pruned3(outfile, mixw, trees, nclust, target=0, minn=0):
    if not isinstance(outfile, file):
        outfile = open(outfile, 'wb')
    # Write header
    outfile.write("s3\nversion 0.3\nn_sen %d\nn_feat %d\nn_density %d\nlogbase 1.0001\n"
                  % mixw.shape)
    pos = outfile.tell() + len("endhdr\n")
    # Align to 4 byte boundary with spaces
    align = 4 - (pos & 3)
    outfile.write(" " * align)
    outfile.write("endhdr\n")
    byteorder = numpy.array((0x11223344,), 'int32')
    byteorder.tofile(outfile)
    # Construct clusters and senone to cluster mapping
    n_mixw, n_feat, n_density = mixw.shape
    senmap = []
    qmixw = []
    for feat, tree in enumerate(trees):
        leafnodes = prunetree(tree, nclust)
        # Add each leafnode's senone set to the mapping
        senmap.append([x[0] for x in leafnodes])
        # Create an array from the centroids and append it to qmixw
        qmixw.append(numpy.array([x[1] for x in leafnodes]))
        # Prune it if requested
        if target != 0:
            if minn != 0:
                pm.prune_mixw_entropy_min(qmixw[feat][numpy.newaxis], target, minn)
            else:
                pm.prune_mixw_entropy(qmixw[feat][numpy.newaxis], target)
    # Write the number of clusters for each feature
    nclust = numpy.array([len(x) for x in qmixw], 'uint16')
    nclust.tofile(outfile)
    # Now write the senone map
    for feat in senmap:
        for senones in feat:
            spam = [len(senones)] + senones
            smap = numpy.array(spam, 'uint16')
            smap.tofile(outfile)
    # And, finally, the clusters themselves
    for feat, centroids in enumerate(qmixw):
        print "feature %d nclust %d" % (feat, len(centroids))
        # Quantize and transpose
        cmixw = quantize_mixw(centroids, 1.0001).T
        cmixw.tofile(outfile)

def prune_mixw_merged(mixw, tree, nclust):
    leafnodes = prunetree(tree, nclust)
    for cluster, centroids in leafnodes:
        for senone in cluster:
            mixw[senone] = centroids

def norm_floor_mixw(mixw, floor=1e-7):
    return (mixw.T / mixw.T.sum(0)).T.clip(floor, 1.0)

if __name__ == '__main__':
    nclust, mixw, outfile = sys.argv[1:]
    mixw = s3mixw.open(mixw).getall()
    # Cluster mixw for each feature stream
    trees = []
    for f in range(0, mixw.shape[1]):
        trees.append(cluster(mixw[:,f,:]))
    write_pruned(outfile, mixw, trees, nclust)
