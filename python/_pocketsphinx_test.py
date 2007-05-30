#!/usr/bin/env python

import _pocketsphinx
import unittest

sphinx_config = { 'hmm' : '../model/hmm/tidigits',
                  'lm' : '../model/lm/tidigits/tidigits.lm.DMP',
                  'dict' : '../model/lm/tidigits/tidigits.dic',
                  'input_endian' : 'little' }

class TestDecode(unittest.TestCase):
    def setUp(self):
        _pocketsphinx.parse_argdict(sphinx_config)
        _pocketsphinx.init()

    def tearDown(self):
        _pocketsphinx.close()

    def test_decode_cep_file(self):
        text, segs = _pocketsphinx.decode_cep_file('../test/data/tidigits/man.ah.2934za.mfc')
        self.assertEqual(text.strip(), "two nine three four zero")

    def test_decode_raw(self):
        wav = open('../test/data/tidigits/dhd.2934z.raw')
        data = wav.read()
        text, segs = _pocketsphinx.decode_raw(data, 'foo')
        self.assertEqual(text.strip(), "two nine three four zero")

    def test_process_raw(self):
        wav = open('../test/data/tidigits/dhd.2934z.raw')
        data = wav.read()
        _pocketsphinx.begin_utt('foo')
        for i in range(0, len(data)/4096):
            start = i * 4096
            end = min(len(data), (i + 1) * 4096)
            _pocketsphinx.process_raw(data[start:end])
        _pocketsphinx.end_utt()
        text, segs = _pocketsphinx.get_hypothesis()
        self.assertEqual(text.strip(), "two nine three four zero")

if __name__ == '__main__':
    unittest.main()
