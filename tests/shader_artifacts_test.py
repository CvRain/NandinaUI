"""Negative checks for the compiler-free shader artifact gate."""

import copy
import importlib.util
import pathlib
import tempfile
import unittest
from unittest import mock

ROOT = pathlib.Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("shader_artifacts", ROOT / "tools/shader_artifacts.py")
artifacts = importlib.util.module_from_spec(spec)
spec.loader.exec_module(artifacts)


class ArtifactChecks(unittest.TestCase):
    def test_metadata_and_content_mutations(self):
        config = artifacts.load()
        for entry in config["shaders"]:
            source = (ROOT / entry["source"]).read_bytes()
            header = (ROOT / entry["header"]).read_text()
            with tempfile.TemporaryDirectory() as directory:
                root = pathlib.Path(directory)
                local = dict(entry, source="shader.hlsl", header="shader.hpp")
                (root / local["source"]).write_bytes(source)
                (root / local["header"]).write_text(header)
                with mock.patch.object(artifacts, "ROOT", root):
                    artifacts.check_entry(local, config["toolchain"])
                    changed = copy.deepcopy(config["toolchain"])
                    changed["glslc_version"] = "wrong-version"
                    with self.assertRaisesRegex(RuntimeError, "toolchain metadata"):
                        artifacts.check_entry(local, changed)
                    (root / local["source"]).write_bytes(source + b"\n// changed\n")
                    with self.assertRaisesRegex(RuntimeError, "source changed"):
                        artifacts.check_entry(local, config["toolchain"])
                    (root / local["source"]).write_bytes(source)
                    (root / local["header"]).write_text(header.replace("0x00010000", "0x00010001", 1))
                    with self.assertRaisesRegex(RuntimeError, "does not match its hash"):
                        artifacts.check_entry(local, config["toolchain"])


if __name__ == "__main__":
    unittest.main()
