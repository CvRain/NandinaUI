#!/usr/bin/env python3
"""Verify embedded probe artifacts; --emit regenerates with pinned glslc and spirv-val."""

import argparse
import hashlib
import pathlib
import re
import struct
import subprocess
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[1]
SOURCE = ROOT / "probe/cross_platform/desktop/sdf_state.frag.hlsl"
HEADER = SOURCE.with_name("sdf_state_spirv.hpp")
VARIANTS = {"SHAPE": 4, "BUDGET": 256}


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def generate() -> str:
    version = subprocess.check_output(["glslc", "--version"], text=True)
    if version.splitlines()[0] != "2026.4":
        raise RuntimeError(f"expected glslc 2026.4, got {version}")
    validator = subprocess.check_output(["spirv-val", "--version"], text=True)
    if "v2026.4" not in validator:
        raise RuntimeError(f"expected SPIRV-Tools v2026.4, got {validator}")
    lines = [
        "#ifndef NANDINA_PROBE_SDF_STATE_SPIRV_HPP",
        "#define NANDINA_PROBE_SDF_STATE_SPIRV_HPP",
        "",
        "#include <cstdint>",
        "",
        "namespace nandina::probe",
        "{",
        f'    inline constexpr char SOURCE_SHA256[] = "{digest(SOURCE.read_bytes())}";',
        '    inline constexpr char COMPILER[] = "glslc 2026.4; SPIRV-Tools v2026.4; vulkan1.0";',
    ]
    with tempfile.TemporaryDirectory(prefix="nandina-sdf-shader-") as directory:
        for name, count in VARIANTS.items():
            binary = pathlib.Path(directory) / f"{name}.spv"
            subprocess.run([
                "glslc", "-x", "hlsl", "-fshader-stage=frag", "--target-env=vulkan1.0",
                f"-DPARAMETER_VECTORS={count}", "-O", str(SOURCE), "-o", str(binary),
            ], check=True)
            subprocess.run(["spirv-val", "--target-env", "vulkan1.0", str(binary)], check=True)
            data = binary.read_bytes()
            words = struct.unpack(f"<{len(data) // 4}I", data)
            lines.extend([
                f'    inline constexpr char {name}_SHA256[] = "{digest(data)}";',
                f"    inline constexpr std::uint32_t {name}_SPIRV[] = {{",
            ])
            for offset in range(0, len(words), 8):
                lines.append("        " + ", ".join(f"0x{word:08x}" for word in words[offset:offset + 8]) + ",")
            lines.append("    };")
    lines.extend(["}", "", "#endif", ""])
    return "\n".join(lines)


def check() -> None:
    header = HEADER.read_text(encoding="utf-8")
    source_hash = re.search(r'SOURCE_SHA256\[\]\s*=\s*"([0-9a-f]{64})"', header)
    if source_hash is None or source_hash[1] != digest(SOURCE.read_bytes()):
        raise RuntimeError("shader source changed: explicitly regenerate and review the artifacts")
    for name in VARIANTS:
        expected = re.search(rf'{name}_SHA256\[\]\s*=\s*"([0-9a-f]{{64}})"', header)
        array = re.search(rf"{name}_SPIRV\[\] = \{{(.*?)\}};", header, re.S)
        if expected is None or array is None:
            raise RuntimeError(f"missing shader metadata: {name}")
        words = [int(word, 16) for word in re.findall(r"0x[0-9a-f]{8}", array[1])]
        data = struct.pack(f"<{len(words)}I", *words)
        if not words or words[0] != 0x07230203 or digest(data) != expected[1]:
            raise RuntimeError(f"shader artifact corrupted: {name}")
    print("SDF probe source/artifact hashes match (no compiler required)")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    modes = parser.add_mutually_exclusive_group()
    modes.add_argument("--emit", action="store_true")
    modes.add_argument("--verify-compiler", action="store_true")
    args = parser.parse_args()
    if args.emit:
        print(generate(), end="")
    elif args.verify_compiler:
        generated = generate()
        existing = re.sub(r"//[^\n]*", "", HEADER.read_text(encoding="utf-8"))
        if re.sub(r"\s+", "", generated) != re.sub(r"\s+", "", existing):
            raise RuntimeError("regenerated shader differs from the checked-in artifact")
        print("pinned compiler reproduces both embedded shaders; spirv-val passed")
    else:
        check()


if __name__ == "__main__":
    main()
