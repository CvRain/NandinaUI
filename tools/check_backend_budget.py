#!/usr/bin/env python3
"""页面后端预算检查：确定性门禁 + 候选计时阈值。

确定性门禁（总是生效，失败退出 1）：
- 稳态场景（interaction=none）每帧布局 pass 必须为 0；
- 使用 SDF 的页面在预热后 miss/帧必须为 0（全命中）；
- binds/帧不得超过基线的 1.25 倍 + 2。

计时阈值（候选：先报告；`--enforce` 且基线 `enforce_timing=true` 时阻断）：
- p50：超过 baseline*1.10 或 baseline+0.20ms → warn；超过 baseline*1.20 或 +0.40ms → block；
- p95：超过 baseline*1.25 或 +0.75ms → block。
基线必须来自同机同构建类型；debug 构建的基线不应开启计时阻断（契约 §11/§14）。
"""

from __future__ import annotations

import argparse
import json
import pathlib
import sys


def load(path: pathlib.Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--report", type=pathlib.Path, required=True)
    parser.add_argument("--baseline", type=pathlib.Path, required=True)
    parser.add_argument("--enforce", action="store_true")
    options = parser.parse_args()

    report = load(options.report)
    baseline = load(options.baseline)
    indexed = {
        (entry["page"], entry["mode"], entry.get("interaction", "none")): entry
        for entry in baseline.get("entries", [])
    }
    problems: list[str] = []
    warnings: list[str] = []
    blocks: list[str] = []
    if options.enforce and baseline.get("enforce_timing") and baseline.get("buildtype") != "release":
        print(
            "refusing to enforce timing against a non-release baseline "
            f"(buildtype={baseline.get('buildtype')!r})",
            file=sys.stderr,
        )
        return 1
    checked = 0
    for entry in report.get("configurations", []):
        key = (entry["page"], entry["mode"], entry.get("interaction", "none"))
        reference = indexed.get(key)
        if reference is None:
            warnings.append(f"{key}: no baseline entry")
            continue
        checked += 1
        interaction = entry.get("interaction", "none")
        if interaction == "none":
            if entry["layout_passes_per_frame"] != 0:
                problems.append(
                    f"{key}: steady-state layout passes {entry['layout_passes_per_frame']}"
                )
            if entry["binds_per_frame"] > 0 and entry["misses_per_frame"] != 0:
                problems.append(f"{key}: steady-state misses/frame {entry['misses_per_frame']}")
        allowed_binds = reference["binds_per_frame"] * 1.25 + 2
        if entry["binds_per_frame"] > allowed_binds:
            problems.append(
                f"{key}: binds/frame {entry['binds_per_frame']} > {allowed_binds:.1f}"
            )
        for percentile in ("p50", "p95"):
            actual = entry[f"{percentile}_ms"]
            expected = reference[f"{percentile}_ms"]
            ratio = actual / expected if expected > 0 else 0.0
            warn_limit = max(expected * 1.10, expected + 0.20)
            block_limit = max(expected * 1.20, expected + 0.40)
            if percentile == "p95":
                block_limit = max(expected * 1.25, expected + 0.75)
                warn_limit = max(expected * 1.10, expected + 0.20)
            if actual > block_limit:
                blocks.append(
                    f"{key} {percentile}: {actual:.3f}ms > {block_limit:.3f}ms "
                    f"(baseline {expected:.3f}, x{ratio:.2f})"
                )
            elif actual > warn_limit:
                warnings.append(
                    f"{key} {percentile}: {actual:.3f}ms > {warn_limit:.3f}ms "
                    f"(baseline {expected:.3f}, x{ratio:.2f})"
                )
        print(
            f"{entry['page']:<8} {entry['mode']:<4} {interaction:<7} "
            f"p50={entry['p50_ms']:.3f} (base {reference['p50_ms']:.3f}) "
            f"p95={entry['p95_ms']:.3f} (base {reference['p95_ms']:.3f})"
        )
    if checked == 0:
        print("no comparable entries", file=sys.stderr)
        return 1
    for problem in problems:
        print(f"FAIL: {problem}", file=sys.stderr)
    for block in blocks:
        print(f"{'FAIL' if options.enforce and baseline.get('enforce_timing') else 'WARN'}: {block}", file=sys.stderr)
    for warning in warnings:
        print(f"WARN: {warning}", file=sys.stderr)
    if options.enforce and not baseline.get("enforce_timing"):
        print(
            "note: timing enforcement is disabled by the baseline (enforce_timing=false); "
            "only deterministic gates are enforced",
            file=sys.stderr,
        )
    if problems:
        return 1
    if options.enforce and baseline.get("enforce_timing") and blocks:
        return 1
    print(
        f"budget checked: {checked} configurations, {len(problems)} deterministic failures, "
        f"{len(warnings)} warnings, {len(blocks)} timing block candidates"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
