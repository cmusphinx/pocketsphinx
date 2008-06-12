import pocketsphinx as ps

decoder = ps.Decoder(hmm="../model/hmm/wsj1",
                     lm="../test/data/wsj/wlist5o.nvp.lm.DMP",
                     dict="../model/lm/cmudict.0.6d",
                     fwdtree="yes",
                     fwdflat="yes",
                     bestpath="no")
fh = open("../test/data/goforward.raw", "rb")
nsamp = decoder.decode_raw(fh)
print "Decoded %d samples" % nsamp
hyp, uttid, score = decoder.get_hyp()
print "%s (%s %d)" % (hyp, uttid, score)

lmset = decoder.get_lmset()
print "P(FORWARD|GO) = %f, %d" % lmset.prob('FORWARD', 'GO')

dag = decoder.get_lattice()
dag.bestpath(lmset, 1.0, 1.0/15)
prob = dag.posterior(lmset, 1.0/15)
print "P(S|O) = %e" % prob

for n in dag.nodes():
    if n.prob > 0.0001:
        print "%s %s %d -> (%d,%d) %f" % (n.word, n.baseword,
                                          n.sf, n.fef, n.lef, n.prob)
        if n.baseword == 'FORWARD':
            forward = n

print "FORWARD is:", forward.word, forward.sf, forward.fef, forward.lef, forward.prob
print "FORWARD entries:"
for x in forward.entries():
    if x.prob > 0.0001:
        print "%s %d -> %d prob %f" % (x.baseword, x.sf, x.ef, x.prob)
print "FORWARD exits:"
for x in forward.exits():
    if x.prob > 0.0001:
        print "%d -> %d prob %f" % (x.sf, x.ef, x.prob)
