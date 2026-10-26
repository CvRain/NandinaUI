#!/usr/bin/env python3

import contextlib
import json
import os
import pathlib
import shutil
import sqlite3
import subprocess
import sys
import tempfile


# Log signatures the framework emits for failures the smoke run must not hide.
RUNTIME_ERROR_MARKERS = (
    "NanWindow: page error",
    "resource lookup failed",
    "texture decode/upload failed",
    "[error]",
)
ROUTES = ("login", "notes", "focus", "preview")


def run_route(executable: pathlib.Path, route: str, timeout: int = 15) -> subprocess.CompletedProcess[str]:
    environment = os.environ.copy()
    environment.update(
        {
            "NANDINA_LAB_START_ROUTE": route,
            "NANDINA_LAB_SMOKE_SECONDS": "1",
        }
    )
    if environment.get("DISPLAY"):
        environment.setdefault("SDL_VIDEODRIVER", "x11")
    return subprocess.run(
        [str(executable)],
        check=False,
        text=True,
        capture_output=True,
        env=environment,
        timeout=timeout,
    )


def assert_clean(result: subprocess.CompletedProcess[str], context: str) -> None:
    combined = result.stdout + result.stderr
    assert result.returncode == 0, (context, combined)
    for marker in RUNTIME_ERROR_MARKERS:
        assert marker not in combined, (context, marker, combined)


def check_build(executable: pathlib.Path, resources: pathlib.Path, header: pathlib.Path) -> None:
    assert executable.is_file(), executable
    database = resources / "resources.db"
    assert database.is_file(), database
    with contextlib.closing(sqlite3.connect(database)) as connection:
        rows = connection.execute(
            "SELECT resource_key, media_type, storage FROM resources ORDER BY resource_key"
        ).fetchall()
    assert rows == [
        ("images/lab-cover.png", "image/png", 0),
        ("images/lab-logo.png", "image/png", 0),
    ], rows
    metadata = json.loads((resources / "resource-location.json").read_text(encoding="utf-8"))
    assert metadata["package_id"] == "com.nandina.lab", metadata
    assert pathlib.Path(metadata["package_root"]).is_dir(), metadata
    text = header.read_text(encoding="utf-8")
    assert 'RESOURCE_IMAGES_LAB_COVER_PNG = "images/lab-cover.png"' in text, text
    assert 'RESOURCE_IMAGES_LAB_LOGO_PNG = "images/lab-logo.png"' in text, text
    assert "sha256" not in text.lower(), text


def check_run(
    executable: pathlib.Path,
    resources: pathlib.Path,
    stage_helper: pathlib.Path,
) -> int:
    if not (os.environ.get("DISPLAY") or os.environ.get("WAYLAND_DISPLAY")):
        return 77

    # Build-tree package path: `<exe-dir>/resources/resource-location.json` is what
    # development builds use; portable staging below covers the metadata-free fallback.
    assert (resources / "resource-location.json").is_file(), resources
    assert_clean(run_route(executable, "preview"), "build-tree preview")

    with tempfile.TemporaryDirectory(prefix="nandina-lab-stage-") as directory:
        root = pathlib.Path(directory)
        staged = root / "portable"
        moved = root / "relocated"
        subprocess.run(
            [
                sys.executable,
                str(stage_helper),
                "--executable",
                str(executable),
                "--resources",
                str(resources),
                "--output",
                str(staged),
            ],
            check=True,
            text=True,
            capture_output=True,
        )
        assert (staged / "resources" / "resources.db").is_file()
        assert not (staged / "resources" / "resource-location.json").exists()
        staged.rename(moved)

        # Every route is built and rendered once from the relocated tree: page build
        # errors, a broken resource lookup or a timer-control regression must fail here.
        executable_in_stage = moved / executable.name
        for route in ROUTES:
            assert_clean(run_route(executable_in_stage, route), f"portable {route}")
    return 0


def check_incremental(
    nanres: pathlib.Path,
    helper: pathlib.Path,
    policy_dir: pathlib.Path,
) -> None:
    with tempfile.TemporaryDirectory(prefix="nandina-lab-incremental-") as directory:
        root = pathlib.Path(directory)
        policy = root / "policy"
        output = root / "out"
        shutil.copytree(policy_dir, policy)
        policy_file = policy / "resources.toml"
        assets = policy / "assets"

        def build() -> subprocess.CompletedProcess[str]:
            return subprocess.run(
                [
                    sys.executable,
                    str(helper),
                    "build",
                    str(nanres),
                    str(policy_file),
                    str(output),
                ],
                check=False,
                text=True,
                capture_output=True,
            )

        def keys() -> list[str]:
            with contextlib.closing(sqlite3.connect(output / "resources.db")) as connection:
                return [
                    row[0]
                    for row in connection.execute(
                        "SELECT resource_key FROM resources ORDER BY resource_key"
                    )
                ]

        first = build()
        assert first.returncode == 0, first.stdout + first.stderr
        assert keys() == ["images/lab-cover.png", "images/lab-logo.png"]

        # Directory membership must be discovered without a policy edit.
        (assets / "probe.txt").write_text("probe", encoding="ascii")
        second = build()
        assert second.returncode == 0, second.stdout + second.stderr
        assert "probe.txt" in keys()

        database = output / "resources.db"
        unchanged_mtime = database.stat().st_mtime_ns
        third = build()
        assert third.returncode == 0, third.stdout + third.stderr
        assert database.stat().st_mtime_ns == unchanged_mtime

        (assets / "probe.txt").unlink()
        fourth = build()
        assert fourth.returncode == 0, fourth.stdout + fourth.stderr
        assert "probe.txt" not in keys()

        before_failure = database.read_bytes()
        policy_file.write_text(
            policy_file.read_text(encoding="utf-8")
            + '\n[[resources]]\nkey = "broken"\npath = "assets/missing.txt"\n',
            encoding="utf-8",
        )
        failed = build()
        assert failed.returncode != 0, failed.stdout + failed.stderr
        assert database.read_bytes() == before_failure


def main() -> int:
    mode = sys.argv[1]
    executable = pathlib.Path(sys.argv[2]).resolve()
    resources = pathlib.Path(sys.argv[3]).resolve()
    if mode == "build":
        check_build(executable, resources, pathlib.Path(sys.argv[4]).resolve())
        return 0
    if mode == "run":
        return check_run(executable, resources, pathlib.Path(sys.argv[4]).resolve())
    if mode == "incremental":
        check_incremental(
            executable,
            resources,
            pathlib.Path(sys.argv[4]).resolve(),
        )
        return 0
    raise SystemExit(f"unknown example-lab test mode: {mode}")


if __name__ == "__main__":
    raise SystemExit(main())
