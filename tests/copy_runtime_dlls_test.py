import contextlib
import importlib.util
import io
import json
import pathlib
import sys
import tempfile
import unittest
from unittest import mock


spec = importlib.util.spec_from_file_location("runtime_dlls", pathlib.Path(sys.argv.pop(1)))
runtime = importlib.util.module_from_spec(spec)
spec.loader.exec_module(runtime)


class RuntimeDllTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = pathlib.Path(temporary.name)
        self.build = self.root / "build"
        self.build.mkdir()
        self.compiler = self.root / "compiler"
        self.compiler.mkdir()
        self.path_bin = self.root / "path-bin"
        self.path_bin.mkdir()

    def report(self, source="vendored", linkage="static", enabled=True):
        (self.build / "deps.json").write_text(json.dumps({"deps": [{
            "name": "sdl3", "source": source, "linkage": linkage, "enabled": enabled,
        }]}), encoding="utf-8")

    def dll(self, directory, name="SDL3.dll"):
        directory.mkdir(parents=True, exist_ok=True)
        result = directory / name
        result.write_bytes(b"fixture DLL")
        return result

    def invoke(self):
        with (
            mock.patch.object(sys, "argv", ["copy_runtime_dlls", "--build-dir", str(self.build)]),
            mock.patch.object(runtime, "search_roots", return_value=[self.compiler, self.path_bin]),
            contextlib.redirect_stdout(io.StringIO()) as output,
            contextlib.redirect_stderr(io.StringIO()) as errors,
        ):
            status = runtime.main()
        return status, output.getvalue(), errors.getvalue()

    def test_static_and_disabled_ignore_old_shared_artifacts(self):
        self.dll(self.build / "subprojects" / "SDL3", "libSDL3_shared.dll")
        self.dll(self.compiler)
        for enabled, linkage in ((True, "static"), (False, "none")):
            self.report(linkage=linkage, enabled=enabled)
            self.assertIsNone(runtime.find_sdl_dll(self.build, [self.compiler]))
            status, output, errors = self.invoke()
            self.assertEqual(status, 0)
            self.assertIn("no SDL3 DLL required", output)
            self.assertNotIn("SDL3", errors)

    def test_vendored_shared_uses_build_artifact(self):
        self.report(linkage="shared")
        expected = self.dll(self.build / "subprojects" / "SDL3", "libSDL3_shared.dll")
        self.dll(self.compiler)
        self.assertEqual(runtime.find_sdl_dll(self.build, [self.compiler]), expected)

    def test_missing_vendored_artifact_does_not_use_an_unrelated_system_dll(self):
        self.report(linkage="shared")
        self.dll(self.compiler)
        self.assertIsNone(runtime.find_sdl_dll(self.build, [self.compiler]))

    def test_system_searches_compiler_then_path_and_ignores_old_build_dll(self):
        self.report(source="system", linkage="unknown")
        self.dll(self.build / "subprojects" / "SDL3", "libSDL3_shared.dll")
        fallback = self.dll(self.path_bin)
        roots = [self.compiler, self.path_bin]
        self.assertEqual(runtime.find_sdl_dll(self.build, roots), fallback)
        preferred = self.dll(self.compiler)
        self.assertEqual(runtime.find_sdl_dll(self.build, roots), preferred)

    def test_system_dll_is_copied_beside_application_only(self):
        self.report(source="system", linkage="unknown")
        original = self.dll(self.path_bin)
        application = self.build / "showcase"
        application.mkdir()
        (application / "app.exe").write_bytes(b"fixture executable")
        dependency = self.build / "subprojects" / "dependency"
        dependency.mkdir(parents=True)
        (dependency / "tool.exe").write_bytes(b"fixture executable")
        self.assertEqual(self.invoke()[0], 0)
        self.assertEqual((application / original.name).read_bytes(), original.read_bytes())
        self.assertFalse((dependency / original.name).exists())

    def test_missing_system_dll_reports_unknown_linkage(self):
        self.report(source="system", linkage="unknown")
        status, _, errors = self.invoke()
        self.assertEqual(status, 0)
        self.assertIn("system SDL3 linkage is unknown", errors)
        self.assertIn("--compiler-dir/PATH", errors)


if __name__ == "__main__":
    unittest.main()
