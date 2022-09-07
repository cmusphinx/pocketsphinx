#!/usr/bin/python

import unittest
import os
from pocketsphinx import Decoder

DATADIR = os.path.join(os.path.dirname(__file__), "../../test/data")


class LatticeTest(unittest.TestCase):
    def test_lattice(self):
        # Create a decoder with the default model
        decoder = Decoder()

        decoder.start_utt()
        stream = open(os.path.join(DATADIR, "goforward.raw"), "rb")
        while True:
            buf = stream.read(1024)
            if buf:
                decoder.process_raw(buf, False, False)
            else:
                break
        stream.close()
        decoder.end_utt()

        decoder.get_lattice().write("goforward.lat")
        decoder.get_lattice().write_htk("goforward.htk")
        os.unlink("goforward.lat")
        os.unlink("goforward.htk")


if __name__ == "__main__":
    unittest.main()
