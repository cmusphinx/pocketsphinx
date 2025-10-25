#!/usr/bin/env python

import argparse
import re
import sys
import unicodedata as ud
from collections import defaultdict
from datetime import datetime
from io import StringIO
from math import log, exp
from typing import Any, Dict, Optional, TextIO, List

# Author: Kevin Lenzo
# Based, once upon a time, on a Perl script by Alex Rudnicky


class ArpaBoLM:
    """
    Builds statistical language models that can predict the next word in a sequence.

    A language model learns patterns from text to predict what word comes next.
    For example, after seeing "the quick brown", it might predict "fox" is likely.

    This class supports multiple smoothing methods (Good-Turing, Katz backoff, Kneser-Ney)
    to handle unseen word combinations. Methods like Katz backoff use "backing off" to
    shorter sequences when an n-gram hasn't been seen before.

    Key features:
    - Supports arbitrary n-gram orders (default: trigrams for better context)
    - Multiple smoothing methods (Good-Turing, Katz backoff, Kneser-Ney)
    - Good-Turing smoothing by default (best for typical sparse data)
    - Outputs standard ARPA format language models

    Smoothing methods:
    - "good_turing": Good-Turing smoothing (default, best for sparse data)
    - "auto": Optimizes Katz backoff discount mass automatically
    - "fixed": Uses fixed discount mass for Katz backoff (use -d 0.0 for MLE)
    - "kneser_ney": Kneser-Ney smoothing with continuation counts

    Example:
        lm = ArpaBoLM()
        lm.read_corpus(open("corpus.txt"))
        lm.compute()
        lm.write_file("model.arpa")

    Normalization:
        - unicode_norm (default: True): Applies NFC normalization to entire lines
        - token_norm (default: False): Strips punctuation/symbols from tokens

    Binary Format:
        This tool outputs ARPA text format. To convert to binary format for
        use with PocketSphinx, use the pocketsphinx_lm_convert tool:

        $ pocketsphinx_lm_convert -i model.arpa -o model.lm.bin
    """

    # Mathematical constants
    LOG10 = log(10.0)  # Used for converting probabilities to log base 10

    # Unicode categories to remove during text normalization (punctuation, symbols, etc.)
    NORM_EXCLUDE_CATEGORIES = {"P", "S", "C", "M", "Z"}

    # Default values
    DEFAULT_DISCOUNT_MASS = 0.3
    DEFAULT_DISCOUNT_STEP = 0.05
    MAX_DISCOUNT_MASS = 1.0
    KNESER_NEY_DISCOUNT = 0.75

    @staticmethod
    def _get_default_discount_mass(max_order: int) -> float:
        """Get default discount mass based on n-gram order.

        Higher-order n-grams are more sparse and need more smoothing.
        """
        # Base discount mass increases with order (more smoothing for higher orders)
        # Order 1 (unigrams): 0.1, Order 2 (bigrams): 0.2, Order 3 (trigrams): 0.3
        return min(0.1 * max_order, 0.5)

    @staticmethod
    def _make_nested_defaultdict(depth: int) -> Any:
        """Create a nested defaultdict with the specified depth."""
        if depth == 0:
            return defaultdict(int)
        return defaultdict(lambda: ArpaBoLM._make_nested_defaultdict(depth - 1))

    def __init__(
        self,
        add_start: bool = True,
        word_file: Optional[str] = None,
        word_file_count: int = 1,
        discount_mass: Optional[float] = None,
        discount_step: float = 0.05,
        case: Optional[str] = None,
        unicode_norm: bool = True,
        token_norm: bool = False,
        verbose: bool = False,
        max_order: int = 3,
        smoothing_method: str = "good_turing",
    ):
        self.add_start = add_start
        self.word_file = word_file
        self.word_file_count = word_file_count
        self.discount_mass = discount_mass
        self.case = case
        self.unicode_norm = unicode_norm
        self.token_norm = token_norm
        self.verbose = verbose
        self.max_order = max_order
        self.smoothing_method = smoothing_method

        if not 0.0 < discount_step < self.MAX_DISCOUNT_MASS:
            raise AttributeError(f"Discount step {discount_step} out of range (0.0, {self.MAX_DISCOUNT_MASS})")
        self.discount_step = discount_step

        self.logfile = sys.stderr
        self.start_time = datetime.now()

        if self.verbose:
            print("Started", self.start_time.strftime("%Y-%m-%d %H:%M:%S"), file=self.logfile)

        self._init_smoothing()

        self.deflator = 1.0 if self.discount_mass is None else 1.0 - self.discount_mass

        self.sent_count = 0
        self.grams: List[Any] = []
        self.counts: List[int] = []
        self.probs: List[Any] = []
        self.alphas: List[Any] = []

        for order in range(self.max_order):
            if order == 0:
                self.grams.append(defaultdict(int))
                self.probs.append({})
                self.alphas.append({})
            else:
                self.grams.append(self._make_nested_defaultdict(order))
                self.probs.append(self._make_nested_defaultdict(order))
                self.alphas.append(self._make_nested_defaultdict(order))
            self.counts.append(0)

        self.sum_1: int = 0

    def _init_smoothing(self) -> None:
        """Initialize smoothing parameters based on chosen method"""
        if self.smoothing_method == "fixed":
            if self.discount_mass is None:
                self.discount_mass = self._get_default_discount_mass(self.max_order)
            elif not 0.0 <= self.discount_mass < self.MAX_DISCOUNT_MASS:
                raise AttributeError(
                    f"Discount mass {self.discount_mass} out of range [0.0, {self.MAX_DISCOUNT_MASS})"
                )
            return

        if self.smoothing_method == "auto":
            self.discount_mass = self._get_default_discount_mass(self.max_order)
            return

        if self.smoothing_method in ["good_turing", "kneser_ney"]:
            self.discount_mass = None
            return

        raise AttributeError(f"Unknown smoothing method: {self.smoothing_method}")

    def _optimize_discount_mass(self) -> None:
        """Find the best discount mass by testing different values and picking the one that works best"""
        if self.smoothing_method != "auto":
            return

        if self.verbose:
            print("Optimizing discount mass...", file=self.logfile)

        candidates = [round(i * self.discount_step, 3) for i in range(1, int(self.MAX_DISCOUNT_MASS / self.discount_step))]
        best_mass = self.DEFAULT_DISCOUNT_MASS
        best_likelihood = float('-inf')

        for mass in candidates:
            self.discount_mass = mass
            self.deflator = 1.0 - mass

            likelihood = self._compute_likelihood()

            if likelihood > best_likelihood:
                best_likelihood = likelihood
                best_mass = mass

        self.discount_mass = best_mass
        self.deflator = 1.0 - best_mass

        if self.verbose:
            print(f"Optimal discount mass: {best_mass:.3f} ({len(candidates)} candidates tested)", file=self.logfile)

    def _compute_likelihood(self) -> float:
        """Calculate how well our language model predicts the words in our corpus"""
        if self.sum_1 == 0:
            return 0.0

        likelihood = 0.0
        for word, count in self.grams[0].items():
            if count > 0:
                prob = count / self.sum_1
                likelihood += count * log(prob)

        return likelihood

    def _compute_frequency_of_frequencies(self, gram_dict: Any, order: int) -> Dict[int, int]:
        """Compute frequency of frequencies: how many n-grams appear c times.

        Returns:
            Dict mapping count -> number of n-grams with that count
        """
        freq_of_freq = defaultdict(int)

        def count_frequencies(d: Any, current_order: int) -> None:
            if current_order == 0:
                for word, count in d.items():
                    freq_of_freq[count] += 1
            else:
                for word, value in d.items():
                    count_frequencies(value, current_order - 1)

        count_frequencies(gram_dict, order)
        return dict(freq_of_freq)

    def _compute_good_turing_probabilities(self) -> None:
        """Compute probabilities using Good-Turing smoothing

        Good-Turing re-estimates counts using frequency-of-frequencies:
        c* = (c+1) * N(c+1) / N(c)
        where N(c) is the number of n-grams that appear exactly c times.
        """
        for order in range(self.max_order):
            if order == 0:
                freq_of_freq = self._compute_frequency_of_frequencies(self.grams[0], 0)
                total_count = sum(self.grams[0].values())

                n1 = freq_of_freq.get(1, 0)
                total_adjusted = total_count

                if n1 > 0:
                    total_adjusted = total_count - n1 / 2.0

                for word, count in self.grams[0].items():
                    if count + 1 in freq_of_freq and count in freq_of_freq and freq_of_freq[count] > 0:
                        adjusted_count = (count + 1) * freq_of_freq[count + 1] / freq_of_freq[count]
                    else:
                        adjusted_count = count

                    prob = adjusted_count / total_adjusted if total_adjusted > 0 else 0.0
                    prob = min(prob, 1.0)
                    self.probs[0][word] = prob
            else:
                self._compute_order_probabilities_good_turing(order)

    def _compute_order_probabilities_good_turing(self, order: int) -> None:
        """Compute probabilities for n-grams of a given order using Good-Turing"""
        freq_of_freq = self._compute_frequency_of_frequencies(self.grams[order], order)

        def process_ngrams(gram_dict: Any, parent_words: List[str], current_order: int) -> None:
            if current_order == 0:
                total_count = sum(gram_dict.values())

                for word, count in gram_dict.items():
                    ngram_words = parent_words + [word]

                    if count + 1 in freq_of_freq and count in freq_of_freq and freq_of_freq[count] > 0:
                        adjusted_count = (count + 1) * freq_of_freq[count + 1] / freq_of_freq[count]
                    else:
                        adjusted_count = count

                    prob = adjusted_count / total_count if total_count > 0 else 0.0
                    prob = min(prob, 1.0)
                    self._set_ngram_prob(self.probs[order], ngram_words, prob)
            else:
                for word, value in gram_dict.items():
                    process_ngrams(value, parent_words + [word], current_order - 1)

        process_ngrams(self.grams[order], [], order)

    def _compute_continuation_counts(self, order: int) -> Any:
        """Compute continuation counts: how many different contexts each word appears in.

        For Kneser-Ney, we need to know: in how many unique contexts does this word appear?
        This is different from raw counts.
        """
        if order == 0:
            continuation_counts = defaultdict(int)
            if self.max_order > 1:
                for w1, bigrams in self.grams[1].items():
                    for w2 in bigrams.keys():
                        continuation_counts[w2] += 1
            return continuation_counts

        continuation_counts = self._make_nested_defaultdict(order - 1)

        def count_contexts(gram_dict: Any, parent_words: List[str], current_order: int) -> None:
            if current_order == 0:
                for word in gram_dict.keys():
                    ngram_words = parent_words + [word]
                    suffix = ngram_words[1:]
                    if suffix:
                        current = continuation_counts
                        for w in suffix[:-1]:
                            current = current[w]
                        current[suffix[-1]] += 1
            else:
                for word, value in gram_dict.items():
                    count_contexts(value, parent_words + [word], current_order - 1)

        count_contexts(self.grams[order], [], order)
        return continuation_counts

    def _compute_kneser_ney_probabilities(self) -> None:
        """Compute probabilities using Kneser-Ney smoothing with absolute discounting.

        Kneser-Ney uses:
        - Absolute discounting: subtract D from all counts
        - Continuation counts: for lower orders, use unique context counts
        - Interpolation: blend with lower-order probabilities
        """
        discount = self.KNESER_NEY_DISCOUNT

        for order in range(self.max_order):
            if order == 0:
                continuation_counts = self._compute_continuation_counts(0)
                total_continuations = sum(continuation_counts.values()) if continuation_counts else self.sum_1

                for word in self.grams[0].keys():
                    if continuation_counts and word in continuation_counts:
                        prob = continuation_counts[word] / total_continuations
                    else:
                        prob = self.grams[0][word] / self.sum_1
                    self.probs[0][word] = prob
            else:
                self._compute_order_probabilities_kneser_ney(order, discount)

    def _compute_order_probabilities_kneser_ney(self, order: int, discount: float) -> None:
        """Compute Kneser-Ney probabilities for n-grams with absolute discounting"""
        def process_ngrams(gram_dict: Any, parent_words: List[str], current_order: int) -> None:
            if current_order == 0:
                total_count = sum(gram_dict.values())
                num_types = len(gram_dict)

                for word, count in gram_dict.items():
                    ngram_words = parent_words + [word]
                    discounted_count = max(count - discount, 0.0)
                    interpolation_weight = (discount * num_types) / total_count if total_count > 0 else 0.0

                    prob = (discounted_count / total_count) if total_count > 0 else 0.0
                    self._set_ngram_prob(self.probs[order], ngram_words, prob)
            else:
                for word, value in gram_dict.items():
                    process_ngrams(value, parent_words + [word], current_order - 1)

        process_ngrams(self.grams[order], [], order)

    def _read_input_data(self, files: List[str], text_strings: Optional[List[str]] = None, word_file: Optional[str] = None) -> None:
        """Helper to read input files, text strings, and word file"""
        for filepath in files:
            with open(filepath, "rt") as f:
                self.read_corpus(f)

        if text_strings:
            for text in text_strings:
                self.read_corpus(StringIO(text))

        if word_file:
            self.read_word_file(word_file)

    def read_word_file(self, path: str, count: Optional[int] = None) -> None:
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
                if self.unicode_norm:
                    token = ud.normalize('NFC', token)
                if self.case == "lower":
                    token = token.lower()
                elif self.case == "upper":
                    token = token.upper()
                if self.token_norm:
                    token = self.norm_token(token)
                token_count += 1
                if token not in self.grams[0]:
                    self.grams[0][token] = count
                    new_word_count += 1

        if self.verbose:
            print(
                f"{new_word_count} new unique words",
                f"from {token_count} tokens,",
                f"each with count {count}",
                file=self.logfile,
            )

    def norm_token(self, token: str) -> str:
        """
        Normalize token: strip leading/trailing punctuation/symbols
        """
        if token in ('<s>', '</s>'):
            return token

        while token and ud.category(token[0])[0] in self.NORM_EXCLUDE_CATEGORIES:
            token = token[1:]
        while token and ud.category(token[-1])[0] in self.NORM_EXCLUDE_CATEGORIES:
            token = token[:-1]
        return token

    def _process_corpus_line(self, line: str, add_markers: bool) -> Optional[List[str]]:
        """Process a single line from corpus: normalize, clean, add markers.

        Returns:
            List of words, or None if line should be skipped
        """
        # Unicode normalize the whole line first (default on)
        if self.unicode_norm:
            line = ud.normalize('NFC', line)

        if self.case == "lower":
            line = line.lower()
        elif self.case == "upper":
            line = line.upper()

        line = line.strip()

        # Determine if line has markers:
        # 1. Sphinx sentfile format: <s> .* </s> (filename) - strip parens, has markers
        # 2. Text with markers: <s> .* </s> - has markers
        # 3. Plain text without markers - needs markers added
        has_markers = False
        if re.search(r'^<s>.*</s>\s+\(.*\)$', line):
            # Sphinx sentfile format - strip the (filename) part
            line = re.sub(r'\s+\(.*\)$', '', line)
            has_markers = True
        elif line.startswith('<s>') and line.endswith('</s>'):
            # Text with markers (but not sentfile format)
            has_markers = True

        words = line.split()
        if not words:
            return None

        # Add markers if needed and requested
        if add_markers and not has_markers:
            words = [w for w in words if w not in ('<s>', '</s>')]
            if not words:
                return None
            words = ["<s>"] + words + ["</s>"]

        if self.token_norm:
            # Normalize individual tokens (strip punctuation, etc.)
            words = [self.norm_token(w) for w in words]
            words = [w for w in words if len(w)]
            if not words:
                return None

        return words

    def read_corpus(self, infile):
        """
        Read in a text training corpus from a file handle.
        Handles three formats automatically per line:
        - Sphinx sentfile: <s> .* </s> (filename)
        - Text with markers: <s> .* </s>
        - Plain text without markers
        """
        if self.verbose:
            print("Reading corpus file, breaking per newline.", file=self.logfile)

        sent_count = 0
        for line in infile:
            words = self._process_corpus_line(line, self.add_start)
            if words is None:
                continue

            sent_count += 1
            wc = len(words)

            for j in range(wc):
                for order in range(min(self.max_order, wc - j)):
                    if order == 0:
                        self.grams[0][words[j]] += 1
                    else:
                        ngram_words = words[j:j + order + 1]
                        self._increment_ngram(self.grams[order], ngram_words)

        self.sent_count += sent_count
        if self.verbose:
            print(f"{sent_count} sentences", file=self.logfile)

    def _traverse_ngrams(self, gram_dict: Any, order: int, callback: Any) -> None:
        """Generic helper to traverse nested n-gram structure and apply a callback.

        Args:
            gram_dict: The nested defaultdict to traverse
            order: Current order (depth) to traverse
            callback: Function called with (gram_dict, parent_words, current_order)
                     at each level
        """
        def traverse(d: Any, parent_words: List[str], current_order: int) -> None:
            if current_order == 0:
                callback(d, parent_words, 0)
            else:
                for word, value in (sorted(d.items()) if isinstance(d, dict) else d.items()):
                    traverse(value, parent_words + [word], current_order - 1)

        traverse(gram_dict, [], order)

    def _increment_ngram(self, gram_dict: Any, ngram_words: List[str]) -> None:
        """Helper method to increment n-gram counts in nested defaultdict structure"""
        current = gram_dict
        for word in ngram_words[:-1]:
            current = current[word]
        current[ngram_words[-1]] += 1

    def compute(self) -> None:
        """
        Compute language model probabilities using Katz backoff smoothing.

        This method builds a statistical language model that can predict the next word
        in a sequence. It uses Katz backoff smoothing to handle unseen word combinations.

        How it works:
        1. Count how often each word appears (unigrams)
        2. Count how often each pair of words appears (bigrams)
        3. Count how often each triple of words appears (trigrams)
        4. Calculate probabilities: P(word|previous_words)
        5. Use "backoff" when we haven't seen a word combination before

        Katz backoff smoothing:
        - If we've seen "the quick brown fox" before, use that probability
        - If we've only seen "quick brown fox" before, use that probability × backoff_weight
        - If we've only seen "brown fox" before, use that probability × backoff_weight
        - Continue backing off to shorter sequences until we find something we've seen

        The backoff weights (alphas) ensure probabilities still sum to 1.0.

        """
        if not self.grams[0]:
            sys.exit("No input?")

        self.sum_1 = sum(self.grams[0].values())

        for order in range(self.max_order):
            self.counts[order] = self._count_ngrams(self.grams[order], order)

        if self.smoothing_method == "auto":
            self._optimize_discount_mass()

        if self.smoothing_method == "good_turing":
            self._compute_good_turing_probabilities()
            for order in range(self.max_order - 1):
                self._compute_order_alphas(order)
        elif self.smoothing_method == "kneser_ney":
            self._compute_kneser_ney_probabilities()
            for order in range(self.max_order - 1):
                self._compute_order_alphas(order)
        else:
            for order in range(self.max_order):
                if order == 0:
                    for word, count in self.grams[0].items():
                        self.probs[0][word] = count / self.sum_1
                else:
                    self._compute_order_probabilities(order)
                    self._compute_order_alphas(order)

    def _count_ngrams(self, gram_dict: Any, order: int) -> int:
        """Count how many different n-grams we have of a given order"""
        if order == 0:
            return len(gram_dict)

        count = 0
        for key, value in gram_dict.items():
            if isinstance(value, dict):
                count += self._count_ngrams(value, order - 1)
            else:
                count += 1
        return count

    def _compute_order_probabilities(self, order: int) -> None:
        """Calculate probabilities for n-grams of a given order (e.g., bigrams, trigrams)"""
        def process_ngrams(gram_dict: Any, parent_words: List[str], current_order: int) -> None:
            if current_order == 0:
                # We've reached the final word in the n-gram
                for word, count in gram_dict.items():
                    ngram_words = parent_words + [word]
                    parent_count = self._get_parent_count(self.grams[order], ngram_words)
                    if parent_count > 0:
                        # Probability = (count × discount_factor) / parent_count
                        self._set_ngram_prob(self.probs[order], ngram_words,
                                           count * self.deflator / parent_count)
            else:
                # Keep traversing the nested dictionary structure
                for word, value in gram_dict.items():
                    process_ngrams(value, parent_words + [word], current_order - 1)

        process_ngrams(self.grams[order], [], order)

    def _compute_order_alphas(self, order: int) -> None:
        """Calculate backoff weights (alphas) for n-grams of a given order"""
        if self.discount_mass is None:
            return

        def process_alphas(gram_dict: Any, parent_words: List[str], current_order: int) -> None:
            if current_order == 0:
                if not parent_words:
                    return

                sum_denom = 0.0
                for word, count in gram_dict.items():
                    ngram_words = parent_words + [word]
                    if order + 1 < self.max_order:
                        next_prob = self._get_next_word_prob(ngram_words, order + 1)
                        sum_denom += next_prob

                if sum_denom >= 1.0:
                    alpha = 1.0
                else:
                    alpha = self.discount_mass / (1.0 - sum_denom)

                self._set_ngram_prob(self.alphas[order], parent_words, alpha)
            else:
                for word, value in gram_dict.items():
                    process_alphas(value, parent_words + [word], current_order - 1)

        process_alphas(self.grams[order], [], order)

    def _get_next_word_prob(self, context_words: List[str], order: int) -> float:
        """Calculate the probability of any next word given the current context"""
        if order >= self.max_order:
            return 0.0

        # Look for n-grams that start with this context
        gram_dict = self.grams[order]
        current = gram_dict
        for word in context_words:
            if word not in current:
                return 0.0
            current = current[word]

        # Sum probabilities of all possible next words
        total_prob = 0.0
        for word, count in current.items():
            ngram_words = context_words + [word]
            parent_count = self._get_parent_count(self.grams[order], ngram_words)
            if parent_count > 0:
                total_prob += count / parent_count
        return total_prob

    def _get_parent_count(self, gram_dict: Any, ngram_words: List[str]) -> int:
        """Get the count of the parent n-gram (needed for probability calculation)"""
        if len(ngram_words) == 1:
            return self.sum_1  # For unigrams, parent is total word count
        parent_words = ngram_words[:-1]
        return self._get_ngram_count(self.grams[len(parent_words) - 1], parent_words)

    def _get_ngram_count(self, gram_dict: Any, ngram_words: List[str]) -> int:
        """Get the count of a specific n-gram from our nested dictionary structure"""
        current = gram_dict
        for word in ngram_words[:-1]:
            if word not in current:
                return 0
            current = current[word]
        return current.get(ngram_words[-1], 0)

    def _set_ngram_prob(self, prob_dict: Any, ngram_words: List[str], prob: float) -> None:
        """Store a probability value in our nested dictionary structure"""
        current = prob_dict
        for word in ngram_words[:-1]:
            current = current[word]
        current[ngram_words[-1]] = prob

    def write_file(self, out_path: str) -> bool:
        """
        Write out the ARPAbo model to a file path
        """
        try:
            with open(out_path, "w") as outfile:
                self.write(outfile)
        except (OSError, IOError):
            return False
        return True

    def write(self, outfile: TextIO) -> None:
        """
        Write the ARPAbo model to a file handle
        """
        if self.verbose:
            print("Writing output file", file=self.logfile)

        corpus_stats = [
            f"{self.sent_count} sentences;",
            f"{self.sum_1} words,",
        ]

        for order in range(self.max_order):
            if self.counts[order] > 0:
                corpus_stats.append(f"{self.counts[order]} {order+1}-grams,")

        corpus_stats.extend([
            f"with fixed discount mass {self.discount_mass}",
            "with unicode normalization" if self.unicode_norm else "",
            "with token normalization" if self.token_norm else "",
        ])

        print("Corpus:", " ".join(corpus_stats), file=outfile)

        print(file=outfile)
        print("\\data\\", file=outfile)

        for order in range(self.max_order):
            if self.counts[order] > 0:
                print(f"ngram {order+1}={self.counts[order]}", file=outfile)
        print(file=outfile)

        for order in range(self.max_order):
            if self.counts[order] > 0:
                print(f"\\{order+1}-grams:", file=outfile)
                self._write_order_ngrams(outfile, order)
                print(file=outfile)

        print(file=outfile)
        print("\\end\\", file=outfile)
        if self.verbose:
            end_time = datetime.now()
            elapsed = end_time - self.start_time
            print(f"Finished {end_time.strftime('%Y-%m-%d %H:%M:%S')} (elapsed: {elapsed.total_seconds():.2f}s)", file=self.logfile)

    def _write_order_ngrams(self, outfile: TextIO, order: int) -> None:
        """Write n-grams of given order to output file"""
        def write_ngrams(gram_dict: Any, parent_words: List[str], current_order: int) -> None:
            if current_order == 0:
                for word, prob in sorted(gram_dict.items()):
                    if prob <= 0:
                        continue
                    ngram_words = parent_words + [word]
                    log_prob = log(prob) / self.LOG10

                    if order < self.max_order - 1:
                        alpha = self._get_ngram_prob(self.alphas[order], parent_words)
                        if isinstance(alpha, dict):
                            alpha = 1.0
                        if alpha > 0:
                            log_alpha = log(alpha) / self.LOG10
                        else:
                            log_alpha = 0.0
                        words_str = " ".join(ngram_words)
                        print(f"{log_prob:6.4f} {words_str} {log_alpha:6.4f}", file=outfile)
                    else:
                        words_str = " ".join(ngram_words)
                        print(f"{log_prob:6.4f} {words_str}", file=outfile)
            else:
                for word, value in sorted(gram_dict.items()):
                    write_ngrams(value, parent_words + [word], current_order - 1)

        write_ngrams(self.probs[order], [], order)

    def _get_ngram_prob(self, prob_dict: Any, ngram_words: List[str]) -> float:
        """Get probability for n-gram from nested structure"""
        current = prob_dict
        for word in ngram_words:
            if word not in current:
                return 0.0
            current = current[word]
        return current

    def _parse_arpa_ngram_line(self, line: str, current_order: int) -> None:
        """Parse a single n-gram line from ARPA format and store it."""
        parts = line.split()
        if len(parts) < 2:
            return

        log_prob = float(parts[0])
        prob = exp(log_prob * self.LOG10)

        try:
            alpha = float(parts[-1])
            words = parts[1:-1]
        except ValueError:
            alpha = 1.0
            words = parts[1:]

        if len(words) == 1:
            word = words[0]
            self.probs[0][word] = prob
            self.grams[0][word] = int(prob * 1000)
            self.sum_1 += int(prob * 1000)
        else:
            context_words = words[:-1]
            self._set_ngram_prob(self.probs[current_order], words, prob)

            if current_order < self.max_order - 1:
                self._set_ngram_prob(self.alphas[current_order], context_words, alpha)

            self._set_ngram_prob(self.grams[current_order], words, int(prob * 1000))

    @classmethod
    def from_arpa_file(cls, arpa_file: str, verbose: bool = False) -> 'ArpaBoLM':
        """
        Load an existing ARPA format language model from a file.

        This allows you to debug existing models without rebuilding them.
        """
        if verbose:
            print(f"Loading ARPA model from: {arpa_file}", file=sys.stderr)

        lm = cls(verbose=verbose)

        # Initialize data structures
        lm.max_order = 0
        lm.grams = []
        lm.probs = []
        lm.alphas = []
        lm.counts = []
        lm.sum_1 = 0

        with open(arpa_file, 'r') as f:
            lines = f.readlines()

        # Parse ARPA format
        current_order = -1
        reading_ngrams = False

        for line in lines:
            line = line.strip()
            if not line:
                continue

            if line == "\\data\\":
                reading_ngrams = False
                continue
            elif line.startswith("ngram "):
                parts = line.split("=")
                if len(parts) == 2:
                    order = int(parts[0].split()[1]) - 1
                    count = int(parts[1])

                    while len(lm.counts) <= order:
                        lm.counts.append(0)
                        if order == 0:
                            lm.grams.append(defaultdict(int))
                            lm.probs.append({})
                            lm.alphas.append({})
                        else:
                            lm.grams.append(lm._make_nested_defaultdict(order))
                            lm.probs.append(lm._make_nested_defaultdict(order))
                            lm.alphas.append(lm._make_nested_defaultdict(order))

                    lm.counts[order] = count
                    lm.max_order = max(lm.max_order, order + 1)

            elif line.startswith("\\") and line.endswith("-grams:"):
                order = int(line.split("-")[0][1:]) - 1
                current_order = order
                reading_ngrams = True
                continue

            elif reading_ngrams and current_order >= 0:
                lm._parse_arpa_ngram_line(line, current_order)

        if verbose:
            print(f"Loaded model with max order {lm.max_order}", file=sys.stderr)
            print(f"Vocabulary size: {len(lm.probs[0])}", file=sys.stderr)
            print(f"N-gram counts: {lm.counts}", file=sys.stderr)

        return lm

    def debug_sentence(self, sentence: str) -> None:
        """
        Interactive debug mode for stepping through a sentence with the language model.

        This method allows you to see how the model processes each word, including:
        - Word probabilities at different n-gram orders
        - Backoff weights (alphas) used
        - Which n-gram order was used for each prediction
        - Total probability of the sentence
        """
        words = sentence.strip().split()
        if not words:
            print("Empty sentence!")
            return

        print(f"Debugging sentence: '{sentence}'")
        print(f"Words: {words}")
        print("=" * 60)

        total_log_prob = 0.0

        for i, word in enumerate(words):
            print(f"\nStep {i+1}: Processing word '{word}'")
            print("-" * 40)

            # Try to find probability at each order, starting from highest
            found_prob = False
            used_order = -1

            for order in range(min(i, self.max_order - 1), -1, -1):
                if order == 0:
                    # Unigram probability
                    if word in self.probs[0]:
                        prob = self.probs[0][word]
                        print(f"  Found unigram probability: P({word}) = {prob:.6f}")
                        used_order = 0
                        found_prob = True
                        break
                    else:
                        print(f"  Word '{word}' not in unigram vocabulary")
                else:
                    # Higher-order n-gram
                    context_words = words[max(0, i-order):i]
                    ngram_words = context_words + [word]

                    # Check if this n-gram exists
                    prob = self._get_ngram_prob(self.probs[order], ngram_words)
                    if isinstance(prob, dict):
                        prob = None

                    if prob is not None and prob > 0:
                        print(f"  Found {order+1}-gram probability: P({word}|{' '.join(context_words)}) = {prob:.6f}")

                        # Show backoff weights for lower orders
                        if order > 0:
                            alpha = self._get_ngram_prob(self.alphas[order-1], context_words)
                            if isinstance(alpha, dict):
                                alpha = 1.0
                            print(f"  Backoff weight α({order}) = {alpha:.6f}")

                        used_order = order
                        found_prob = True
                        break
                    else:
                        print(f"  {order+1}-gram '{' '.join(ngram_words)}' not found")

            if found_prob:
                log_prob = log(prob) if prob > 0 else float('-inf')
                total_log_prob += log_prob
                print(f"  → Using order {used_order+1}, log probability = {log_prob:.6f}")
            else:
                print(f"  → Word '{word}' not found in any n-gram order!")
                print(f"  → Using OOV (out-of-vocabulary) probability")
                # Use a small probability for unseen words
                oov_prob = 1.0 / (self.sum_1 + 1)  # Laplace smoothing
                log_prob = log(oov_prob)
                total_log_prob += log_prob
                print(f"  → OOV log probability = {log_prob:.6f}")

        print("\n" + "=" * 60)
        print(f"Total sentence log probability: {total_log_prob:.6f}")
        print(f"Total sentence probability: {exp(total_log_prob):.2e}")
        print(f"Perplexity: {exp(-total_log_prob / len(words)):.2f}")

        entropy, perplexity = self._compute_perplexity()
        print(f"\nModel statistics:")
        print(f"  Vocabulary size: {len(self.probs[0])}")
        print(f"  Total word count: {self.sum_1}")
        print(f"  Max order: {self.max_order}")
        print(f"  Smoothing method: {self.smoothing_method}")
        if self.discount_mass is not None:
            print(f"  Discount mass: {self.discount_mass:.3f}")
        print(f"  Unigram entropy: {entropy:.4f}")
        print(f"  Unigram perplexity: {perplexity:.2f}")

    def interactive_debug(self) -> None:
        """
        Start an interactive debug session where you can input sentences
        and step through them with the language model.
        """
        print("Language Model Interactive Debug Mode")
        print("=" * 50)
        print("Commands:")
        print("  <sentence>  - Debug a sentence")
        print("  /stats     - Show model statistics")
        print("  /vocab     - Show vocabulary")
        print("  /quit      - Exit debug mode")
        print()

        while True:
            try:
                command = input("debug> ").strip()

                if command.lower() in ['/quit', '/exit', '/q', 'quit', 'exit', 'q']:
                    print("Goodbye!")
                    break
                elif command.lower() in ['/stats', 'stats']:
                    entropy, perplexity = self._compute_perplexity()
                    print(f"\nModel Statistics:")
                    print(f"  Vocabulary size: {len(self.probs[0])}")
                    print(f"  Total word count: {self.sum_1}")
                    print(f"  Max order: {self.max_order}")
                    print(f"  Smoothing method: {self.smoothing_method}")
                    if self.discount_mass is not None:
                        print(f"  Discount mass: {self.discount_mass:.3f}")
                    print(f"  N-gram counts: {self.counts}")
                    print(f"  Unigram entropy: {entropy:.4f}")
                    print(f"  Unigram perplexity: {perplexity:.2f}")
                    print()
                elif command.lower() in ['/vocab', 'vocab']:
                    print(f"\nVocabulary ({len(self.probs[0])} words):")
                    vocab_words = sorted(self.probs[0].keys())
                    for i, word in enumerate(vocab_words):
                        if i % 5 == 0:
                            print()
                        print(f"{word:12}", end="")
                    print("\n")
                elif command:
                    self.debug_sentence(command)
                else:
                    print("Please enter a sentence to debug or a command")

            except KeyboardInterrupt:
                print("\nGoodbye!")
                break
            except Exception as e:
                print(f"Error: {e}")

    def _compute_perplexity(self) -> tuple[float, float]:
        """Compute unigram entropy and perplexity on training data.

        Returns:
            (entropy, perplexity) tuple based on unigram distribution
        """
        if self.sum_1 == 0:
            return 0.0, 1.0

        log_prob_sum = 0.0
        for word, count in self.grams[0].items():
            if word in self.probs[0] and self.probs[0][word] > 0:
                log_prob_sum += count * log(self.probs[0][word])

        entropy = -log_prob_sum / self.sum_1
        perplexity = exp(entropy)
        return entropy, perplexity

    def print_stats(self) -> None:
        """Print model statistics to stdout"""
        print("Model Statistics:")
        print("=" * 50)
        print(f"Vocabulary size: {len(self.probs[0])}")
        print(f"Total word count: {self.sum_1}")
        print(f"Max order: {self.max_order}")
        print(f"Smoothing method: {self.smoothing_method}")
        if self.discount_mass is not None:
            print(f"Discount mass: {self.discount_mass:.3f}")
        print(f"N-gram counts: {self.counts}")

        entropy, perplexity = self._compute_perplexity()
        print(f"Unigram entropy: {entropy:.4f}")
        print(f"Unigram perplexity: {perplexity:.2f}")

        print(f"\nVocabulary ({len(self.probs[0])} words):")
        vocab_words = sorted(self.probs[0].keys())
        for i, word in enumerate(vocab_words):
            if i % 5 == 0:
                print()
            print(f"{word:12}", end="")
        print("\n")


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Create an ARPA language model with smoothing (Katz backoff, Good-Turing, Kneser-Ney) and discount mass optimization",
        epilog="To convert ARPA text format to binary format for PocketSphinx, use: pocketsphinx_lm_convert -i model.arpa -o model.lm.bin"
    )
    parser.add_argument("files", nargs="*",
                        help="input text files")
    parser.add_argument("-t", "--text", type=str, action="append",
                        help="input text string (repeatable)")
    parser.add_argument("-w", "--word-file", type=str,
                        help="vocabulary file (count set by -C)")
    parser.add_argument("-C", "--word-file-count", type=int, default=1,
                        help="count for word file entries (default: 1)")
    parser.add_argument("-d", "--discount-mass", type=float,
                        help="discount mass [0.0, 1.0) for fixed method")
    parser.add_argument("--discount-step", type=float, default=0.05,
                        help="step size for auto optimization (default: 0.05)")
    parser.add_argument("-c", "--case", type=str,
                        help="case fold: lower or upper")
    parser.add_argument("--no-add-start", dest="add_start", action="store_false",
                        help="skip sentence markers (auto-detects sphinx format)")
    parser.add_argument("--no-unicode-norm", dest="unicode_norm", action="store_false",
                        help="disable unicode normalization (NFC) on lines (default: enabled)")
    parser.add_argument("-n", "--token-norm", dest="token_norm", action="store_true",
                        help="normalize tokens (strip punctuation/symbols)")
    parser.add_argument("-o", "--output", type=str,
                        help="output file (default: stdout)")
    parser.add_argument("-v", "--verbose", action="store_true",
                        help="verbose output to stderr")
    parser.add_argument("-m", "--max-order", type=int, default=3,
                        help="n-gram order (default: 3)")
    parser.add_argument("-s", "--smoothing-method", type=str, default="good_turing",
                        choices=["auto", "fixed", "mle", "good_turing", "kneser_ney"],
                        help="good_turing|auto|fixed|kneser_ney (-d 0.0 for MLE, default: good_turing)")
    parser.add_argument("--debug", action="store_true",
                        help="interactive debug mode")
    parser.add_argument("-l", "--load-model", type=str,
                        help="load and optionally update existing model")
    parser.add_argument("-S", "--stats", action="store_true",
                        help="show statistics and exit")

    args = parser.parse_args()

    if args.case and args.case not in ["lower", "upper"]:
        parser.error("--case must be lower or upper (if given)")

    if args.load_model:
        lm = ArpaBoLM.from_arpa_file(args.load_model, verbose=args.verbose)

        if args.files or args.text:
            if args.verbose:
                print("Updating loaded model with new training data...", file=sys.stderr)

            if args.case:
                lm.case = args.case
            if args.add_start is not None:
                lm.add_start = args.add_start
            if args.unicode_norm is not None:
                lm.unicode_norm = args.unicode_norm
            if args.token_norm is not None:
                lm.token_norm = args.token_norm
            if args.discount_step:
                lm.discount_step = args.discount_step

            lm._read_input_data(args.files, args.text, args.word_file)
            lm.compute()
    else:
        if not args.files and args.text is None:
            parser.error("Input must be specified with input files or --text")

        lm = ArpaBoLM(
            word_file=args.word_file,
            word_file_count=args.word_file_count,
            discount_mass=args.discount_mass,
            discount_step=args.discount_step,
            case=args.case,
            add_start=args.add_start,
            unicode_norm=args.unicode_norm,
            token_norm=args.token_norm,
            verbose=args.verbose,
            max_order=args.max_order,
            smoothing_method=args.smoothing_method,
        )

        lm._read_input_data(args.files, args.text, args.word_file)
        lm.compute()

    if args.stats:
        lm.print_stats()
        return

    if args.debug:
        lm.interactive_debug()
        return

    if args.output:
        with open(args.output, "w") as outfile:
            lm.write(outfile)
        return

    lm.write(sys.stdout)


if __name__ == "__main__":
    main()
