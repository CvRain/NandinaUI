#!/usr/bin/env python3

import contextlib
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
    schema = run(nanres, pathlib.Path.cwd(), "schema")
    assert schema.returncode == 0 and schema.stdout.strip() == "1", schema
    version = run(nanres, pathlib.Path.cwd(), "--version")
    assert version.returncode == 0 and version.stdout.startswith("nanres "), version
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
        # 派生 key 随源路径改变；ID 由 key 派生，不再按内容哈希继承旧身份。
        assert moved["app/moved.txt"]["id"] != original_id
        assert moved["app/moved.txt"]["revision"] == 1

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
        with contextlib.closing(sqlite3.connect(database)) as connection:
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
        # Fingerprint files written by a text-mode Windows build carry CRLF; reading must
        # normalize the line ending instead of forcing a full repack.
        fingerprint = root / "dist/resources/.nanres-pack.sha256"
        fingerprint.write_bytes(fingerprint.read_bytes().rstrip(b"\r\n") + b"\r\n")
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
        duplicate_lock = tomllib.loads((root / "resources.lock.toml").read_text(encoding="utf-8"))
        duplicate = {item["key"]: item for item in duplicate_lock["resources"]}
        assert duplicate["one.txt"]["id"] != duplicate["two.txt"]["id"]
        (duplicate_root / "one.txt").unlink()
        (duplicate_root / "two.txt").unlink()
        (duplicate_root / "moved.txt").write_text("same", encoding="ascii")
        assert run(nanres, root, "scan").returncode == 0
        moved_duplicate_lock = tomllib.loads(
            (root / "resources.lock.toml").read_text(encoding="utf-8")
        )
        moved_duplicate = {item["key"]: item for item in moved_duplicate_lock["resources"]}
        assert "moved.txt" in moved_duplicate
        assert moved_duplicate["moved.txt"]["id"] not in {
            duplicate["one.txt"]["id"],
            duplicate["two.txt"]["id"],
        }
        # 派生 ID 稳定：再次扫描无变化时不换 ID、不推进 revision。
        assert run(nanres, root, "scan").returncode == 0
        rescanned = {
            item["key"]: item
            for item in tomllib.loads((root / "resources.lock.toml").read_text(encoding="utf-8"))[
                "resources"
            ]
        }
        assert rescanned["moved.txt"]["id"] == moved_duplicate["moved.txt"]["id"]
        assert rescanned["moved.txt"]["revision"] == 1

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

        # 显式 key→路径映射：源文件重命名但 key 保持时，记录 ID 不变。
        mapping_root = root / "mapping"
        mapping_assets = mapping_root / "assets"
        mapping_assets.mkdir(parents=True)
        (mapping_assets / "first.txt").write_text("mapped", encoding="ascii")
        mapping_policy = mapping_root / "resources.toml"
        mapping_policy.write_text(
            'package = "org.nandina.mapping"\n\n'
            '[[roots]]\n'
            'path = "assets"\n\n'
            '[[resources]]\n'
            'key = "app/stable"\n'
            'path = "assets/first.txt"\n'
            'media_type = "text/custom"\n',
            encoding="utf-8",
        )
        assert run(nanres, mapping_root, "scan").returncode == 0
        mapping_lock_path = mapping_root / "resources.lock.toml"
        mapping_once = {
            item["key"]: item
            for item in tomllib.loads(mapping_lock_path.read_text(encoding="utf-8"))["resources"]
        }
        assert set(mapping_once) == {"app/stable"}
        assert mapping_once["app/stable"]["source"] == "assets/first.txt"
        assert mapping_once["app/stable"]["media_type"] == "text/custom"
        stable_id = mapping_once["app/stable"]["id"]
        assert stable_id == "c73735d1-eb2a-b655-1d26-f5891ba9eeb4"

        (mapping_assets / "first.txt").rename(mapping_assets / "renamed.txt")
        (mapping_root / "shared.txt").write_text("shared", encoding="ascii")
        mapping_policy.write_text(
            'package = "org.nandina.mapping"\n\n'
            '[[roots]]\n'
            'path = "assets"\n\n'
            '[[rules]]\n'
            'glob = "app/*"\n'
            'storage = "external"\n'
            'streaming = true\n\n'
            '[[resources]]\n'
            'key = "app/stable"\n'
            'path = "assets/renamed.txt"\n'
            'media_type = "text/custom"\n\n'
            '[[resources]]\n'
            'key = "app/shared"\n'
            'path = "shared.txt"\n',
            encoding="utf-8",
        )
        assert run(nanres, mapping_root, "scan").returncode == 0
        mapping_moved = {
            item["key"]: item
            for item in tomllib.loads(mapping_lock_path.read_text(encoding="utf-8"))["resources"]
        }
        assert mapping_moved["app/stable"]["source"] == "assets/renamed.txt"
        assert mapping_moved["app/stable"]["id"] == stable_id
        # 映射源可以在扫描根之外（仍在 resources.toml 同目录）；media_type 按扩展名检测。
        assert mapping_moved["app/shared"]["source"] == "shared.txt"
        assert mapping_moved["app/shared"]["media_type"] == "text/plain"
        assert mapping_moved["app/shared"]["id"] != stable_id
        # [[rules]] 的 storage/streaming 也作用于显式映射条目。
        assert mapping_moved["app/stable"]["storage"] == "external"
        assert mapping_moved["app/stable"]["streaming"] is True
        assert mapping_moved["app/shared"]["storage"] == "external"

        # 同一个 key 的内容变化：ID 保持，revision 递增。
        (mapping_assets / "renamed.txt").write_text("mapped-v2", encoding="ascii")
        assert run(nanres, mapping_root, "scan").returncode == 0
        mapping_changed = {
            item["key"]: item
            for item in tomllib.loads(mapping_lock_path.read_text(encoding="utf-8"))["resources"]
        }
        assert mapping_changed["app/stable"]["id"] == stable_id
        assert mapping_changed["app/stable"]["revision"] == 2
        assert mapping_changed["app/shared"]["revision"] == 1

        mapping_policy.write_text(
            'package = "org.nandina.mapping"\n\n'
            '[[resources]]\n'
            'key = "app/stable"\n'
            'path = "assets/missing.txt"\n',
            encoding="utf-8",
        )
        missing = run(nanres, mapping_root, "scan")
        assert missing.returncode == 1
        assert "not a readable file" in missing.stderr

        # 映射错误路径：目录、绝对路径、上跳越界、空路径、同路径多 key、与派生 key 重复。
        error_root = root / "mapping-errors"
        error_assets = error_root / "assets"
        error_assets.mkdir(parents=True)
        (error_assets / "plain.txt").write_text("plain", encoding="ascii")
        (error_root / "shared.txt").write_text("shared", encoding="ascii")
        error_policy = error_root / "resources.toml"

        def scan_mapping(policy_body: str) -> subprocess.CompletedProcess[str]:
            error_policy.write_text(
                'package = "org.nandina.errors"\n' + policy_body, encoding="utf-8"
            )
            return run(nanres, error_root, "scan")

        directory = scan_mapping('[[resources]]\nkey = "app/dir"\npath = "assets"\n')
        assert directory.returncode == 1
        assert "not a readable file" in directory.stderr

        absolute_path = str(error_assets / "plain.txt").replace("\\", "/")
        absolute = scan_mapping(f'[[resources]]\nkey = "app/abs"\npath = "{absolute_path}"\n')
        assert absolute.returncode == 1
        assert "resources[].path must be relative" in absolute.stderr

        (root / "outside.txt").write_text("outside", encoding="ascii")
        traversal = scan_mapping('[[resources]]\nkey = "app/up"\npath = "../outside.txt"\n')
        assert traversal.returncode == 1
        assert "resources[].path contains traversal" in traversal.stderr

        empty = scan_mapping('[[resources]]\nkey = "app/empty"\npath = ""\n')
        assert empty.returncode == 1
        assert "resources[].path must not be empty" in empty.stderr

        same_path = scan_mapping(
            '[[resources]]\nkey = "app/one"\npath = "shared.txt"\n\n'
            '[[resources]]\nkey = "app/two"\npath = "shared.txt"\n'
        )
        assert same_path.returncode == 1
        assert "same path" in same_path.stderr

        derived_duplicate = scan_mapping(
            '[[roots]]\npath = "assets"\n\n'
            '[[resources]]\nkey = "plain.txt"\npath = "shared.txt"\n'
        )
        assert derived_duplicate.returncode == 1
        assert "duplicate resource key" in derived_duplicate.stderr

        if sys.platform != "win32":
            link = error_root / "link.txt"
            try:
                link.symlink_to(error_assets / "plain.txt")
            except OSError:
                link = None
            if link is not None:
                symlink = scan_mapping('[[resources]]\nkey = "app/link"\npath = "link.txt"\n')
                assert symlink.returncode == 1
                assert "explicit resource path is a symlink" in symlink.stderr

        if sys.platform == "win32":
            # Windows 的 `C:foo` / `\foo` 是 rooted 但非绝对路径，必须同样拒绝。
            rooted = scan_mapping('[[resources]]\nkey = "app/rooted"\npath = "\\\\foo"\n')
            assert rooted.returncode == 1
            assert "resources[].path must be relative" in rooted.stderr

            # NTFS 大小写不敏感：不同大小写拼写的映射路径仍是同一物理文件，只产出一条记录。
            case_root = root / "mapping-case"
            case_assets = case_root / "assets"
            case_assets.mkdir(parents=True)
            (case_assets / "first.txt").write_text("first", encoding="ascii")
            (case_root / "resources.toml").write_text(
                'package = "org.nandina.case"\n\n'
                '[[roots]]\n'
                'path = "assets"\n\n'
                '[[resources]]\n'
                'key = "app/stable"\n'
                'path = "assets/First.txt"\n',
                encoding="utf-8",
            )
            assert run(nanres, case_root, "scan").returncode == 0
            case_lock = {
                item["key"]: item
                for item in tomllib.loads(
                    (case_root / "resources.lock.toml").read_text(encoding="utf-8")
                )["resources"]
            }
            assert set(case_lock) == {"app/stable"}

        # 扫描根内无法派生规范 key 的文件也可以由显式映射声明：映射优先于路径派生。
        upper_root = root / "mapping-upper"
        (upper_root / "assets" / "Fonts").mkdir(parents=True)
        (upper_root / "assets" / "Fonts" / "Default.ttf").write_bytes(b"font")
        (upper_root / "resources.toml").write_text(
            'package = "org.nandina.upper"\n\n'
            '[[roots]]\n'
            'path = "assets"\n\n'
            '[[resources]]\n'
            'key = "app/default-font"\n'
            'path = "assets/Fonts/Default.ttf"\n',
            encoding="utf-8",
        )
        assert run(nanres, upper_root, "scan").returncode == 0
        upper_lock = {
            item["key"]: item
            for item in tomllib.loads(
                (upper_root / "resources.lock.toml").read_text(encoding="utf-8")
            )["resources"]
        }
        assert set(upper_lock) == {"app/default-font"}
        assert upper_lock["app/default-font"]["source"] == "assets/Fonts/Default.ttf"

        # 旧 lock 的随机 ID：同 key 内容变化时保留旧 ID，只推进 revision（迁移留给 P2.3b）。
        legacy_root = root / "legacy-id"
        legacy_assets = legacy_root / "assets"
        legacy_assets.mkdir(parents=True)
        (legacy_assets / "keep.txt").write_text("v1", encoding="ascii")
        (legacy_root / "resources.toml").write_text(
            'package = "org.nandina.legacy"\n\n[[roots]]\npath = "assets"\n',
            encoding="utf-8",
        )
        assert run(nanres, legacy_root, "scan").returncode == 0
        legacy_lock_path = legacy_root / "resources.lock.toml"
        legacy_entry = tomllib.loads(legacy_lock_path.read_text(encoding="utf-8"))["resources"][0]
        fabricated = "00000000-1111-2222-3333-444444444444"
        legacy_lock_path.write_text(
            legacy_lock_path.read_text(encoding="utf-8").replace(
                legacy_entry["id"], fabricated
            ),
            encoding="utf-8",
        )
        (legacy_assets / "keep.txt").write_text("v2", encoding="ascii")
        migration = run(nanres, legacy_root, "scan")
        assert migration.returncode == 1
        assert "rewrite-lock" in migration.stderr
        legacy_validate = run(nanres, legacy_root, "validate")
        assert legacy_validate.returncode == 1
        assert "rewrite-lock" in legacy_validate.stderr
        rewritten = run(nanres, legacy_root, "scan", "--rewrite-lock")
        assert rewritten.returncode == 0
        assert "rewrote" in rewritten.stderr
        legacy_after = tomllib.loads(legacy_lock_path.read_text(encoding="utf-8"))["resources"][0]
        assert legacy_after["id"] == "d63abd13-6861-bb00-2b51-88c24cc19288"
        assert legacy_after["revision"] == 2
        assert run(nanres, legacy_root, "scan").returncode == 0
        legacy_stable = tomllib.loads(legacy_lock_path.read_text(encoding="utf-8"))["resources"][0]
        assert legacy_stable["id"] == legacy_after["id"]
        assert legacy_stable["revision"] == 2

        # --lock：lock 写入构建目录，源码树不出现生成文件；无变化重扫不重写。
        lock_root = root / "lock-path"
        lock_assets = lock_root / "assets"
        lock_assets.mkdir(parents=True)
        (lock_assets / "value.txt").write_text("value", encoding="ascii")
        (lock_root / "resources.toml").write_text(
            'package = "org.nandina.lock"\n\n[[roots]]\npath = "assets"\n',
            encoding="utf-8",
        )
        lock_file = lock_root / "build/resources.lock.toml"
        assert run(nanres, lock_root, "scan", "--lock", "build/resources.lock.toml").returncode == 0
        assert lock_file.is_file()
        assert not (lock_root / "resources.lock.toml").exists()
        lock_bytes = lock_file.read_bytes()
        lock_mtime = lock_file.stat().st_mtime_ns
        assert run(nanres, lock_root, "scan", "--lock", "build/resources.lock.toml").returncode == 0
        assert lock_file.read_bytes() == lock_bytes
        assert lock_file.stat().st_mtime_ns == lock_mtime
        assert run(nanres, lock_root, "validate", "--lock", "build/resources.lock.toml").returncode == 0
        assert run(
            nanres, lock_root, "pack", "--lock", "build/resources.lock.toml", "--output", "pkg"
        ).returncode == 0

        # --lock/--rewrite-lock 属于 manifest 模式：带 scan 覆盖参数时必须报错而不是静默忽略。
        raw_lock = run(
            nanres, lock_root, "scan", "--root", "assets", "--lock", "build/ignored.lock.toml"
        )
        assert raw_lock.returncode == 2
        assert "require manifest mode" in raw_lock.stderr
        raw_rewrite = run(nanres, lock_root, "scan", "--root", "assets", "--rewrite-lock")
        assert raw_rewrite.returncode == 2
        invalid_rewrite = run(nanres, lock_root, "validate", "--rewrite-lock")
        assert invalid_rewrite.returncode == 2
        assert "only valid with scan" in invalid_rewrite.stderr

        # 重建确定性：相同 policy/assets 的独立目录得到逐字节相同的 lock。
        determinant_locks = []
        for name in ("determinism-a", "determinism-b"):
            copy_root = root / name
            copy_assets = copy_root / "assets"
            copy_assets.mkdir(parents=True)
            (copy_assets / "value.txt").write_text("value", encoding="ascii")
            (copy_root / "resources.toml").write_text(
                'package = "org.nandina.deterministic"\n\n[[roots]]\npath = "assets"\n',
                encoding="utf-8",
            )
            assert run(nanres, copy_root, "scan").returncode == 0
            determinant_locks.append((copy_root / "resources.lock.toml").read_bytes())
        assert determinant_locks[0] == determinant_locks[1]

        # 碰撞注入：同一 ID 对应两个不同 key 必须失败并列出冲突 key。
        collision_root = root / "collision"
        collision_assets = collision_root / "assets"
        collision_assets.mkdir(parents=True)
        (collision_assets / "a.txt").write_text("a", encoding="ascii")
        (collision_assets / "b.txt").write_text("b", encoding="ascii")
        (collision_root / "resources.toml").write_text(
            'package = "org.nandina.collision"\n\n[[roots]]\npath = "assets"\n',
            encoding="utf-8",
        )
        assert run(nanres, collision_root, "scan").returncode == 0
        collision_lock = collision_root / "resources.lock.toml"
        collision_entries = tomllib.loads(collision_lock.read_text(encoding="utf-8"))["resources"]
        first_id = collision_entries[0]["id"]
        second_id = collision_entries[1]["id"]
        collision_lock.write_text(
            collision_lock.read_text(encoding="utf-8").replace(second_id, first_id),
            encoding="utf-8",
        )
        collision = run(nanres, collision_root, "scan")
        assert collision.returncode == 1
        assert collision_entries[0]["key"] in collision.stderr
        assert collision_entries[1]["key"] in collision.stderr

        # 碰撞的另一半：同一 key 对应两个不同 ID 也必须失败并列出冲突 ID。
        key_collision_root = root / "collision-key"
        (key_collision_root / "assets").mkdir(parents=True)
        (key_collision_root / "assets" / "first.txt").write_text("first", encoding="ascii")
        (key_collision_root / "assets" / "second.txt").write_text("second", encoding="ascii")
        (key_collision_root / "resources.toml").write_text(
            'package = "org.nandina.keycollision"\n\n[[roots]]\npath = "assets"\n',
            encoding="utf-8",
        )
        assert run(nanres, key_collision_root, "scan").returncode == 0
        key_lock = key_collision_root / "resources.lock.toml"
        key_entries = tomllib.loads(key_lock.read_text(encoding="utf-8"))["resources"]
        key_lock.write_text(
            key_lock.read_text(encoding="utf-8").replace(
                'key = "' + key_entries[1]["key"] + '"',
                'key = "' + key_entries[0]["key"] + '"',
            ),
            encoding="utf-8",
        )
        key_collision = run(nanres, key_collision_root, "scan")
        assert key_collision.returncode == 1
        assert key_entries[0]["id"] in key_collision.stderr
        assert key_entries[1]["id"] in key_collision.stderr

        # lock 属于别的包：先报包不匹配，而不是误报“旧 ID 需要迁移”。
        mismatch_root = root / "package-mismatch"
        (mismatch_root / "assets").mkdir(parents=True)
        (mismatch_root / "assets" / "item.txt").write_text("item", encoding="ascii")
        mismatch_policy = mismatch_root / "resources.toml"
        mismatch_policy.write_text(
            'package = "org.nandina.packagea"\n\n[[roots]]\npath = "assets"\n',
            encoding="utf-8",
        )
        assert run(nanres, mismatch_root, "scan").returncode == 0
        mismatch_policy.write_text(
            'package = "org.nandina.packageb"\n\n[[roots]]\npath = "assets"\n',
            encoding="utf-8",
        )
        mismatch = run(nanres, mismatch_root, "scan")
        assert mismatch.returncode == 1
        assert "does not match resources.toml" in mismatch.stderr
        assert "rewrite-lock" not in mismatch.stderr

        deferred = run(nanres, root, "watch")
        assert deferred.returncode == 3
        assert "hot-reload" in deferred.stderr

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
