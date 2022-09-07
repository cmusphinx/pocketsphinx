from pocketsphinx import LiveSpeech
for phrase in LiveSpeech(): print(phrase)
from pocketsphinx import LiveSpeech

speech = LiveSpeech(lm=False, keyphrase='forward', kws_threshold=1e-20)
for phrase in speech:
    print(phrase.segments(detailed=True))

import os
from pocketsphinx import LiveSpeech, get_model_path

model_path = get_model_path()

speech = LiveSpeech(
    verbose=False,
    sampling_rate=16000,
    buffer_size=2048,
    no_search=False,
    full_utt=False,
    hmm=os.path.join(model_path, 'en-us'),
    lm=os.path.join(model_path, 'en-us.lm.bin'),
    dic=os.path.join(model_path, 'cmudict-en-us.dict')
)

for phrase in speech:
    print(phrase)

from pocketsphinx import AudioFile
for phrase in AudioFile(): print(phrase) # => "go forward ten meters"

from pocketsphinx import AudioFile

audio = AudioFile(lm=False, keyphrase='forward', kws_threshold=1e-20)
for phrase in audio:
    print(phrase.segments(detailed=True)) # => "[('forward', -617, 63, 121)]"

import os
from pocketsphinx import AudioFile, get_model_path, get_data_path

model_path = get_model_path()
data_path = get_data_path()

config = {
    'verbose': False,
    'audio_file': os.path.join(data_path, 'goforward.raw'),
    'buffer_size': 2048,
    'no_search': False,
    'full_utt': False,
    'hmm': os.path.join(model_path, 'en-us'),
    'lm': os.path.join(model_path, 'en-us.lm.bin'),
    'dict': os.path.join(model_path, 'cmudict-en-us.dict')
}

audio = AudioFile(**config)
for phrase in audio:
    print(phrase)


from pocketsphinx import AudioFile

# Frames per Second
fps = 100

for phrase in AudioFile(frate=fps):  # frate (default=100)
    print('-' * 28)
    print('| %5s |  %3s  |   %4s   |' % ('start', 'end', 'word'))
    print('-' * 28)
    for s in phrase.seg():
        print('| %4ss | %4ss | %8s |' % (s.start_frame / fps, s.end_frame / fps, s.word))
    print('-' * 28)

# ----------------------------
# | start |  end  |   word   |
# ----------------------------
# |  0.0s | 0.24s | <s>      |
# | 0.25s | 0.45s | <sil>    |
# | 0.46s | 0.63s | go       |
# | 0.64s | 1.16s | forward  |
# | 1.17s | 1.52s | ten      |
# | 1.53s | 2.11s | meters   |
# | 2.12s |  2.6s | </s>     |
# ----------------------------


from pocketsphinx import Pocketsphinx
print(Pocketsphinx().decode()) # => "go forward ten meters"



from __future__ import print_function
import os
from pocketsphinx import Pocketsphinx, get_model_path, get_data_path

model_path = get_model_path()
data_path = get_data_path()

config = {
    'hmm': os.path.join(model_path, 'en-us'),
    'lm': os.path.join(model_path, 'en-us.lm.bin'),
    'dict': os.path.join(model_path, 'cmudict-en-us.dict')
}

ps = Pocketsphinx(**config)
ps.decode(
    audio_file=os.path.join(data_path, 'goforward.raw'),
    buffer_size=2048,
    no_search=False,
    full_utt=False
)

print(ps.segments()) # => ['<s>', '<sil>', 'go', 'forward', 'ten', 'meters', '</s>']
print('Detailed segments:', *ps.segments(detailed=True), sep='\n') # => [
#     word, prob, start_frame, end_frame
#     ('<s>', 0, 0, 24)
#     ('<sil>', -3778, 25, 45)
#     ('go', -27, 46, 63)
#     ('forward', -38, 64, 116)
#     ('ten', -14105, 117, 152)
#     ('meters', -2152, 153, 211)
#     ('</s>', 0, 212, 260)
# ]

print(ps.hypothesis())  # => go forward ten meters
print(ps.probability()) # => -32079
print(ps.score())       # => -7066
print(ps.confidence())  # => 0.04042641466841839

print(*ps.best(count=10), sep='\n') # => [
#     ('go forward ten meters', -28034)
#     ('go for word ten meters', -28570)
#     ('go forward and majors', -28670)
#     ('go forward and meters', -28681)
#     ('go forward and readers', -28685)
#     ('go forward ten readers', -28688)
#     ('go forward ten leaders', -28695)
#     ('go forward can meters', -28695)
#     ('go forward and leaders', -28706)
#     ('go for work ten meters', -28722)
# ]


from pocketsphinx import Pocketsphinx

ps = Pocketsphinx(verbose=True)
ps.decode()

print(ps.hypothesis())


from pocketsphinx import Pocketsphinx

ps = Pocketsphinx(verbose=True, logfn='pocketsphinx.log')
ps.decode()

print(ps.hypothesis())

import os
from pocketsphinx import DefaultConfig, Decoder, get_model_path, get_data_path

model_path = get_model_path()
data_path = get_data_path()

# Create a decoder with a certain model
config = DefaultConfig()
config.set_string('-hmm', os.path.join(model_path, 'en-us'))
config.set_string('-lm', os.path.join(model_path, 'en-us.lm.bin'))
config.set_string('-dict', os.path.join(model_path, 'cmudict-en-us.dict'))
decoder = Decoder(config)

# Decode streaming data
buf = bytearray(1024)
with open(os.path.join(data_path, 'goforward.raw'), 'rb') as f:
    decoder.start_utt()
    while f.readinto(buf):
        decoder.process_raw(buf, False, False)
    decoder.end_utt()
print('Best hypothesis segments:', [seg.word for seg in decoder.seg()])
