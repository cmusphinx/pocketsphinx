#!/usr/bin/python

import unittest
from pocketsphinx import LogMath, FsgModel


class FsgTest(unittest.TestCase):
    def testfsg(self):
        lmath = LogMath()
        fsg = FsgModel("simple_grammar", lmath, 1.0, 10)
        fsg.word_add("hello")
        fsg.word_add("world")
        print(fsg.word_id("world"))
        self.assertEqual(fsg.word_id("world"), 1)
        self.assertEqual(fsg.word_add("world"), 1)

        fsg.add_silence("<sil>", 1, 0.5)
        # TODO: Test everything else!!!


if __name__ == "__main__":
    unittest.main()
