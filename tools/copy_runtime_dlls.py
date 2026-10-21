#!/usr/bin/env python3
"""把 Windows 运行时 DLL 复制到构建目录中各可执行文件旁。

Windows 的 DLL 搜索顺序不包含构建目录：MSYS2 的 gcc 动态链接 libstdc++/libgcc/
libwinpthread，freetype 又动态链接 zlib/bz2/png/brotli，SDL3 可选共享模式，所以直接运行
`buildDir\\tests\\xxx.exe` 或 `buildDir\\showcase\\nandina_showcase.exe` 会以
0xC0000135 (STATUS_DLL_NOT_FOUND) 退出（终端里看起来“什么都没发生”）。

本脚本在每次构建后运行（Meson custom_target, Windows only）：扫描构建目录里所有 .exe
所在目录，把需要的 DLL 复制过去；幂等，已存在且大小相同则跳过。只复制运行所需的少量
DLL，不复制整个 MSYS2 bin 目录。
"""

from __future__ import annotations

import argparse
import json
import os
import shutil
import sys
from pathlib import Path

# 顺序无关；缺失的项会跳过（不同 MSYS2 包版本命名可能略有差异）。
RUNTIME_DLLS = (
    "libstdc++-6.dll",
    "libgcc_s_seh-1.dll",
    "libwinpthread-1.dll",
    "zlib1.dll",
    "libbz2-1.dll",
    "libpng16-16.dll",
    "libbrotlidec.dll",
    "libbrotlicommon.dll",
)
# 我们的 CMake 子项目产出带 lib 前缀的共享库；两种名字都认。
SDL_DLL_NAMES = ("libSDL3_shared.dll", "SDL3.dll")


def search_roots(compiler_dir: Path | None) -> list[Path]:
    """按可靠性排序的 DLL 搜索目录。"""
    roots: list[Path] = []
    if compiler_dir is not None:
        roots.append(compiler_dir)
    # meson 通常由 MSYS2 的 python 运行；运行时 DLL 就在 python.exe 旁边。
    roots.append(Path(sys.executable).resolve().parent)
    roots.append(Path("C:/msys64/ucrt64/bin"))
    roots.append(Path("C:/msys64/mingw64/bin"))
    for entry in os.environ.get("PATH", "").split(os.pathsep):
        if entry:
            roots.append(Path(entry))
    return roots


def locate(name: str, roots: list[Path]) -> Path | None:
    for root in roots:
        candidate = root / name
        if candidate.is_file():
            return candidate
    return None


def sdl_dependency(build_dir: Path) -> dict | None:
    report = build_dir / "deps.json"
    if report.is_file():
        dependencies = json.loads(report.read_text(encoding="utf-8"))["deps"]
        return next((item for item in dependencies if item["name"] == "sdl3"), None)
    return None


def find_sdl_dll(build_dir: Path, roots: list[Path]) -> Path | None:
    sdl3 = sdl_dependency(build_dir)
    if sdl3 is not None:
        if not sdl3["enabled"] or sdl3["linkage"] == "static":
            return None
        if sdl3["source"] == "system":
            for root in roots:
                for name in SDL_DLL_NAMES:
                    candidate = root / name
                    if candidate.is_file():
                        return candidate
            return None
    for name in SDL_DLL_NAMES:
        known = build_dir / "subprojects" / "SDL3" / name
        if known.is_file():
            return known
    for name in SDL_DLL_NAMES:
        found = next(iter(build_dir.glob(f"subprojects/*/{name}")), None)
        if found is not None:
            return found
    return None


def executable_dirs(build_dir: Path) -> list[Path]:
    """所有含 .exe 的目录，跳过 subprojects（CMake 子项目的产物不需要我们管）。"""
    dirs: set[Path] = set()
    for exe in build_dir.rglob("*.exe"):
        relative = exe.relative_to(build_dir)
        if relative.parts and relative.parts[0] == "subprojects":
            continue
        dirs.add(exe.parent)
    return sorted(dirs)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", required=True, type=Path)
    parser.add_argument(
        "--compiler-dir",
        default="",
        help="MSYS2 的 bin 目录（找不到时回退到 PATH 搜索）",
    )
    options = parser.parse_args()
    build_dir = options.build_dir.resolve()
    compiler_dir = Path(options.compiler_dir) if options.compiler_dir else None

    roots = search_roots(compiler_dir)
    sources: dict[str, Path] = {}
    for name in RUNTIME_DLLS:
        candidate = locate(name, roots)
        if candidate is not None:
            sources[name] = candidate
    sdl3 = sdl_dependency(build_dir)
    sdl = find_sdl_dll(build_dir, roots)
    if sdl is not None:
        sources[sdl.name] = sdl

    destinations = executable_dirs(build_dir)
    static_sdl = sdl3 is not None and (not sdl3["enabled"] or sdl3["linkage"] == "static")
    if static_sdl:
        # 从共享 SDL3 切换到静态/关闭后，旧构建目录里可能还留着上一轮复制的 SDL DLL。
        # Windows 会优先从 exe 同目录加载，残留的旧 DLL 会掩盖静态链接状态，必须清掉。
        removed = 0
        for destination in destinations:
            for name in SDL_DLL_NAMES:
                stale = destination / name
                if stale.is_file():
                    stale.unlink()
                    removed += 1
        print(
            "windows runtime: SDL3 disabled or statically linked; no SDL3 DLL required"
            + (f" (removed {removed} stale copy)" if removed else "")
        )
    elif sdl is None and sdl3 is not None and sdl3["source"] == "system":
        print(
            "windows runtime: system SDL3 linkage is unknown and no SDL3 DLL was found in "
            "the compiler directory or PATH; verify static linkage or provide SDL3.dll "
            "with --compiler-dir/PATH before distributing the application",
            file=sys.stderr,
        )
    elif sdl is None:
        print(
            "windows runtime: SDL3 共享库还没生成，下次构建会补上（SDL3 链接完成之前本步骤会跳过）",
            file=sys.stderr,
        )
    if not sources:
        print("windows runtime: 没找到任何运行时 DLL，跳过", file=sys.stderr)
        return 0

    copied = 0
    for destination in destinations:
        for name, source in sources.items():
            target = destination / name
            try:
                if target.is_file() and target.stat().st_size == source.stat().st_size:
                    continue
                shutil.copy2(source, target)
                copied += 1
            except OSError as error:
                print(f"windows runtime: 复制 {name} 到 {destination} 失败: {error}", file=sys.stderr)
                return 1
    print(f"windows runtime: {len(destinations)} 个可执行文件目录，复制 {copied} 个 DLL")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
