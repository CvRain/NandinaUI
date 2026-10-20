#!/usr/bin/env python3
"""发布检查：把各门禁串成一条可重复的命令。

默认只跑无 GPU 的检查（可放 CI）；性能预算与页面 golden 需要 GPU/窗口，只有在显式
提供采样报告时才检查，否则标 SKIP 并说明原因，而不是假装通过。

用法：
    python3 tools/release_check.py --build-dir buildDir
    python3 tools/release_check.py --build-dir buildDir --report <perf-report.json>
    python3 tools/release_check.py --ci --build-dir buildDir      # 仅无 GPU 项
    python3 tools/release_check.py --verbose                       # 失败时也打印通过项输出

退出码：有 FAIL 时 1，否则 0（SKIP 不算失败）。
"""

from __future__ import annotations

import argparse
import pathlib
import re
import shutil
import subprocess
import sys
import time

ROOT = pathlib.Path(__file__).resolve().parents[1]
PYTHON = sys.executable
DEFAULT_BASELINE = (
    ROOT / "probe/cross_platform/results/backend-baseline-release-2026-10-20.json"
)


class Result:
    def __init__(self, status: str, detail: str = "", output: str = "") -> None:
        self.status = status
        self.detail = detail
        self.output = output


def run(command: list[str], timeout: float = 1800.0) -> Result:
    try:
        completed = subprocess.run(
            command,
            cwd=ROOT,
            capture_output=True,
            text=True,
            timeout=timeout,
        )
    except FileNotFoundError:
        return Result("skip", f"命令不存在：{command[0]}")
    except subprocess.TimeoutExpired:
        return Result("fail", f"超时（{timeout:.0f}s）")
    output = (completed.stdout or "") + (completed.stderr or "")
    if completed.returncode == 0:
        return Result("ok", output=output)
    return Result("fail", f"退出码 {completed.returncode}", output)


def check_docs() -> Result:
    """检查 docs/ 与 README 的相对链接和标题锚点。"""
    headings_cache: dict[pathlib.Path, set[str]] = {}

    def slugs(path: pathlib.Path) -> set[str]:
        if path not in headings_cache:
            found: set[str] = set()
            for line in path.read_text(encoding="utf-8").splitlines():
                match = re.match(r"^#{1,6} (.+)$", line)
                if not match:
                    continue
                text = match.group(1).strip().lower()
                text = re.sub(r"[`*_]", "", text)
                text = re.sub(r"[^\w\u4e00-\u9fff\- ]", "", text)
                found.add(text.replace(" ", "-"))
            headings_cache[path] = found
        return headings_cache[path]

    problems: list[str] = []
    files = list((ROOT / "docs").rglob("*.md")) + [ROOT / "README.md"]
    for file in files:
        text = file.read_text(encoding="utf-8")
        for match in re.finditer(r"\]\(([^)\s#]+)#([^)\s]+)\)", text):
            target = (file.parent / match.group(1)).resolve()
            if target.is_file() and match.group(2) not in slugs(target):
                problems.append(f"{file.relative_to(ROOT)}: 锚点不存在 {match.group(1)}#{match.group(2)}")
        for match in re.finditer(r"\]\(([^)#\s]+)(#[^)\s]*)?\)", text):
            target = match.group(1)
            if target.startswith(("http://", "https://", "mailto:")):
                continue
            # 组件模板里的 xxx.md 是文档化占位符，不是坏链接。
            if target == "xxx.md":
                continue
            if not (file.parent / target).resolve().exists():
                problems.append(f"{file.relative_to(ROOT)}: 链接目标不存在 {target}")
    if problems:
        return Result("fail", f"{len(problems)} 处问题", "\n".join(problems))
    return Result("ok", f"{len(files)} 个文档")


def check_whitespace() -> Result:
    output = ""
    for args in (["diff", "--check"], ["diff", "--cached", "--check"]):
        completed = subprocess.run(
            ["git", *args], cwd=ROOT, capture_output=True, text=True, check=False
        )
        output += completed.stdout + completed.stderr
        if completed.returncode != 0:
            return Result("fail", "git diff --check 发现问题", output)
    return Result("ok")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=pathlib.Path, default=ROOT / "buildDir")
    parser.add_argument("--report", type=pathlib.Path, help="GPU 性能采样报告 JSON")
    parser.add_argument("--baseline", type=pathlib.Path, default=DEFAULT_BASELINE)
    parser.add_argument("--ci", action="store_true", help="只跑无 GPU 检查")
    parser.add_argument("--verbose", action="store_true")
    options = parser.parse_args()
    build_dir = options.build_dir.resolve()

    clang_format = shutil.which("clang-format")
    checks: list[tuple[str, str, callable]] = []
    checks.append(
        (
            "format",
            "clang-format 23.1.1",
            lambda: (
                Result("skip", "未安装固定版本 clang-format（pip install clang-format==23.1.1）")
                if clang_format is None
                else run([PYTHON, "tools/check_format.py"])
            ),
        )
    )
    checks.append(
        (
            "headers",
            "头文件分类与独立包含",
            lambda: run([PYTHON, "tools/check_headers.py", "--build-dir", str(build_dir)]),
        )
    )
    checks.append(
        ("artifacts", "shader 产物哈希与固定工具链", lambda: run([PYTHON, "tools/shader_artifacts.py"]))
    )
    checks.append(
        (
            "golden-manifest",
            "golden 清单与审阅元数据",
            lambda: run([PYTHON, "tests/golden_manifest_test.py"]),
        )
    )
    checks.append(
        (
            "test-suites",
            "测试注册与套件",
            lambda: (
                Result("skip", f"构建目录不存在：{build_dir}")
                if not (build_dir / "meson-info").is_dir()
                else run([PYTHON, "tools/check_test_suites.py", str(build_dir)])
            ),
        )
    )
    checks.append(("docs", "文档链接与锚点", check_docs))
    checks.append(("whitespace", "git diff --check", check_whitespace))
    if not options.ci:
        checks.append(
            (
                "budget",
                "性能预算（确定性 + 计时阈值）",
                lambda: (
                    Result("skip", "无 GPU 采样报告；用 --report <json> 指定，或去掉 --ci 的发布检查")
                    if options.report is None
                    else run(
                        [
                            PYTHON,
                            "tools/check_backend_budget.py",
                            "--report",
                            str(options.report),
                            "--baseline",
                            str(options.baseline),
                            "--enforce",
                        ]
                    )
                ),
            )
        )

    failures = 0
    skips = 0
    passes = 0
    started = time.monotonic()
    print(f"release check: {ROOT}")
    for name, description, check in checks:
        started_at = time.monotonic()
        result = check()
        elapsed = time.monotonic() - started_at
        line = f"  [{result.status:>4}] {name:<16} {elapsed:6.2f}s  {description}"
        if result.detail:
            line += f" — {result.detail}"
        print(line)
        if result.status == "fail":
            failures += 1
            if result.output:
                for output_line in result.output.rstrip().splitlines()[-40:]:
                    print(f"         {output_line}")
        elif result.status == "skip":
            skips += 1
        else:
            passes += 1
            if options.verbose and result.output:
                for output_line in result.output.rstrip().splitlines():
                    print(f"         {output_line}")
    elapsed = time.monotonic() - started
    print(
        f"release check: {passes} passed, {skips} skipped, {failures} failed "
        f"({elapsed:.1f}s)"
    )
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
