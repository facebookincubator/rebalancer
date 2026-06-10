#!/usr/bin/env python3
# Copyright (c) Meta Platforms, Inc. and affiliates.
# Licensed under the Apache License, Version 2.0

# pyre-strict

"""Check C++ source files for identifiers that conflict with Windows macros.

Windows headers (winbase.h, wingdi.h, windef.h, etc.) define macros like
IGNORE, ERROR, DELETE, etc. that silently replace C++ identifiers, causing
cryptic MSVC build failures. This script scans source files and reports
any identifiers that match known problematic Windows macros.

Usage:
    python3 tools/check_windows_macros.py [directories...]

    # Scan default directories (algopt/)
    python3 tools/check_windows_macros.py

    # Scan specific directories
    python3 tools/check_windows_macros.py algopt/lp algopt/rebalancer
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

# Windows macros that are #define'd as simple tokens (not function-like macros)
# and commonly collide with C++ identifiers. Each entry is (macro_name, source_header).
# Excluded from this list (too pervasive as Thrift enum values):
#   ABSOLUTE (wingdi.h), RELATIVE (wingdi.h)
WINDOWS_MACROS: list[tuple[str, str]] = [
    ("ALTERNATE", "wingdi.h"),
    ("DELETE", "winnt.h"),
    ("DIFFERENCE", "wingdi.h"),
    ("DOMAIN", "math.h"),
    ("ERROR", "wingdi.h"),
    ("FAR", "windef.h"),
    ("IGNORE", "winbase.h"),
    ("NEAR", "windef.h"),
    ("OPAQUE", "wingdi.h"),
    ("OVERFLOW", "math.h"),
    ("STRICT", "windows.h"),
    ("TRANSPARENT", "wingdi.h"),
    ("UNDERFLOW", "math.h"),
]

MACRO_NAMES: frozenset[str] = frozenset(name for name, _ in WINDOWS_MACROS)

# Directories to exclude from scanning (relative to scan root)
EXCLUDE_DIRS: frozenset[str] = frozenset({"fb", "build", "third-party", "third_party"})

# Line-level suppression comment
SUPPRESSION_COMMENT: str = "NOLINT(windows-macro)"


def _strip_comments_and_strings(line: str) -> str:
    """Remove single-line comments and string literals from a line."""
    # Remove string literals (handles escaped quotes)
    line = re.sub(r'"(?:[^"\\]|\\.)*"', '""', line)
    # Remove single-line comments
    comment_pos = line.find("//")
    if comment_pos >= 0:
        line = line[:comment_pos]
    return line


def _is_preprocessor_directive(line: str) -> bool:
    """Check if line is a preprocessor directive that legitimately references macros."""
    stripped = line.lstrip()
    return stripped.startswith("#")


def scan_file(filepath: Path) -> list[tuple[int, str, str]]:
    """Scan a single file for Windows macro conflicts.

    Returns a list of (line_number, macro_name, line_text) tuples.
    """
    violations: list[tuple[int, str, str]] = []

    try:
        content = filepath.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return violations

    in_block_comment = False

    for line_num, raw_line in enumerate(content.splitlines(), start=1):
        line = raw_line

        # Handle suppression
        if SUPPRESSION_COMMENT in line:
            continue

        # Track block comments
        if in_block_comment:
            if "*/" in line:
                line = line[line.index("*/") + 2 :]
                in_block_comment = False
            else:
                continue

        # Check for block comment start
        while "/*" in line:
            start = line.index("/*")
            end = line.find("*/", start + 2)
            if end >= 0:
                line = line[:start] + line[end + 2 :]
            else:
                line = line[:start]
                in_block_comment = True
                break

        # Skip preprocessor directives
        if _is_preprocessor_directive(line):
            continue

        # Strip remaining comments and strings
        cleaned = _strip_comments_and_strings(line)

        # Search for macro names as whole words
        for macro_name in MACRO_NAMES:
            if re.search(rf"\b{macro_name}\b", cleaned):
                violations.append((line_num, macro_name, raw_line.rstrip()))

    return violations


def scan_directory(
    root: Path,
    extensions: frozenset[str] | None = None,
) -> dict[Path, list[tuple[int, str, str]]]:
    """Scan all C++ source files in a directory tree.

    Returns a dict mapping file paths to their violations.
    """
    if extensions is None:
        extensions = frozenset({".h", ".hpp", ".cpp", ".cc", ".cxx"})

    all_violations: dict[Path, list[tuple[int, str, str]]] = {}

    for filepath in sorted(root.rglob("*")):
        if not filepath.is_file():
            continue
        if filepath.suffix not in extensions:
            continue
        # Skip excluded directories
        if any(part in EXCLUDE_DIRS for part in filepath.relative_to(root).parts):
            continue

        violations = scan_file(filepath)
        if violations:
            all_violations[filepath] = violations

    return all_violations


def format_violations(
    violations: dict[Path, list[tuple[int, str, str]]],
    root: Path | None = None,
) -> str:
    """Format violations into a human-readable report."""
    macro_sources = dict(WINDOWS_MACROS)
    lines: list[str] = []
    total = 0

    for filepath, file_violations in sorted(violations.items()):
        rel_path = filepath.relative_to(root) if root else filepath
        for line_num, macro_name, line_text in file_violations:
            source = macro_sources.get(macro_name, "unknown")
            lines.append(
                f"{rel_path}:{line_num}: '{macro_name}' conflicts with "
                f"Windows macro from {source}"
            )
            lines.append(f"    {line_text}")
            total += 1

    if total > 0:
        lines.append(f"\n{total} Windows macro conflict(s) found.")
        lines.append(f"Suppress false positives with: // {SUPPRESSION_COMMENT}")

    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Check C++ source files for Windows macro conflicts"
    )
    parser.add_argument(
        "directories",
        nargs="*",
        default=["algopt"],
        help="Directories to scan (default: algopt/)",
    )

    args = parser.parse_args()

    all_violations: dict[Path, list[tuple[int, str, str]]] = {}
    for directory in args.directories:
        root = Path(directory)
        if not root.is_dir():
            print(f"Warning: {directory} is not a directory, skipping", file=sys.stderr)
            continue
        all_violations.update(scan_directory(root))

    if all_violations:
        print(format_violations(all_violations), file=sys.stderr)
        return 1

    print("No Windows macro conflicts found.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
