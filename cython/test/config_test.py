#!/usr/bin/env python3

import os
from pocketsphinx5 import Decoder, Config
import unittest

MODELDIR = os.path.join(os.path.dirname(__file__), "../../model")


class TestConfig(unittest.TestCase):
    def test_config(self):
        config = Decoder.default_config()
        intval = 256
        floatval = 8000.0
        stringval = "~/pocketsphinx"
        boolval = True

        # Check values that was previously set.
        s = config.get_float("-samprate")
        print("Float:", floatval, " ", s)
        self.assertEqual(s, 16000.0)
        config.set_float("-samprate", floatval)
        s = config.get_float("-samprate")
        self.assertEqual(s, floatval)

        s = config.get_int("-nfft")
        print("Int:", intval, " ", s)
        config.set_int("-nfft", intval)
        s = config.get_int("-nfft")
        self.assertEqual(s, intval)

        s = config.get_string("-rawlogdir")
        print("String:", stringval, " ", s)
        config.set_string("-rawlogdir", stringval)
        s = config.get_string("-rawlogdir")
        self.assertEqual(s, stringval)

        s = config.get_boolean("-backtrace")
        print("Boolean:", boolval, " ", s)
        config.set_boolean("-backtrace", boolval)
        s = config.get_boolean("-backtrace")
        self.assertEqual(s, boolval)

        config.set_string_extra("-something12321", "abc")
        s = config.get_string("-something12321")
        self.assertEqual(s, "abc")

    def test_config_file(self):
        config = Config.parse_file(
            os.path.join(MODELDIR, "en-us", "en-us", "feat.params")
        )
        self.assertEqual(config["lowerf"], 130)
        self.assertEqual(config["upperf"], 6800)
        self.assertEqual(config["nfilt"], 25)
        self.assertEqual(config["transform"], "dct")
        self.assertEqual(config["lifter"], 22)
        self.assertEqual(config["feat"], "1s_c_d_dd")
        self.assertEqual(config["svspec"], "0-12/13-25/26-38")
        self.assertEqual(config["agc"], "none")
        self.assertEqual(config["cmn"], "batch")
        self.assertEqual(config["varnorm"], False)
        # not an actual config parameter, it seems!
        with self.assertRaises(KeyError):
            self.assertEqual(config["model"], "ptm")
        self.assertEqual(config["remove_noise"], True)


if __name__ == "__main__":
    unittest.main()
