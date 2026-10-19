#!/usr/bin/env python3
"""页面级后端性能 harness：同一页面分别走生产 SDF 与 CPU 几何，按固定协议采样。

驱动 `nandina_sdl_page_probe`（受追踪的页面探针），每个配置：
预热 `--warmup` 帧、采样 `--samples` 帧、重复 `--repeats` 次；每次从逐帧 CSV 取
p50/p95，再对重复次数取中位数（不把多次样本混成一个分布）。
同时记录每帧实际布局次数、SDF 绑定/解绑、cache hit/miss、纹理字节与进程 RSS。

这是**报告模式**，不是 CI 门禁：性能阈值按 performance_budget.md 的候选规则单独启用。
用法示例：
    tools/backend_performance.py --pages label --modes sdf,cpu --stress 0
    tools/backend_performance.py --pages home,label,anchors --interaction
"""

from __future__ import annotations

import argparse
import json
import os
import pathlib
import statistics
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
DEFAULT_PROBE = ROOT / "buildDir-probe/probe/cross_platform/desktop/nandina_sdl_page_probe"


def parse_summary(output: str) -> dict[str, str]:
    summary: dict[str, str] = {}
    for line in output.splitlines():
        for key, marker in (
            ("sdf", "sdf: "),
            ("resources", "resources: "),
            ("layout", "layout passes="),
            ("frame_cost", "frame cost over "),
            ("capabilities", "device capabilities: "),
        ):
            if marker in line:
                summary[key] = line.strip()
    return summary


def run_once(
    probe: pathlib.Path,
    page: str,
    mode: str,
    frames: int,
    stress: int,
    interaction: bool,
    overlay: bool,
) -> tuple[list[dict[str, float]], dict[str, str]]:
    with tempfile.TemporaryDirectory(prefix="nandina-perf-") as directory:
        samples = pathlib.Path(directory) / "samples.csv"
        environment = {
            **os.environ,
            "NANDINA_PROBE_PAGE": page,
            "NANDINA_PROBE_FRAMES": str(frames),
            "NANDINA_PROBE_SAMPLES_OUT": str(samples),
        }
        environment.pop("NANDINA_PROBE_CAPTURE", None)
        if mode == "cpu":
            environment["NANDINA_SDL_SDF"] = "off"
        else:
            environment.pop("NANDINA_SDL_SDF", None)
        if stress > 0:
            environment["NANDINA_PROBE_STRESS"] = str(stress)
        if interaction:
            environment["NANDINA_PROBE_INTERACTION"] = "1"
        if overlay:
            environment["NANDINA_PROBE_OVERLAY"] = "1"
        result = subprocess.run(
            [str(probe)], capture_output=True, text=True, timeout=600, env=environment
        )
        if result.returncode != 0:
            raise RuntimeError(
                f"probe failed for {page}/{mode}: {result.returncode}\n{result.stdout}\n{result.stderr}"
            )
        rows: list[dict[str, float]] = []
        for line in samples.read_text().splitlines()[1:]:
            values = [float(item) for item in line.split(",")]
            rows.append(
                {
                    "total_ms": values[1],
                    "layout_ms": values[2],
                    "draw_ms": values[3],
                    "layout_passes": values[4],
                    "frame_binds": values[5],
                    "frame_unbinds": values[6],
                    "hits": values[7],
                    "misses": values[8],
                    "cached": values[9],
                }
            )
        return rows, parse_summary(result.stdout + result.stderr)


def percentile(values: list[float], fraction: float) -> float:
    ordered = sorted(values)
    index = max(0, min(len(ordered) - 1, int(len(ordered) * fraction + 0.999999) - 1))
    return ordered[index]


def summarise(rows: list[dict[str, float]], warmup: int) -> dict[str, float]:
    sampled = rows[warmup:]
    if not sampled:
        raise RuntimeError("no samples after warmup")
    totals = [row["total_ms"] for row in sampled]
    # hits/misses 是设备生命周期累计值：必须取相邻帧差值，不能对累计值求均值。
    misses = [later["misses"] - earlier["misses"] for earlier, later in zip(sampled, sampled[1:])]
    hits = [later["hits"] - earlier["hits"] for earlier, later in zip(sampled, sampled[1:])]
    return {
        "frames": len(sampled),
        "p50_ms": statistics.median(totals),
        "p95_ms": percentile(totals, 0.95),
        "layout_passes_per_frame": statistics.fmean(row["layout_passes"] for row in sampled),
        "binds_per_frame": statistics.fmean(row["frame_binds"] for row in sampled),
        "unbinds_per_frame": statistics.fmean(row["frame_unbinds"] for row in sampled),
        "misses_per_frame": statistics.fmean(misses) if misses else 0.0,
        "hits_per_frame": statistics.fmean(hits) if hits else 0.0,
        "cached": sampled[-1]["cached"],
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--probe", type=pathlib.Path, default=DEFAULT_PROBE)
    parser.add_argument("--pages", default="label")
    parser.add_argument("--modes", default="sdf,cpu")
    parser.add_argument("--warmup", type=int, default=300)
    parser.add_argument("--samples", type=int, default=1200)
    parser.add_argument("--repeats", type=int, default=5)
    parser.add_argument("--stress", type=int, default=0)
    parser.add_argument("--interaction", action="store_true")
    parser.add_argument("--overlay", action="store_true")
    parser.add_argument("--output", type=pathlib.Path)
    options = parser.parse_args()

    if not options.probe.is_file():
        raise SystemExit(f"probe not built: {options.probe} (run meson compile -C buildDir-probe)")
    report: dict[str, object] = {
        "protocol": {
            "warmup": options.warmup,
            "samples": options.samples,
            "repeats": options.repeats,
            "stress": options.stress,
            "interaction": options.interaction,
            "overlay": options.overlay,
        },
        "configurations": [],
    }
    for page in options.pages.split(","):
        for mode in options.modes.split(","):
            repeats = []
            summary_lines = {}
            for repeat in range(options.repeats):
                rows, summary = run_once(
                    options.probe,
                    page.strip(),
                    mode.strip(),
                    options.warmup + options.samples,
                    options.stress,
                    options.interaction,
                    options.overlay,
                )
                repeats.append(summarise(rows, options.warmup))
                summary_lines = summary
            entry = {
                "page": page.strip(),
                "mode": mode.strip(),
                "p50_ms": statistics.median(item["p50_ms"] for item in repeats),
                "p95_ms": statistics.median(item["p95_ms"] for item in repeats),
                "p50_range_ms": [
                    min(item["p50_ms"] for item in repeats),
                    max(item["p50_ms"] for item in repeats),
                ],
                "layout_passes_per_frame": statistics.median(
                    item["layout_passes_per_frame"] for item in repeats
                ),
                "binds_per_frame": statistics.median(item["binds_per_frame"] for item in repeats),
                "unbinds_per_frame": statistics.median(
                    item["unbinds_per_frame"] for item in repeats
                ),
                "misses_per_frame": statistics.median(
                    item["misses_per_frame"] for item in repeats
                ),
                "hits_per_frame": statistics.median(item["hits_per_frame"] for item in repeats),
                "cached": statistics.median(item["cached"] for item in repeats),
                "summary": summary_lines,
            }
            report["configurations"].append(entry)
            print(
                f"{entry['page']:<8} {entry['mode']:<4} p50={entry['p50_ms']:.3f} "
                f"p95={entry['p95_ms']:.3f} binds/frame={entry['binds_per_frame']:.0f} "
                f"misses/frame={entry['misses_per_frame']:.1f} hits/frame={entry['hits_per_frame']:.0f} "
                f"cached={entry['cached']:.0f}"
            )

    if options.output:
        options.output.parent.mkdir(parents=True, exist_ok=True)
        options.output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
        print(f"wrote {options.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
