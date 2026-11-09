#!/usr/bin/env python3
"""Regression checks for the header gate's `compile_commands` parsing.

Meson emits an `arguments` array on some platforms and a single quoted
`command` string on others. On Windows every token is quoted and include paths
use backslashes, so a whitespace split silently collects no flags at all and the
standalone-include check fails on headers that need third-party includes. These
tests pin both shapes without needing a configured build directory.
"""

import importlib.util
import json
import pathlib
import sys
import tempfile
import unittest


helper_path = pathlib.Path(sys.argv.pop(1))
spec = importlib.util.spec_from_file_location("check_headers", helper_path)
headers = importlib.util.module_from_spec(spec)
spec.loader.exec_module(headers)


class CompileFlagsTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="nandina-header-flags-")
        self.root = pathlib.Path(self.temporary.name).resolve()
        self.build = self.root / "build"
        self.build.mkdir()

    def tearDown(self):
        self.temporary.cleanup()

    def write(self, entries):
        (self.build / "compile_commands.json").write_text(json.dumps(entries), encoding="utf-8")

    def test_windows_quoted_command_is_split_and_absolutized(self):
        command = (
            '"C:\\msys64\\ucrt64\\bin\\ccache.exe" "c++" "-I..\\nandina" '
            '"-Isubprojects\\nlohmann_json\\single_include" "-Isubprojects\\my deps\\include" '
            '"-isystem..\\subprojects\\SDL3\\include" "-D_GLIBCXX_ASSERTIONS=1" '
            '"-std=c++26" "-c" ../nandina/foundation/json.cpp'
        )
        self.write(
            [
                {
                    "directory": str(self.build),
                    "command": command,
                    "file": "../nandina/foundation/json.cpp",
                }
            ]
        )
        flags = headers.compile_flags(self.build)
        for flag in (
            f"-I{(self.build / '..' / 'nandina').as_posix()}",
            f"-I{(self.build / 'subprojects' / 'nlohmann_json' / 'single_include').as_posix()}",
            f"-I{(self.build / 'subprojects' / 'my deps' / 'include').as_posix()}",
            f"-isystem{(self.build / '..' / 'subprojects' / 'SDL3' / 'include').as_posix()}",
            "-D_GLIBCXX_ASSERTIONS=1",
            "-std=c++26",
        ):
            self.assertIn(flag, flags)

    def test_arguments_array_and_separate_flag_values(self):
        include = self.root / "deps include"
        self.write(
            [
                {
                    "directory": str(self.build),
                    "arguments": [
                        "c++",
                        "-I../nandina",
                        "-isystem",
                        str(include),
                        "-I",
                        str(self.root / "bare"),
                        "-DSOME_DEFINE=1",
                        "-std=c++26",
                        "-c",
                        "../nandina/widget/authoring.cpp",
                    ],
                    "file": "../nandina/widget/authoring.cpp",
                }
            ]
        )
        flags = headers.compile_flags(self.build)
        for flag in (
            f"-I{(self.build / '..' / 'nandina').as_posix()}",
            f"-isystem{include.as_posix()}",
            f"-I{(self.root / 'bare').as_posix()}",
            "-DSOME_DEFINE=1",
            "-std=c++26",
        ):
            self.assertIn(flag, flags)

    def test_non_nandina_translation_units_do_not_contribute_flags(self):
        self.write(
            [
                {
                    "directory": str(self.build),
                    "command": '"c++" "-Ishould/not/appear" "-c" ../tools/embed_binary.cpp',
                    "file": "../tools/embed_binary.cpp",
                }
            ]
        )
        self.assertEqual(headers.compile_flags(self.build), ["-std=c++26"])

    def test_missing_compile_database_is_reported(self):
        with self.assertRaises(SystemExit):
            headers.compile_flags(self.build / "absent")


if __name__ == "__main__":
    unittest.main()
