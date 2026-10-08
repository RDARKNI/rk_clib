#!/usr/bin/env python3
"""
Flatten a C file by recursively inlining all quoted includes:

    #include "foo.h"

System includes like:

    #include <stdio.h>

are left untouched.

Marked implementation sections are appended at the bottom in dependency order.
Headers must already separate public declarations from their function bodies.
Sections use `#pragma region implementation` / `#pragma endregion implementation`,
or (when no region is present) `/// @cond INTERNAL` / `/// @endcond`.
RK_IFALLOC remains early because public tree types need it. This tool does not evaluate conditional compilation;
it preserves the original section's activation using a temporary macro.

The per-header `// SPDX-License-Identifier` lines and trailing MIT license blocks
are stripped; the root file's license is emitted once at the top of the output.

Resolution strategy for quoted includes:
1. Relative to the including file's directory
2. Relative to every parent directory of the including file
3. Relative to every parent directory of the root input file

This helps when projects use nested relative include layouts.

Usage:
    python make_one_simple.py input.c > flattened.c

Optional:
    python make_one_simple.py input.c -o flattened.c
    python make_one_simple.py input.c --keep-pragma-once
    python make_one_simple.py include/rklib.h -o single_include/rklib.h
    python make_one_simple.py input.c --no-defer-implementations
"""

from __future__ import annotations

import argparse
import hashlib
import os
import re
import sys
from pathlib import Path
from typing import Iterable


INCLUDE_RE = re.compile(
    r'^(?P<prefix>\s*#\s*include\s*)"(?P<name>[^"]+)"(?P<suffix>.*)$'
)

PRAGMA_ONCE_RE = re.compile(r'^\s*#\s*pragma\s+once\b')

IMPL_BEGIN_RE = re.compile(r'^\s*#\s*pragma\s+region\s+implementation\s*$')
IMPL_END_RE = re.compile(r'^\s*#\s*pragma\s+endregion\s+implementation\s*$')
COND_BEGIN_RE = re.compile(r'^\s*///\s*@cond\s+INTERNAL\s*$')
COND_END_RE = re.compile(r'^\s*///\s*@endcond\s*$')
SPDX_RE = re.compile(r'^\s*//\s*SPDX-License-Identifier:')
LICENSE_BEGIN_RE = re.compile(r'^\s*//\s*MIT License\s*$')
LICENSE_END_RE = re.compile(r'OTHER DEALINGS IN THE SOFTWARE\.\s*$')


def strip_license(lines: list[str], licenses: list[list[str]]) -> list[str]:
    """Remove SPDX tags and MIT license blocks, collecting the blocks."""
    out: list[str] = []
    i = 0
    while i < len(lines):
        line = lines[i]
        if SPDX_RE.match(line):
            i += 1
            continue
        if LICENSE_BEGIN_RE.match(line):
            end = next((j for j in range(i, len(lines))
                        if LICENSE_END_RE.search(lines[j])), None)
            if end is None:
                raise ValueError("Unterminated MIT license block")
            licenses.append(lines[i:end + 1])
            while out and not out[-1].strip():
                out.pop()  # the blank separator before the license
            i = end + 1
            continue
        out.append(line)
        i += 1
    return out


def implementation_span(lines: list[str]) -> tuple[int, int] | None:
    """Find one explicitly marked section; never try to parse C function bodies."""
    starts = [i for i, line in enumerate(lines) if IMPL_BEGIN_RE.match(line)]
    ends = [i for i, line in enumerate(lines) if IMPL_END_RE.match(line)]
    if not starts and not ends:
        starts = [i for i, line in enumerate(
            lines) if COND_BEGIN_RE.match(line)]
        ends = [i for i, line in enumerate(lines) if COND_END_RE.match(line)]
    if not starts and not ends:
        return None
    if len(starts) != 1 or len(ends) != 1 or starts[0] >= ends[0]:
        raise ValueError(
            "Expected one balanced implementation section per header")
    span = starts[0], ends[0] + 1
# The section must not cut across an enclosing preprocessor conditional.
    depth = 0
    continued = False
    for line in lines[span[0]:span[1]]:
        if not continued:
            directive = re.match(
                r'^\s*#\s*(if|ifdef|ifndef|elif|else|endif)\b', line)
            if directive:
                name = directive.group(1)
                if name in {"if", "ifdef", "ifndef"}:
                    depth += 1
                elif name == "endif":
                    depth -= 1
                elif depth == 0:
                    raise ValueError(
                        "Implementation section crosses a conditional branch")
                if depth < 0:
                    raise ValueError(
                        "Implementation section closes an outer conditional")
        continued = line.rstrip().endswith("\\")
    if depth:
        raise ValueError("Implementation section leaves a conditional open")
    return span


def scope_event(line: str) -> tuple[str, str] | None:
    """Recognize standalone rk_clib linkage/diagnostic scope markers."""
    stripped = re.sub(r'\s*//.*$', '', line).strip()
    if stripped == 'RKI_HEADER_BEGIN':
        return 'open', 'RKI_HEADER_END\n'
    if stripped == 'RK_EXTERNC_BEG':
        return 'open', 'RK_EXTERNC_END\n'
    if re.fullmatch(r'RKI_IGNWARN_(CLANG|MSC)_BEG\(.*\)', stripped):
        return 'open', stripped.split('_BEG', 1)[0] + '_END()\n'
    if stripped in {'RKI_HEADER_END', 'RK_EXTERNC_END'} or re.fullmatch(
        r'RKI_IGNWARN_(CLANG|MSC)_END\(\)', stripped
    ):
        return 'close', stripped + '\n'
    return None


def advance_scope(scopes: list[tuple[str, str]], line: str) -> None:
    event = scope_event(line)
    if event is None:
        return
    action, closing = event
    if action == 'open':
        scopes.append((line, closing))
    else:
        if not scopes or scopes[-1][1] != closing:
            raise ValueError(
                f"Unbalanced linkage/diagnostic scope: {line.strip()}")
        scopes.pop()


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


def display_path(p: Path) -> str:
    """Path for diagnostics and implementation flags: relative to cwd when
    possible, so the output doesn't depend on absolute build-machine paths
    (e.g. CI runner workspace dirs), falling back to the absolute path if
    there's no common root (e.g. different drives on Windows)."""
    try:
        return str(os.path.relpath(p, Path.cwd()))
    except ValueError:
        return str(p)


def flatten_file(
    file_path: Path,
    root_file: Path,
    visited: set[Path],
    keep_pragma_once: bool,
    deferred: list[str] | None = None,
    keep_implementations: set[str] | None = None,
    licenses: list[list[str]] | None = None,
) -> str:
    """
    Recursively inline quoted includes.
    Files are included once by resolved absolute path.
    """
    resolved = file_path.resolve()
    shown = display_path(resolved)

    if resolved in visited:
        return ''

    visited.add(resolved)

    try:
        text = resolved.read_text(encoding="utf-8")
    except UnicodeDecodeError:
        text = resolved.read_text(encoding="latin-1")

    out: list[str] = []

    lines = text.splitlines(keepends=True)
    if lines and not lines[-1].endswith('\n'):
        lines[-1] += '\n'
    if licenses is not None:
        lines = strip_license(lines, licenses)
    keep_implementations = keep_implementations or set()
    span = None
    if deferred is not None and resolved.name not in keep_implementations:
        span = implementation_span(lines)
    scopes: list[tuple[str, str]] = []
    line_no = 0
    while line_no < len(lines):
        if span is not None and line_no == span[0]:
            body = lines[span[0]:span[1]]
            if any(INCLUDE_RE.match(line) for line in body):
                raise ValueError(
                    f"Move quoted includes before the implementation section: {shown}")

# TreeNode/base declarations need this allocator configuration macro
# immediately. Keep just this prerequisite early, not allocator bodies.
            if resolved.name == 'rk_alloc.h':
                matches = [line for line in body if re.match(
                    r'^\s*#\s*define\s+RK_IFALLOC\(', line)]
                if matches:
                    if len(matches) != 2:
                        raise ValueError(
                            f"Unexpected RK_IFALLOC definitions in {shown}")
                    out.extend(['#if RK_CUSTOM_ALLOCATORS\n',
                               matches[0], '#else\n', matches[1], '#endif\n'])
                    body = [line for line in body if line not in matches]

# This flag is set inside the original guard and conditional branch.
# Do not reopen #ifndef HEADER_H: that guard is already defined later.
            digest = hashlib.sha256(shown.encode(
                'utf-8')).hexdigest()[:16].upper()
            flag = f'RKI_AMALG_IMPL_{digest}'
            out.append(f'#define {flag} 1\n')
            replay_scopes = scopes.copy()
            continued = False
            for line in body:
                if not continued:
                    event = scope_event(line)
                    if event is not None and event[0] == 'close' and len(replay_scopes) <= len(scopes):
                        # This pop originally occurred inside the moved section.
                        # Balance the public half's corresponding push too.
                        out.append(event[1])
                    advance_scope(replay_scopes, line)
                continued = line.rstrip().endswith('\\')
# Preserve the scopes that remain open after the original section.
            original_scopes = scopes.copy()
            scopes = replay_scopes.copy()
            deferred.append(''.join([
                f'#ifdef {flag}\n#undef {flag}\n',
                *(opening for opening, _ in original_scopes),
                *body,
                *(closing for _, closing in reversed(replay_scopes)),
                '#endif\n',
            ]))
            line_no = span[1]
            continue

        line = lines[line_no]
        line_no += 1
# Macro replacement lists can contain scope tokens. They are not scopes
# entered while reading the header, so ignore continuation lines.
        if line_no == 1 or not lines[line_no - 2].rstrip().endswith('\\'):
            advance_scope(scopes, line)
        if not keep_pragma_once and PRAGMA_ONCE_RE.match(line):
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
                f'in {shown}:{line_no}'
            )

        out.append(flatten_file(included, root_file, visited, keep_pragma_once,
                                deferred, keep_implementations, licenses))

    return "".join(out)


def collapse_blank_lines(text: str) -> str:
    """Squeeze runs of blank lines left behind by removed sections."""
    return re.sub(r'\n(?:[ \t]*\n){2,}', '\n\n', text).lstrip('\n')


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
    parser.add_argument(
        '--keep-implementation', action='append', default=[], metavar='HEADER',
        help='Keep the implementation section of this header in place',
    )
    parser.add_argument(
        '--no-defer-implementations', action='store_true',
        help='Use the original include-order-only layout',
    )
    args = parser.parse_args()

    root = Path(args.input)
    if not root.is_file():
        print(f"error: input file does not exist: {root}", file=sys.stderr)
        return 1

    try:
        deferred = None if args.no_defer_implementations else []
        licenses: list[list[str]] = []
        flattened = flatten_file(
            file_path=root,
            root_file=root,
            visited=set(),
            keep_pragma_once=args.keep_pragma_once,
            deferred=deferred,
            keep_implementations=set(args.keep_implementation),
            licenses=licenses,
        )
        if deferred:
            flattened += '\n' + '\n'.join(deferred)
        header = [
            '// SPDX-License-Identifier: MIT\n',
            f'// Single-header amalgamation generated from {display_path(root.resolve())}'
            ' by make_one_simple.py. Do not edit.\n',
        ]
        if licenses:
            header += ['//\n', *licenses[0]]
        flattened = ''.join(header) + '\n' + collapse_blank_lines(flattened)
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
