#!/usr/bin/env python3

import argparse
import datetime
import hashlib
import io
import json
import lzma
import os
import pathlib
import re
import shutil
import stat
import subprocess
import tarfile
import tempfile
import tomllib
import zipfile


IDENTIFIER = re.compile(r"^[A-Za-z0-9_.-]+$")


def git(source: pathlib.Path, *arguments: str) -> bytes:
    result = subprocess.run(
        ["git", "-C", str(source), *arguments],
        check=False,
        capture_output=True,
    )
    if result.returncode != 0:
        raise RuntimeError(result.stderr.decode("utf-8", errors="replace").strip())
    return result.stdout


def git_paths(source: pathlib.Path, allow_dirty: bool) -> list[pathlib.Path]:
    status = git(
        source,
        "status",
        "--porcelain=v1",
        "--untracked-files=all",
        "--ignore-submodules=none",
    )
    if status and not allow_dirty:
        raise RuntimeError("SDK bundles require a clean recursive checkout")
    tracked = git(source, "ls-files", "--recurse-submodules", "-z").split(b"\0")
    paths = {pathlib.Path(value.decode("utf-8")) for value in tracked if value}
    if allow_dirty:
        untracked = git(source, "ls-files", "--others", "--exclude-standard", "-z").split(b"\0")
        paths.update(pathlib.Path(value.decode("utf-8")) for value in untracked if value)
    return sorted(path for path in paths if (source / path).is_file() or (source / path).is_symlink())


def submodule_manifest(source: pathlib.Path) -> list[dict[str, object]]:
    result: list[dict[str, object]] = []
    output = git(source, "submodule", "status", "--recursive").decode("utf-8")
    for line in output.splitlines():
        state = line[0]
        revision, path, *_ = line[1:].split()
        result.append(
            {
                "path": path,
                "revision": revision,
                "initialized": state != "-",
                "matches_index": state == " ",
            }
        )
    return result


def archive_metadata(timestamp: int, mode: int) -> dict[str, object]:
    return {
        "mtime": timestamp,
        "mode": mode,
        "uid": 0,
        "gid": 0,
        "uname": "",
        "gname": "",
    }


def safe_symlink_target(path: pathlib.Path, target: str) -> pathlib.PurePosixPath:
    if pathlib.PurePosixPath(target).is_absolute():
        raise RuntimeError(f"absolute symlink is not allowed in SDK bundle: {path}")
    normalized = pathlib.PurePosixPath(os.path.normpath(path.parent.as_posix() + "/" + target))
    if normalized.parts and normalized.parts[0] == "..":
        raise RuntimeError(f"escaping symlink is not allowed in SDK bundle: {path}")
    return normalized


def tar_info(name: str, timestamp: int, mode: int) -> tarfile.TarInfo:
    info = tarfile.TarInfo(name)
    for key, value in archive_metadata(timestamp, mode).items():
        setattr(info, key, value)
    return info


def write_tar_bundle(
    source: pathlib.Path,
    output: pathlib.Path,
    root_name: str,
    paths: list[pathlib.Path],
    internal_manifest: bytes,
    timestamp: int,
    compression: str,
) -> None:
    with tempfile.TemporaryDirectory(prefix="nandina-sdk-tar-") as directory:
        tar_path = pathlib.Path(directory) / "bundle.tar"
        with tarfile.open(tar_path, "w", format=tarfile.PAX_FORMAT) as archive:
            directories = {pathlib.PurePosixPath(root_name)}
            for path in paths:
                parent = pathlib.PurePosixPath(root_name) / path.parent.as_posix()
                directories.update(parent.parents)
                directories.add(parent)
            for directory_path in sorted(
                directories, key=lambda value: (len(value.parts), str(value))
            ):
                if str(directory_path) == ".":
                    continue
                info = tar_info(str(directory_path) + "/", timestamp, 0o755)
                info.type = tarfile.DIRTYPE
                archive.addfile(info)
            manifest_info = tar_info(f"{root_name}/nandina-sdk.json", timestamp, 0o644)
            manifest_info.size = len(internal_manifest)
            archive.addfile(manifest_info, io.BytesIO(internal_manifest))
            for relative in paths:
                path = source / relative
                archive_path = f"{root_name}/{relative.as_posix()}"
                file_stat = path.lstat()
                if path.is_symlink():
                    target = os.readlink(path)
                    safe_symlink_target(relative, target)
                    info = tar_info(archive_path, timestamp, 0o777)
                    info.type = tarfile.SYMTYPE
                    info.linkname = target
                    archive.addfile(info)
                    continue
                mode = 0o755 if file_stat.st_mode & stat.S_IXUSR else 0o644
                info = tar_info(archive_path, timestamp, mode)
                info.size = file_stat.st_size
                with path.open("rb") as stream:
                    archive.addfile(info, stream)
        if compression == "xz":
            with tar_path.open("rb") as stream, lzma.open(output, "wb", preset=6) as compressed:
                shutil.copyfileobj(stream, compressed)
            return
        if compression == "zstd":
            result = subprocess.run(
                ["zstd", "-15", "--threads=1", "--force", str(tar_path), "-o", str(output)],
                check=False,
                capture_output=True,
                text=True,
            )
            if result.returncode != 0:
                raise RuntimeError(result.stderr.strip() or "zstd failed")
            return
        raise RuntimeError(f"unsupported tar compression: {compression}")


def write_zip(
    source: pathlib.Path,
    output: pathlib.Path,
    root_name: str,
    paths: list[pathlib.Path],
    internal_manifest: bytes,
    timestamp: int,
) -> None:
    instant = datetime.datetime.fromtimestamp(max(timestamp, 315532800), datetime.UTC)
    date_time = (
        instant.year,
        instant.month,
        instant.day,
        instant.hour,
        instant.minute,
        instant.second,
    )
    with zipfile.ZipFile(output, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        entries: list[tuple[str, bytes, int]] = [
            (f"{root_name}/nandina-sdk.json", internal_manifest, 0o644)
        ]
        for relative in paths:
            path = source / relative
            if path.is_symlink():
                resolved = source / safe_symlink_target(relative, os.readlink(path))
                if not resolved.is_file():
                    raise RuntimeError(f"ZIP bundle cannot materialize symlink: {relative}")
                content = resolved.read_bytes()
                mode = 0o644
            else:
                content = path.read_bytes()
                mode = 0o755 if path.stat().st_mode & stat.S_IXUSR else 0o644
            entries.append((f"{root_name}/{relative.as_posix()}", content, mode))
        for name, content, mode in entries:
            info = zipfile.ZipInfo(name, date_time=date_time)
            info.create_system = 3
            info.external_attr = (stat.S_IFREG | mode) << 16
            info.compress_type = zipfile.ZIP_DEFLATED
            archive.writestr(info, content, compress_type=zipfile.ZIP_DEFLATED, compresslevel=9)


def file_record(path: pathlib.Path, archive_format: str) -> dict[str, object]:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        while block := stream.read(1024 * 1024):
            digest.update(block)
    return {
        "format": archive_format,
        "filename": path.name,
        "size": path.stat().st_size,
        "sha256": digest.hexdigest(),
    }


def write_atomic(path: pathlib.Path, content: str) -> None:
    temporary = path.with_suffix(path.suffix + ".tmp")
    temporary.write_text(content, encoding="utf-8")
    os.replace(temporary, path)


def release_metadata(source: pathlib.Path) -> dict[str, object]:
    path = source / "release-metadata.toml"
    if not path.is_file():
        return {}
    with path.open("rb") as stream:
        return tomllib.load(stream)


def main() -> int:
    parser = argparse.ArgumentParser(description="Build reproducible NandinaUI source SDK bundles")
    parser.add_argument("--source", type=pathlib.Path, default=pathlib.Path.cwd())
    parser.add_argument("--output", type=pathlib.Path, required=True)
    parser.add_argument("--version")
    parser.add_argument("--minimum-cli")
    parser.add_argument("--repository", default="official")
    parser.add_argument("--channel")
    parser.add_argument("--allow-dirty", action="store_true")
    options = parser.parse_args()
    metadata = release_metadata(options.source.resolve())
    version = options.version or str(metadata.get("sdk_version", ""))
    minimum_cli = options.minimum_cli or str(metadata.get("minimum_cli", ""))
    channel = options.channel or str(metadata.get("release_channel", "stable"))
    for name, value in {
        "version": version,
        "minimum CLI version": minimum_cli,
        "repository": options.repository,
        "channel": channel,
    }.items():
        if not IDENTIFIER.fullmatch(value):
            raise SystemExit(f"invalid {name}: {value}")

    source = options.source.resolve()
    output = options.output.resolve()
    revision = git(source, "rev-parse", "HEAD").decode("ascii").strip()
    dirty = bool(git(source, "status", "--porcelain=v1", "--ignore-submodules=none"))
    timestamp_text = os.environ.get("SOURCE_DATE_EPOCH")
    timestamp = int(timestamp_text) if timestamp_text else int(
        git(source, "show", "-s", "--format=%ct", "HEAD").decode("ascii")
    )
    paths = git_paths(source, options.allow_dirty)
    output.mkdir(parents=True, exist_ok=True)
    root_name = f"nandina-sdk-{version}"
    license_path = next(
        (name for name in ("LICENSE", "LICENSE.txt", "COPYING") if (source / name).is_file()),
        None,
    )
    internal = {
        "schema": 1,
        "name": "NandinaUI",
        "version": version,
        "minimum_cli": minimum_cli,
        "revision": revision,
        "dirty": dirty,
        "license": license_path,
        "submodules": submodule_manifest(source),
    }
    for key in ("release_channel", "support_profile", "license_status", "known_limitations"):
        if key in metadata:
            internal[key] = metadata[key]
    internal_bytes = (json.dumps(internal, indent=2, sort_keys=True) + "\n").encode("utf-8")

    xz_path = output / f"{root_name}-source.tar.xz"
    zstd_path = output / f"{root_name}-source.tar.zst"
    zip_path = output / f"{root_name}-source.zip"
    write_tar_bundle(source, xz_path, root_name, paths, internal_bytes, timestamp, "xz")
    write_tar_bundle(source, zstd_path, root_name, paths, internal_bytes, timestamp, "zstd")
    write_zip(source, zip_path, root_name, paths, internal_bytes, timestamp)
    artifacts = [
        file_record(xz_path, "tar.xz"),
        file_record(zstd_path, "tar.zst"),
        file_record(zip_path, "zip"),
    ]
    manifest = dict(internal)
    manifest["directory"] = root_name
    manifest["artifacts"] = artifacts
    write_atomic(output / "sdk-manifest.json", json.dumps(manifest, indent=2, sort_keys=True) + "\n")

    primary = artifacts[0]
    index = (
        "schema = 1\n"
        f'repository = "{options.repository}"\n\n'
        "[[packages]]\n"
        'name = "NandinaUI"\n'
        f'version = "{version}"\n'
        f'channel = "{channel}"\n'
        f'artifact = "{primary["filename"]}"\n'
        f'size = {primary["size"]}\n'
        f'sha256 = "{primary["sha256"]}"\n'
        f'directory = "{root_name}"\n'
        f'minimum_cli = "{minimum_cli}"\n'
    )
    write_atomic(output / "index.toml", index)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
