#!/usr/bin/env python3

import wave
import subprocess
from contextlib import closing


def get_wavfile_length(path):
    with closing(wave.open(path)) as reader:
        nfr = reader.getnframes()
        frate = reader.getframerate()
        return nfr / frate


def get_labels(path, pos):
    with open(path, "rt") as infh:
        labels = [(pos, "silence")]
        for spam in infh:
            # The labels are a bit odd
            start, _, label = spam.strip().split()
            labels.append((pos + float(start), label))
    return labels


def make_single_track():
    labels = []
    soxcmd = ["sox"]
    with open("fileids", "rt") as infh:
        pos = 0.0
        for spam in infh:
            fileid = spam.strip()
            path = fileid + ".wav"
            soxcmd.append(path)
            nsec = get_wavfile_length(path)
            path = fileid + ".lab"
            labels.extend(get_labels(path, pos))
            pos += nsec
    with open("single_track.lab", "wt") as outfh:
        start_time, label = labels[0]
        for end_time, next_label in labels[1:]:
            if next_label != label:
                outfh.write("%.3f\t%.3f\t%s\n" % (start_time, end_time, label))
                start_time = end_time
            label = next_label
        outfh.write("%.3f\t%.3f\t%s\n" % (start_time, pos, label))
    soxcmd.extend(["-r", "16000", "single_track.raw"])
    subprocess.run(soxcmd)


if __name__ == "__main__":
    make_single_track()
