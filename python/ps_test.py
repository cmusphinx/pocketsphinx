import pocketsphinx as ps

config = ps.Config(hmm="../model/hmm/wsj1",
                   lm="../model/lm/turtle/turtle.lm",
                   dict="../model/lm/turtle/turtle.dic")
decoder = ps.Decoder(config)
