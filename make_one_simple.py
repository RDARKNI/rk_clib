#!/usr/bin/env python3
"""
Flatten a C file by recursively inlining all quoted includes:

    #include "foo.h"

System includes like:

    #include <stdio.h>

are left untouched.

Resolution strategy for quoted includes:
1. Relative to the including file's directory
2. Relative to every parent directory of the including file
3. Relative to every parent directory of the root input file

This helps when projects use nested relative include layouts.

Usage:
    python flatten_c_includes.py input.c > flattened.c

Optional:
    python flatten_c_includes.py input.c -o flattened.c
    python flatten_c_includes.py input.c --keep-pragma-once

python3 mkgod.py  "./tests/actual_tests.c" > fl.c
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path
from typing import Iterable


INCLUDE_RE = re.compile(
    r'^(?P<prefix>\s*#\s*include\s*)"(?P<name>[^"]+)"(?P<suffix>.*)$'
)

PRAGMA_ONCE_RE = re.compile(r'^\s*#\s*pragma\s+once\b')


def parent_chain(path: Path) -> list[Path]:
    """Return [path.parent, path.parent.parent, ..., filesystem root]."""
    out = []
    cur = path.resolve().parent
    while True:
        out.append(cur)
        if cur.parent == cur:
            break
        cur = cur.parent
    return out


def unique_paths(paths: Iterable[Path]) -> list[Path]:
    seen = set()
    out = []
    for p in paths:
        rp = p.resolve()
        if rp not in seen:
            seen.add(rp)
            out.append(rp)
    return out


def resolve_quoted_include(
    include_name: str,
    including_file: Path,
    root_file: Path,
) -> Path | None:
    """
    Resolve #include "..." by searching:
      - including file's parent chain
      - root file's parent chain
    """
    candidates = []

    # Start from including file's directory, then walk upward.
    for base in parent_chain(including_file):
        candidates.append(base / include_name)

    # Also search from root file's directory upward.
    for base in parent_chain(root_file):
        candidates.append(base / include_name)

    for candidate in unique_paths(candidates):
        if candidate.is_file():
            return candidate

    return None


def flatten_file(
    file_path: Path,
    root_file: Path,
    visited: set[Path],
    keep_pragma_once: bool,
) -> str:
    """
    Recursively inline quoted includes.
    Files are included once by resolved absolute path.
    """
    resolved = file_path.resolve()

    if resolved in visited:
        return f'/* skipped already-included: "{resolved}" */\n'

    visited.add(resolved)

    try:
        text = resolved.read_text(encoding="utf-8")
    except UnicodeDecodeError:
        text = resolved.read_text(encoding="latin-1")

    out: list[str] = []
    out.append(f'/* BEGIN INLINE: {resolved} */\n')

    for line_no, line in enumerate(text.splitlines(keepends=True), start=1):
        if not keep_pragma_once and PRAGMA_ONCE_RE.match(line):
            out.append(
                f"/* removed #pragma once from {resolved}:{line_no} */\n")
            continue

        m = INCLUDE_RE.match(line)
        if not m:
            out.append(line)
            continue

        include_name = m.group("name")
        included = resolve_quoted_include(include_name, resolved, root_file)

        if included is None:
            raise FileNotFoundError(
                f'Could not resolve #include "{include_name}" '
                f'in {resolved}:{line_no}'
            )

        out.append(
            f'/* inlined from {resolved}:{line_no}: #include "{include_name}" */\n'
        )
        out.append(flatten_file(included, root_file,
                   visited, keep_pragma_once))

    out.append(f'/* END INLINE: {resolved} */\n')
    return "".join(out)


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Recursively inline quoted C includes."
    )
    parser.add_argument("input", help="Path to the root .c file")
    parser.add_argument(
        "-o",
        "--output",
        help="Write output to this file instead of stdout",
    )
    parser.add_argument(
        "--keep-pragma-once",
        action="store_true",
        help="Keep '#pragma once' lines instead of removing them",
    )
    args = parser.parse_args()

    root = Path(args.input)
    if not root.is_file():
        print(f"error: input file does not exist: {root}", file=sys.stderr)
        return 1

    try:
        flattened = flatten_file(
            file_path=root,
            root_file=root,
            visited=set(),
            keep_pragma_once=args.keep_pragma_once,
        )
    except Exception as e:
        print(f"error: {e}", file=sys.stderr)
        return 1

    if args.output:
        out_path = Path(args.output)
        out_path.write_text(flattened, encoding="utf-8")
    else:
        sys.stdout.write(flattened)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
