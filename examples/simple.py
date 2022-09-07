#!/usr/bin/env python3
"""
Recognize speech endlessly from the default audio device.
"""

# MIT license (c) 2022, see LICENSE for more information.
# Author: David Huggins-Daines <dhdaines@gmail.com>

from pocketsphinx import Decoder
import subprocess
import os
BUFSIZE = 4096  # about 250ms

decoder = Decoder()
sample_rate = decoder.config["samprate"]
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
