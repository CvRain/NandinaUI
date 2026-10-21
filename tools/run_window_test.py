#!/usr/bin/env python3

import os
import subprocess
import sys


def main(arguments: list[str] | None = None) -> int:
    command = sys.argv[1:] if arguments is None else arguments
    if not command:
        print("usage: run_window_test.py <test executable> [arguments...]", file=sys.stderr)
        return 2
    if os.environ.get("NANDINA_SKIP_WINDOW_TESTS", "").strip().lower() in {"1", "true", "yes", "on"}:
        print("SKIP: native window tests disabled by NANDINA_SKIP_WINDOW_TESTS")
        return 77
    result = subprocess.run(command, check=False)
    if result.returncode == 4:
        return 77
    return result.returncode if result.returncode >= 0 else 128 - result.returncode


if __name__ == "__main__":
    raise SystemExit(main())
