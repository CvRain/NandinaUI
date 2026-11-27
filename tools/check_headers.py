#!/usr/bin/env python3
"""头文件表面与模块分层门禁：分类完整性、包含方向、模块单向 DAG、第三方头隔离、独立可包含。

对应 docs/references/sdk_surface.md §4 与 docs/references/module_dependency.md 的门禁：

1. **分类完整**：`nandina/` 下每个头文件在 `tools/header_surface.toml` 里恰好一条；多陈述/漏列/
   陈旧条目都失败。
2. **不越界**：public/experimental 头不得 `#include` internal 头（含相对路径、模块根路径与
   `<nandina/...>` 规范化解析），也不得引入未豁免的第三方/目录外头文件；同时全仓 `nandina/`
   的 `.hpp`/`.cpp` 必须满足 11 模块单向无环依赖图（`ALLOWED_MODULE_DEPS`）。
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
import shlex
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
SOURCE_GLOBS = ("**/*.hpp", "**/*.cpp")
INCLUDE_RE = re.compile(r'^\s*#\s*include\s*([<"])([^>"]+)[>"]', re.MULTILINE)

# 严格单向模块依赖有向无环图（docs/references/module_dependency.md）：
#   app -> { widget, physics2d } -> { scene, semantics, text, render }
#       -> theme -> { foundation, reactive, resource }
ALLOWED_MODULE_DEPS: dict[str, set[str]] = {
    "foundation": set(),
    "reactive": set(),
    "resource": {"foundation"},
    "theme": {"foundation", "reactive", "resource"},
    "semantics": {"foundation"},
    "render": {"foundation", "resource"},
    "text": {"foundation", "resource", "theme", "render"},
    "scene": {"foundation", "theme", "render", "text", "semantics"},
    "widget": {
        "foundation",
        "reactive",
        "resource",
        "theme",
        "render",
        "text",
        "semantics",
        "scene",
    },
    "physics2d": {"foundation", "scene"},
    "app": {
        "foundation",
        "reactive",
        "resource",
        "theme",
        "render",
        "text",
        "semantics",
        "scene",
        "widget",
        "physics2d",
    },
}

# module_dependency.md §约束 5：第三方库头不得进入 public/experimental 公开头，
# 唯一有意暴露的包装头为 foundation/json.hpp -> <nlohmann/json.hpp>。
ALLOWED_PUBLIC_THIRD_PARTY_INCLUDES: dict[str, set[str]] = {
    "nandina/foundation/json.hpp": {"nlohmann/json.hpp"},
}

# C++26 与 C23 标准库头闭集：public/experimental 头的非框架 `<...>` 包含必须落在此集合或显式豁免表内。
STANDARD_HEADERS = {
    # C++ standard headers (C++11..C++26)
    "algorithm",
    "any",
    "array",
    "atomic",
    "barrier",
    "bit",
    "bitset",
    "charconv",
    "chrono",
    "codecvt",
    "compare",
    "complex",
    "concepts",
    "condition_variable",
    "coroutine",
    "debugging",
    "deque",
    "exception",
    "execution",
    "expected",
    "filesystem",
    "flat_map",
    "flat_set",
    "format",
    "forward_list",
    "fstream",
    "functional",
    "future",
    "generator",
    "hazard_pointer",
    "hive",
    "initializer_list",
    "inplace_vector",
    "iomanip",
    "ios",
    "iosfwd",
    "iostream",
    "istream",
    "iterator",
    "latch",
    "linalg",
    "limits",
    "list",
    "locale",
    "map",
    "mdspan",
    "memory",
    "memory_resource",
    "mutex",
    "new",
    "numbers",
    "numeric",
    "optional",
    "ostream",
    "print",
    "queue",
    "random",
    "ranges",
    "ratio",
    "rcu",
    "regex",
    "scoped_allocator",
    "semaphore",
    "set",
    "shared_mutex",
    "simd",
    "source_location",
    "span",
    "spanstream",
    "sstream",
    "stack",
    "stacktrace",
    "stdexcept",
    "stdfloat",
    "stop_token",
    "streambuf",
    "string",
    "string_view",
    "strstream",
    "syncstream",
    "system_error",
    "text_encoding",
    "thread",
    "tuple",
    "type_traits",
    "typeindex",
    "typeinfo",
    "unordered_map",
    "unordered_set",
    "utility",
    "valarray",
    "variant",
    "vector",
    "version",
    # C++ <c*> wrappers
    "cassert",
    "cctype",
    "cerrno",
    "cfenv",
    "cfloat",
    "cinttypes",
    "climits",
    "clocale",
    "cmath",
    "csetjmp",
    "csignal",
    "cstdarg",
    "cstddef",
    "cstdint",
    "cstdio",
    "cstdlib",
    "cstring",
    "ctime",
    "cuchar",
    "cwchar",
    "cwctype",
    # Standard C headers
    "assert.h",
    "complex.h",
    "ctype.h",
    "errno.h",
    "fenv.h",
    "float.h",
    "inttypes.h",
    "iso646.h",
    "limits.h",
    "locale.h",
    "math.h",
    "setjmp.h",
    "signal.h",
    "stdalign.h",
    "stdarg.h",
    "stdatomic.h",
    "stdbit.h",
    "stdbool.h",
    "stdckdint.h",
    "stddef.h",
    "stdint.h",
    "stdio.h",
    "stdlib.h",
    "stdnoreturn.h",
    "string.h",
    "tgmath.h",
    "threads.h",
    "time.h",
    "uchar.h",
    "wchar.h",
    "wctype.h",
}

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


def normalize_rel_posix(raw_path: str) -> str | None:
    """规范化相对 POSIX 路径（处理 `\\`、`//`、`.`、`..`）；若向上越出根则返回 None。"""
    cleaned = raw_path.replace("\\", "/")
    if not cleaned or cleaned.startswith("/") or ":" in cleaned:
        return None
    parts: list[str] = []
    for part in cleaned.split("/"):
        if not part or part == ".":
            continue
        if part == "..":
            if not parts:
                return None
            parts.pop()
        else:
            parts.append(part)
    return "/".join(parts) if parts else None


def resolve_include(
    source: pathlib.Path,
    headers: dict[str, dict[str, str]],
    includer: str,
    delimiter: str,
    spec: str,
) -> str | None:
    """把 #include 解析成规范化的 `nandina/...` 仓库相对路径；非框架头返回 None。"""
    normalized_spec = normalize_rel_posix(spec)
    if normalized_spec is not None and normalized_spec.startswith("nandina/"):
        return normalized_spec

    if delimiter == '"':
        includer_parent = pathlib.PurePosixPath(includer).parent.as_posix()
        rel_joined = normalize_rel_posix(f"{includer_parent}/{spec.replace('\\', '/')}")
        if rel_joined is not None and rel_joined.startswith("nandina/"):
            if rel_joined in headers or (source / rel_joined).is_file():
                return rel_joined
            # 若未走 `-I../nandina` 模块根前缀（首段不是已知模块名），仍按相对路径返回以供诊断
            if normalized_spec is None or normalized_spec.split("/", 1)[0] not in ALLOWED_MODULE_DEPS:
                return rel_joined

    # Meson 构建对库内 TU 传入 `-I../nandina`，允许 `"text/..."` 或 `<text/...>` 直指模块根。
    if normalized_spec is not None and "/" in normalized_spec:
        first_seg = normalized_spec.split("/", 1)[0]
        mod_candidate = f"nandina/{normalized_spec}"
        if first_seg in ALLOWED_MODULE_DEPS and (
            mod_candidate in headers
            or (source / mod_candidate).is_file()
            or delimiter == '"'
            or first_seg not in STANDARD_HEADERS
        ):
            return mod_candidate

    return None


def module_of(relative: str) -> str | None:
    parts = relative.split("/")
    if len(parts) >= 3 and parts[0] == "nandina":
        return parts[1]
    return None


def check_module_and_third_party_rules(
    source: pathlib.Path, headers: dict[str, dict[str, str]]
) -> list[str]:
    problems: list[str] = []
    root = source / "nandina"
    source_files = sorted(
        {
            path.relative_to(source).as_posix()
            for pattern in SOURCE_GLOBS
            for path in root.glob(pattern)
            if path.is_file()
        }
    )
    for relative in source_files:
        src_module = module_of(relative)
        if src_module is None:
            continue
        if src_module not in ALLOWED_MODULE_DEPS:
            problems.append(f"unknown framework module directory in {relative}: {src_module}")
            continue
        text = (source / relative).read_text(encoding="utf-8", errors="replace")
        entry = headers.get(relative)
        check_public_third_party = entry is not None and entry["level"] in {"public", "experimental"}
        allowed_third_party = ALLOWED_PUBLIC_THIRD_PARTY_INCLUDES.get(relative, set())

        for delimiter, spec in INCLUDE_RE.findall(text):
            normalized_spec = normalize_rel_posix(spec)
            target = resolve_include(source, headers, relative, delimiter, spec)
            is_known_framework_header = target is not None and (
                target in headers or (source / target).is_file()
            )

            if check_public_third_party and not is_known_framework_header:
                is_std = delimiter == "<" and normalized_spec in STANDARD_HEADERS
                is_allowed_tp = normalized_spec is not None and normalized_spec in allowed_third_party
                if not is_std and not is_allowed_tp:
                    close_delim = ">" if delimiter == "<" else '"'
                    problems.append(
                        f"{entry['level']} header includes a third-party header: "
                        f"{relative} -> {delimiter}{spec}{close_delim}"
                    )

            if target is None:
                continue
            dst_module = module_of(target)
            if dst_module is None or dst_module == src_module:
                continue
            if dst_module not in ALLOWED_MODULE_DEPS[src_module]:
                problems.append(
                    f"disallowed module dependency: {relative} ({src_module}) -> "
                    f"{target} ({dst_module})"
                )
    return problems


def command_tokens(entry: dict) -> list[str]:
    """Recover one compile_commands entry's argv on every platform.

    Meson emits an ``arguments`` array on some platforms and a single quoted
    ``command`` string on others; the latter needs real shell-style splitting
    because Windows paths contain backslashes and may contain spaces.
    """
    arguments = entry.get("arguments")
    if arguments:
        return [str(argument) for argument in arguments]
    return shlex.split(entry.get("command", ""), posix=True)


def mentions_nandina(token: str) -> bool:
    # Windows command strings spell paths with backslashes; normalize for this
    # repository-relative match only (POSIX paths in this repository never
    # contain a literal backslash).
    normalized = token.replace("\\", "/")
    return normalized.endswith("nandina/meson.build") or "/nandina/" in normalized


def absolute_include(directory: pathlib.Path, value: str) -> str:
    """Resolve a compile-database relative include against the compile directory."""
    include = pathlib.Path(value.replace("\\", "/"))
    if not include.is_absolute():
        include = directory / include
    return include.as_posix()


def compile_flags(build_dir: pathlib.Path) -> list[str]:
    commands_path = build_dir / "compile_commands.json"
    if not commands_path.is_file():
        raise SystemExit(
            f"check_headers: {commands_path} not found; configure the build dir or use --skip-compile"
        )
    flags: list[str] = []
    seen: set[str] = set()
    for entry in json.loads(commands_path.read_text(encoding="utf-8")):
        tokens = command_tokens(entry)
        if not any(mentions_nandina(token) for token in tokens):
            continue
        directory = pathlib.Path(entry.get("directory", build_dir))
        for index, token in enumerate(tokens):
            if token == "-isystem" and index + 1 < len(tokens):
                value = f"-isystem{absolute_include(directory, tokens[index + 1])}"
            elif token == "-I" and index + 1 < len(tokens):
                value = f"-I{absolute_include(directory, tokens[index + 1])}"
            elif token.startswith("-isystem"):
                value = f"-isystem{absolute_include(directory, token[len('-isystem'):])}"
            elif token.startswith("-I"):
                value = f"-I{absolute_include(directory, token[2:])}"
            elif token.startswith(("-D", "-std=")):
                value = token
            else:
                continue
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
            # Header names are literal tokens; POSIX spelling keeps every platform's
            # synthetic TU identical and avoids backslash escapes in the include name.
            unit.write_text(f'#include "{(source / relative).as_posix()}"\n', encoding="utf-8")
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
        for delimiter, spec in INCLUDE_RE.findall(text):
            target = resolve_include(source, headers, relative, delimiter, spec)
            if target is None or target not in headers:
                continue
            if headers[target]["level"] == "internal":
                problems.append(
                    f"{entry['level']} header includes an internal header: {relative} -> {target}"
                )

    # 门禁 2b：单向模块依赖 DAG 与公开头第三方头文件隔离（docs/references/module_dependency.md）
    problems.extend(check_module_and_third_party_rules(source, headers))

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
