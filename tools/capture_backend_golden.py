#!/usr/bin/env python3
"""采集页面 golden 候选图（BMP → PNG）并更新 manifest。

候选图**不是**已接受基线：manifest 里 `status = "candidate"`，由维护者人工审阅后改成
`approved`。已经 approved 的条目拒绝被覆盖（除非 `--force`），避免一次自动采集把基线换掉。
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import pathlib
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

import golden_image  # noqa: E402

DEFAULT_PROBE = ROOT / "buildDir-probe/probe/cross_platform/desktop/nandina_sdl_page_probe"


def render_manifest(entries: dict[tuple[str, str], dict[str, object]], protocol: str) -> str:
    lines = [
        "# 页面 golden 清单（docs/references/visual_regression.md 的候选层）。",
        "# 候选图由 tools/capture_backend_golden.py 生成；只有维护者把 status 改成",
        "# \"approved\" 之后才可作为基线，比较器也只对 approved 条目判失败。",
        "",
        "schema = 1",
        "",
        "[capture]",
        f'protocol = "{protocol}"',
        "",
    ]
    for key in sorted(entries):
        entry = entries[key]
        lines.append("[[entries]]")
        lines.append(f'page = "{entry["page"]}"')
        lines.append(f'backend = "{entry["backend"]}"')
        lines.append(f'image = "{entry["image"]}"')
        lines.append(f'width = {entry["width"]}')
        lines.append(f'height = {entry["height"]}')
        lines.append(f'sha256 = "{entry["sha256"]}"')
        lines.append(f'status = "{entry["status"]}"')
        lines.append(f'captured = "{entry["captured"]}"')
        for field in ("reviewed_by", "reviewed_on", "review_note"):
            if entry.get(field):
                lines.append(f'{field} = "{entry[field]}"')
        lines.append("")
    return "\n".join(lines).rstrip() + "\n"


def load_existing(manifest: pathlib.Path) -> dict[tuple[str, str], dict[str, object]]:
    try:
        import tomllib
    except ModuleNotFoundError:  # pragma: no cover
        raise SystemExit("capture_backend_golden: requires Python 3.11+ (tomllib)")
    if not manifest.is_file():
        return {}
    data = tomllib.loads(manifest.read_text(encoding="utf-8"))
    return {(entry["page"], entry["backend"]): entry for entry in data.get("entries", [])}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--probe", type=pathlib.Path, default=DEFAULT_PROBE)
    parser.add_argument("--pages", default="home,label,anchors")
    parser.add_argument("--modes", default="sdf,cpu")
    parser.add_argument("--frames", type=int, default=30)
    parser.add_argument("--output", type=pathlib.Path, default=ROOT / "tests/golden/backend")
    parser.add_argument("--force", action="store_true", help="overwrite approved entries")
    options = parser.parse_args()
    if not options.probe.is_file():
        raise SystemExit(f"probe not built: {options.probe}")

    manifest = options.output / "manifest.toml"
    candidates = options.output / "candidates"
    candidates.mkdir(parents=True, exist_ok=True)
    entries = load_existing(manifest)
    captured = "2026-10-19"
    for page in options.pages.split(","):
        for backend in options.modes.split(","):
            page, backend = page.strip(), backend.strip()
            key = (page, backend)
            if entries.get(key, {}).get("status") == "approved" and not options.force:
                print(f"skip {page}/{backend}: approved baseline (use --force to replace)")
                continue
            with tempfile.TemporaryDirectory(prefix="nandina-golden-") as directory:
                bmp = pathlib.Path(directory) / "capture.bmp"
                environment = {
                    **os.environ,
                    "NANDINA_PROBE_PAGE": page,
                    "NANDINA_PROBE_FRAMES": str(options.frames),
                    "NANDINA_PROBE_CAPTURE": str(bmp),
                }
                if backend == "cpu":
                    environment["NANDINA_SDL_SDF"] = "off"
                else:
                    environment.pop("NANDINA_SDL_SDF", None)
                result = subprocess.run(
                    [str(options.probe)], capture_output=True, text=True, timeout=300, env=environment
                )
                if result.returncode != 0:
                    raise RuntimeError(
                        f"capture failed for {page}/{backend}: {result.returncode}\n{result.stdout}\n{result.stderr}"
                    )
                image = golden_image.read_bmp(str(bmp))
                target = candidates / f"{page}-{backend}.png"
                golden_image.write_png(str(target), image)
                digest = hashlib.sha256(target.read_bytes()).hexdigest()
                entries[key] = {
                    "page": page,
                    "backend": backend,
                    "image": f"candidates/{target.name}",
                    "width": image.width,
                    "height": image.height,
                    "sha256": digest,
                    "status": "candidate",
                    "captured": captured,
                }
                print(f"captured {page}/{backend} -> {target.relative_to(options.output)} ({image.width}x{image.height}, {target.stat().st_size} bytes)")

    manifest.write_text(
        render_manifest(entries, f"{options.frames} frames, dt=1/60, 1200x800 window"),
        encoding="utf-8",
    )
    print(f"wrote {manifest}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
