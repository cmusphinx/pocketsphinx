#!/usr/bin/env python3

from pocketsphinx import LogMath
import unittest


class TestLogMath(unittest.TestCase):
    def assertLogEqual(self, a, b):
        self.assertTrue(abs(a - b) < 200)

    def test_logmath(self):
        lmath = LogMath()
        self.assertTrue(lmath is not None)
        self.assertLogEqual(lmath.log(1e-150), -3454050)
        self.assertAlmostEqual(lmath.exp(lmath.log(1e-150)), 1e-150)
        self.assertAlmostEqual(lmath.exp(lmath.log(1e-48)), 1e-48)
        self.assertLogEqual(lmath.log(42), 37378)
        self.assertAlmostEqual(lmath.exp(lmath.log(42)), 42, 1)
        print(
            "log(1e-3 + 5e-3) = %d + %d = %d\n"
            % (
                lmath.log(1e-3),
                lmath.log(5e-3),
                lmath.add(lmath.log(1e-3), lmath.log(5e-3)),
            )
        )
        print(
            "log(1e-3 + 5e-3) = %e + %e = %e\n"
            % (
                lmath.exp(lmath.log(1e-3)),
                lmath.exp(lmath.log(5e-3)),
                lmath.exp(lmath.add(lmath.log(1e-3), lmath.log(5e-3))),
            )
        )
        self.assertLogEqual(
            lmath.add(lmath.log(1e-48), lmath.log(5e-48)), lmath.log(6e-48)
        )
        self.assertLogEqual(lmath.add(lmath.log(1e-48), lmath.log(42)), lmath.log(42))


if __name__ == "__main__":
    unittest.main()
