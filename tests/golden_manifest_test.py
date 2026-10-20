"""golden 清单完整性：文件存在、哈希与尺寸匹配、状态合法、无重复条目。

这只验证清单自身可信，不代表图像已批准或与当前代码一致；像素比较由
tools/compare_backend_golden.py 手动执行（approved 条目才判失败）。
"""

import pathlib
import sys
import tomllib

ROOT = pathlib.Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "tests/golden/backend/manifest.toml"
VALID_STATUS = {"candidate", "approved"}


def main() -> int:
    if not MANIFEST.is_file():
        raise SystemExit(f"missing manifest: {MANIFEST}")
    data = tomllib.loads(MANIFEST.read_text(encoding="utf-8"))
    entries = data.get("entries", [])
    if not entries:
        raise SystemExit("manifest has no entries")
    problems = []
    seen = set()
    for entry in entries:
        key = (entry.get("page"), entry.get("backend"))
        if key in seen:
            problems.append(f"duplicate entry: {key}")
        seen.add(key)
        if entry.get("status") not in VALID_STATUS:
            problems.append(f"{key}: invalid status {entry.get('status')}")
        if entry.get("status") == "approved":
            if not entry.get("reviewed_by") or not entry.get("reviewed_on"):
                problems.append(f"{key}: approved entry lacks reviewed_by/reviewed_on")
        image = MANIFEST.parent / str(entry.get("image", ""))
        if not image.is_file():
            problems.append(f"{key}: missing image {entry.get('image')}")
            continue
        import hashlib

        digest = hashlib.sha256(image.read_bytes()).hexdigest()
        if digest != entry.get("sha256"):
            problems.append(f"{key}: sha256 mismatch (file changed without updating manifest)")
        import struct

        header = image.read_bytes()[:24]
        width, height = struct.unpack(">II", header[16:24])
        if (width, height) != (entry.get("width"), entry.get("height")):
            problems.append(f"{key}: dimensions {width}x{height} != manifest")
    if problems:
        print("golden manifest check failed:", file=sys.stderr)
        for problem in problems:
            print(f"  - {problem}", file=sys.stderr)
        return 1
    approved = sum(1 for entry in entries if entry["status"] == "approved")
    print(f"golden manifest ok: {len(entries)} entries ({approved} approved, {len(entries) - approved} candidates)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
