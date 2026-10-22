#!/usr/bin/env python3
"""Real Meson/Ninja fixtures: never-used UI libraries must still be rejected."""

import pathlib
import shutil
import subprocess
import sys
import tempfile


def main():
    checker = pathlib.Path(sys.argv[1]).resolve()
    meson = sys.argv[2] if len(sys.argv) > 2 else "meson"
    fixture = pathlib.Path(__file__).parent / "fixtures" / "tool_dependencies"
    with tempfile.TemporaryDirectory(prefix="nandina-tool-boundary-") as temporary:
        root = pathlib.Path(temporary)
        source = root / "source"
        shutil.copytree(fixture, source)
        for injection in ("none", "direct", "indirect"):
            build = root / injection
            subprocess.run([meson, "setup", str(build), str(source), f"-Dinjection={injection}"], check=True, capture_output=True)
            subprocess.run([meson, "compile", "-C", str(build)], check=True, capture_output=True)
            command = [sys.executable, str(checker), str(build), "--meson", meson]
            result = subprocess.run(command, capture_output=True, text=True)
            assert (result.returncode == 0) == (injection == "none"), result.stdout + result.stderr
            if injection != "none":
                assert "forbidden" in result.stderr or "outside resource tool" in result.stderr, result.stderr
            else:
                response = build / "link.rsp"
                response.unlink()
                missing_response = subprocess.run(command, capture_output=True, text=True)
                assert missing_response.returncode != 0 and "link.rsp" in missing_response.stderr, missing_response.stderr
                response.write_text("\n")
                # Actual archive tampering must not pass just because target metadata is clean.
                extra = root / "extra.o"
                shutil.copyfile(next(build.rglob("*.o")), extra)
                archive = next(build.glob("*.a"))
                subprocess.run(["ar", "rT", str(archive), str(extra)], cwd=build, check=True, capture_output=True)
                tampered = subprocess.run(command, capture_output=True, text=True)
                assert tampered.returncode != 0 and "archive members" in tampered.stderr, tampered.stderr
        missing = subprocess.run([sys.executable, str(checker), str(root / "missing"), "--meson", meson], capture_output=True)
        assert missing.returncode != 0
    print("dependency boundary: clean fixture passes; direct/indirect UI, archive tampering and missing metadata fail")


if __name__ == "__main__":
    main()
