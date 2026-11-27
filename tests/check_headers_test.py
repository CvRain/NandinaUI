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


class ModuleDependencyAndThirdPartyTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="nandina-module-deps-")
        self.root = pathlib.Path(self.temporary.name).resolve()

    def tearDown(self):
        self.temporary.cleanup()

    def write_file(self, relative: str, content: str) -> None:
        target = self.root / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(content, encoding="utf-8")

    def test_rejects_upward_and_cyclic_module_dependencies(self):
        self.write_file("nandina/foundation/geometry.hpp", "#pragma once\n")
        self.write_file("nandina/text/font_family.hpp", "#pragma once\n")
        self.write_file("nandina/widget/button.hpp", "#pragma once\n")
        self.write_file("nandina/physics2d/physics_world2d.hpp", "#pragma once\n")
        self.write_file(
            "nandina/theme/style_document.hpp",
            '#include "../text/font_family.hpp"\n',
        )
        self.write_file(
            "nandina/theme/style_document_root_inc.cpp",
            '#include "text/font_family.hpp"\n'
            "#include <nandina/theme/../text/font_family.hpp>\n"
            '#include "..\\\\text\\\\font_family.hpp"\n',
        )
        self.write_file(
            "nandina/scene/bad_bridge.cpp",
            '#include "../widget/button.hpp"\n#include <nandina/physics2d/physics_world2d.hpp>\n',
        )
        self.write_file(
            "nandina/reactive/bad_signal.hpp",
            '#include "../foundation/geometry.hpp"\n',
        )
        manifest = {
            "nandina/foundation/geometry.hpp": {"level": "experimental", "reason": "", "requires": ""},
            "nandina/text/font_family.hpp": {"level": "experimental", "reason": "", "requires": ""},
            "nandina/widget/button.hpp": {"level": "experimental", "reason": "", "requires": ""},
            "nandina/physics2d/physics_world2d.hpp": {
                "level": "experimental",
                "reason": "",
                "requires": "physics2d",
            },
            "nandina/theme/style_document.hpp": {
                "level": "experimental",
                "reason": "",
                "requires": "",
            },
            "nandina/reactive/bad_signal.hpp": {
                "level": "experimental",
                "reason": "",
                "requires": "",
            },
        }
        problems = headers.check_module_and_third_party_rules(self.root, manifest)
        joined = "\n".join(problems)
        self.assertIn("nandina/theme/style_document.hpp (theme) -> nandina/text/font_family.hpp (text)", joined)
        self.assertEqual(
            problems.count(
                "disallowed module dependency: "
                "nandina/theme/style_document_root_inc.cpp (theme) -> "
                "nandina/text/font_family.hpp (text)"
            ),
            3,
        )
        self.assertIn("nandina/scene/bad_bridge.cpp (scene) -> nandina/widget/button.hpp (widget)", joined)
        self.assertIn(
            "nandina/scene/bad_bridge.cpp (scene) -> nandina/physics2d/physics_world2d.hpp (physics2d)",
            joined,
        )
        self.assertIn(
            "nandina/reactive/bad_signal.hpp (reactive) -> nandina/foundation/geometry.hpp (foundation)",
            joined,
        )

    def test_rejects_third_party_includes_in_public_headers_except_json_wrapper(self):
        self.write_file("nandina/scene/node2d.hpp", "#pragma once\n")
        self.write_file("nandina/foundation/json.hpp", "#include <nlohmann/json.hpp>\n#include <vector>\n")
        self.write_file(
            "nandina/physics2d/physics_world2d.hpp",
            '#include "../scene/node2d.hpp"\n'
            "#include <box2d/box2d.h>\n"
            '#include "SDL3/SDL.h"\n'
            '#include "../../third_party/stb/stb_image.h"\n'
            "#include <eigen_custom>\n",
        )
        self.write_file(
            "nandina/app/detail/sdl_platform_window.hpp",
            "#include <SDL3/SDL.h>\n",
        )
        manifest = {
            "nandina/scene/node2d.hpp": {"level": "experimental", "reason": "", "requires": ""},
            "nandina/foundation/json.hpp": {"level": "experimental", "reason": "", "requires": ""},
            "nandina/physics2d/physics_world2d.hpp": {
                "level": "experimental",
                "reason": "",
                "requires": "physics2d",
            },
            "nandina/app/detail/sdl_platform_window.hpp": {
                "level": "internal",
                "reason": "",
                "requires": "",
            },
        }
        problems = headers.check_module_and_third_party_rules(self.root, manifest)
        self.assertEqual(
            problems,
            [
                "experimental header includes a third-party header: "
                "nandina/physics2d/physics_world2d.hpp -> <box2d/box2d.h>",
                "experimental header includes a third-party header: "
                'nandina/physics2d/physics_world2d.hpp -> "SDL3/SDL.h"',
                "experimental header includes a third-party header: "
                'nandina/physics2d/physics_world2d.hpp -> "../../third_party/stb/stb_image.h"',
                "experimental header includes a third-party header: "
                "nandina/physics2d/physics_world2d.hpp -> <eigen_custom>",
            ],
        )


if __name__ == "__main__":
    unittest.main()
