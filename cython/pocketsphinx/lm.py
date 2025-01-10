#!/usr/bin/env python

import argparse
import re
import sys
import unicodedata as ud
from collections import defaultdict
from datetime import date
from io import StringIO
from math import log
from typing import Any, Dict, Optional, TextIO

# Author: Kevin Lenzo
# Based on a Perl script by Alex Rudnicky


class ArpaBoLM:
    """
    A simple ARPA model builder
    """

    log10 = log(10.0)
    norm_exclude_categories = set(["P", "S", "C", "M", "Z"])

    def __init__(
        self,
        sentfile: Optional[TextIO] = None,
        text: Optional[str] = None,
        add_start: bool = False,
        word_file: Optional[str] = None,
        word_file_count: int = 1,
        discount_mass: float = 0.5,
        case: Optional[str] = None,  # lower, upper
        norm: bool = False,
        verbose: bool = False,
    ):
        self.add_start = add_start
        self.word_file = word_file
        self.word_file_count = word_file_count
        self.discount_mass = discount_mass
        self.case = case
        self.norm = norm
        self.verbose = verbose

        self.logfile = sys.stdout

        if self.verbose:
            print("Started", date.today(), file=self.logfile)

        if discount_mass is None:  # TODO: add other smoothing methods
            self.discount_mass = 0.5
        elif not 0.0 < discount_mass < 1.0:
            raise AttributeError(
                f"Discount value ({discount_mass}) out of range [0.0, 1.0]"
            )

        self.deflator: float = 1.0 - self.discount_mass

        self.sent_count = 0

        self.grams_1: Any = defaultdict(int)
        self.grams_2: Any = defaultdict(lambda: defaultdict(int))
        self.grams_3: Any = defaultdict(lambda: defaultdict(lambda: defaultdict(int)))

        self.sum_1: int = 0
        self.count_1: int = 0
        self.count_2: int = 0
        self.count_3: int = 0

        self.prob_1: Dict[str, float] = {}
        self.alpha_1: Dict[str, float] = {}
        self.prob_2: Any = defaultdict(lambda: defaultdict(float))
        self.alpha_2: Any = defaultdict(lambda: defaultdict(float))

        if sentfile is not None:
            self.read_corpus(sentfile)
        if text is not None:
            self.read_corpus(StringIO(text))

        if self.word_file is not None:
            self.read_word_file(self.word_file)

    def read_word_file(self, path: str, count: Optional[int] = None) -> bool:
        """
        Read in a file of words to add to the model,
        if not present, with the given count (default 1)
        """
        if self.verbose:
            print("Reading word file:", path, file=self.logfile)

        if count is None:
            count = self.word_file_count

        new_word_count = token_count = 0
        with open(path) as words_file:
            for token in words_file:
                token = token.strip()
                if not token:
                    continue
                if self.case == "lower":
                    token = token.lower()
                elif self.case == "upper":
                    token = token.upper()
                if self.norm:
                    token = self.norm_token(token)
                token_count += 1
                # Here, we could just add one, bumping all the word counts;
                # or just add N for the missing ones. We do the latter.
                if token not in self.grams_1:
                    self.grams_1[token] = count
                    new_word_count += 1

        if self.verbose:
            print(
                f"{new_word_count} new unique words",
                f"from {token_count} tokens,",
                f"each with count {count}",
                file=self.logfile,
            )
        return True

    def norm_token(self, token: str) -> str:
        """
        Remove excluded leading and trailing character categories from a token
        """
        while (
            len(token) and ud.category(token[0])[0] in ArpaBoLM.norm_exclude_categories
        ):
            token = token[1:]
        while (
            len(token) and ud.category(token[-1])[0] in ArpaBoLM.norm_exclude_categories
        ):
            token = token[:-1]
        return token

    def read_corpus(self, infile):
        """
        Read in a text training corpus from a file handle
        """
        if self.verbose:
            print("Reading corpus file, breaking per newline.", file=self.logfile)

        sent_count = 0
        for line in infile:
            if self.case == "lower":
                line = line.lower()
            elif self.case == "upper":
                line = line.upper()
            line = line.strip()
            line = re.sub(
                r"(.+)\(.+\)$", r"\1", line
            )  # trailing file name in transcripts

            words = line.split()
            if self.add_start:
                words = ["<s>"] + words + ["</s>"]
            if self.norm:
                words = [self.norm_token(w) for w in words]
                words = [w for w in words if len(w)]
            if not words:
                continue
            sent_count += 1
            wc = len(words)
            for j in range(wc):
                w1 = words[j]
                self.grams_1[w1] += 1
                if j + 1 < wc:
                    w2 = words[j + 1]
                    self.grams_2[w1][w2] += 1
                    if j + 2 < wc:
                        w3 = words[j + 2]
                        self.grams_3[w1][w2][w3] += 1

        if self.verbose:
            print(f"{sent_count} sentences", file=self.logfile)

    def compute(self) -> bool:
        """
        Compute all the things (derived values).

        If an n-gram is not present, the back-off is

            P( word_N | word_{N-1}, word_{N-2}, ...., word_1 ) =
                P( word_N | word_{N-1}, word_{N-2}, ...., word_2 )
                * backoff-weight(  word_{N-1} | word_{N-2}, ...., word_1 )

        If the sequence

            ( word_{N-1}, word_{N-2}, ...., word_1 )

        is also not listed, then the term

            backoff-weight( word_{N-1} | word_{N-2}, ...., word_1 )

        gets replaced with 1.0 and the recursion continues.

        """
        if not self.grams_1:
            sys.exit("No input?")
            return False

        # token counts
        self.sum_1 = sum(self.grams_1.values())

        # type counts
        self.count_1 = len(self.grams_1)
        for w1, gram2 in self.grams_2.items():
            self.count_2 += len(gram2)
            for w2 in gram2:
                self.count_3 += len(self.grams_3[w1][w2])

        # unigram probabilities
        for gram1, count in self.grams_1.items():
            self.prob_1[gram1] = count * self.deflator / self.sum_1

        # unigram alphas
        for w1 in self.grams_1:
            sum_denom = 0.0
            for w2, count in self.grams_2[w1].items():
                sum_denom += self.prob_1[w2]
            self.alpha_1[w1] = self.discount_mass / (1.0 - sum_denom)

        # bigram probabilities
        for w1, grams2 in self.grams_2.items():
            for w2, count in grams2.items():
                self.prob_2[w1][w2] = count * self.deflator / self.grams_1[w1]

        # bigram alphas
        for w1, grams2 in self.grams_2.items():
            for w2, count in grams2.items():
                sum_denom = 0.0
                for w3 in self.grams_3[w1][w2]:
                    sum_denom += self.prob_2[w2][w3]
                self.alpha_2[w1][w2] = self.discount_mass / (1.0 - sum_denom)
        return True

    def write_file(self, out_path: str) -> bool:
        """
        Write out the ARPAbo model to a file path
        """
        try:
            with open(out_path, "w") as outfile:
                self.write(outfile)
        except Exception:
            return False
        return True

    def write(self, outfile: TextIO) -> bool:
        """
        Write the ARPAbo model to a file handle
        """
        if self.verbose:
            print("Writing output file", file=self.logfile)

        print(
            "Corpus:",
            f"{self.sent_count} sentences;",
            f"{self.sum_1} words,",
            f"{self.count_1} 1-grams,",
            f"{self.count_2} 2-grams,",
            f"{self.count_3} 3-grams,",
            f"with fixed discount mass {self.discount_mass}",
            "with simple normalization" if self.norm else "",
            file=outfile,
        )

        print(file=outfile)
        print("\\data\\", file=outfile)

        print(f"ngram 1={self.count_1}", file=outfile)
        if self.count_2:
            print(f"ngram 2={self.count_2}", file=outfile)
        if self.count_3:
            print(f"ngram 3={self.count_3}", file=outfile)
        print(file=outfile)

        print("\\1-grams:", file=outfile)
        for w1, prob in sorted(self.prob_1.items()):
            log_prob = log(prob) / ArpaBoLM.log10
            log_alpha = log(self.alpha_1[w1]) / ArpaBoLM.log10
            print(f"{log_prob:6.4f} {w1} {log_alpha:6.4f}", file=outfile)

        if self.count_2:
            print(file=outfile)
            print("\\2-grams:", file=outfile)
            for w1, grams2 in sorted(self.prob_2.items()):
                for w2, prob in sorted(grams2.items()):
                    log_prob = log(prob) / ArpaBoLM.log10
                    log_alpha = log(self.alpha_2[w1][w2]) / ArpaBoLM.log10
                    print(f"{log_prob:6.4f} {w1} {w2} {log_alpha:6.4f}", file=outfile)
        if self.count_3:
            print(file=outfile)
            print("\\3-grams:", file=outfile)
            for w1, grams2 in sorted(self.grams_3.items()):
                for w2, grams3 in sorted(grams2.items()):
                    for w3, count in sorted(grams3.items()):  # type: ignore
                        prob = count * self.deflator / self.grams_2[w1][w2]
                        log_prob = log(prob) / ArpaBoLM.log10
                        print(f"{log_prob:6.4f} {w1} {w2} {w3}", file=outfile)

        print(file=outfile)
        print("\\end\\", file=outfile)
        if self.verbose:
            print("Finished", date.today(), file=self.logfile)

        return True


def main() -> None:
    parser = argparse.ArgumentParser(description="Create a fixed-backoff ARPA LM")
    parser.add_argument(
        "-s",
        "--sentfile",
        type=argparse.FileType("rt"),
        help="sentence transcripts in sphintrain style or one-per-line texts",
    )
    parser.add_argument("-t", "--text", type=str)
    parser.add_argument(
        "-w", "--word-file", type=str, help="add words from this file with count -C"
    )
    parser.add_argument(
        "-C",
        "--word-file-count",
        type=int,
        default=1,
        help="word count set for each word in --word-file (default 1)",
    )
    parser.add_argument(
        "-d", "--discount-mass", type=float, help="fixed discount mass [0.0, 1.0]"
    )
    parser.add_argument(
        "-c", "--case", type=str, help="fold case (values: lower, upper)"
    )
    parser.add_argument(
        "-a",
        "--add-start",
        action="store_true",
        help="add <s> at start, and at end of lines </s> for -s or -t",
    )
    parser.add_argument(
        "-n",
        "--norm",
        action="store_true",
        help="do rudimentary token normalization / remove punctuation",
    )
    parser.add_argument(
        "-o", "--output", type=str, help="output to this file (default stdout)"
    )
    parser.add_argument(
        "-v", "--verbose", action="store_true", help="extra log info (to stderr)"
    )

    args = parser.parse_args()

    if args.case and args.case not in ["lower", "upper"]:
        parser.error("--case must be lower or upper (if given)")

    if args.sentfile is None and args.text is None:
        parser.error("Input must be specified with --sentfile and/or --text")

    lm = ArpaBoLM(
        sentfile=args.sentfile,
        text=args.text,
        word_file=args.word_file,
        word_file_count=args.word_file_count,
        discount_mass=args.discount_mass,
        case=args.case,
        add_start=args.add_start,
        norm=args.norm,
        verbose=args.verbose,
    )
    lm.compute()

    if args.output:
        outfile: TextIO = open(args.output, "w")
    else:
        outfile = sys.stdout

    lm.write(outfile)


if __name__ == "__main__":
    main()
