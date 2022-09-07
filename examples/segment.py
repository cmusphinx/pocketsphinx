#!/usr/bin/env python3
"""
Segment speech endlessly from the default audio device.
"""

# MIT license (c) 2022, see LICENSE for more information.
# Author: David Huggins-Daines <dhdaines@gmail.com>

from pocketsphinx import Segmenter
import subprocess

seg = Segmenter()
incmd = f"sox -q -r {seg.sample_rate} -c 1 -b 16 -e signed-integer -d -t raw -".split()
outcmd = f"sox -q -t raw -r {seg.sample_rate} -c 1 -b 16 -e signed-integer -".split()
with subprocess.Popen(incmd, stdout=subprocess.PIPE) as sox:
    try:
        for idx, speech in enumerate(seg.segment(sox.stdout)):
            outfile = "%03d_%.2f-%.2f.wav" % (
                idx,
                speech.start_time,
                speech.end_time,
            )
            with subprocess.Popen(outcmd + [outfile], stdin=subprocess.PIPE) as soxout:
                soxout.stdin.write(speech.pcm)
            print("Wrote %s" % outfile)
    except KeyboardInterrupt:
        pass
