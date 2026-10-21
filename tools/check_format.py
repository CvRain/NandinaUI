#!/usr/bin/env python3
"""Keep the tracked C/C++ tree identical to the pinned clang-format output.

The formatting baseline is only meaningful if the tool version is fixed: 21 and
23 disagree on a handful of files, so an unpinned check either fails on a
correct tree or silently accepts drift.  This guard therefore refuses to run
under any version other than the pinned one, and CI installs exactly that
version (`pip install clang-format==<PINNED_VERSION>`).

Submodule contents are not part of the baseline: `git ls-files` in the
superproject lists the gitlink but never the files behind it, so dependencies
stay out of the check by construction.
"""

import argparse
import pathlib
import re
import subprocess
import sys


PINNED_VERSION = "23.1.1"
SOURCE_SUFFIXES = (".c", ".cc", ".cpp", ".cxx", ".h", ".hpp", ".hxx")
VERSION_PATTERN = re.compile(r"clang-format version (\S+)")


def tracked_sources(source_root: pathlib.Path) -> list[pathlib.Path]:
    """Return every tracked C/C++ file, in git's deterministic order."""
    result = subprocess.run(
        [
            "git",
            "-C",
            str(source_root),
            "ls-files",
            "-z",
            "--",
            *[f"*{suffix}" for suffix in SOURCE_SUFFIXES],
        ],
        check=True,
        capture_output=True,
        text=True,
    )
    return [
        source_root / value
        for value in result.stdout.split("\0")
        if value and (source_root / value).is_file()
    ]


def clang_format_version(executable: str) -> str:
    result = subprocess.run(
        [executable, "--version"],
        check=True,
        capture_output=True,
        text=True,
    )
    match = VERSION_PATTERN.search(result.stdout)
    if match is None:
        raise ValueError(f"cannot parse a version out of {executable!r} --version")
    return match.group(1)


def unformatted(executable: str, sources: list[pathlib.Path]) -> list[pathlib.Path]:
    """Return the files whose on-disk bytes differ from clang-format's output."""
    offenders = []
    for source in sources:
        result = subprocess.run(
            [executable, "--dry-run", "--Werror", str(source)],
            check=False,
            capture_output=True,
            text=True,
        )
        if result.returncode != 0:
            offenders.append(source)
    return offenders


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source_root", nargs="?", default=".")
    parser.add_argument("--clang-format", default="clang-format")
    parser.add_argument("--expected-version", default=PINNED_VERSION)
    args = parser.parse_args(argv)
    try:
        source_root = pathlib.Path(args.source_root).resolve()
        version = clang_format_version(args.clang_format)
        if version != args.expected_version:
            raise ValueError(
                f"clang-format {version} is not the pinned {args.expected_version}; "
                f"install it with 'pip install clang-format=={args.expected_version}'"
            )
        sources = tracked_sources(source_root)
        if not sources:
            raise ValueError(f"no tracked C/C++ sources under {source_root}")
        offenders = unformatted(args.clang_format, sources)
        if offenders:
            listing = "\n".join(f"  {path.relative_to(source_root)}" for path in offenders)
            raise ValueError(
                f"{len(offenders)} of {len(sources)} files are not clang-format "
                f"{args.expected_version} clean:\n{listing}\n"
                "run: clang-format -i <file>..."
            )
    except (OSError, subprocess.CalledProcessError, ValueError) as error:
        print(f"ERROR: {error}", file=sys.stderr)
        if isinstance(error, subprocess.CalledProcessError):
            print(error.stderr or error.stdout or "", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
