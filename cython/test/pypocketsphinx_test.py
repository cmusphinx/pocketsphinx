#!/usr/bin/env python3

"""Test the various use cases for the old abandoned
python-pocketsphinx module and its simple classes.  We don't support
the truly useless parts of the API like defaulting to "goforward.raw"
as the input, and some results have changed, other than that it should
be compatible.
"""

import os
from pocketsphinx import Pocketsphinx, AudioFile, NGramModel, Jsgf
from unittest import TestCase, main

MODELDIR = os.path.join(os.path.dirname(__file__), "../../model")
DATADIR = os.path.join(os.path.dirname(__file__), "../../test/data")


class TestAudioFile(TestCase):
    def test_audiofile_raw(self):
        hypothesis = ""
        for phrase in AudioFile(audio_file=os.path.join(DATADIR, "goforward.raw")):
            hypothesis = str(phrase)
        self.assertEqual(hypothesis, "go forward ten meters")


class TestRawDecoder(TestCase):
    def setUp(self):
        self.ps = Pocketsphinx(
            hmm=os.path.join(MODELDIR, "en-us/en-us"),
            lm=os.path.join(MODELDIR, "en-us/en-us.lm.bin"),
            dict=os.path.join(MODELDIR, "en-us/cmudict-en-us.dict"),
        )
        self.ps.decode(os.path.join(DATADIR, "goforward.raw"))

    def test_raw_decoder_lookup_word(self):
        self.assertEqual(self.ps.lookup_word("hello"), "HH AH L OW")
        self.assertEqual(self.ps.lookup_word("abcdf"), None)

    def test_raw_decoder_hypothesis(self):
        self.assertEqual(self.ps.hypothesis(), "go forward ten years")
        self.assertEqual(self.ps.score(), -8237)
        self.assertAlmostEqual(self.ps.confidence(), 0.01, 3)

    def test_raw_decoder_segments(self):
        self.assertEqual(
            self.ps.segments(),
            ["<s>", "go", "forward", "ten", "years", "</s>"],
        )

    def test_raw_decoder_best_hypothesis(self):
        self.assertEqual(
            self.ps.best(),
            [
                ("go forward ten years", -28492),
                ("go forward ten meters", -28547),
                ("go for word ten meters", -29079),
                ("go forward ten liters", -29084),
                ("go forward ten leaders", -29098),
                ("go forward can meters", -29174),
                ("go for word ten years", -29216),
                ("go forward ten readers", -29254),
                ("go for work ten meters", -29259),
                ("go forward can leaders", -29261),
            ],
        )


class TestCepDecoder(TestCase):
    def test_cep_decoder_hypothesis(self):
        ps = Pocketsphinx(
            hmm=os.path.join(MODELDIR, "en-us/en-us"),
            lm=os.path.join(MODELDIR, "en-us/en-us.lm.bin"),
            dict=os.path.join(MODELDIR, "en-us/cmudict-en-us.dict"),
            verbose=True,
        )
        with open(os.path.join(DATADIR, "goforward.mfc"), "rb") as f:
            with ps.start_utterance():
                f.read(4)
                buf = f.read(13780)
                ps.process_cep(buf, False, True)
        self.assertEqual(ps.hypothesis(), "go forward ten meters")
        self.assertEqual(ps.score(), -7103)
        self.assertEqual(ps.probability(), -33134)


class TestJsgf(TestCase):
    def test_jsgf(self):
        ps = Pocketsphinx(
            hmm=os.path.join(MODELDIR, "en-us/en-us"),
            lm=os.path.join(DATADIR, "turtle.lm.bin"),
            dic=os.path.join(DATADIR, "turtle.dic"),
        )

        # Decoding with 'turtle' language model
        ps.decode(os.path.join(DATADIR, "goforward.raw"))
        self.assertEqual(ps.hypothesis(), "go forward ten meters")

        # Switch to JSGF grammar
        jsgf = Jsgf(os.path.join(DATADIR, "goforward.gram"))
        rule = jsgf.get_rule("goforward.move2")
        fsg = jsgf.build_fsg(rule, ps.get_logmath(), 7.5)
        ps.add_fsg("goforward", fsg)
        ps.activate_search("goforward")

        # Decoding with 'goforward' grammar
        ps.decode(os.path.join(DATADIR, "goforward.raw"))
        self.assertEqual(ps.hypothesis(), "go forward ten meters")


class TestKws(TestCase):
    def test_kws(self):
        segments = []
        for phrase in AudioFile(
                os.path.join(DATADIR, "goforward.raw"),
                lm=None,
                keyphrase="forward",
                kws_threshold=1e20,
        ):
            segments = phrase.segments(detailed=True)
        self.assertEqual(segments, [("forward", -706, 63, 121)])

    def test_kws_badapi(self):
        segments = []
        for phrase in AudioFile(
            audio_file=os.path.join(DATADIR, "goforward.raw"),
            lm=False,  # Make sure this still works
            keyphrase="forward",
            kws_threshold=1e20,
        ):
            segments = phrase.segments(detailed=True)
        self.assertEqual(segments, [("forward", -706, 63, 121)])


class TestLm(TestCase):
    def test_lm(self):
        ps = Pocketsphinx(
            hmm=os.path.join(MODELDIR, "en-us/en-us"),
            lm=os.path.join(MODELDIR, "en-us/en-us.lm.bin"),
            dic=os.path.join(DATADIR, "defective.dic"),
        )

        # Decoding with 'defective' dictionary
        ps.decode(os.path.join(DATADIR, "goforward.raw"))
        self.assertEqual(ps.hypothesis(), "")

        # Switch to 'turtle' language model
        turtle_lm = os.path.join(DATADIR, "turtle.lm.bin")
        lm = NGramModel(ps.get_config(), ps.get_logmath(), turtle_lm)
        ps.add_lm("turtle", lm)
        ps.activate_search("turtle")

        # Decoding with 'turtle' language model
        ps.decode(os.path.join(DATADIR, "goforward.raw"))
        self.assertEqual(ps.hypothesis(), "")

        # The word 'meters' isn't in the loaded dictionary
        # Let's add it manually
        ps.add_word("foobie", "F UW B IY", False)
        ps.add_word("meters", "M IY T ER Z", True)

        # Decoding with 'turtle' language model
        ps.decode(os.path.join(DATADIR, "goforward.raw"))
        self.assertEqual(ps.hypothesis(), "foobie meters meters")


class TestPhoneme(TestCase):
    def setUp(self):
        self.ps = Pocketsphinx(
            allphone=os.path.join(MODELDIR, "en-us/en-us-phone.lm.bin"),
            lw=2.0,
            pip=0.3,
            beam=1e-200,
            pbeam=1e-20,
        )
        self.ps.decode(os.path.join(DATADIR, "goforward.raw"))

    def test_phoneme_hypothesis(self):
        self.assertEqual(
            self.ps.hypothesis(), "SIL G OW F AO R D T AE N NG IY ZH ER S SIL"
        )

    def test_phoneme_best_phonemes(self):
        self.assertEqual(
            self.ps.segments(),
            [
                "SIL",
                "G",
                "OW",
                "F",
                "AO",
                "R",
                "D",
                "T",
                "AE",
                "N",
                "NG",
                "IY",
                "ZH",
                "ER",
                "S",
                "SIL",
            ],
        )


if __name__ == "__main__":
    main(verbosity=2)
