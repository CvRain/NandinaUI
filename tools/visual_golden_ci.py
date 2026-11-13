#!/usr/bin/env python3
"""CI 无 GPU 页面视觉门禁：headless CPU golden 采集 + 确定性复跑 + 基线比较。

为什么单独一个 harness：

- `capture_backend_golden.py` 是"再生候选"的人工工具（会写 manifest、默认拒绝覆盖 approved）；
- `compare_backend_golden.py` 是单页人工比较；
- CI 需要的是：多页批量、**同一输入连续采两次必须逐字节一致**、每个 approved 条目都判
  失败，且失败时仍把候选图/差异图/统计留在 artifact 里。这里不写 manifest、不 approve。

安全边界（与 docs/references/visual_regression.md §4/§7 一致）：

- 只比较**当前平台**的 manifest 条目；`--platform` 与检测值不同直接拒绝，杜绝跨平台像素比较；
- 只读 manifest 与仓库内 approved 图，全部输出写到 `--work-dir`，绝不修改基线；
- `status = "candidate"` 只报告、不判失败；`status = "approved"` 超阈值时退出码 1；
- 采集环境（含 headless 的 `SDL_VIDEODRIVER=dummy` + `--renderer sdl2d`）由调用方设置，
  本脚本只负责 probe 的逐页环境变量；确认采集环境是"同一次 CI 运行可复现的"是本脚本
  的首要职责（两次采集哈希不一致直接失败，不做像素比较）。

退出码：0 = 全部通过（可能夹带 candidate 报告）；1 = 采集失败、两次采集不一致、
manifest 缺条目、或 approved 基线超阈值（`--report-only` 时只有基线差异不置 1）。
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import pathlib
import platform as platform_module
import subprocess
import sys
import time

HERE = pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))

import compare_backend_golden as golden_compare  # noqa: E402
import golden_image  # noqa: E402

ROOT = HERE.parent
DEFAULT_PROBE = ROOT / "buildDir-probe/probe/cross_platform/desktop/nandina_sdl_page_probe"
DEFAULT_MANIFEST = ROOT / "tests/golden/backend/manifest.toml"

# 与 nandina/text/system_fonts.cpp 的 cjk_hints 保持一致。这里只用于 CI 环境诊断：
# 报告实际会被 `register_system_cjk_fallback()` 选中的字体，并与期望哈希比对，
# 让"runner 镜像换了字体"在像素比较之前就报出可读的原因。
CJK_HINTS: tuple[str, ...] = (
    "notosanscjk",
    "sourcehansans",
    "sarasa",
    "pingfang",
    "hiragino",
    "msyh",
    "simsun",
    "meiryo",
    "yugoth",
    "wqy",
    "droidsansfallback",
)


def sha256_file(path: pathlib.Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def os_release() -> dict[str, str]:
    """Best-effort `/etc/os-release` capture for CI environment diagnostics."""
    path = pathlib.Path("/etc/os-release")
    if not path.is_file():
        return {}
    values: dict[str, str] = {}
    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        if "=" in line:
            key, value = line.split("=", 1)
            values[key] = value.strip().strip('"')
    return values


def system_font_directories() -> list[pathlib.Path]:
    """Mirror of nandina/text/system_fonts.cpp `system_font_directories()`."""
    directories: list[pathlib.Path] = []
    if sys.platform.startswith("win"):
        directories.append(pathlib.Path(os.environ.get("WINDIR", r"C:\Windows")) / "Fonts")
        local = os.environ.get("LOCALAPPDATA")
        if local:
            directories.append(pathlib.Path(local) / "Microsoft" / "Windows" / "Fonts")
    elif sys.platform == "darwin":
        home = os.environ.get("HOME")
        if home:
            directories.append(pathlib.Path(home) / "Library" / "Fonts")
        directories += [pathlib.Path("/Library/Fonts"), pathlib.Path("/System/Library/Fonts")]
    else:
        home = os.environ.get("HOME")
        if home:
            directories += [
                pathlib.Path(home) / ".local" / "share" / "fonts",
                pathlib.Path(home) / ".fonts",
            ]
        directories += [pathlib.Path("/usr/local/share/fonts"), pathlib.Path("/usr/share/fonts")]
    return directories


def discover_cjk_font() -> pathlib.Path | None:
    """Approximate `find_cjk_font_in(system_font_directories())` for diagnostics.

    The C++ side walks directories in order and keeps the best-ranked filename hint;
    this mirror has the same root order and rank rules. It is a diagnostic check, not
    the source of truth — a mismatch only means the CI environment changed.
    """
    best: pathlib.Path | None = None
    best_rank = len(CJK_HINTS)
    for directory in system_font_directories():
        if not directory.is_dir():
            continue
        for path in directory.rglob("*"):
            try:
                if not path.is_file():
                    continue
            except OSError:
                continue
            if path.suffix.lower() not in (".ttf", ".otf", ".ttc"):
                continue
            name = path.name.lower()
            for rank, hint in enumerate(CJK_HINTS):
                if hint in name and rank < best_rank:
                    best, best_rank = path, rank
                    break
    return best


def run_capture(
    probe: pathlib.Path,
    page: str,
    backend: str,
    frames: int,
    renderer: str,
    target: pathlib.Path,
    log_path: pathlib.Path,
) -> None:
    environment = golden_compare.probe_environment(page, backend, frames, target, renderer)
    started = time.monotonic()
    completed = subprocess.run(
        [str(probe)], capture_output=True, text=True, timeout=600, env=environment
    )
    elapsed = time.monotonic() - started
    log_path.write_text(
        f"probe={probe}\npage={page} backend={backend} renderer={renderer} frames={frames}\n"
        f"exit={completed.returncode} elapsed_s={elapsed:.2f}\n"
        f"--- stdout ---\n{completed.stdout}\n--- stderr ---\n{completed.stderr}\n",
        encoding="utf-8",
    )
    if completed.returncode != 0:
        raise RuntimeError(f"probe exited {completed.returncode} (log: {log_path})")
    if not target.is_file():
        raise RuntimeError(f"probe did not write {target} (log: {log_path})")


def parse_list(value: str) -> list[str]:
    return [item.strip() for item in value.split(",") if item.strip()]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--probe", type=pathlib.Path, default=DEFAULT_PROBE)
    parser.add_argument("--manifest", type=pathlib.Path, default=DEFAULT_MANIFEST)
    parser.add_argument(
        "--pages",
        default="",
        help="comma separated pages; defaults to every page with an entry for the current "
        "platform and the requested backends (so new approved baselines are covered)",
    )
    parser.add_argument("--backends", default="cpu")
    parser.add_argument("--frames", type=int, default=30)
    parser.add_argument(
        "--renderer",
        choices=("gpu", "sdl2d"),
        default="sdl2d",
        help="sdl2d is the headless CPU path used by CI (needs SDL_VIDEODRIVER=dummy); "
        "gpu keeps the production SDF renderer for manual runs",
    )
    parser.add_argument(
        "--platform",
        default="",
        help="safety check only: must equal the detected platform; cross-platform pixel "
        "comparison is forbidden",
    )
    parser.add_argument(
        "--work-dir",
        type=pathlib.Path,
        default=ROOT / "buildDir-probe/visual-golden",
        help="artifact directory: captures, diff images, stats JSON and summary",
    )
    parser.add_argument("--tolerance", type=int, default=3, help="per-channel threshold")
    parser.add_argument("--max-changed-fraction", type=float, default=0.001)
    parser.add_argument(
        "--report-only",
        action="store_true",
        help="record approved-baseline mismatches but exit 0 (determinism/capture "
        "failures still exit 1); for environment diagnostics, not for the gate",
    )
    parser.add_argument(
        "--expect-cjk-font-sha256",
        default="",
        help="fail when the discovered CJK fallback font does not match this sha256 "
        "(pins the CI text rendering environment)",
    )
    parser.add_argument(
        "--summary-file",
        type=pathlib.Path,
        help="append the markdown summary here (e.g. $GITHUB_STEP_SUMMARY)",
    )
    options = parser.parse_args()

    detected = golden_compare.current_platform()
    if options.platform and options.platform != detected:
        raise SystemExit(
            f"refusing to compare across platforms: --platform {options.platform!r} != detected "
            f"{detected!r} (docs/references/visual_regression.md §7)"
        )
    platform = detected
    probe = options.probe.resolve()
    manifest = options.manifest.resolve()
    work_dir = options.work_dir.resolve()
    if not probe.is_file():
        raise SystemExit(f"probe not built: {probe}")
    if not manifest.is_file():
        raise SystemExit(f"manifest not found: {manifest}")
    entries = golden_compare.load_entries(manifest)
    backends = parse_list(options.backends)
    if options.pages:
        pages = parse_list(options.pages)
    else:
        pages = sorted(
            {
                page
                for (page, backend, entry_platform) in entries
                if entry_platform == platform and backend in backends
            }
        )
    if not pages:
        raise SystemExit(f"no pages selected for platform {platform} backends {backends}")
    work_dir.mkdir(parents=True, exist_ok=True)

    environment_problems: list[str] = []
    cjk_font = discover_cjk_font()
    cjk_sha = sha256_file(cjk_font) if cjk_font is not None else ""
    if options.expect_cjk_font_sha256:
        if cjk_font is None:
            environment_problems.append(
                "no CJK fallback font discovered; Chinese text would render differently"
            )
        elif cjk_sha != options.expect_cjk_font_sha256:
            environment_problems.append(
                f"CJK fallback font {cjk_font} sha256 {cjk_sha} != expected "
                f"{options.expect_cjk_font_sha256}"
            )

    environment = {
        "platform": platform,
        "platform_fingerprint": platform_module.platform(),
        "os_release": os_release(),
        "python": sys.version.split()[0],
        "probe": str(probe),
        "manifest": str(manifest),
        "renderer": options.renderer,
        "backends": backends,
        "pages": pages,
        "frames": options.frames,
        "tolerance": options.tolerance,
        "max_changed_fraction": options.max_changed_fraction,
        "report_only": options.report_only,
        "cjk_font": {"path": str(cjk_font) if cjk_font else "", "sha256": cjk_sha},
    }
    (work_dir / "environment.json").write_text(
        json.dumps(environment, indent=2) + "\n", encoding="utf-8"
    )

    failures = len(environment_problems)
    visual_failures = 0
    results: list[dict[str, object]] = []
    for page in pages:
        for backend in backends:
            key = f"{page}/{backend}/{platform}"
            case_dir = work_dir / f"{page}-{backend}-{platform}"
            case_dir.mkdir(parents=True, exist_ok=True)
            result: dict[str, object] = {
                "key": key,
                "page": page,
                "backend": backend,
                "platform": platform,
                "deterministic": None,
                "baseline": None,
                "result": "error",
                "stats": None,
                "error": None,
            }
            entry = entries.get((page, backend, platform))
            try:
                first = case_dir / "capture-a.bmp"
                second = case_dir / "capture-b.bmp"
                run_capture(
                    probe, page, backend, options.frames, options.renderer, first,
                    case_dir / "capture-a.log",
                )
                run_capture(
                    probe, page, backend, options.frames, options.renderer, second,
                    case_dir / "capture-b.log",
                )
                digest_a = sha256_file(first)
                digest_b = sha256_file(second)
                deterministic = digest_a == digest_b
                result["deterministic"] = deterministic
                result["sha256"] = digest_a
                (case_dir / "determinism.json").write_text(
                    json.dumps(
                        {
                            "page": page,
                            "backend": backend,
                            "platform": platform,
                            "frames": options.frames,
                            "renderer": options.renderer,
                            "capture_a_sha256": digest_a,
                            "capture_b_sha256": digest_b,
                            "identical": deterministic,
                        },
                        indent=2,
                    )
                    + "\n",
                    encoding="utf-8",
                )
                if not deterministic:
                    raise RuntimeError(
                        f"two captures of the same input differ: {digest_a} != {digest_b}"
                    )
                image = golden_image.read_bmp(str(first))
                golden_image.write_png(str(case_dir / "capture.png"), image)
                # PNG is lossless from the raw BMP; drop the ~2.8 MB raw captures from the
                # artifact once both hashes are recorded (the artifact is uploaded per run).
                first.unlink(missing_ok=True)
                second.unlink(missing_ok=True)
                if entry is None:
                    raise RuntimeError(f"manifest has no entry for {key}")
                result["baseline"] = entry["image"]
                golden_image_path = manifest.parent / str(entry["image"])
                golden = golden_image.load(str(golden_image_path))
                if (golden.width, golden.height) != (image.width, image.height):
                    raise RuntimeError(
                        f"size mismatch: golden {golden.width}x{golden.height} vs "
                        f"capture {image.width}x{image.height}"
                    )
                comparison, diff = golden_compare.compare_images(
                    golden, image, options.tolerance
                )
                stats = {
                    "page": page,
                    "backend": backend,
                    "status": entry["status"],
                    "width": image.width,
                    "height": image.height,
                    **comparison,
                    "max_changed_fraction": options.max_changed_fraction,
                }
                (case_dir / "stats.json").write_text(
                    json.dumps(stats, indent=2) + "\n", encoding="utf-8"
                )
                golden_image.write_png(str(case_dir / "diff.png"), diff)
                result["stats"] = stats
                if entry["status"] != "approved":
                    result["result"] = "candidate"
                elif float(comparison["changed_fraction"]) > options.max_changed_fraction:
                    result["result"] = "mismatch"
                    if options.report_only:
                        result["result"] = "mismatch-report-only"
                    else:
                        failures += 1
                        visual_failures += 1
                else:
                    result["result"] = "pass"
            except Exception as error:  # noqa: BLE001 - CI harness records, never masks
                result["error"] = str(error)
                result["result"] = "error"
                failures += 1
                visual_failures += 1
            results.append(result)

    def describe_fraction(value: object) -> str:
        return f"{float(value) * 100:.4f}%" if isinstance(value, (int, float)) else "-"

    def describe_stats(result: dict[str, object], field: str, suffix: str = "") -> str:
        stats = result.get("stats")
        if not isinstance(stats, dict):
            return "-"
        return f"{stats[field]}{suffix}"

    summary_lines = [
        "## CPU page golden (headless, no GPU)",
        "",
        f"- platform: `{platform}` — only this platform's baselines are compared; "
        "cross-platform pixels are never compared",
        f"- probe: `{probe}`",
        f"- renderer: `{options.renderer}`, backends: `{', '.join(backends)}`, "
        f"frames: {options.frames}",
        f"- tolerance: ±{options.tolerance} per channel, max changed fraction "
        f"{options.max_changed_fraction * 100:.3f}%",
        f"- manifest: `{manifest}`",
        f"- CJK fallback font: `{cjk_font if cjk_font else '(none discovered)'}` "
        f"(sha256 `{cjk_sha or '-'}`)",
        "- approval: **manual only** — this harness never writes the manifest, "
        "never marks a baseline approved and never copies images across platforms",
        "",
        "| case | deterministic | result | changed px | fraction | max channel diff |",
        "| --- | --- | --- | --- | --- | --- |",
    ]
    for result in results:
        stats = result.get("stats")
        changed = describe_stats(result, "changed_pixels")
        fraction = describe_fraction(stats.get("changed_fraction")) if isinstance(stats, dict) else "-"
        max_diff = describe_stats(result, "max_channel_diff")
        deterministic = result["deterministic"]
        deterministic_text = "yes" if deterministic is True else ("no" if deterministic is False else "-")
        summary_lines.append(
            f"| `{result['key']}` | {deterministic_text} | {result['result']} | "
            f"{changed} | {fraction} | {max_diff} |"
        )
    if environment_problems:
        summary_lines += ["", "### Environment problems", ""]
        summary_lines += [f"- {problem}" for problem in environment_problems]
    errors = [result for result in results if result["error"]]
    if errors:
        summary_lines += ["", "### Errors", ""]
        summary_lines += [f"- `{result['key']}`: {result['error']}" for result in errors]
    summary = "\n".join(summary_lines) + "\n"
    (work_dir / "summary.md").write_text(summary, encoding="utf-8")
    if options.summary_file:
        with options.summary_file.open("a", encoding="utf-8") as handle:
            handle.write(summary)
    print(summary)

    for result in results:
        stats = result.get("stats")
        detail = ""
        if isinstance(stats, dict):
            detail = (
                f"changed={stats['changed_pixels']} "
                f"({describe_fraction(stats['changed_fraction'])}) "
                f"max={stats['max_channel_diff']}"
            )
        elif result["error"]:
            detail = str(result["error"])
        print(
            f"  [{result['result']:>18}] {result['key']:<28} "
            f"deterministic={result['deterministic']}  {detail}"
        )
    for problem in environment_problems:
        print(f"  [environment] {problem}", file=sys.stderr)
    report_only = sum(1 for result in results if result["result"] == "mismatch-report-only")
    print(
        f"visual golden CI: {len(results) - visual_failures} ok, {visual_failures} failed, "
        f"{report_only} report-only mismatch, "
        f"{len(environment_problems)} environment problem(s) -> {work_dir}"
    )
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
