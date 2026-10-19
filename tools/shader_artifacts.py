#!/usr/bin/env python3
"""校验/再生嵌入的 SPIR-V 产物（每个注册的 shader）。

登记表是 `tools/shader_artifacts.toml`。默认的 `--check` 不需要编译器，测试/CI 跑的就是它：
核对源码 SHA-256、每个变体的产物 SHA-256 与 SPIR-V magic，并确认头文件里的变体集合与登记表一致。
`--emit NAME` 打印再生后的头文件；`--verify-compiler` 用固定工具链重编并逐字节比较（除注释/空白）。
"""

from __future__ import annotations

import argparse
import hashlib
import pathlib
import re
import struct
import subprocess
import sys
import tempfile

try:
    import tomllib
except ModuleNotFoundError:  # pragma: no cover - Python < 3.11
    print("shader_artifacts: requires Python 3.11+ (tomllib)", file=sys.stderr)
    raise SystemExit(2)

ROOT = pathlib.Path(__file__).resolve().parents[1]
REGISTRY = ROOT / "tools" / "shader_artifacts.toml"


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def load() -> dict:
    return tomllib.loads(REGISTRY.read_text(encoding="utf-8"))


def entry_by_name(config: dict, name: str) -> dict:
    for entry in config["shaders"]:
        if entry["name"] == name:
            return entry
    raise SystemExit(f"unknown shader: {name}; known: {[e['name'] for e in config['shaders']]}")


def check_toolchain(toolchain: dict) -> None:
    version = subprocess.check_output(["glslc", "--version"], text=True).splitlines()[0].strip()
    if version != toolchain["glslc_version"]:
        raise RuntimeError(f"expected glslc {toolchain['glslc_version']}, got {version}")
    validator = subprocess.check_output(["spirv-val", "--version"], text=True)
    if toolchain["spirv_tools_version"] not in validator:
        raise RuntimeError(
            f"expected SPIRV-Tools {toolchain['spirv_tools_version']}, got {validator}"
        )


def compile_variant(source: pathlib.Path, variant: dict, toolchain: dict, output: pathlib.Path) -> bytes:
    arguments = [
        "glslc",
        "-x",
        "hlsl",
        "-fshader-stage=frag",
        f"--target-env={toolchain['target_env']}",
    ]
    if "vectors" in variant:
        arguments.append(f"-DPARAMETER_VECTORS={variant['vectors']}")
    arguments += ["-O", str(source), "-o", str(output)]
    subprocess.run(arguments, check=True)
    subprocess.run(
        ["spirv-val", "--target-env", toolchain["target_env"], str(output)], check=True
    )
    return output.read_bytes()


def render_header(entry: dict, toolchain: dict) -> str:
    source = ROOT / entry["source"]
    lines = [
        f"#ifndef {entry['guard']}",
        f"#define {entry['guard']}",
        "",
        "#include <cstdint>",
        "",
        f"namespace {entry['namespace']}",
        "{",
        f'    inline constexpr char SOURCE_SHA256[] = "{digest(source.read_bytes())}";',
        f'    inline constexpr char COMPILER[] = "glslc {toolchain["glslc_version"]}; '
        f'SPIRV-Tools {toolchain["spirv_tools_version"]}; {toolchain["target_env"]}";',
    ]
    with tempfile.TemporaryDirectory(prefix="nandina-shader-") as directory:
        for variant in entry["variants"]:
            binary = pathlib.Path(directory) / f"{variant['name']}.spv"
            data = compile_variant(source, variant, toolchain, binary)
            words = struct.unpack(f"<{len(data) // 4}I", data)
            lines.extend(
                [
                    f'    inline constexpr char {variant["name"]}_SHA256[] = "{digest(data)}";',
                    f"    inline constexpr std::uint32_t {variant['name']}_SPIRV[] = {{",
                ]
            )
            for offset in range(0, len(words), 8):
                lines.append(
                    "        "
                    + ", ".join(f"0x{word:08x}" for word in words[offset : offset + 8])
                    + ","
                )
            lines.append("    };")
    lines.extend(["} // namespace " + entry["namespace"], "", "#endif", ""])
    return "\n".join(lines)


def check_entry(entry: dict, toolchain: dict) -> None:
    header_path = ROOT / entry["header"]
    if not header_path.is_file():
        raise RuntimeError(f"missing artifact header: {entry['header']}; run --emit {entry['name']}")
    header = header_path.read_text(encoding="utf-8")
    compiler = re.search(r'COMPILER\[\]\s*=\s*"([^"]+)"', header)
    expected_compiler = (
        f'glslc {toolchain["glslc_version"]}; '
        f'SPIRV-Tools {toolchain["spirv_tools_version"]}; {toolchain["target_env"]}'
    )
    if compiler is None or compiler[1] != expected_compiler:
        raise RuntimeError(f"{entry['header']}: toolchain metadata differs from registry")
    source = ROOT / entry["source"]
    source_hash = re.search(r'SOURCE_SHA256\[\]\s*=\s*"([0-9a-f]{64})"', header)
    if source_hash is None or source_hash[1] != digest(source.read_bytes()):
        raise RuntimeError(
            f"{entry['source']}: shader source changed; regenerate with --emit {entry['name']} "
            "and review the artifact"
        )
    embedded = set(re.findall(r"(\w+)_SPIRV\[\]\s*=\s*\{", header))
    declared = {variant["name"] for variant in entry["variants"]}
    if embedded != declared:
        raise RuntimeError(
            f"{entry['header']}: embedded variants {sorted(embedded)} != declared {sorted(declared)}"
        )
    for variant in entry["variants"]:
        name = variant["name"]
        expected = re.search(rf'{name}_SHA256\[\]\s*=\s*"([0-9a-f]{{64}})"', header)
        array = re.search(rf"{name}_SPIRV\[\] = \{{(.*?)\}};", header, re.S)
        if expected is None or array is None:
            raise RuntimeError(f"{entry['header']}: missing metadata for {name}")
        words = [int(word, 16) for word in re.findall(r"0x[0-9a-f]{8}", array[1])]
        if not words or words[0] != 0x07230203:
            raise RuntimeError(f"{entry['header']}: {name} is not SPIR-V")
        data = struct.pack(f"<{len(words)}I", *words)
        if digest(data) != expected[1]:
            raise RuntimeError(f"{entry['header']}: {name} artifact does not match its hash")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    modes = parser.add_mutually_exclusive_group()
    modes.add_argument("--emit", metavar="NAME", help="regenerate and print that shader header")
    modes.add_argument(
        "--verify-compiler",
        nargs="?",
        const="all",
        metavar="NAME",
        help="recompile with the pinned toolchain and compare against the checked-in header",
    )
    options = parser.parse_args()
    config = load()
    toolchain = config["toolchain"]

    if options.emit:
        check_toolchain(toolchain)
        print(render_header(entry_by_name(config, options.emit), toolchain), end="")
        return 0

    if options.verify_compiler:
        check_toolchain(toolchain)
        entries = (
            config["shaders"]
            if options.verify_compiler == "all"
            else [entry_by_name(config, options.verify_compiler)]
        )
        for entry in entries:
            # 注释与空白都不参与比较：头文件会被 clang-format 排版，注释也只供人读。
            generated = re.sub(r"//[^\n]*", "", render_header(entry, toolchain))
            existing = re.sub(r"//[^\n]*", "", (ROOT / entry["header"]).read_text(encoding="utf-8"))
            if re.sub(r"\s+", "", generated) != re.sub(r"\s+", "", existing):
                raise RuntimeError(f"{entry['header']}: regenerated shader differs from artifact")
            print(f"{entry['name']}: pinned compiler reproduces the embedded artifacts; spirv-val passed")
        return 0

    for entry in config["shaders"]:
        check_entry(entry, toolchain)
    print(
        "shader artifacts verified (no compiler required): "
        + ", ".join(entry["name"] for entry in config["shaders"])
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
