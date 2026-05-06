#!/usr/bin/env python
"""Convert PocketSphinx JSON alignment output to Praat TextGrid format.

This utility reads JSON output from PocketSphinx (e.g., from the `align`
command) and produces a Praat TextGrid file with tiers for words, phones,
and/or states depending on what alignment levels are present in the input.

Example usage:

    # Convert alignment JSON to TextGrid (stdout)
    pocketsphinx align audio.wav "hello world" | pocketsphinx_to_textgrid

    # Write to file
    pocketsphinx -phone_align yes align audio.wav "hello" | pocketsphinx_to_textgrid -o out.TextGrid

    # Open result in Praat with audio
    pocketsphinx -phone_align yes align audio.wav "hello" | pocketsphinx_to_textgrid -o out.TextGrid -w audio.wav --praat

The input JSON format (from pocketsphinx align):
    {
      "b": 0.0,           // begin time (seconds)
      "d": 1.5,           // duration (seconds)
      "t": "hello world", // text
      "w": [              // word segments (with -phone_align: nested phones)
        {"b": 0.1, "d": 0.4, "t": "hello", "w": [...]},
        ...
      ]
    }

The output TextGrid has one tier per alignment level present:
    - "words" tier (always present if alignment succeeded)
    - "phones" tier (if -phone_align was used)
    - "states" tier (if -state_align was used)
"""

import argparse
import json
import os
import subprocess
import sys
from typing import Any, Dict, List, Optional, TextIO, Tuple


def _escape_text(text: str) -> str:
    """Escape text for TextGrid format (double quotes become two double quotes)."""
    return text.replace('"', '""')


def _write_interval_tier(
    f: TextIO,
    name: str,
    xmin: float,
    xmax: float,
    intervals: List[Tuple[float, float, str]],
) -> None:
    """Write an IntervalTier to a TextGrid file."""
    print('        class = "IntervalTier"', file=f)
    print('        name = "%s"' % _escape_text(name), file=f)
    print("        xmin = %g" % xmin, file=f)
    print("        xmax = %g" % xmax, file=f)
    print("        intervals: size = %d" % len(intervals), file=f)
    for i, (imin, imax, text) in enumerate(intervals, 1):
        print("        intervals [%d]:" % i, file=f)
        print("            xmin = %g" % imin, file=f)
        print("            xmax = %g" % imax, file=f)
        print('            text = "%s"' % _escape_text(text), file=f)


def _collect_intervals(
    segments: List[Dict[str, Any]], parent_xmax: float
) -> List[Tuple[float, float, str]]:
    """Collect intervals from segments, filling gaps with empty intervals."""
    intervals: List[Tuple[float, float, str]] = []
    last_end = 0.0
    epsilon = 0.001  # 1ms threshold for gaps
    for seg in segments:
        start = seg["b"]
        end = start + seg["d"]
        if start > last_end + epsilon:
            intervals.append((last_end, start, ""))
        intervals.append((start, end, seg["t"]))
        last_end = end
    if last_end + epsilon < parent_xmax:
        intervals.append((last_end, parent_xmax, ""))
    return intervals


def json_to_textgrid(data: Dict[str, Any], f: Optional[TextIO] = None) -> None:
    """Convert PocketSphinx JSON alignment to Praat TextGrid format.

    The input JSON should have the structure produced by PocketSphinx:
      - b: begin time in seconds
      - d: duration in seconds
      - t: text (utterance or segment)
      - w: array of word segments, each optionally containing nested
           phone segments (with -phone_align), which may contain
           state segments (with -state_align)

    The output TextGrid will have one tier for each level present:
    words, phones, and/or states.

    Args:
        data: Parsed JSON dict from PocketSphinx output.
        f: Output file object (default: sys.stdout).
    """
    if f is None:
        f = sys.stdout

    xmin = data["b"]
    xmax = xmin + data["d"]

    tiers: List[Tuple[str, List[Tuple[float, float, str]]]] = []
    words = data.get("w", [])
    if words:
        tiers.append(("words", _collect_intervals(words, xmax)))
        phones: List[Dict[str, Any]] = []
        states: List[Dict[str, Any]] = []
        for word in words:
            if "w" in word:
                phones.extend(word["w"])
                for phone in word["w"]:
                    if "w" in phone:
                        states.extend(phone["w"])
        if phones:
            tiers.append(("phones", _collect_intervals(phones, xmax)))
        if states:
            tiers.append(("states", _collect_intervals(states, xmax)))

    print('File type = "ooTextFile"', file=f)
    print('Object class = "TextGrid"', file=f)
    print(file=f)
    print("xmin = %g" % xmin, file=f)
    print("xmax = %g" % xmax, file=f)
    print("tiers? <exists>", file=f)
    print("size = %d" % len(tiers), file=f)
    print("item []:", file=f)
    for i, (name, intervals) in enumerate(tiers, 1):
        print("    item [%d]:" % i, file=f)
        _write_interval_tier(f, name, xmin, xmax, intervals)


def open_praat(wav_path: str, textgrid_path: str, quiet: bool = False) -> bool:
    """Open Praat with the given wav and TextGrid files.

    Opens both files in Praat. Select both objects in Praat's object
    window and click "View & Edit" to see them together.

    Args:
        wav_path: Path to the audio file.
        textgrid_path: Path to the TextGrid file.
        quiet: If True, suppress informational messages.

    Returns:
        True if Praat was launched, False if not found.
    """
    wav_path = os.path.abspath(wav_path)
    textgrid_path = os.path.abspath(textgrid_path)

    if sys.platform == "darwin":
        if not os.path.exists("/Applications/Praat.app"):
            if not quiet:
                print("Praat not found at /Applications/Praat.app", file=sys.stderr)
            return False
        cmd = ["open", "-a", "Praat", wav_path, textgrid_path]
    elif sys.platform == "win32":
        cmd = ["Praat.exe", "--open", wav_path, textgrid_path]
    else:
        cmd = ["praat", "--open", wav_path, textgrid_path]

    if not quiet:
        from shlex import join as shlex_join
        print(shlex_join(cmd), file=sys.stderr)
        print("Select both objects in Praat and click 'View & Edit'", file=sys.stderr)

    try:
        subprocess.Popen(cmd)
        return True
    except FileNotFoundError:
        if not quiet:
            print("Praat not found", file=sys.stderr)
        return False


def main() -> None:
    """Command-line interface for JSON to TextGrid conversion."""
    parser = argparse.ArgumentParser(
        description="Convert PocketSphinx JSON alignment to Praat TextGrid.",
        epilog="""\
examples:
  pocketsphinx align audio.wav "hello" | %(prog)s
  pocketsphinx align audio.wav "hello" | %(prog)s -o output.TextGrid
  pocketsphinx -phone_align yes align audio.wav "hello" | %(prog)s -o out.TextGrid -w audio.wav --praat
""",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "input",
        nargs="?",
        type=argparse.FileType("r"),
        default=sys.stdin,
        help="input JSON file (default: stdin)",
    )
    parser.add_argument(
        "-o",
        "--output",
        type=str,
        default=None,
        help="output TextGrid file (default: stdout)",
    )
    parser.add_argument(
        "-w",
        "--wav",
        type=str,
        metavar="FILE",
        help="audio file to open alongside the TextGrid",
    )
    parser.add_argument(
        "-p",
        "--praat",
        action="store_true",
        help="open result in Praat (requires -w and -o)",
    )
    parser.add_argument(
        "-q",
        "--quiet",
        action="store_true",
        help="suppress informational messages",
    )
    args = parser.parse_args()

    if args.praat and not args.wav:
        parser.error("--praat requires --wav")
    if args.praat and not args.output:
        parser.error("--praat requires --output")

    data = json.load(args.input)

    if args.output:
        with open(args.output, "w") as f:
            json_to_textgrid(data, f)
    else:
        json_to_textgrid(data, sys.stdout)

    if args.praat:
        open_praat(args.wav, args.output, args.quiet)


if __name__ == "__main__":
    main()
