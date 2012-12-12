import pocketsphinx as ps

fh = open("../test/data/goforward.raw", "rb")
data = fh.read()
decoder = ps.Decoder()
decoder.start_utt()
decoder.process_raw(data)
decoder.end_utt()
hyp, uttid, score = decoder.get_hyp()
print "%s (%s %d)" % (hyp, uttid, score)
