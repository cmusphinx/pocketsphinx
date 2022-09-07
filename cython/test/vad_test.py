import unittest
import wave
import os
from memory_profiler import memory_usage

from pocketsphinx import Vad

DATADIR = os.path.join(os.path.dirname(__file__), "../../test/data/vad")


class VadTests(unittest.TestCase):
    @staticmethod
    def _load_wave(file_name):
        fp = wave.open(file_name, 'rb')
        try:
            assert fp.getnchannels() == 1, (
                '{0}: sound format is incorrect! Sound must be mono.'.format(
                    file_name))
            assert fp.getsampwidth() == 2, (
                '{0}: sound format is incorrect! '
                'Sample width of sound must be 2 bytes.').format(file_name)
            assert fp.getframerate() in (8000, 16000, 32000), (
                '{0}: sound format is incorrect! '
                'Sampling frequency must be 8000 Hz, 16000 Hz or 32000 Hz.')
            sampling_frequency = fp.getframerate()
            sound_data = fp.readframes(fp.getnframes())
        finally:
            fp.close()
            del fp
        return sound_data, sampling_frequency

    def test_constructor(self):
        _ = Vad()

    def test_set_mode(self):
        _ = Vad(0)
        _ = Vad(1)
        _ = Vad(2)
        _ = Vad(3)
        with self.assertRaises(ValueError):
            _ = Vad(4)

    def test_valid_rate_and_frame_length(self):
        _ = Vad(sample_rate=8000, frame_length=0.01)
        _ = Vad(sample_rate=16000, frame_length=0.02)
        _ = Vad(sample_rate=32000, frame_length=0.01)
        _ = Vad(sample_rate=48000, frame_length=0.03)
        with self.assertRaises(ValueError):
            _ = Vad(sample_rate=283423, frame_length=1e-5)

    def test_process_zeroes(self):
        frame_len = 160
        sample = b'\x00' * frame_len * 2
        vad = Vad(sample_rate=16000, frame_length=0.01)
        self.assertFalse(vad.is_speech(sample))

    def test_process_file(self):
        with open(os.path.join(DATADIR, 'test-audio.raw'), 'rb') as f:
            data = f.read()
        # 30ms frames at 8kHz
        n = int(8000 * 2 * 30 / 1000.0)
        chunks = list(data[pos:pos + n] for pos in range(0, len(data), n))
        if len(chunks[-1]) != n:
            chunks = chunks[:-1]
        expecteds = [
            '011110111111111111111111111100',
            '011110111111111111111111111100',
            '000000111111111111111111110000',
            '000000111111111111111100000000'
        ]
        for mode in (0, 1, 2, 3):
            vad = Vad(mode=mode, sample_rate=8000, frame_length=0.03)
            result = ''
            for chunk in chunks:
                voiced = vad.is_speech(chunk)
                result += '1' if voiced else '0'
            self.assertEqual(expecteds[mode], result)

    def test_leak(self):
        sound, fs = self._load_wave(os.path.join(DATADIR, 'leak-test.wav'))
        frame_ms = 0.010
        frame_len = int(round(fs * frame_ms))
        n = int(len(sound) / (2 * frame_len))
        nrepeats = 1000
        vad = Vad(mode=3, sample_rate=fs, frame_length=frame_ms)
        used_memory_before = memory_usage(-1)[0]
        for counter in range(nrepeats):
            find_voice = False
            for frame_ind in range(n):
                slice_start = (frame_ind * 2 * frame_len)
                slice_end = ((frame_ind + 1) * 2 * frame_len)
                if vad.is_speech(sound[slice_start:slice_end], fs):
                    find_voice = True
            self.assertTrue(find_voice)
        used_memory_after = memory_usage(-1)[0]
        self.assertGreaterEqual(
            used_memory_before / 5.0,
            used_memory_after - used_memory_before)


if __name__ == '__main__':
    unittest.main(verbosity=2)
