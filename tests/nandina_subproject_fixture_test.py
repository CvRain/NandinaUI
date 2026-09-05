#!/usr/bin/env python3

import json
import os
import pathlib
import shutil
import sqlite3
import subprocess
import sys
import tempfile
import tomllib


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
        "nandina = subproject('nandina', default_options: "
        "['build_tests=false', 'physics2d=disabled'])\n"
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
        with sqlite3.connect(package) as database:
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
