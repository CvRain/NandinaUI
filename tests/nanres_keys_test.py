#!/usr/bin/env python3

import os
import pathlib
import re
import shutil
import subprocess
import sys
import tempfile


POLICY = """\
package = "org.nandina.keys"
source = "assets"

[[resources]]
key = "images/icon.png"
path = "assets/icon.png"

[[resources]]
key = "labels/title"
path = "assets/title.txt"

[aliases]
"images/current" = "images/icon.png"
"""


def run(nanres: str, cwd: pathlib.Path, *args: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [nanres, *args],
        cwd=cwd,
        check=False,
        text=True,
        capture_output=True,
    )


def compile_consumer(
    compiler: str, source: pathlib.Path, include: pathlib.Path
) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [compiler, "-std=c++17", "-fsyntax-only", "-I", str(include), str(source)],
        check=False,
        text=True,
        capture_output=True,
    )


def main() -> int:
    nanres = str(pathlib.Path(sys.argv[1]).resolve())
    compiler = shutil.which(os.environ.get("CXX", "c++"))
    assert compiler, "nanres-keys requires a C++ compiler in PATH or CXX"

    with tempfile.TemporaryDirectory(prefix="nandina-keys-") as directory:
        root = pathlib.Path(directory)
        assets = root / "assets"
        assets.mkdir()
        (assets / "icon.png").write_bytes(b"\x89PNG\r\n\x1a\n")
        (assets / "title.txt").write_text("title", encoding="ascii")
        policy = root / "resources.toml"
        policy.write_text(POLICY, encoding="utf-8")
        output = root / "build"
        header = output / "resource_keys.hpp"

        generated = run(nanres, root, "keys", "--header", "build/resource_keys.hpp")
        assert generated.returncode == 0, generated.stdout + generated.stderr
        assert generated.stdout.startswith("generated\t"), generated.stdout
        assert not (root / "resources.lock.toml").exists()

        text = header.read_text(encoding="utf-8")
        assert 'RESOURCE_IMAGES_ICON_PNG = "images/icon.png"' in text, text
        assert 'RESOURCE_IMAGES_CURRENT = "images/current"' in text, text
        assert 'RESOURCE_LABELS_TITLE = "labels/title"' in text, text
        assert "sha256" not in text.lower(), text
        assert str(root) not in text, text
        assert "assets/" not in text, text
        assert (
            re.search(
                r"[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}", text
            )
            is None
        ), text
        assert "#include <string_view>" in text, text

        consumer = root / "consumer.cpp"
        consumer.write_text(
            '#include "resource_keys.hpp"\n'
            "#include <string_view>\n"
            "namespace keys = nandina::resource_keys::pkg_orgDOTnandinaDOTkeys;\n"
            'static_assert(keys::RESOURCE_IMAGES_ICON_PNG == "images/icon.png");\n'
            'static_assert(keys::RESOURCE_LABELS_TITLE == "labels/title");\n'
            "int main() { return 0; }\n",
            encoding="utf-8",
        )
        compiled = compile_consumer(compiler, consumer, output)
        assert compiled.returncode == 0, compiled.stderr

        # 无变化时不重写头文件，避免下游因为纯时间戳变化而重编。
        modified = header.stat().st_mtime_ns
        unchanged = run(nanres, root, "keys", "--header", "build/resource_keys.hpp")
        assert unchanged.returncode == 0, unchanged.stderr
        assert unchanged.stdout.startswith("unchanged\t"), unchanged.stdout
        assert header.stat().st_mtime_ns == modified

        # 删除 `labels/title` 声明并重生成：旧符号必须消失，使用它的消费者编译失败。
        policy.write_text(
            POLICY.replace(
                '\n[[resources]]\nkey = "labels/title"\npath = "assets/title.txt"\n', ""
            ),
            encoding="utf-8",
        )
        regenerated = run(nanres, root, "keys", "--header", "build/resource_keys.hpp")
        assert regenerated.returncode == 0, regenerated.stdout + regenerated.stderr
        assert "RESOURCE_LABELS_TITLE" not in header.read_text(encoding="utf-8")
        title_consumer = root / "title_consumer.cpp"
        title_consumer.write_text(
            '#include "resource_keys.hpp"\n'
            "namespace keys = nandina::resource_keys::pkg_orgDOTnandinaDOTkeys;\n"
            "int main() { return keys::RESOURCE_LABELS_TITLE.empty(); }\n",
            encoding="utf-8",
        )
        missing = compile_consumer(compiler, title_consumer, output)
        assert missing.returncode != 0, missing.stdout + missing.stderr
        assert "RESOURCE_LABELS_TITLE" in missing.stderr, missing.stderr

        policy.write_text(POLICY, encoding="utf-8")
        restored = run(nanres, root, "keys", "--header", "build/resource_keys.hpp")
        assert restored.returncode == 0, restored.stdout + restored.stderr
        assert compile_consumer(compiler, title_consumer, output).returncode == 0

        # 两个 key 折叠成同一符号时必须报错，不能静默改名。
        conflict = root / "conflict"
        conflict_assets = conflict / "assets"
        conflict_assets.mkdir(parents=True)
        (conflict_assets / "a").write_text("a", encoding="ascii")
        (conflict_assets / "b").write_text("b", encoding="ascii")
        (conflict / "resources.toml").write_text(
            'package = "org.nandina.collision"\n'
            'source = "assets"\n\n'
            "[[resources]]\n"
            'key = "foo/bar"\n'
            'path = "assets/a"\n\n'
            "[[resources]]\n"
            'key = "foo-bar"\n'
            'path = "assets/b"\n',
            encoding="utf-8",
        )
        collision = run(nanres, conflict, "keys", "--header", "keys.hpp")
        assert collision.returncode == 1, collision.stdout + collision.stderr
        assert "foo/bar" in collision.stderr and "foo-bar" in collision.stderr, collision.stderr
        assert "RESOURCE_FOO_BAR" in collision.stderr, collision.stderr
        assert not (conflict / "keys.hpp").exists()

        alias_conflict = root / "alias-conflict"
        alias_assets = alias_conflict / "assets"
        alias_assets.mkdir(parents=True)
        (alias_assets / "icon.png").write_bytes(b"icon")
        (alias_conflict / "resources.toml").write_text(
            'package = "org.nandina.alias-collision"\n'
            'source = "assets"\n\n'
            "[[resources]]\n"
            'key = "images/icon.png"\n'
            'path = "assets/icon.png"\n\n'
            "[aliases]\n"
            '"images/icon-png" = "images/icon.png"\n',
            encoding="utf-8",
        )
        alias_collision = run(nanres, alias_conflict, "keys", "--header", "keys.hpp")
        assert alias_collision.returncode == 1, alias_collision.stdout + alias_collision.stderr
        assert "images/icon-png" in alias_collision.stderr, alias_collision.stderr
        assert "RESOURCE_IMAGES_ICON_PNG" in alias_collision.stderr, alias_collision.stderr

        # 包名中的 . - _ 必须转义成不同的 C++ 命名空间，否则跨包会折叠到同一个符号空间。
        escaped_root = root / "escaped"
        (escaped_root / "assets").mkdir(parents=True)
        (escaped_root / "assets" / "icon.png").write_bytes(b"icon")
        (escaped_root / "resources.toml").write_text(
            'package = "org.nandina-key_demo"\n'
            'source = "assets"\n\n'
            "[[resources]]\n"
            'key = "images/icon.png"\n'
            'path = "assets/icon.png"\n',
            encoding="utf-8",
        )
        escaped = run(nanres, escaped_root, "keys", "--header", "build/key_demo.hpp")
        assert escaped.returncode == 0, escaped.stdout + escaped.stderr
        escaped_header = (escaped_root / "build/key_demo.hpp").read_text(encoding="utf-8")
        escaped_namespace = "pkg_orgDOTnandinaDASHkeyUNDERSCOREdemo"
        assert (
            f"namespace nandina::resource_keys::{escaped_namespace}" in escaped_header
        ), escaped_header
        escaped_consumer = escaped_root / "consumer.cpp"
        escaped_consumer.write_text(
            '#include "key_demo.hpp"\n'
            f"namespace keys = nandina::resource_keys::{escaped_namespace};\n"
            'static_assert(keys::RESOURCE_IMAGES_ICON_PNG == "images/icon.png");\n'
            "int main() { return 0; }\n",
            encoding="utf-8",
        )
        escaped_compiled = compile_consumer(compiler, escaped_consumer, escaped_root / "build")
        assert escaped_compiled.returncode == 0, escaped_compiled.stderr

        # 空 policy 也能生成头：没有资源时不产生常量，只有命名空间与包含保护。
        empty_root = root / "empty"
        (empty_root / "assets").mkdir(parents=True)
        (empty_root / "resources.toml").write_text(
            'package = "org.nandina.empty"\nsource = "assets"\n', encoding="utf-8"
        )
        empty = run(nanres, empty_root, "keys", "--header", "keys.hpp")
        assert empty.returncode == 0, empty.stdout + empty.stderr
        empty_header = (empty_root / "keys.hpp").read_text(encoding="utf-8")
        assert "inline constexpr" not in empty_header, empty_header
        assert (
            "namespace nandina::resource_keys::pkg_orgDOTnandinaDOTempty" in empty_header
        ), empty_header

        # 生成头只接受 --header；扫描覆盖必须走原始 scan 命令。
        missing_header = run(nanres, root, "keys")
        assert missing_header.returncode == 2, missing_header.stderr
        scanned = run(nanres, root, "keys", "--header", "other.hpp", "--root", "assets")
        assert scanned.returncode == 2, scanned.stderr
        locked = run(nanres, root, "keys", "--header", "other.hpp", "--lock", "other.lock")
        assert locked.returncode == 2, locked.stderr
        output_override = run(
            nanres, root, "keys", "--header", "other.hpp", "--output", "package"
        )
        assert output_override.returncode == 2, output_override.stderr
        wrong_command = run(nanres, root, "scan", "--header", "other.hpp")
        assert wrong_command.returncode == 2, wrong_command.stderr

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
