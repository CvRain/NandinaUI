#!/usr/bin/env python3
"""批准/撤销页面 golden 候选。

批准前校验候选文件与 manifest 的哈希一致（避免批准一个已经变化的图）；批准写入
`reviewed_by` / `reviewed_on` / `review_note`，比较器据此把 approved 条目当基线判失败。
撤销把状态改回 candidate 并清掉审阅字段。
"""

from __future__ import annotations

import argparse
import hashlib
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
DEFAULT_MANIFEST = ROOT / "tests/golden/backend/manifest.toml"


def parse_manifest(path: pathlib.Path) -> list[dict[str, str]]:
    import tomllib

    data = tomllib.loads(path.read_text(encoding="utf-8"))
    return data.get("entries", [])


def render(entries: list[dict[str, str]]) -> str:
    lines = [
        "# 页面 golden 清单（docs/references/visual_regression.md 的候选层）。",
        "# 候选图由 tools/capture_backend_golden.py 生成；只有维护者把 status 改成",
        '# "approved" 之后才可作为基线，比较器也只对 approved 条目判失败。',
        "",
        "schema = 1",
        "",
        "[capture]",
        'protocol = "30 frames, dt=1/60, 1200x800 window"',
        "",
    ]
    for entry in entries:
        lines.append("[[entries]]")
        for field in ("page", "backend", "platform", "image", "width", "height", "sha256", "status", "captured"):
            value = entry[field]
            lines.append(f'{field} = "{value}"' if field != "width" and field != "height" else f"{field} = {value}")
        for field in ("reviewed_by", "reviewed_on", "review_note"):
            if entry.get(field):
                lines.append(f'{field} = "{entry[field]}"')
        lines.append("")
    return "\n".join(lines).rstrip() + "\n"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=pathlib.Path, default=DEFAULT_MANIFEST)
    parser.add_argument("--entries", default="all", help="all or comma separated page/backend")
    parser.add_argument("--reviewer", default="")
    parser.add_argument("--note", default="")
    parser.add_argument("--on", default="2026-10-20")
    parser.add_argument("--revoke", action="store_true")
    options = parser.parse_args()
    if not options.revoke and not options.reviewer:
        raise SystemExit("approval requires --reviewer (who reviewed these candidates)")

    entries = parse_manifest(options.manifest)
    wanted = None if options.entries == "all" else set(options.entries.split(","))
    changed = 0
    for entry in entries:
        key = f"{entry['page']}/{entry['backend']}/{entry.get('platform', 'linux')}"
        if wanted is not None and key not in wanted:
            continue
        image = options.manifest.parent / entry["image"]
        if not image.is_file():
            raise SystemExit(f"missing candidate image: {image}")
        if hashlib.sha256(image.read_bytes()).hexdigest() != entry["sha256"]:
            raise SystemExit(f"{key}: candidate changed since capture; re-capture before approving")
        if options.revoke:
            entry["status"] = "candidate"
            entry.pop("reviewed_by", None)
            entry.pop("reviewed_on", None)
            entry.pop("review_note", None)
            changed += 1
        else:
            entry["status"] = "approved"
            entry["reviewed_by"] = options.reviewer
            entry["reviewed_on"] = options.on
            if options.note:
                entry["review_note"] = options.note
            changed += 1
    if changed == 0:
        raise SystemExit("no matching entries")
    options.manifest.write_text(render(entries), encoding="utf-8")
    print(f"{'revoked' if options.revoke else 'approved'} {changed} entries in {options.manifest}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
