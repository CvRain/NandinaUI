#!/usr/bin/env python3
"""共享依赖的运行时分发守卫（便携应用目录）。

默认静态 SDL3 不应出现在可执行文件的动态依赖中。选择共享 SDL3 时，
CMake 子项目没有 Meson install 规则——
`meson introspect --installed` 里 SDL3 条目为 0——所以"只装可执行文件"的程序在运行期找不到
SDL3 的 DSO。

共享模式证明两件事，且都不需要显示器：

1. 反例：只拷二进制 → `ldd` 解析不到 SDL3 的 soname；
2. 正例：`bin/ + lib/` 便携布局 + `$ORIGIN/../lib` RUNPATH → `ldd` 解析到打包目录内的 DSO。

soname 从构建产物自身的 `SONAME` 动态项读取，不假设版本后缀。vendored SDL3 由
`cmake.subproject` 构建，Meson 按 target 名（`SDL3-shared`）给产物命名，实测为
`libSDL3_shared.so`，CMake 的 `VERSION`/`SOVERSION` 不会传到 Meson 的产物名——构建树里
没有 `.so.0` 版本文件。按 `.so.0` 硬编码会让共享配置的检查永远失败；读取真实 soname 后，
即使将来 Meson 或上游开始带版本后缀，同一条检查仍然成立。

没有 patchelf 时明确跳过（CI 安装它）。
"""

import json
import pathlib
import re
import shutil
import subprocess
import sys
import tempfile


def ldd_lines(binary: pathlib.Path) -> list[str]:
    result = subprocess.run(["ldd", str(binary)], check=False, text=True, capture_output=True)
    return (result.stdout + result.stderr).splitlines()


def find_library(build_root: pathlib.Path) -> list[pathlib.Path]:
    """构建树里的 SDL3 共享库（含可能的 soname 符号链接）。"""
    candidates = sorted(build_root.glob("subprojects/SDL3/libSDL3_shared.so*"))
    return [path for path in candidates if path.is_file() or path.is_symlink()]


def read_soname(dso: pathlib.Path) -> str:
    """读取 DSO 记录的 SONAME（只解析方括号内的名字，不依赖 readelf 的本地化输出）。"""
    dynamic = subprocess.run(
        ["readelf", "-d", str(dso)], check=True, text=True, capture_output=True
    ).stdout
    match = re.search(r"\(SONAME\).*?\[([^]]+)\]", dynamic)
    assert match is not None, f"no SONAME dynamic entry in {dso}:\n{dynamic}"
    return match.group(1)


def main() -> int:
    build_root = pathlib.Path(sys.argv[1]).resolve()
    binary = build_root / "showcase" / "nandina_showcase"
    if not binary.is_file():
        raise SystemExit(f"missing built application: {binary}; run `meson compile` first")

    dependencies = json.loads((build_root / "deps.json").read_text())["deps"]
    sdl3 = next(item for item in dependencies if item["name"] == "sdl3")
    if sdl3["linkage"] == "static":
        dynamic = subprocess.run(
            ["readelf", "-d", str(binary)], check=True, text=True, capture_output=True
        ).stdout.lower()
        assert "sdl3" not in dynamic and "raylib" not in dynamic, dynamic
        print("static SDL3: application has no SDL3 or raylib runtime DSO dependency")
        return 0

    libraries = find_library(build_root)
    assert libraries, "no libSDL3_shared.so* in the build tree; is sdl3_source=vendored?"
    # 优先读真实文件；带 soname 符号链接链时 resolve() 到链接目标。
    real = next((path for path in libraries if not path.is_symlink()), libraries[0]).resolve()
    soname = read_soname(real)
    assert any(path.name == soname for path in libraries), (
        f"no DSO in the build tree is named after the recorded soname {soname!r}: "
        f"{[path.name for path in libraries]}"
    )

    if shutil.which("patchelf") is None:
        print("skipped: patchelf is not installed")
        return 0

    with tempfile.TemporaryDirectory(prefix="nandina-portable-") as directory:
        root = pathlib.Path(directory)

        # 1) 反例：只拷二进制。rpath 仍指向原构建树，先打掉它以模拟"只安装可执行文件"。
        alone = root / "alone" / "bin"
        alone.mkdir(parents=True)
        lone_binary = alone / binary.name
        shutil.copy2(binary, lone_binary)
        subprocess.run(["patchelf", "--remove-rpath", str(lone_binary)], check=True)
        missing = [line for line in ldd_lines(lone_binary) if soname in line]
        assert missing, f"ldd reported nothing for the SDL3 soname {soname!r}"
        assert "not found" in missing[0], (
            "a lone executable unexpectedly found SDL3; the negative case is not representative: "
            f"{missing[0]}"
        )

        # 2) 正例：便携布局 bin/ + lib/ + $ORIGIN/../lib。
        bundle = root / "bundle"
        (bundle / "bin").mkdir(parents=True)
        (bundle / "lib").mkdir()
        staged_binary = bundle / "bin" / binary.name
        shutil.copy2(binary, staged_binary)
        for library in libraries:
            target = bundle / "lib" / library.name
            if library.is_symlink():
                # 保持 soname 链接关系，而不是把真实文件复制成同名文件。
                target.symlink_to(pathlib.Path(library).resolve().name)
                real = bundle / "lib" / pathlib.Path(library).resolve().name
                if not real.exists():
                    shutil.copy2(pathlib.Path(library).resolve(), real)
            else:
                shutil.copy2(library, target)
        subprocess.run(
            ["patchelf", "--set-rpath", "$ORIGIN/../lib", str(staged_binary)], check=True
        )
        resolved = [line for line in ldd_lines(staged_binary) if soname in line]
        assert resolved, f"ldd reported nothing for the SDL3 soname {soname!r} in the bundle"
        assert str(bundle) in resolved[0], (
            f"the bundled SDL3 was not used: {resolved[0]}"
        )
        runpath = subprocess.run(
            ["readelf", "-d", str(staged_binary)], check=True, text=True, capture_output=True
        ).stdout
        assert "$ORIGIN/../lib" in runpath, runpath
        print("portable bundle resolves SDL3 from its own lib/:", resolved[0])
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
