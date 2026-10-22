#!/usr/bin/env python3
"""Clean, offline resource builds with no UI wrap checkouts available.

CI installs only the native compiler/build tools and OpenSSL development package.
Locally the metadata checks also reject any accidentally resolved UI dependency.
"""

import argparse
import json
import os
import pathlib
import shutil
import subprocess
import tempfile


def run(command, **kwargs):
    result = subprocess.run(command, text=True, capture_output=True, timeout=900, **kwargs)
    if result.returncode:
        raise RuntimeError(f"{command}\n{result.stdout}\n{result.stderr}")
    return result.stdout


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=pathlib.Path)
    parser.add_argument("--meson", default="meson")
    parser.add_argument("--jobs", default="4")
    args = parser.parse_args()
    source = args.source.resolve()
    meson = shutil.which(args.meson)
    if not meson:
        raise RuntimeError("Meson executable missing")
    # Nested fixture/checker invocations use the same Meson version.
    os.environ["PATH"] = str(pathlib.Path(meson).parent) + os.pathsep + os.environ["PATH"]
    with tempfile.TemporaryDirectory(prefix="nandina-resource-matrix-") as temporary:
        root = pathlib.Path(temporary)
        clean = root / "source"
        clean.mkdir()
        for name in ("meson.build", "meson_options.txt"):
            shutil.copyfile(source / name, clean / name)
        for name in ("nandina", "tools", "tests", "meson", "third_party"):
            shutil.copytree(source / name, clean / name, ignore=shutil.ignore_patterns("__pycache__"))
        wraps = clean / "subprojects"
        wraps.mkdir()
        for name in ("tomlplusplus", "Catch2"):
            shutil.copyfile(source / "subprojects" / (name + ".wrap"), wraps / (name + ".wrap"))
            shutil.copytree(source / "subprojects" / name, wraps / name, ignore=shutil.ignore_patterns(".git"))
        for tools, sqlite in ((True, "enabled"), (True, "disabled"), (False, "enabled"), (False, "disabled"), (False, "auto")):
            build = root / f"tools-{tools}-sqlite-{sqlite}"
            run([meson, "setup", str(build), str(clean), "--wrap-mode=nodownload", "-Dbuild_ui=false", f"-Dresource_tools={str(tools).lower()}", f"-Dresource_sqlite={sqlite}"])
            targets = json.loads(run([meson, "introspect", "--targets", str(build)]))
            names = {target["name"] for target in targets}
            assert ("nanres" in names) == tools
            assert ("resource_build" in names) == tools
            assert ("resource_sqlite" in names) == (sqlite != "disabled")
            assert ("sqlite3" in names) == (tools or sqlite != "disabled")
            forbidden = {"nandina", "freetype", "harfbuzz", "fribidi", "utf8proc", "SDL3-static", "SDL3-shared"}
            assert not names & forbidden, names & forbidden
            deps = json.loads(run([meson, "introspect", "--dependencies", str(build)]))
            assert not {dep["name"].lower() for dep in deps} & {"sdl3", "freetype2", "harfbuzz", "fribidi"}
            assert ("openssl" in {dep["name"] for dep in deps}) == tools
            run([meson, "compile", "-C", str(build), "-j", args.jobs])
            run([meson, "test", "-C", str(build), "--no-rebuild", "--print-errorlogs"])
            print(f"PASS tools={tools} sqlite={sqlite}: clean configure/build/resource tests", flush=True)
        invalid = subprocess.run([meson, "setup", str(root / "invalid"), str(clean), "--wrap-mode=nodownload", "-Dbuild_ui=false", "-Dexamples=true"], capture_output=True, text=True)
        assert invalid.returncode and "examples=true requires build_ui=true" in invalid.stdout

        # 外部工具：关闭本地工具时，兼容的 external_nanres 通过配置握手；不兼容的必须在配置期失败。
        good = root / "good-nanres"
        good.write_text(
            "#!/bin/sh\n"
            "case \"$1\" in\n"
            "  schema) echo 1 ;;\n"
            "  --version) echo 'nanres external-test' ;;\n"
            "  *) exit 9 ;;\n"
            "esac\n",
            encoding="ascii",
        )
        good.chmod(0o755)
        external = root / "external"
        run([meson, "setup", str(external), str(clean), "--wrap-mode=nodownload",
             "-Dbuild_ui=false", "-Dresource_tools=false",
             f"-Dexternal_nanres={good}"])
        names = {target["name"] for target in json.loads(run([meson, "introspect", "--targets", str(external)]))}
        assert "nanres" not in names and "resource_build" not in names, names

        bad = root / "bad-nanres"
        bad.write_text("#!/bin/sh\necho 2\n", encoding="ascii")
        bad.chmod(0o755)
        incompatible = subprocess.run(
            [meson, "setup", str(root / "incompatible"), str(clean), "--wrap-mode=nodownload",
             "-Dbuild_ui=false", "-Dresource_tools=false", f"-Dexternal_nanres={bad}"],
            capture_output=True,
            text=True,
        )
        assert incompatible.returncode and "schema 1" in incompatible.stdout, incompatible.stdout
    print("resource option matrix passed without UI wrap checkouts")


if __name__ == "__main__":
    main()
