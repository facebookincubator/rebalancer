# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# pyre-strict

"""Test that C++ source files don't use identifiers that conflict with Windows macros.

Windows headers define macros like IGNORE, ERROR, DELETE that silently replace
C++ identifiers, causing cryptic MSVC build failures on the OSS Windows CI.
"""

from __future__ import annotations

import re
import unittest
from pathlib import Path

# Windows macros that are #define'd as simple tokens and commonly collide
# with C++ identifiers. (macro_name, source_header)
#
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

EXCLUDE_DIRS: frozenset[str] = frozenset({"fb", "build", "third-party", "third_party"})

SUPPRESSION_COMMENT: str = "NOLINT(windows-macro)"


def _strip_comments_and_strings(line: str) -> str:
    """Remove single-line comments and string literals from a line."""
    line = re.sub(r'"(?:[^"\\]|\\.)*"', '""', line)
    comment_pos = line.find("//")
    if comment_pos >= 0:
        line = line[:comment_pos]
    return line


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

        if SUPPRESSION_COMMENT in line:
            continue

        if in_block_comment:
            if "*/" in line:
                line = line[line.index("*/") + 2 :]
                in_block_comment = False
            else:
                continue

        while "/*" in line:
            start = line.index("/*")
            end = line.find("*/", start + 2)
            if end >= 0:
                line = line[:start] + line[end + 2 :]
            else:
                line = line[:start]
                in_block_comment = True
                break

        if line.lstrip().startswith("#"):
            continue

        cleaned = _strip_comments_and_strings(line)

        for macro_name in MACRO_NAMES:
            if re.search(rf"\b{macro_name}\b", cleaned):
                violations.append((line_num, macro_name, raw_line.rstrip()))

    return violations


def scan_directory(
    root: Path,
    extensions: frozenset[str] | None = None,
) -> dict[Path, list[tuple[int, str, str]]]:
    """Scan all C++ source files in a directory tree."""
    if extensions is None:
        extensions = frozenset({".h", ".hpp", ".cpp", ".cc", ".cxx"})

    all_violations: dict[Path, list[tuple[int, str, str]]] = {}

    for filepath in sorted(root.rglob("*")):
        if not filepath.is_file():
            continue
        if filepath.suffix not in extensions:
            continue
        if any(part in EXCLUDE_DIRS for part in filepath.relative_to(root).parts):
            continue

        violations = scan_file(filepath)
        if violations:
            all_violations[filepath] = violations

    return all_violations


def _get_algopt_root() -> Path | None:
    """Find the algopt/ source root by navigating from this test file."""
    # __file__ is at algopt/rebalancer/tests/test_windows_macros.py
    tests_dir = Path(__file__).resolve().parent
    algopt_root = tests_dir.parent.parent
    if (algopt_root / "lp" / "generic").is_dir():
        return algopt_root
    return None


# Directories to scan relative to algopt/ (matches the OSS export boundary)
SCAN_DIRS: list[str] = ["lp", "rebalancer"]


class WindowsMacroTest(unittest.TestCase):
    def test_no_windows_macro_conflicts(self) -> None:
        algopt_root = _get_algopt_root()
        if algopt_root is None:
            self.skipTest("Source tree not accessible from test environment")

        violations: dict[Path, list[tuple[int, str, str]]] = {}
        for subdir in SCAN_DIRS:
            scan_root = algopt_root / subdir
            if scan_root.is_dir():
                violations.update(scan_directory(scan_root))

        if violations:
            macro_sources = dict(WINDOWS_MACROS)
            lines: list[str] = []
            for filepath, file_violations in sorted(violations.items()):
                try:
                    rel_path = filepath.relative_to(algopt_root)
                except ValueError:
                    rel_path = filepath
                for line_num, macro_name, line_text in file_violations:
                    source = macro_sources.get(macro_name, "unknown")
                    lines.append(
                        f"  {rel_path}:{line_num}: '{macro_name}' conflicts "
                        f"with Windows macro from {source}\n"
                        f"    {line_text}"
                    )

            self.fail(
                f"Found {sum(len(v) for v in violations.values())} Windows macro "
                f"conflict(s) in C++ source files:\n"
                + "\n".join(lines)
                + f"\n\nSuppress false positives with: // {SUPPRESSION_COMMENT}"
            )
