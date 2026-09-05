#!/usr/bin/env python3

import json
import pathlib
import subprocess
import sys
import tempfile


def run(*arguments: str, cwd: pathlib.Path) -> None:
    result = subprocess.run(arguments, cwd=cwd, check=False, text=True, capture_output=True)
    assert result.returncode == 0, result.stdout + result.stderr


def main() -> int:
    stage_helper = pathlib.Path(sys.argv[1]).resolve()
    nanres = pathlib.Path(sys.argv[2]).resolve()
    package_probe = pathlib.Path(sys.argv[3]).resolve()
    with tempfile.TemporaryDirectory(prefix="nandina-portable-") as directory:
        root = pathlib.Path(directory)
        assets = root / "assets"
        assets.mkdir()
        (assets / "small.txt").write_text("hello", encoding="ascii")
        (assets / "large.bin").write_bytes(b"external")
        (root / "resources.toml").write_text(
            'package = "org.nandina.portable"\n'
            'package_directory = "package"\n'
            "embed_threshold = 6\n\n"
            "[[roots]]\n"
            'path = "assets"\n'
            'key_prefix = "app"\n\n'
            "[aliases]\n"
            '"app/text/default" = "app/small.txt"\n'
            '"app/media/default" = "app/large.bin"\n',
            encoding="utf-8",
        )
        run(str(nanres), "scan", cwd=root)
        run(str(nanres), "pack", cwd=root)
        package = root / "package"
        (package / "resource-location.json").write_text(
            '{"package_root":"/build-only"}\n', encoding="utf-8"
        )
        output = root / "portable"
        run(
            sys.executable,
            str(stage_helper),
            "--executable",
            str(package_probe),
            "--resources",
            str(package),
            "--output",
            str(output),
            cwd=root,
        )

        staged_executable = output / package_probe.name
        staged_database = output / "resources/resources.db"
        assert staged_executable.is_file()
        assert staged_database.is_file()
        assert (output / "resources/external").is_dir()
        assert not (output / "resources/resource-location.json").exists()
        metadata = json.loads((output / "portable-stage.json").read_text(encoding="utf-8"))
        assert metadata["resources"] == "resources/resources.db"
        run(str(staged_executable), str(staged_database), "external", "app/media/default", cwd=root)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
