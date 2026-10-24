#!/usr/bin/env python3

import contextlib
import hashlib
import json
import os
import pathlib
import shutil
import sqlite3
import subprocess
import sys
import tarfile
import tempfile
import tomllib


def working_tree_fingerprint(source: pathlib.Path) -> str:
    """工作区内容指纹：已跟踪改动的内容 + 未跟踪文件清单。

    SDK 可复现性断言会在同一测试内打包两次并比较产物哈希；如果两次之间工作区被改动
    （例如编辑器在测试运行期间保存文件、或并行任务改源码），产物会不同，而原始断言
    只报 artifacts 不相等，难以定位。这里显式检查并给出可操作的错误。
    """
    digest = hashlib.sha256()
    tracked_changes = subprocess.run(
        ["git", "-C", str(source), "diff", "HEAD", "--binary"],
        check=True,
        capture_output=True,
    ).stdout
    digest.update(tracked_changes)
    untracked = subprocess.run(
        ["git", "-C", str(source), "ls-files", "--others", "--exclude-standard", "-z"],
        check=True,
        capture_output=True,
    ).stdout
    digest.update(untracked)
    return digest.hexdigest()


def run(*args: str, cwd: pathlib.Path, environment: dict[str, str] | None = None) -> None:
    process_environment = os.environ.copy()
    if environment:
        process_environment.update(environment)
    result = subprocess.run(
        args, cwd=cwd, check=False, text=True, capture_output=True, env=process_environment
    )
    assert result.returncode == 0, result.stdout + result.stderr


def write_consumer(source: pathlib.Path, archive: pathlib.Path, sha256: str) -> None:
    resources = source / "resources"
    assets = resources / "assets"
    subprojects = source / "subprojects"
    assets.mkdir(parents=True)
    subprojects.mkdir()
    (assets / "hello.txt").write_text("hello from consumer", encoding="ascii")
    (resources / "resources.toml").write_text('package = "org.nandina.fixture"\n', encoding="ascii")
    (subprojects / "nandina.wrap").write_text(
        "[wrap-file]\n"
        "directory = nandina-sdk-0.0.0-test\n"
        "source_url = https://invalid.example/nandina-sdk-0.0.0-test-source.tar.xz\n"
        f"source_filename = {archive.name}\n"
        f"source_hash = {sha256}\n",
        encoding="ascii",
    )
    (source / "meson.build").write_text(
        "project('nandina_consumer_fixture', 'cpp')\n"
        # fixture 只验证 SDK 消费链路；vendored SDL3 的构建由专门的构建目录与视觉测试验证，
        # 这里关掉以免把集成测试的时长翻倍。
        "nandina = subproject('nandina', default_options: "
        "['build_tests=false', 'physics2d=disabled', 'sdl_backend=disabled'])\n"
        "toolchain = nandina.get_variable('nandina_resource_toolchain')\n"
        "policy = files('resources/resources.toml')\n"
        "package = custom_target(\n"
        "  'consumer-resources',\n"
        "  input: policy,\n"
        "  output: 'resources',\n"
        "  command: [toolchain['build_helper'], 'build', toolchain['nanres'], policy, '@OUTPUT@'],\n"
        "  build_by_default: true,\n"
        "  build_always_stale: true,\n"
        ")\n",
        encoding="ascii",
    )


def main() -> int:
    nandina_source = pathlib.Path(sys.argv[1]).resolve()
    bundle_tool = pathlib.Path(sys.argv[2]).resolve()
    with tempfile.TemporaryDirectory(prefix="nandina-consumer-") as directory:
        root = pathlib.Path(directory)
        release = root / "release"
        run(
            sys.executable,
            str(bundle_tool),
            "--source",
            str(nandina_source),
            "--output",
            str(release),
            "--version",
            "0.0.0-test",
            "--repository",
            "fixture",
            "--allow-dirty",
            cwd=nandina_source,
            environment={"SOURCE_DATE_EPOCH": "315532800"},
        )
        stable_fingerprint = working_tree_fingerprint(nandina_source)
        manifest = json.loads((release / "sdk-manifest.json").read_text(encoding="utf-8"))
        tar_artifact = next(item for item in manifest["artifacts"] if item["format"] == "tar.xz")
        assert any(item["format"] == "tar.zst" for item in manifest["artifacts"])
        assert any(item["format"] == "zip" for item in manifest["artifacts"])
        archive = release / tar_artifact["filename"]
        assert archive.stat().st_size == tar_artifact["size"]
        index = tomllib.loads((release / "index.toml").read_text(encoding="utf-8"))
        assert index["packages"][0]["sha256"] == tar_artifact["sha256"]
        assert manifest["directory"] == "nandina-sdk-0.0.0-test"
        assert manifest["minimum_cli"] == "0.2.0"
        # 依赖源码必须随 SDK 交付：默认 sdl3_source=vendored，缺 SDL3 源码会让消费者构建失败。
        dependencies = {item["name"]: item for item in manifest["dependencies"]}
        assert "SDL3" in dependencies, dependencies
        assert len(dependencies["SDL3"]["revision"]) == 40
        with tarfile.open(archive, "r:xz") as bundle:
            names = set(bundle.getnames())
        assert "nandina-sdk-0.0.0-test/subprojects/SDL3/CMakeLists.txt" in names
        assert isinstance(manifest["dirty"], bool)
        repeated_release = root / "release-repeated"
        run(
            sys.executable,
            str(bundle_tool),
            "--source",
            str(nandina_source),
            "--output",
            str(repeated_release),
            "--version",
            "0.0.0-test",
            "--repository",
            "fixture",
            "--allow-dirty",
            cwd=nandina_source,
            environment={"SOURCE_DATE_EPOCH": "315532800"},
        )
        repeated_manifest = json.loads(
            (repeated_release / "sdk-manifest.json").read_text(encoding="utf-8")
        )
        if working_tree_fingerprint(nandina_source) != stable_fingerprint:
            raise SystemExit(
                "working tree changed while the SDK determinism check was running; "
                "re-run on a stable tree (no editor writes or parallel source edits)"
            )
        assert repeated_manifest["artifacts"] == manifest["artifacts"]

        source = root / "source"
        build = root / "build"
        write_consumer(source, archive, tar_artifact["sha256"])
        package_cache = source / "subprojects/packagecache"
        package_cache.mkdir(parents=True)
        shutil.copy2(archive, package_cache / archive.name)
        run("meson", "setup", str(build), "--wrap-mode=nodownload", cwd=source)
        run("meson", "compile", "-C", str(build), "consumer-resources", cwd=source)

        package = build / "resources/resources.db"
        assert package.is_file()
        metadata = json.loads((build / "resources/resource-location.json").read_text())
        assert metadata["package_id"] == "org.nandina.fixture"
        lock = source / "resources/resources.lock.toml"
        assert lock.is_file()
        with contextlib.closing(sqlite3.connect(package)) as database:
            keys = [
                row[0]
                for row in database.execute(
                    "select resource_key from resources order by resource_key"
                )
            ]
        assert keys == ["hello.txt"]
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
