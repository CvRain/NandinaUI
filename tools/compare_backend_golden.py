#!/usr/bin/env python3
"""比较页面截图与 golden 候选/基线，输出差异统计与差异图。

规则（见 docs/references/visual_regression.md 与路径契约 §12）：
- `status = "candidate"`：只报告，不判失败（候选还不是基线）；
- `status = "approved"`：差异像素比例超过 `--max-changed-fraction` 时退出码 1；
- 差异图把变化像素标红，其余保留 golden 的灰度，便于人工审阅。
"""

from __future__ import annotations

import argparse
import json
import pathlib
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

import golden_image  # noqa: E402

DEFAULT_PROBE = ROOT / "buildDir-probe/probe/cross_platform/desktop/nandina_sdl_page_probe"


def load_entries(manifest: pathlib.Path) -> dict[tuple[str, str], dict[str, object]]:
    import tomllib

    data = tomllib.loads(manifest.read_text(encoding="utf-8"))
    return {(entry["page"], entry["backend"]): entry for entry in data.get("entries", [])}


def capture(probe: pathlib.Path, page: str, backend: str, frames: int) -> pathlib.Path:
    directory = pathlib.Path(tempfile.mkdtemp(prefix="nandina-compare-"))
    bmp = directory / "capture.bmp"
    environment = {
        **__import__("os").environ,
        "NANDINA_PROBE_PAGE": page,
        "NANDINA_PROBE_FRAMES": str(frames),
        "NANDINA_PROBE_CAPTURE": str(bmp),
    }
    if backend == "cpu":
        environment["NANDINA_SDL_SDF"] = "off"
    else:
        environment.pop("NANDINA_SDL_SDF", None)
    result = subprocess.run(
        [str(probe)], capture_output=True, text=True, timeout=300, env=environment
    )
    if result.returncode != 0:
        raise SystemExit(f"capture failed: {result.returncode}\n{result.stdout}\n{result.stderr}")
    return bmp


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=pathlib.Path, default=ROOT / "tests/golden/backend/manifest.toml")
    parser.add_argument("--page", default="label")
    parser.add_argument("--backend", default="sdf")
    parser.add_argument("--capture", type=pathlib.Path, help="use an existing screenshot")
    parser.add_argument("--probe", type=pathlib.Path, default=DEFAULT_PROBE)
    parser.add_argument("--frames", type=int, default=30)
    parser.add_argument("--tolerance", type=int, default=3, help="per-channel threshold")
    parser.add_argument("--max-changed-fraction", type=float, default=0.001)
    parser.add_argument("--diff", type=pathlib.Path, help="write a diff PNG here")
    parser.add_argument("--json", type=pathlib.Path, help="write stats JSON here")
    parser.add_argument("--strict", action="store_true", help="fail for candidates too")
    options = parser.parse_args()

    entries = load_entries(options.manifest)
    entry = entries.get((options.page, options.backend))
    if entry is None:
        raise SystemExit(f"no golden entry for {options.page}/{options.backend}")
    golden = golden_image.load(str(options.manifest.parent / str(entry["image"])))
    captured = options.capture or capture(options.probe, options.page, options.backend, options.frames)
    image = golden_image.load(str(captured))
    if (golden.width, golden.height) != (image.width, image.height):
        raise SystemExit(
            f"size mismatch: golden {golden.width}x{golden.height} vs capture {image.width}x{image.height}"
        )

    changed = 0
    total = golden.width * golden.height
    max_diff = 0
    absolute_sum = 0
    diff = golden_image.Image(golden.width, golden.height, bytearray(golden.pixels))
    for y in range(golden.height):
        for x in range(golden.width):
            golden_pixel = golden.at(x, y)
            capture_pixel = image.at(x, y)
            differences = [abs(golden_pixel[i] - capture_pixel[i]) for i in range(3)]
            pixel_max = max(differences)
            max_diff = max(max_diff, pixel_max)
            absolute_sum += sum(differences)
            offset = (y * golden.width + x) * 4
            if pixel_max > options.tolerance:
                changed += 1
                diff.pixels[offset : offset + 4] = bytes((255, 0, 0, 255))
            else:
                grey = (golden_pixel[0] + golden_pixel[1] + golden_pixel[2]) // 3
                diff.pixels[offset : offset + 4] = bytes((grey, grey, grey, 255))

    fraction = changed / total
    stats = {
        "page": options.page,
        "backend": options.backend,
        "status": entry["status"],
        "width": golden.width,
        "height": golden.height,
        "changed_pixels": changed,
        "changed_fraction": fraction,
        "max_channel_diff": max_diff,
        "mean_abs_diff": absolute_sum / (total * 3),
        "tolerance": options.tolerance,
        "max_changed_fraction": options.max_changed_fraction,
    }
    print(json.dumps(stats, indent=2))
    if options.json:
        options.json.write_text(json.dumps(stats, indent=2) + "\n", encoding="utf-8")
    if options.diff:
        golden_image.write_png(str(options.diff), diff)
        print(f"wrote {options.diff}")

    if entry["status"] == "approved":
        if fraction > options.max_changed_fraction:
            print(
                f"FAIL: {fraction * 100:.3f}% of pixels differ beyond ±{options.tolerance} "
                f"(limit {options.max_changed_fraction * 100:.3f}%)",
                file=sys.stderr,
            )
            return 1
        print("golden match within thresholds")
        return 0
    if options.strict and fraction > options.max_changed_fraction:
        return 1
    print("candidate report only (not a baseline)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
