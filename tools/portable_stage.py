#!/usr/bin/env python3

import argparse
import json
import os
import pathlib
import shutil
import tempfile


def overlaps(left: pathlib.Path, right: pathlib.Path) -> bool:
    return left == right or left.is_relative_to(right) or right.is_relative_to(left)


def remove_tree(path: pathlib.Path) -> None:
    if path.is_symlink():
        path.unlink()
    elif path.exists():
        shutil.rmtree(path)


def main() -> int:
    parser = argparse.ArgumentParser(description="Create a relocatable Nandina application tree")
    parser.add_argument("--executable", type=pathlib.Path, required=True)
    parser.add_argument("--resources", type=pathlib.Path, required=True)
    parser.add_argument("--output", type=pathlib.Path, required=True)
    options = parser.parse_args()

    executable = options.executable.resolve()
    resources = options.resources.resolve()
    output = options.output.resolve()
    if not executable.is_file():
        raise SystemExit(f"portable stage executable does not exist: {executable}")
    if not (resources / "resources.db").is_file():
        raise SystemExit(f"portable stage resource package has no resources.db: {resources}")
    if (
        output == pathlib.Path(output.anchor)
        or overlaps(output, resources)
        or executable.is_relative_to(output)
    ):
        raise SystemExit(f"unsafe portable stage output: {output}")
    if any(path.is_symlink() for path in resources.rglob("*")):
        raise SystemExit("portable stage resource package must not contain symlinks")

    output.parent.mkdir(parents=True, exist_ok=True)
    temporary = pathlib.Path(tempfile.mkdtemp(prefix=f".{output.name}-", dir=output.parent))
    backup = output.with_name(f".{output.name}.previous")
    try:
        staged_executable = temporary / executable.name
        shutil.copy2(executable, staged_executable)
        staged_resources = temporary / "resources"
        staged_resources.mkdir()
        shutil.copy2(resources / "resources.db", staged_resources / "resources.db")
        if (resources / "external").is_dir():
            shutil.copytree(resources / "external", staged_resources / "external")
        metadata = {
            "schema": 1,
            "executable": executable.name,
            "resources": "resources/resources.db",
        }
        (temporary / "portable-stage.json").write_text(
            json.dumps(metadata, indent=2, sort_keys=True) + "\n",
            encoding="utf-8",
        )
        remove_tree(backup)
        if output.exists():
            os.replace(output, backup)
        try:
            os.replace(temporary, output)
        except BaseException:
            if backup.exists() and not output.exists():
                os.replace(backup, output)
            raise
        remove_tree(backup)
    finally:
        if temporary.exists():
            shutil.rmtree(temporary)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
