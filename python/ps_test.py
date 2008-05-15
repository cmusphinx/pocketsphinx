import pocketsphinx as ps

decoder = ps.Decoder(hmm="../model/hmm/wsj1",
                     lm="../model/lm/turtle/turtle.lm",
                     dict="../model/lm/turtle/turtle.dic")
