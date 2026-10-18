#!/usr/bin/env python3
"""共享依赖的运行时分发守卫（便携应用目录）。

背景：vendored SDL3 是**共享库**（静态 SDL3 与 raylib 内置 GLFW 的 Wayland 协议符号冲突，
见 docs/references/dependency_policy.md §2.5）。CMake 子项目没有 Meson install 规则——
`meson introspect --installed` 里 SDL3 条目为 0——所以"只装可执行文件"的程序在运行期找不到
`libSDL3_shared.so.0`。

这个测试证明两件事，且都不需要显示器：

1. 反例：只拷二进制 → `ldd` 解析不到 `libSDL3_shared.so.0`；
2. 正例：`bin/ + lib/` 便携布局 + `$ORIGIN/../lib` RUNPATH → `ldd` 解析到打包目录内的 DSO。

没有 patchelf 时明确跳过（CI 安装它）。
"""

import pathlib
import shutil
import subprocess
import sys
import tempfile


def ldd_lines(binary: pathlib.Path) -> list[str]:
    result = subprocess.run(["ldd", str(binary)], check=False, text=True, capture_output=True)
    return (result.stdout + result.stderr).splitlines()


def find_library(build_root: pathlib.Path) -> list[pathlib.Path]:
    """构建树里的 SDL3 共享库（含 soname 符号链接）。"""
    candidates = sorted(build_root.glob("subprojects/SDL3/libSDL3_shared.so*"))
    return [path for path in candidates if path.is_file() or path.is_symlink()]


def main() -> int:
    build_root = pathlib.Path(sys.argv[1]).resolve()
    binary = build_root / "showcase" / "nandina_showcase"
    if not binary.is_file():
        raise SystemExit(f"missing built application: {binary}; run `meson compile` first")

    libraries = find_library(build_root)
    assert libraries, "no libSDL3_shared.so* in the build tree; is sdl3_source=vendored?"
    soname = next((path for path in libraries if path.name.endswith(".so.0")), None)
    assert soname is not None, [path.name for path in libraries]

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
        missing = [line for line in ldd_lines(lone_binary) if soname.name in line]
        assert missing, "ldd reported nothing for the SDL3 soname"
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
                (bundle / "lib").mkdir(exist_ok=True)
                target.symlink_to(pathlib.Path(library).resolve().name)
                real = bundle / "lib" / pathlib.Path(library).resolve().name
                if not real.exists():
                    shutil.copy2(pathlib.Path(library).resolve(), real)
            else:
                shutil.copy2(library, target)
        subprocess.run(
            ["patchelf", "--set-rpath", "$ORIGIN/../lib", str(staged_binary)], check=True
        )
        resolved = [line for line in ldd_lines(staged_binary) if soname.name in line]
        assert resolved, "ldd reported nothing for the SDL3 soname in the bundle"
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
