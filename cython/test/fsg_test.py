#!/usr/bin/python

import unittest
from pocketsphinx import LogMath, FsgModel, Decoder


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

        decoder = Decoder()
        fsg = decoder.create_fsg("mygrammar",
                                 start_state=0, final_state=3,
                                 transitions=[(0, 1, 0.75, "hello"),
                                              (0, 1, 0.25, "goodbye"),
                                              (1, 2, 0.75, "beautiful"),
                                              (1, 2, 0.25, "cruel"),
                                              (2, 3, 1.0, "world")])
        self.assertTrue(fsg.accept("hello beautiful world"))
        self.assertTrue(fsg.accept("hello cruel world"))
        self.assertTrue(fsg.accept("goodbye beautiful world"))
        self.assertTrue(fsg.accept("goodbye cruel world"))
        self.assertFalse(fsg.accept("goodbye world"))
        self.assertFalse(fsg.accept("hello world"))
        self.assertFalse(fsg.accept("hello dead parrot"))


if __name__ == "__main__":
    unittest.main()
