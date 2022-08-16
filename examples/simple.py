from pocketsphinx5 import Decoder
import subprocess
import os
MODELDIR = os.path.join(os.path.dirname(__file__), os.path.pardir, "model")
BUFSIZE = 4096  # about 250ms

decoder = Decoder(
    hmm=os.path.join(MODELDIR, "en-us/en-us"),
    lm=os.path.join(MODELDIR, "en-us/en-us.lm.bin"),
    dict=os.path.join(MODELDIR, "en-us/cmudict-en-us.dict"),
)
sample_rate = int(decoder.config["samprate"])
soxcmd = f"sox -q -r {sample_rate} -c 1 -b 16 -e signed-integer -d -t raw -"
with subprocess.Popen(soxcmd.split(), stdout=subprocess.PIPE) as sox:
    decoder.start_utt()
    try:
        while True:
            buf = sox.stdout.read(BUFSIZE)
            if len(buf) == 0:
                break
            decoder.process_raw(buf)
            hyp = decoder.hyp()
            if hyp is not None:
                print(hyp.hypstr)
    except KeyboardInterrupt:
        pass
    finally:
        decoder.end_utt()
    print(decoder.hyp().hypstr)
