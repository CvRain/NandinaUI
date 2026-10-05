#!/usr/bin/env python3
"""Keep every Meson test in a suite selected by CI."""

import argparse
import json
import subprocess
import sys


def check_suites(tests: object) -> None:
    if not isinstance(tests, list) or not tests:
        raise ValueError("expected a non-empty Meson test list")
    uncovered = []
    for test in tests:
        if not isinstance(test, dict) or not isinstance(test.get("suite"), list):
            raise ValueError("invalid Meson test suite metadata")
        if not any(
            isinstance(suite, str) and suite.rsplit(":", 1)[-1] in {"unit", "integration"}
            for suite in test["suite"]
        ):
            uncovered.append(str(test.get("name", "<unnamed>")))
    if uncovered:
        raise ValueError("tests outside unit/integration suites: " + ", ".join(uncovered))


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("build_dir")
    args = parser.parse_args(argv)
    try:
        result = subprocess.run(
            ["meson", "introspect", "--tests", args.build_dir],
            check=True,
            capture_output=True,
            text=True,
        )
        check_suites(json.loads(result.stdout))
    except (OSError, subprocess.CalledProcessError, ValueError) as error:
        print(f"ERROR: {error}", file=sys.stderr)
        if isinstance(error, subprocess.CalledProcessError):
            print(error.stderr or error.stdout or "", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
