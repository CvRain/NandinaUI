#!/usr/bin/env python3
"""Compare against a pre-split nanres baseline, not a newly generated expectation."""

import copy
import hashlib
import json
import pathlib
import shutil
import sqlite3
import subprocess
import sys
import tempfile


def normalize(snapshot):
    result = copy.deepcopy(snapshot)
    keys = [row["key"] for row in result["resources"]]
    if len(keys) != len(set(keys)):
        raise ValueError("duplicate resource key")
    result["resources"].sort(key=lambda row: row["key"])
    result["aliases"].sort()
    return result


def snapshot(package):
    with sqlite3.connect(package / "resources.db") as db:
        rows = db.execute("SELECT hex(id), resource_key, media_type, storage, data, external_path, size FROM resources").fetchall()
        resources = []
        for identity, key, media, storage, data, external, size in rows:
            payload = data if storage == 0 else (package / external).read_bytes()
            assert len(payload) == size
            resources.append(dict(id=identity, key=key, media_type=media, storage=storage, size=size, sha256=hashlib.sha256(payload).hexdigest()))
        return normalize(dict(
            application_id=db.execute("PRAGMA application_id").fetchone()[0],
            schema=db.execute("PRAGMA user_version").fetchone()[0],
            resources=resources,
            aliases=[list(row) for row in db.execute("SELECT a.alias, r.resource_key FROM aliases a JOIN resources r ON a.resource_id=r.id")],
        ))


def main():
    tool = pathlib.Path(sys.argv[1]).resolve()
    fixture = pathlib.Path(__file__).parent / "fixtures/resource_equivalence"
    expected = json.loads((fixture / "expected.json").read_text())
    shuffled = copy.deepcopy(expected)
    shuffled["resources"].reverse()
    assert normalize(shuffled) == expected
    duplicate = copy.deepcopy(expected)
    duplicate["resources"].append(duplicate["resources"][0])
    try:
        normalize(duplicate)
    except ValueError:
        pass
    else:
        raise AssertionError("duplicate-key checker accepted invalid baseline")
    for field in ("storage", "id", "media_type"):
        changed = copy.deepcopy(expected)
        changed["resources"][0][field] = "changed"
        assert normalize(changed) != expected
    changed = copy.deepcopy(expected)
    changed["aliases"][0][1] = "app/other"
    assert normalize(changed) != expected
    with tempfile.TemporaryDirectory(prefix="nandina-resource-equivalence-") as directory:
        for name in ("first", "second"):
            root = pathlib.Path(directory) / name
            shutil.copytree(fixture, root)
            original_lock = (root / "resources.lock.toml").read_bytes()
            for command in ("validate", "scan", "pack"):
                subprocess.run([str(tool), command], cwd=root, check=True, capture_output=True)
            assert (root / "resources.lock.toml").read_bytes() == original_lock
            assert snapshot(root / "package") == expected
    print("pre-split package identity, aliases, storage and logical bytes preserved")


if __name__ == "__main__":
    main()
