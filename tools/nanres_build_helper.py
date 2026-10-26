#!/usr/bin/env python3

import json
import os
import pathlib
import subprocess
import sys
import tomllib


def run(
    nanres: str,
    policy: pathlib.Path,
    command: str,
    *args: str,
    capture_failure: bool = False,
) -> bool:
    result = subprocess.run(
        [nanres, command, *args],
        cwd=policy.parent,
        check=False,
        text=True,
        capture_output=True,
    )
    if result.returncode != 0:
        if capture_failure:
            return False
        sys.stderr.write(result.stdout)
        sys.stderr.write(result.stderr)
        raise SystemExit(result.returncode)
    return True


def path_argument(target: pathlib.Path, base: pathlib.Path) -> str:
    """Relative when possible; absolute when the two paths live on different drives."""
    try:
        return os.path.relpath(target, base)
    except ValueError:
        return str(target)


def probe_tool(nanres: str) -> None:
    """确认工具可执行且包格式 schema 兼容（本地或外部 nanres 都走同一条检查）。"""
    schema = subprocess.run(
        [nanres, "schema"], check=False, text=True, capture_output=True
    )
    if schema.returncode != 0 or schema.stdout.strip() != "1":
        raise SystemExit(
            "nanres helper: incompatible resource tool; expected package schema 1, got "
            f"{schema.stdout.strip()!r} (exit {schema.returncode})"
        )


def main() -> int:
    mode, nanres, policy_text, output_text = sys.argv[1:5]
    policy = pathlib.Path(policy_text).resolve()
    output = pathlib.Path(output_text).resolve()
    if policy.name != "resources.toml":
        raise SystemExit("nanres helper requires a resources.toml policy")
    probe_tool(nanres)

    if mode == "keys":
        output.parent.mkdir(parents=True, exist_ok=True)
        header_arg = path_argument(output, policy.parent)
        run(nanres, policy, "keys", "--header", header_arg)
        if not output.is_file():
            raise SystemExit("nanres keys did not produce a header")
        return 0

    if mode != "build":
        raise SystemExit(f"unknown nanres helper mode: {mode}")

    lock = output.parent / (output.name + ".lock.toml")
    lock_arg = path_argument(lock, policy.parent)
    output.parent.mkdir(parents=True, exist_ok=True)
    if not run(nanres, policy, "validate", "--lock", lock_arg, capture_failure=True):
        run(nanres, policy, "scan", "--lock", lock_arg)
        run(nanres, policy, "validate", "--lock", lock_arg)
    relative_output = path_argument(output, policy.parent)
    run(nanres, policy, "pack", "--lock", lock_arg, "--output", relative_output)
    if not (output / "resources.db").is_file():
        raise SystemExit("nanres pack did not produce resources.db")
    metadata = {
        "format": 1,
        "package_id": None,
        "package_root": str(output),
        "database": "resources.db",
    }
    with policy.open("rb") as stream:
        manifest = tomllib.load(stream)
    metadata["package_id"] = manifest.get("package_id") or manifest.get("package")
    if not isinstance(metadata["package_id"], str) or not metadata["package_id"]:
        raise SystemExit("nanres metadata requires package_id or package in resources.toml")
    metadata_path = output / "resource-location.json"
    payload = json.dumps(metadata, indent=2) + "\n"
    if not metadata_path.is_file() or metadata_path.read_text(encoding="utf-8") != payload:
        temporary = metadata_path.with_suffix(".json.tmp")
        temporary.write_text(payload, encoding="utf-8")
        os.replace(temporary, metadata_path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
