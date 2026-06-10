#!/usr/bin/env python3
# Copyright (c) Meta Platforms, Inc. and affiliates.
# Licensed under the Apache License, Version 2.0

# pyre-strict

"""Check that benchmark files live under benchmarks/ directories.

The CMake build uses a path-based convention to identify benchmark files:
anything under a benchmarks/ directory is only built when BENCHMARKS=ON.
Benchmark files placed elsewhere (e.g. in test/ or tests/) get
misclassified as regular executables or library files, causing linker
failures because they depend on Folly::follybenchmark.

Usage:
    python3 tools/check_benchmark_paths.py [directories...]

    # Scan default directories (algopt/)
    python3 tools/check_benchmark_paths.py

    # Scan specific directories
    python3 tools/check_benchmark_paths.py algopt/lp algopt/rebalancer
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

# Directories to exclude from scanning
EXCLUDE_DIRS: frozenset[str] = frozenset({"fb", "build", "third-party", "third_party"})

# Patterns that indicate a file is a benchmark (must use benchmark macros,
# not merely include the header — test files may include folly/Benchmark.h
# for utilities like BENCHMARK_SUSPEND without being benchmarks themselves)
BENCHMARK_PATTERNS: list[re.Pattern[str]] = [
    re.compile(r"\bBENCHMARK\s*\("),
    re.compile(r"\bBENCHMARK_NAMED_PARAM\s*\("),
    re.compile(r"\bBENCHMARK_RELATIVE\s*\("),
    re.compile(r"\bBENCHMARK_DRAW_LINE\s*\("),
    re.compile(r"\bfolly::runBenchmarks\s*\("),
]


def is_benchmark_file(filepath: Path) -> bool:
    """Check if a file contains benchmark code."""
    try:
        content = filepath.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return False

    return any(pattern.search(content) for pattern in BENCHMARK_PATTERNS)


def is_in_benchmarks_dir(filepath: Path, root: Path) -> bool:
    """Check if a file is under a benchmarks/ directory."""
    return "benchmarks" in filepath.relative_to(root).parts


def scan_directory(root: Path) -> list[Path]:
    """Find benchmark files that are not under a benchmarks/ directory."""
    extensions = frozenset({".cpp", ".cc", ".cxx"})
    misplaced: list[Path] = []

    for filepath in sorted(root.rglob("*")):
        if not filepath.is_file():
            continue
        if filepath.suffix not in extensions:
            continue
        if any(part in EXCLUDE_DIRS for part in filepath.relative_to(root).parts):
            continue
        if is_in_benchmarks_dir(filepath, root):
            continue
        if is_benchmark_file(filepath):
            misplaced.append(filepath)

    return misplaced


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Check that benchmark files are in benchmarks/ directories"
    )
    parser.add_argument(
        "directories",
        nargs="*",
        default=["algopt"],
        help="Directories to scan (default: algopt/)",
    )

    args = parser.parse_args()

    all_misplaced: list[Path] = []
    for directory in args.directories:
        root = Path(directory)
        if not root.is_dir():
            print(f"Warning: {directory} is not a directory, skipping", file=sys.stderr)
            continue
        all_misplaced.extend(scan_directory(root))

    if all_misplaced:
        print(
            "Benchmark files must be under a benchmarks/ directory.\n"
            "The following files contain benchmark code but are not:\n",
            file=sys.stderr,
        )
        for filepath in all_misplaced:
            print(f"  {filepath}", file=sys.stderr)
        print(
            f"\n{len(all_misplaced)} misplaced benchmark file(s) found.\n"
            "Move them into a benchmarks/ directory so the build system\n"
            "correctly gates them behind BENCHMARKS=ON.",
            file=sys.stderr,
        )
        return 1

    print("All benchmark files are in benchmarks/ directories.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
