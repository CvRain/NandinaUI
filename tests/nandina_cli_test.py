#!/usr/bin/env python3

import pathlib
import subprocess
import sys
import tempfile


def run(nandina: str, cwd: pathlib.Path, *args: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [nandina, *args],
        cwd=cwd,
        check=False,
        text=True,
        capture_output=True,
    )


def main() -> int:
    nandina = str(pathlib.Path(sys.argv[1]).resolve())
    nandina_source = pathlib.Path(sys.argv[2]).resolve()
    with tempfile.TemporaryDirectory(prefix="nandina-cli-") as directory:
        root = pathlib.Path(directory)
        project = root / "hello"

        created = run(
            nandina,
            root,
            "new",
            str(project),
            "--package",
            "org.example.hello",
        )
        assert created.returncode == 0, created.stderr
        assert (project / "meson.build").is_file()
        assert (project / "src" / "main.cpp").is_file()
        assert (project / "resources" / "resources.toml").is_file()
        assert (project / "resources" / "assets" / ".gitkeep").is_file()
        assert (project / ".nandina" / "target").read_text(encoding="utf-8") == "hello\n"
        assert (project / ".gitignore").is_file()
        linked_source = project / "subprojects" / "nandina"
        assert linked_source.is_symlink()
        assert linked_source.resolve() == nandina_source

        meson_text = (project / "meson.build").read_text(encoding="utf-8")
        main_text = (project / "src" / "main.cpp").read_text(encoding="utf-8")
        manifest_text = (project / "resources" / "resources.toml").read_text(encoding="utf-8")
        assert "subproject('nandina'" in meson_text
        assert "nandina_resource_toolchain" in meson_text
        assert "app::run<MainPage>" in main_text
        assert 'package = "org.example.hello"' in manifest_text

        build = project / "build"
        compiled = run(nandina, root, "build", str(project))
        assert compiled.returncode == 0, compiled.stdout + compiled.stderr
        assert (build / "hello").is_file()
        assert (build / "resources" / "resources.db").is_file()
        assert (build / "resources" / "resource-location.json").is_file()
        assert (project / "resources" / "resources.lock.toml").is_file()

        incremental = run(
            nandina,
            root,
            "build",
            str(project),
            "--build-dir",
            "build",
        )
        assert incremental.returncode == 0, incremental.stdout + incremental.stderr
        assert "configuring" not in incremental.stdout

        project_doctor = run(nandina, root, "doctor", str(project))
        assert project_doctor.returncode == 0, project_doctor.stdout + project_doctor.stderr
        assert "project ok" in project_doctor.stdout
        assert "all checks passed" in project_doctor.stdout

        original_executable = build / "hello.real"
        (build / "hello").rename(original_executable)
        marker = root / "run-arguments.txt"
        runner = build / "hello"
        runner.write_text(
            "#!/bin/sh\n"
            "printf 'forwarded\\n' > \"$1\"\n"
            "exit 7\n",
            encoding="utf-8",
        )
        runner.chmod(0o755)
        executed = run(
            nandina,
            root,
            "run",
            str(project),
            "--no-build",
            "--",
            str(marker),
        )
        assert executed.returncode == 7, executed.stdout + executed.stderr
        assert marker.read_text(encoding="utf-8") == "forwarded\n"

        explicit_project = root / "explicit-source"
        explicit = run(
            nandina,
            root,
            "new",
            str(explicit_project),
            "--nandina-source",
            str(nandina_source),
        )
        assert explicit.returncode == 0, explicit.stderr
        assert (explicit_project / "subprojects" / "nandina").resolve() == nandina_source

        empty_destination = root / "empty-destination"
        empty_destination.mkdir()
        empty = run(
            nandina,
            root,
            "new",
            str(empty_destination) + "/",
            "--nandina-source",
            str(nandina_source),
        )
        assert empty.returncode == 0, empty.stderr
        assert (empty_destination / "src" / "main.cpp").is_file()

        # 已存在且非空的目标目录应拒绝。
        duplicate = run(nandina, root, "new", str(project), "--package", "org.example.hello")
        assert duplicate.returncode == 2

        # 缺路径报错。
        no_path = run(nandina, root, "new")
        assert no_path.returncode == 2

        invalid_source_project = root / "invalid-source"
        invalid_source = run(
            nandina,
            root,
            "new",
            str(invalid_source_project),
            "--nandina-source",
            str(root / "missing-nandina"),
        )
        assert invalid_source.returncode == 2
        assert not invalid_source_project.exists()

        version = run(nandina, root, "--version")
        assert version.returncode == 0
        assert version.stdout.startswith("nandina ")

        toolchain_doctor = run(nandina, root, "doctor")
        assert toolchain_doctor.returncode == 0, toolchain_doctor.stderr
        assert "toolchain-only check" in toolchain_doctor.stdout

        source_doctor = run(nandina, nandina_source, "doctor")
        assert source_doctor.returncode == 0, source_doctor.stdout + source_doctor.stderr
        assert "project ok" in source_doctor.stdout

        invalid_project = run(nandina, root, "build", str(root / "missing-project"))
        assert invalid_project.returncode == 2

        invalid_build_option = run(nandina, root, "build", str(project), "--no-build")
        assert invalid_build_option.returncode == 2

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
