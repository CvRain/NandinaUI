#!/usr/bin/env python3

import contextlib
import json
import os
import pathlib
import sqlite3
import subprocess
import sys
import tempfile
import tomllib


def build(helper: str, nanres: str, policy: pathlib.Path, output: pathlib.Path) -> None:
    result = subprocess.run(
        [sys.executable, helper, "build", nanres, str(policy), str(output)],
        check=False,
        text=True,
        capture_output=True,
    )
    assert result.returncode == 0, result.stdout + result.stderr


def main() -> int:
    helper = str(pathlib.Path(sys.argv[1]).resolve())
    nanres = str(pathlib.Path(sys.argv[2]).resolve())
    with tempfile.TemporaryDirectory(prefix="nandina-workflow-") as directory:
        root = pathlib.Path(directory)
        resources = root / "res"
        resources.mkdir()
        (resources / "first.txt").write_text("first", encoding="ascii")
        policy = root / "resources.toml"
        policy.write_text(
            'package = "org.nandina.workflow"\nsource = "res"\n',
            encoding="ascii",
        )
        output = root / "build/resources"

        build(helper, nanres, policy, output)
        metadata = json.loads((output / "resource-location.json").read_text(encoding="utf-8"))
        assert metadata["package_id"] == "org.nandina.workflow"
        assert metadata["database"] == "resources.db"
        lock = tomllib.loads((root / "resources.lock.toml").read_text(encoding="utf-8"))
        assert [item["key"] for item in lock["resources"]] == ["first.txt"]

        (resources / "second.txt").write_text("second", encoding="ascii")
        build(helper, nanres, policy, output)
        lock = tomllib.loads((root / "resources.lock.toml").read_text(encoding="utf-8"))
        assert [item["key"] for item in lock["resources"]] == ["first.txt", "second.txt"]
        with contextlib.closing(sqlite3.connect(output / "resources.db")) as database:
            keys = [row[0] for row in database.execute("select resource_key from resources order by 1")]
        assert keys == ["first.txt", "second.txt"]

        if os.name == "posix":
            # helper 在动手前握手：不兼容 schema 的外部工具必须被拒绝，而不是打包到一半失败。
            bad = root / "bad-nanres"
            bad.write_text("#!/bin/sh\necho 2\n", encoding="ascii")
            bad.chmod(0o755)
            rejected = subprocess.run(
                [sys.executable, helper, "build", str(bad), str(policy), str(root / "bad-output")],
                check=False,
                text=True,
                capture_output=True,
            )
            assert rejected.returncode != 0 and "schema 1" in rejected.stderr, (
                rejected.stdout + rejected.stderr
            )

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
