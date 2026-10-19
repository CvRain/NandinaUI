#!/usr/bin/env python3
"""Require healthy pixels and both observable failures from the GPU experiment."""

import subprocess
import sys


def main() -> int:
    probe = sys.argv[1]
    healthy = subprocess.run([probe], capture_output=True, text=True, timeout=60)
    print(healthy.stdout, end="")
    print(healthy.stderr, end="", file=sys.stderr)
    if healthy.returncode == 77:
        return 77
    if healthy.returncode != 0:
        raise RuntimeError(f"healthy SDF experiment failed: {healthy.returncode}")
    for fault, expected in (("uniform", "second outline uniform"), ("restore", "texture restore")):
        broken = subprocess.run([probe, f"--inject-{fault}", "--require-gpu"], capture_output=True, text=True, timeout=60)
        if broken.returncode != 1 or f"pixel mismatch: {expected}" not in broken.stderr:
            raise RuntimeError(f"{fault} did not fail at the expected pixel oracle:\n{broken.stdout}\n{broken.stderr}")
        print(f"fault={fault}: correctly rejected: {broken.stderr.strip()}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
