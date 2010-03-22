import pocketsphinx as ps

decoder = ps.Decoder()
fh = open("../test/data/goforward.raw", "rb")
nsamp = decoder.decode_raw(fh)
print "Decoded %d samples" % nsamp
hyp, uttid, score = decoder.get_hyp()
print "%s (%s %d)" % (hyp, uttid, score)

lmset = decoder.get_lmset()
print "P(forward|go) = %f, %d" % lmset.prob('forward', 'go')

dag = decoder.get_lattice()
dag.bestpath(lmset, 1.0, 1.0/15)
prob = dag.posterior(lmset, 1.0/15)
print "P(S|O) = %e" % prob

for n in dag.nodes(50, 150):
    if n.prob > -100:
        print "%s %s %d -> (%d,%d) %f" % (n.word, n.baseword,
                                          n.sf, n.fef, n.lef, n.prob)
        if n.baseword == 'forward':
            forward = n

print "forward is:", forward.word, forward.sf, forward.fef, forward.lef, forward.prob
print "forward entries:"
for x in forward.entries():
    if x.prob > 0.0001:
        print "%s %d -> %d prob %f" % (x.baseword, x.sf, x.ef, x.prob)
print "forward exits:"
for x in forward.exits():
    if x.prob > 0.0001:
        print "%d -> %d prob %f" % (x.sf, x.ef, x.prob)

dag.write("goforward.lat")
dag = ps.Lattice(decoder, "goforward.lat")
dag.bestpath(lmset, 1.0, 1.0/15)
prob = dag.posterior(lmset, 1.0/15)
print "P(S|O) = %e" % prob

for n in dag.nodes(50, 150):
    if n.prob > 0.0001:
        print "%s %s %d -> (%d,%d) %f" % (n.word, n.baseword,
                                          n.sf, n.fef, n.lef, n.prob)
        if n.baseword == 'forward':
            forward = n

print "forward is:", forward.word, forward.sf, forward.fef, forward.lef, forward.prob
print "forward entries:"
for x in forward.entries():
    if x.prob > 0.0001:
        print "%s %d -> %d prob %f" % (x.baseword, x.sf, x.ef, x.prob)
print "forward exits:"
for x in forward.exits():
    if x.prob > 0.0001:
        print "%d -> %d prob %f" % (x.sf, x.ef, x.prob)
