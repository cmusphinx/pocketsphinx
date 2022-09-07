#!/usr/bin/env python3

import os
from pocketsphinx import Decoder, Config
import unittest

MODELDIR = os.path.join(os.path.dirname(__file__), "../../model")
DATADIR = os.path.join(os.path.dirname(__file__), "../../test/data")


class TestConfig(unittest.TestCase):
    def test_config(self):
        config = Decoder.default_config()
        intval = 256
        floatval = 0.025
        stringval = "~/pocketsphinx"
        boolval = True

        # Check values that was previously set.
        s = config.get_float("-wlen")
        print("Float:", floatval, " ", s)
        self.assertEqual(s, 0.025625)
        config.set_float("-wlen", floatval)
        s = config.get_float("-wlen")
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


class TestConfigHash(unittest.TestCase):
    def test_config__getitem(self):
        config = Config()
        self.assertEqual(config["samprate"], 16000)
        self.assertEqual(config["nfft"], 0)
        self.assertEqual(config["fsg"], None)
        self.assertEqual(config["backtrace"], False)
        self.assertEqual(config["feat"], "1s_c_d_dd")

    def test_config_easyinit(self):
        config = Config(samprate=11025, fsg=None, backtrace=False, feat="1s_c_d_dd")
        self.assertEqual(config["samprate"], 11025)
        self.assertEqual(config.get_int("-samprate"), 11025)
        self.assertEqual(config["nfft"], 0)
        self.assertEqual(config["fsg"], None)
        self.assertEqual(config["backtrace"], False)
        self.assertEqual(config["feat"], "1s_c_d_dd")

    def test_config_coercion(self):
        config = Config()
        config["samprate"] = 48000.0
        self.assertEqual(config["samprate"], 48000)
        config["nfft"] = "1024"
        self.assertEqual(config["nfft"], 1024)


class TestConfigIter(unittest.TestCase):
    def test_config__iter(self):
        config = Config()
        default_len = len(config)
        for key in config:
            self.assertTrue(key in config)
        for key, value in config.items():
            self.assertTrue(key in config)
            self.assertEqual(config[key], value)
        config = Decoder.default_config()
        self.assertEqual(default_len, len(config))
        config["hmm"] = os.path.join(MODELDIR, "en-us", "en-us")
        config["fsg"] = os.path.join(DATADIR, "goforward.fsg")
        config["dict"] = os.path.join(DATADIR, "turtle.dic")
        self.assertEqual(default_len, len(config))
        for key in config:
            self.assertTrue(key in config)
        for key, value in config.items():
            self.assertTrue(key in config)
            self.assertEqual(config[key], value)
        self.assertIsNone(config["mdef"])
        # Now check weird extra value stuff.  Length should never change
        _ = Decoder(config)
        # But mdef, etc, should be filled in
        default_mdef = config["mdef"]
        self.assertIsNotNone(default_mdef)
        # And we should get them for dash and underscore versions too
        self.assertEqual(default_mdef, config["-mdef"])
        self.assertEqual(default_mdef, config["_mdef"])
        self.assertEqual(default_len, len(config))
        for key in config:
            self.assertTrue(key in config)
        for key, value in config.items():
            self.assertTrue(key in config)
            self.assertEqual(config[key], value)


class TestConfigDefn(unittest.TestCase):
    def test_config_describe(self):
        config = Config()
        for defn in config.describe():
            if defn.name == "hmm":
                self.assertTrue(defn.required)


if __name__ == "__main__":
    unittest.main()
