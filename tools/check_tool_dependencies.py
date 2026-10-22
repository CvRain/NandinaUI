#!/usr/bin/env python3
"""Audit nanres metadata and the Ninja input graph, including archive provenance.

Run after building nanres. Unknown local libraries/objects, missing metadata and
unreadable response files fail closed. The allowlist describes D5's tool boundary.
"""

import argparse
from collections import Counter
import json
import pathlib
import re
import shlex
import subprocess
import sys


TARGETS = {"nanres", "resource_core", "resource_build", "sqlite3"}
SOURCES = {
    "tools/nanres_main.cpp", "tools/resource_manifest.cpp", "tools/resource_packager.cpp",
    "nandina/resource/resource.cpp", "nandina/resource/resource_backend.cpp",
    "nandina/resource/resource_stream.cpp", "nandina/resource/file_resource_stream.cpp",
    "nandina/resource/resource_manager.cpp", "nandina/resource/resource_uri.cpp",
    "nandina/resource/resource_scanner.cpp", "third_party/sqlite3/sqlite3.c",
}
FORBIDDEN = re.compile(
    r"(?:nandina[/_](?:app|scene|text|render|widget)[/_]|libnandina[.]|"
    r"freetype|harfbuzz|fribidi|utf8proc|sdl3|raylib|resource_runtime|sqlite_backend)", re.I
)


def run(command, cwd=None):
    result = subprocess.run(command, cwd=cwd, text=True, capture_output=True, check=True)
    return result.stdout


def audit(build, meson="meson", ninja="ninja"):
    build = pathlib.Path(build).resolve()
    targets = json.loads(run([meson, "introspect", "--targets", str(build)]))
    dependencies = json.loads(run([meson, "introspect", "--dependencies", str(build)]))
    if not isinstance(targets, list) or not targets or not isinstance(dependencies, list):
        raise ValueError("missing target/dependency metadata")
    by_id = {target["id"]: target for target in targets}
    by_output = {pathlib.Path(name).resolve(): target for target in targets for name in target["filename"]}
    by_dep = {dep.get("id", dep["name"]): dep for dep in dependencies}
    roots = [target for target in targets if target["name"] == "nanres"]
    if len(roots) != 1 or roots[0]["type"] != "executable":
        raise ValueError("expected exactly one nanres executable")
    source_root = pathlib.Path(roots[0]["defined_in"]).parent
    visited = set()

    def safe(value):
        if FORBIDDEN.search(str(value).replace("\\", "/")):
            raise ValueError(f"forbidden tool dependency: {value}")

    def path(value):
        result = pathlib.Path(value)
        return (result if result.is_absolute() else build / result).resolve()

    def parameters(values, active=None):
        active = set() if active is None else active
        for value in values:
            safe(value)
            if value.startswith("@") or value.startswith("-Wl,@"):
                response = path(value.split("@", 1)[1])
                if response in active:
                    raise ValueError(f"recursive response file: {response}")
                parameters(shlex.split(response.read_text()), active | {response})
            elif path(value) in by_output:
                visit(by_output[path(value)])
            elif re.search(r"\.(?:a|lib|o|obj)$", value, re.I):
                candidate = path(value)
                if candidate.is_relative_to(build) or not re.search(r"(?:lib)?(?:ssl|crypto)[.]", candidate.name):
                    raise ValueError(f"unattributed archive/object: {value}")

    visited_dependencies = set()

    def visit_dependency(identity):
        if identity in visited_dependencies:
            return
        if identity not in by_dep:
            raise ValueError(f"unknown external dependency: {identity}")
        visited_dependencies.add(identity)
        dep = by_dep[identity]
        safe(dep["name"])
        parameters(dep.get("compile_args", []) + dep.get("link_args", []))
        for child in dep.get("dependencies", []):
            visit_dependency(child)
        for child in dep.get("depends", []):
            visit(by_id[child])

    def visit(target):
        if target["id"] in visited:
            return
        visited.add(target["id"])
        if target["name"] not in TARGETS:
            raise ValueError(f"target outside resource tool boundary: {target['name']}")
        groups = target.get("target_sources")
        if not groups:
            raise ValueError(f"empty target inputs: {target['name']}")
        count = 0
        for group in groups:
            for source in group.get("sources", []) + group.get("generated_sources", []):
                safe(source)
                relative = pathlib.Path(source).resolve().relative_to(source_root).as_posix()
                if relative not in SOURCES:
                    raise ValueError(f"source outside tool boundary: {relative}")
                count += 1
            parameters(group.get("parameters", []))
        if not count:
            raise ValueError(f"no attributed sources: {target['name']}")
        for dependency in target["depends"]:
            if dependency not in by_id:
                raise ValueError(f"unknown target dependency: {dependency}")
            visit(by_id[dependency])
        for dependency in target["dependencies"]:
            visit_dependency(dependency)

    visit(roots[0])
    # Unlike command-string grepping, query walks the actual explicit/implicit
    # build inputs, including extracted objects and libraries passed via rsp files.
    seen_inputs = set()

    def inputs(node):
        if node in seen_inputs:
            return
        seen_inputs.add(node)
        safe(node)
        candidate = path(node)
        if candidate in by_output:
            visit(by_output[candidate])
        query = run([ninja, "-t", "query", node], cwd=build)
        lines = query.splitlines()
        input_lines = []
        in_inputs = False
        for line in lines[1:]:
            if line.startswith("  input:"):
                in_inputs = True
            elif line.startswith("  ") and not line.startswith("    "):
                in_inputs = False
            elif in_inputs and line.startswith("    "):
                entry = line.strip()
                if not entry.startswith("|| "):
                    input_lines.append(entry.removeprefix("| "))
        if input_lines:
            for child in input_lines:
                inputs(child)
            if candidate.suffix in {".a", ".lib"}:
                members = run(["ar", "t", str(candidate)]).splitlines()
                expected = Counter(path(child).name for child in input_lines if path(child).suffix in {".o", ".obj"})
                if not members or Counter(pathlib.Path(member).name for member in members) != expected:
                    raise ValueError(f"archive members do not match attributed objects: {candidate}")
        elif candidate.is_relative_to(source_root) and not candidate.is_relative_to(build):
            relative = candidate.relative_to(source_root).as_posix()
            if relative not in SOURCES:
                raise ValueError(f"unattributed source input: {relative}")
        elif candidate.is_relative_to(build):
            raise ValueError(f"missing build inputs: {node}")
        elif not candidate.is_file():
            raise ValueError(f"missing external link input: {node}")

    output = pathlib.Path(roots[0]["filename"][0])
    if not output.is_file():
        raise ValueError("build nanres before auditing its link/archive inputs")
    inputs(output.relative_to(build).as_posix())
    print(f"nanres boundary OK: {len(visited)} targets, {len(seen_inputs)} build inputs")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("build_dir")
    parser.add_argument("--meson", default="meson")
    parser.add_argument("--ninja", default="ninja")
    args = parser.parse_args()
    try:
        audit(args.build_dir, args.meson, args.ninja)
    except (OSError, ValueError, KeyError, TypeError, subprocess.CalledProcessError) as error:
        print(f"ERROR: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
