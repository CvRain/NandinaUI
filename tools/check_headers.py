#!/usr/bin/env python3
"""头文件表面门禁：分类完整性、包含方向、独立可包含。

对应 docs/references/sdk_surface.md §4 的三条门禁：

1. **分类完整**：`nandina/` 下每个头文件在 `tools/header_surface.toml` 里恰好一条；多陈述/漏列/
   陈旧条目都失败。
2. **不越界**：public/experimental 头不得 `#include` internal 头（相对路径与 `<nandina/...>`
   两种写法都解析）。
3. **独立可包含**：public/experimental 头各用一个只包含它的 TU 做 `-fsyntax-only`；编译 flag 从
   `compile_commands.json` 汇总（`-I` / `-isystem` / `-D` / `-std=`），避免手写第二份。

用法：
    tools/check_headers.py --source . --build-dir buildDir        # 三条门禁全跑
    tools/check_headers.py --source . --write                     # 补全新头/清理陈旧条目
    tools/check_headers.py --source . --skip-compile              # 只跑前两条（无构建目录时）

"internal 头不得出现在 public 签名里"无法只靠脚本判定，属于评审项（见 §4）。
"""

from __future__ import annotations

import argparse
import concurrent.futures
import json
import pathlib
import re
import subprocess
import sys
import tempfile

try:
    import tomllib
except ModuleNotFoundError:  # pragma: no cover - Python < 3.11
    print("check_headers: requires Python 3.11+ (tomllib)", file=sys.stderr)
    raise SystemExit(2)

LEVELS = ("public", "experimental", "internal")
HEADER_GLOB = "**/*.hpp"
INCLUDE_RE = re.compile(r'^\s*#\s*include\s*([<"])([^>"]+)[>"]', re.MULTILINE)

# 目录规则：只用于 `--write` 给新头文件一个**默认**等级；已登记条目永不自动改写。
INTERNAL_MARKERS = ("/detail/", "/internal/")
DEFAULT_INTERNAL = {
    "nandina/render/recording_device.hpp",  # 取证格式不是对外契约（见 sdk_surface §3）
}
# 可选功能前置：对应功能关闭时，该头在"正确配置的消费者"里本就不可用，
# 因此独立可包含检查按配置跳过（而不是硬编码某个依赖的 include 路径）。
DEFAULT_REQUIRES = {
    "nandina/physics2d/": "physics2d",
}


def is_internal_default(relative: str) -> bool:
    return any(marker in f"/{relative}" for marker in INTERNAL_MARKERS) or relative in DEFAULT_INTERNAL


def discover(source: pathlib.Path) -> list[str]:
    root = source / "nandina"
    return sorted(
        path.relative_to(source).as_posix() for path in root.glob(HEADER_GLOB) if path.is_file()
    )


def load_manifest(path: pathlib.Path) -> dict[str, dict[str, str]]:
    data = tomllib.loads(path.read_text(encoding="utf-8"))
    headers: dict[str, dict[str, str]] = {}
    for entry in data.get("headers", []):
        headers[entry["path"]] = {
            "level": entry["level"],
            "reason": entry.get("reason", ""),
            "requires": entry.get("requires", ""),
        }
    return headers


def render_manifest(source: pathlib.Path, headers: dict[str, dict[str, str]]) -> str:
    lines = [
        "# 框架头文件的公共表面分类（docs/references/sdk_surface.md §4）。",
        "#",
        "# 等级：public（推荐使用）/ experimental（允许使用，可能调整）/ internal（仅构建与内部实现）。",
        "# 新增头文件必须在这里登记：`tools/check_headers.py --write` 会补一条 experimental/internal 默认项，",
        "# 分类本身仍需人确认。删掉的头文件要从清单里移除，否则门禁失败。",
        "",
        "schema = 1",
        "",
    ]
    for relative in sorted(headers):
        entry = headers[relative]
        lines.append("[[headers]]")
        lines.append(f'path = "{relative}"')
        lines.append(f'level = "{entry["level"]}"')
        if entry.get("reason"):
            lines.append(f'reason = "{entry["reason"]}"')
        if entry.get("requires"):
            lines.append(f'requires = "{entry["requires"]}"')
        lines.append("")
    return "\n".join(lines).rstrip() + "\n"


def resolve_include(includer: str, spec: str) -> str | None:
    """把 #include 的路径解析成仓库相对路径；无法映射到框架头时返回 None。"""
    if spec.startswith("nandina/"):
        return spec
    if spec.startswith("<") or ":" in spec:
        return None
    # 相对路径：相对于当前文件所在目录规范化
    candidate = pathlib.PurePosixPath(includer).parent / spec
    parts: list[str] = []
    for part in candidate.parts:
        if part == "..":
            if parts:
                parts.pop()
        elif part != ".":
            parts.append(part)
    resolved = "/".join(parts)
    return resolved if resolved.startswith("nandina/") else None


def compile_flags(build_dir: pathlib.Path) -> list[str]:
    commands_path = build_dir / "compile_commands.json"
    if not commands_path.is_file():
        raise SystemExit(
            f"check_headers: {commands_path} not found; configure the build dir or use --skip-compile"
        )
    flags: list[str] = []
    seen: set[str] = set()
    for entry in json.loads(commands_path.read_text(encoding="utf-8")):
        tokens = entry.get("arguments") or entry.get("command", "").split()
        if not any(token.endswith("nandina/meson.build") or "/nandina/" in token for token in tokens):
            continue
        for index, token in enumerate(tokens):
            keep = (
                token.startswith(("-I", "-isystem", "-D", "-std="))
                or token in ("-isystem",)
            )
            if not keep:
                continue
            value = token
            if token in ("-isystem",) and index + 1 < len(tokens):
                value = f"{token}{tokens[index + 1]}"
            if value not in seen:
                seen.add(value)
                flags.append(value)
    if not any(flag.startswith("-std=") for flag in flags):
        flags.append("-std=c++26")
    return flags


def read_build_options(build_dir: pathlib.Path) -> dict[str, str]:
    path = build_dir / "meson-info" / "intro-buildoptions.json"
    if not path.is_file():
        return {}
    options = json.loads(path.read_text(encoding="utf-8"))
    return {option["name"]: str(option.get("value", "")) for option in options}


def is_disabled(build_options: dict[str, str], feature: str) -> bool:
    value = build_options.get(feature, "")
    return value in {"false", "disabled", ""}


def syntax_check(
    source: pathlib.Path, headers: list[str], flags: list[str], jobs: int
) -> list[str]:
    failures: list[str] = []
    with tempfile.TemporaryDirectory(prefix="nandina-header-surface-") as directory:
        work = pathlib.Path(directory)
        units = []
        for index, relative in enumerate(headers):
            unit = work / f"unit_{index}.cpp"
            unit.write_text(f'#include "{source / relative}"\n', encoding="utf-8")
            units.append((relative, unit))

        def run(item: tuple[str, pathlib.Path]) -> tuple[str, str | None]:
            relative, unit = item
            result = subprocess.run(
                ["c++", "-fsyntax-only", *flags, str(unit)],
                check=False,
                capture_output=True,
                text=True,
            )
            if result.returncode != 0:
                return relative, (result.stdout + result.stderr)[:4000]
            return relative, None

        with concurrent.futures.ThreadPoolExecutor(max_workers=jobs) as pool:
            for relative, error in pool.map(run, units):
                if error is not None:
                    failures.append(f"{relative}\n{error}")
    return failures


def main() -> int:
    parser = argparse.ArgumentParser(description="Framework header surface gate")
    parser.add_argument("--source", type=pathlib.Path, default=pathlib.Path.cwd())
    parser.add_argument("--build-dir", type=pathlib.Path)
    parser.add_argument("--write", action="store_true", help="补全新头/清理陈旧条目后退出")
    parser.add_argument("--skip-compile", action="store_true", help="跳过独立可包含检查")
    parser.add_argument("--jobs", type=int, default=8)
    options = parser.parse_args()

    source = options.source.resolve()
    manifest_path = source / "tools" / "header_surface.toml"
    discovered = discover(source)
    headers = load_manifest(manifest_path) if manifest_path.is_file() else {}
    problems: list[str] = []

    if options.write:
        changed = False
        for relative in discovered:
            default_requires = next(
                (value for prefix, value in DEFAULT_REQUIRES.items() if relative.startswith(prefix)),
                "",
            )
            if relative not in headers:
                level = "internal" if is_internal_default(relative) else "experimental"
                headers[relative] = {"level": level, "reason": "", "requires": ""}
                changed = True
                print(f"added   {relative} -> {level}")
            if default_requires and not headers[relative].get("requires"):
                headers[relative]["requires"] = default_requires
                changed = True
                print(f"requires {relative} -> {default_requires}")
        for relative in sorted(set(headers) - set(discovered)):
            del headers[relative]
            changed = True
            print(f"removed {relative} (file no longer exists)")
        if changed:
            manifest_path.write_text(render_manifest(source, headers), encoding="utf-8")
            print(f"wrote {manifest_path.relative_to(source)}")
        else:
            print("manifest already up to date")
        return 0

    # 门禁 1：分类完整
    missing = sorted(set(discovered) - set(headers))
    stale = sorted(set(headers) - set(discovered))
    invalid = sorted(
        relative for relative, entry in headers.items() if entry["level"] not in LEVELS
    )
    for relative in missing:
        problems.append(f"unclassified header (run --write, then confirm the level): {relative}")
    for relative in stale:
        problems.append(f"manifest lists a header that no longer exists: {relative}")
    for relative in invalid:
        problems.append(f"invalid level for {relative}: {headers[relative]['level']}")

    # 门禁 2：包含方向（public/experimental 不得包含 internal）
    for relative in discovered:
        entry = headers.get(relative)
        if entry is None or entry["level"] == "internal":
            continue
        text = (source / relative).read_text(encoding="utf-8", errors="replace")
        for _, spec in INCLUDE_RE.findall(text):
            target = resolve_include(relative, spec)
            if target is None or target not in headers:
                continue
            if headers[target]["level"] == "internal":
                problems.append(
                    f"{entry['level']} header includes an internal header: {relative} -> {target}"
                )

    # 门禁 3：独立可包含
    if not options.skip_compile:
        if options.build_dir is None:
            problems.append("--build-dir is required for the standalone-include check")
        else:
            flags = compile_flags(options.build_dir.resolve())
            build_options = read_build_options(options.build_dir.resolve())
            standalone: list[str] = []
            for relative in discovered:
                entry = headers.get(relative, {})
                if entry.get("level") not in {"public", "experimental"}:
                    continue
                feature = entry.get("requires", "")
                if feature and is_disabled(build_options, feature):
                    print(f"skip (feature {feature} is disabled): {relative}")
                    continue
                standalone.append(relative)
            failures = syntax_check(source, standalone, flags, max(1, options.jobs))
            for failure in failures:
                problems.append(f"header is not independently includable:\n{failure}")

    if problems:
        print("header surface gate failed:", file=sys.stderr)
        for problem in problems:
            print(f"  - {problem}", file=sys.stderr)
        return 1
    print(
        f"header surface ok: {len(discovered)} headers classified "
        f"({sum(1 for e in headers.values() if e['level'] == 'internal')} internal)"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
