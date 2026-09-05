#!/usr/bin/env python3

import pathlib
import sqlite3
import subprocess
import sys
import tempfile
import tomllib


def run(nanres: str, cwd: pathlib.Path, *args: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [nanres, *args],
        cwd=cwd,
        check=False,
        text=True,
        capture_output=True,
    )


def main() -> int:
    nanres = str(pathlib.Path(sys.argv[1]).resolve())
    package_probe = str(pathlib.Path(sys.argv[2]).resolve())
    with tempfile.TemporaryDirectory(prefix="nandina-nanres-") as directory:
        root = pathlib.Path(directory)
        resources = root / "assets"
        resources.mkdir()
        (resources / "z.txt").write_text("z", encoding="ascii")
        (resources / "a.bin").write_bytes(b"\x89PNG\r\n\x1a\n")

        scanned = run(nanres, root, "scan", "--root", "assets:app")
        assert scanned.returncode == 0, scanned.stderr
        lines = scanned.stdout.splitlines()
        assert lines[0].startswith("app/a.bin\timage/png\t8\t"), lines
        assert lines[1].startswith("app/z.txt\ttext/plain\t1\t"), lines

        validated = run(nanres, root, "validate", "--root", "assets:app")
        assert validated.returncode == 0, validated.stderr
        assert validated.stdout == "", validated.stdout

        initialized = run(nanres, root, "init")
        assert initialized.returncode == 0, initialized.stderr
        assert (root / "resources.toml").is_file()
        assert (root / "assets").is_dir()
        duplicate = run(nanres, root, "init")
        assert duplicate.returncode == 2

        simple_root = root / "simple"
        simple_assets = simple_root / "assets"
        simple_assets.mkdir(parents=True)
        (simple_assets / "nested.txt").write_text("nested", encoding="ascii")
        (simple_root / "resources.toml").write_text(
            'package = "org.nandina.simple"\n', encoding="utf-8"
        )
        simple = run(nanres, simple_root, "scan")
        assert simple.returncode == 0, simple.stderr
        simple_fields = simple.stdout.split("\t")
        assert simple_fields[1:4] == ["nested.txt", "text/plain", "6"], simple.stdout

        conflict = root / "conflict"
        conflict.mkdir()
        (conflict / "resources.toml").write_text(
            'package = "org.nandina.simple"\npackage_id = "org.nandina.other"\n',
            encoding="utf-8",
        )
        assert run(nanres, conflict, "scan").returncode == 1

        policy = root / "resources.toml"
        policy.write_text(
            'package = "org.nandina.test"\n'
            'excludes = ["**/*.tmp"]\n\n'
            '[[roots]]\n'
            'path = "assets"\n'
            'key_prefix = "app"\n\n'
            '[[rules]]\n'
            'glob = "app/*.bin"\n'
            'storage = "external"\n'
            'streaming = true\n',
            encoding="utf-8",
        )
        locked_scan = run(nanres, root, "scan")
        assert locked_scan.returncode == 0, locked_scan.stderr
        lock_path = root / "resources.lock.toml"
        first_lock = tomllib.loads(lock_path.read_text(encoding="utf-8"))
        first = {item["key"]: item for item in first_lock["resources"]}
        assert first["app/a.bin"]["storage"] == "external"
        assert first["app/a.bin"]["streaming"] is True
        original_id = first["app/z.txt"]["id"]
        assert run(nanres, root, "validate").returncode == 0

        (resources / "z.txt").write_text("changed", encoding="ascii")
        stale = run(nanres, root, "validate")
        assert stale.returncode == 1
        assert "missing or stale" in stale.stderr
        assert run(nanres, root, "scan").returncode == 0
        changed_lock = tomllib.loads(lock_path.read_text(encoding="utf-8"))
        changed = {item["key"]: item for item in changed_lock["resources"]}
        assert changed["app/z.txt"]["id"] == original_id
        assert changed["app/z.txt"]["revision"] == 2

        (resources / "z.txt").rename(resources / "moved.txt")
        assert run(nanres, root, "scan").returncode == 0
        moved_lock = tomllib.loads(lock_path.read_text(encoding="utf-8"))
        moved = {item["key"]: item for item in moved_lock["resources"]}
        assert moved["app/moved.txt"]["id"] == original_id
        assert moved["app/moved.txt"]["revision"] == 2

        package_assets = root / "package-assets"
        package_assets.mkdir()
        (package_assets / "small.txt").write_text("hello", encoding="ascii")
        (package_assets / "large.bin").write_bytes(b"external")
        policy.write_text(
            'package_id = "org.nandina.test"\n'
            'package_directory = "dist/resources"\n'
            'embed_threshold = 6\n\n'
            '[aliases]\n'
            '"app/text/default" = "app/small.txt"\n'
            '"app/media/default" = "app/large.bin"\n\n'
            '[[roots]]\n'
            'path = "package-assets"\n'
            'key_prefix = "app"\n\n'
            '[[rules]]\n'
            'glob = "app/large.bin"\n'
            'streaming = true\n',
            encoding="utf-8",
        )
        assert run(nanres, root, "scan").returncode == 0
        packed = run(nanres, root, "pack")
        assert packed.returncode == 0, packed.stderr
        assert packed.stdout.startswith("packed\t")
        database = root / "dist/resources/resources.db"
        external_root = root / "dist/resources/external"
        assert database.is_file()
        with sqlite3.connect(database) as connection:
            assert connection.execute("PRAGMA application_id").fetchone()[0] == 0x4E414E44
            assert connection.execute("PRAGMA user_version").fetchone()[0] == 1
            rows = connection.execute(
                "SELECT resource_key, storage, data, external_path, size FROM resources ORDER BY resource_key"
            ).fetchall()
            assert rows[0][0:2] == ("app/large.bin", 1)
            assert rows[0][2] is None and rows[0][3].startswith("external/")
            assert rows[1][0:2] == ("app/small.txt", 0)
            assert rows[1][2] == b"hello" and rows[1][3] is None
            assert connection.execute("SELECT count(*) FROM aliases").fetchone()[0] == 2
        probe = subprocess.run(
            [package_probe, str(database), "external", "app/media/default"],
            check=False,
            text=True,
            capture_output=True,
        )
        assert probe.returncode == 0, probe.stderr
        unchanged = run(nanres, root, "pack")
        assert unchanged.returncode == 0
        assert unchanged.stdout.startswith("unchanged\t")
        sidecars = list(external_root.iterdir())
        assert len(sidecars) == 1
        sidecars[0].unlink()
        rebuilt = run(nanres, root, "pack")
        assert rebuilt.returncode == 0
        assert rebuilt.stdout.startswith("packed\t")

        duplicate_root = root / "duplicate"
        duplicate_root.mkdir()
        (duplicate_root / "one.txt").write_text("same", encoding="ascii")
        (duplicate_root / "two.txt").write_text("same", encoding="ascii")
        policy.write_text(
            'package_id = "org.nandina.test"\n\n'
            '[[roots]]\n'
            'path = "duplicate"\n',
            encoding="utf-8",
        )
        assert run(nanres, root, "scan").returncode == 0
        (duplicate_root / "one.txt").unlink()
        (duplicate_root / "two.txt").unlink()
        (duplicate_root / "moved.txt").write_text("same", encoding="ascii")
        ambiguous = run(nanres, root, "scan")
        assert ambiguous.returncode == 1
        assert "ambiguous content-hash move" in ambiguous.stderr

        # per-resource overrides（[[resources]]）：对单个 key 覆盖 media_type / storage，
        # 优先级高于 glob 规则与自动检测。
        override_root = root / "override"
        override_assets = override_root / "assets"
        override_assets.mkdir(parents=True)
        (override_assets / "plain.txt").write_text("data", encoding="ascii")
        (override_root / "resources.toml").write_text(
            'package = "org.nandina.override"\n'
            'embed_threshold = 100\n\n'
            '[[resources]]\n'
            'key = "plain.txt"\n'
            'media_type = "text/custom"\n'
            'storage = "external"\n',
            encoding="utf-8",
        )
        assert run(nanres, override_root, "scan").returncode == 0
        override_lock = tomllib.loads((override_root / "resources.lock.toml").read_text(encoding="utf-8"))
        override_item = {item["key"]: item for item in override_lock["resources"]}["plain.txt"]
        assert override_item["media_type"] == "text/custom", override_item
        assert override_item["storage"] == "external", override_item

        (override_root / "resources.toml").write_text(
            'package = "org.nandina.override"\n'
            '[[resources]]\n'
            'key = "not a canonical/key"\n',
            encoding="utf-8",
        )
        assert run(nanres, override_root, "scan").returncode == 1

        deferred = run(nanres, root, "watch")
        assert deferred.returncode == 3
        assert "hot-reload" in deferred.stderr

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
