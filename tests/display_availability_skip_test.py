#!/usr/bin/env python3
"""Unavailable display/GPU environments must classify as skip (77), not failure.

The SDF probes used to treat the mere presence of DISPLAY/WAYLAND_DISPLAY as proof
that a desktop session exists. Sandboxed tool sessions can inherit dead variables,
so the probes must rely on real SDL initialization instead. This test forces SDL
to a nonexistent video driver to make the environment deterministically unusable
and checks the classification contract:

- without flags: exit 77 (Meson skip) and a SKIP diagnostic;
- with --require-gpu: exit 1 (explicit requirement is an error).
"""

import os
import subprocess
import sys


def run(command: list[str], extra_arguments: list[str]) -> subprocess.CompletedProcess[str]:
    environment = dict(os.environ)
    environment["SDL_VIDEODRIVER"] = "nandina-display-probe-missing-driver"
    return subprocess.run(
        [*command, *extra_arguments],
        env=environment,
        capture_output=True,
        text=True,
        timeout=120,
        check=False,
    )


def main() -> int:
    binaries = sys.argv[1:]
    if not binaries:
        print("usage: display_availability_skip_test.py <probe> [<probe>...]", file=sys.stderr)
        return 2
    failures: list[str] = []
    for binary in binaries:
        skipped = run([binary], [])
        if skipped.returncode != 77:
            failures.append(
                f"{binary}: expected exit 77 for an unavailable environment, "
                f"got {skipped.returncode}\nstdout: {skipped.stdout}\nstderr: {skipped.stderr}"
            )
        if "SKIP" not in (skipped.stdout + skipped.stderr):
            failures.append(f"{binary}: skip diagnostics are missing")
        required = run([binary], ["--require-gpu"])
        if required.returncode != 1:
            failures.append(
                f"{binary}: --require-gpu must turn the unavailable environment into exit 1, "
                f"got {required.returncode}\nstdout: {required.stdout}\nstderr: {required.stderr}"
            )
    if failures:
        for failure in failures:
            print(failure, file=sys.stderr)
        return 1
    print(f"display availability classification verified for {len(binaries)} probe(s)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
