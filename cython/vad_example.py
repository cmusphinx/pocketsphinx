"""
Example of using webrtcvad with pocketsphinx5
"""

import collections
import argparse
import subprocess
import sys
import os

from pocketsphinx5 import Decoder
import webrtcvad

MODELDIR = os.path.join(os.path.dirname(__file__), "../model")
DATADIR = os.path.join(os.path.dirname(__file__), "../test/data")
Frame = collections.namedtuple("Frame", ["pcm", "timestamp", "duration"])


def frame_generator(frame_duration_ms, audiofh, sample_rate):
    """Generates audio frames from a stream of raw PCM data."""
    n = int(sample_rate * (frame_duration_ms / 1000.0) * 2)
    offset = 0
    timestamp = 0.0
    duration = (float(n) / sample_rate) / 2.0
    while True:
        pcm = audiofh.read(n)
        if len(pcm) == 0:
            break
        yield Frame(pcm, timestamp, duration)
        timestamp += duration
        offset += n


def vad_collector(sample_rate, frame_duration_ms, padding_duration_ms, vad, frames):
    """Filters out non-voiced audio frames.

    Given a webrtcvad.Vad and a source of audio frames, yields speech
    segments

    Uses a padded, sliding window algorithm over the audio frames.
    When more than 90% of the frames in the window are voiced (as
    reported by the VAD), the collector triggers and begins yielding
    audio frames. Then the collector waits until 90% of the frames in
    the window are unvoiced to detrigger.

    The window is padded at the front and back to provide a small
    amount of silence or the beginnings/endings of speech around the
    voiced frames.

    Arguments:

    sample_rate - The audio sample rate, in Hz.
    frame_duration_ms - The frame duration in milliseconds.
    padding_duration_ms - The amount to pad the window, in milliseconds.
    vad - An instance of webrtcvad.Vad.
    frames - a source of audio frames (sequence or generator).

    Returns: A generator that yields (start_time, end_time, pcm_data)

    """
    num_padding_frames = int(padding_duration_ms / frame_duration_ms)
    # We use a deque for our sliding window/ring buffer.
    ring_buffer = collections.deque(maxlen=num_padding_frames)
    # We have two states: TRIGGERED and NOTTRIGGERED. We start in the
    # NOTTRIGGERED state.
    triggered = False

    voiced_frames = []
    for frame in frames:
        is_speech = vad.is_speech(frame.pcm, sample_rate)

        if not triggered:
            ring_buffer.append((frame, is_speech))
            num_voiced = len([f for f, speech in ring_buffer if speech])
            # If we're NOTTRIGGERED and more than 90% of the frames in
            # the ring buffer are voiced frames, then enter the
            # TRIGGERED state.
            if num_voiced > 0.9 * ring_buffer.maxlen:
                triggered = True
                # We want to yield all the audio we see from now until
                # we are NOTTRIGGERED, but we have to start with the
                # audio that's already in the ring buffer.
                for f, s in ring_buffer:
                    voiced_frames.append(f)
                ring_buffer.clear()
        else:
            # We're in the TRIGGERED state, so collect the audio data
            # and add it to the ring buffer.
            voiced_frames.append(frame)
            ring_buffer.append((frame, is_speech))
            num_unvoiced = len([f for f, speech in ring_buffer if not speech])
            # If more than 90% of the frames in the ring buffer are
            # unvoiced, then enter NOTTRIGGERED and yield whatever
            # audio we've collected.
            if num_unvoiced > 0.9 * ring_buffer.maxlen:
                triggered = False
                start_time = voiced_frames[0].timestamp + voiced_frames[0].duration
                end_time = voiced_frames[-1].timestamp + voiced_frames[-1].duration
                yield start_time, end_time, b"".join([f.pcm for f in voiced_frames])
                ring_buffer.clear()
                voiced_frames = []
    # If we have any leftover voiced audio when we run out of input,
    # yield it.
    if voiced_frames:
        start_time = voiced_frames[0].timestamp + voiced_frames[0].duration
        end_time = voiced_frames[-1].timestamp + voiced_frames[-1].duration
        yield start_time, end_time, b"".join([f.pcm for f in voiced_frames])


def popen_sox(sample_rate):
    """Use sox to get microphone input at sample_rate"""
    soxcmd = [
        "sox",
        "-q",
        "-r",
        str(sample_rate),
        "-c",
        "1",
        "-b",
        "16",
        "-e",
        "signed-integer",
        "-d",
        "-t",
        "raw",
        "-",
    ]
    sox = subprocess.Popen(soxcmd, stdout=subprocess.PIPE)
    # Yes, it will never get closed.  No, we don't care.
    return sox.stdout


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "-a",
        "--aggressiveness",
        type=int,
        default=3,
        help="Aggressiveness of voice activity detection",
    )
    parser.add_argument(
        "-s",
        "--stdin",
        action="store_true",
        help="Read from standard input instead of microphone",
    )
    args = parser.parse_args(argv)

    decoder = Decoder(
        hmm=os.path.join(MODELDIR, "en-us/en-us"),
        lm=os.path.join(MODELDIR, "en-us/en-us.lm.bin"),
        dict=os.path.join(MODELDIR, "en-us/cmudict-en-us.dict"),
    )
    sample_rate = int(decoder.config["samprate"])
    vad = webrtcvad.Vad(args.aggressiveness)
    if args.stdin:
        infh = sys.stdin.buffer
    else:
        infh = popen_sox(sample_rate)
        print("Recognizer ready. Ctrl-C to stop")
    frames = frame_generator(
        frame_duration_ms=30, sample_rate=sample_rate, audiofh=infh
    )
    segments = vad_collector(sample_rate, 30, 300, vad, frames)
    for start_time, end_time, segment in segments:
        decoder.start_utt()
        decoder.process_raw(segment, full_utt=True)
        decoder.end_utt()
        print("%.3f\t%.3f\t%s" % (start_time, end_time, decoder.hyp().hypstr))


if __name__ == "__main__":
    main()
