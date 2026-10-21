#!/usr/bin/env python3
"""Exercise the formatting guard without needing clang-format installed.

The guard's value is in the decisions it makes, not in clang-format itself, so
this test drives `tools/check_format.py` against a fake git/clang-format pair.
The real formatting baseline is enforced by the dedicated CI job, which is the
only place the pinned version is actually installed.
"""

import contextlib
import importlib.util
import io
import pathlib
import subprocess
import sys
import unittest
from unittest import mock


helper_path = pathlib.Path(sys.argv.pop(1))
spec = importlib.util.spec_from_file_location("check_format", helper_path)
helper = importlib.util.module_from_spec(spec)
spec.loader.exec_module(helper)


SOURCE_ROOT = "/repo"
TRACKED = ("nandina/alpha.cpp", "nandina/beta.hpp", "tests/gamma.cpp")


def completed(arguments, returncode=0, stdout="", stderr=""):
    return subprocess.CompletedProcess(arguments, returncode, stdout=stdout, stderr=stderr)


class FakeToolchain:
    """Stands in for git and clang-format and records how they were invoked."""

    def __init__(
        self,
        version="23.1.1",
        dirty=(),
        tracked=TRACKED,
        version_output=None,
        git_error=None,
        tool_error=None,
    ):
        self.version = version
        self.dirty = set(dirty)
        self.tracked = tracked
        self.version_output = version_output
        self.git_error = git_error
        self.tool_error = tool_error
        self.calls = []
        self.checked = []

    def __call__(self, arguments, **kwargs):
        arguments = [str(argument) for argument in arguments]
        self.calls.append(arguments)
        if arguments[0] == "git":
            if self.git_error is not None:
                raise self.git_error
            return completed(arguments, stdout="".join(f"{path}\0" for path in self.tracked))
        if self.tool_error is not None:
            raise self.tool_error
        if arguments[1] == "--version":
            text = self.version_output
            if text is None:
                text = f"clang-format version {self.version}\n"
            return completed(arguments, stdout=text)
        if arguments[1] == "--dry-run":
            relative = pathlib.Path(arguments[-1]).relative_to(SOURCE_ROOT).as_posix()
            self.checked.append(relative)
            return completed(arguments, returncode=1 if relative in self.dirty else 0)
        raise AssertionError(f"unexpected invocation: {arguments}")


class FormatGuardTests(unittest.TestCase):
    def invoke(self, toolchain, missing=()):
        with (
            mock.patch.object(helper.subprocess, "run", toolchain),
            mock.patch.object(
                pathlib.Path,
                "is_file",
                lambda path: path.relative_to(SOURCE_ROOT).as_posix() not in missing,
            ),
        ):
            errors = io.StringIO()
            with contextlib.redirect_stderr(errors):
                status = helper.main([SOURCE_ROOT])
        return status, errors.getvalue()

    def test_clean_tree_passes_and_checks_every_tracked_source(self):
        toolchain = FakeToolchain()
        status, errors = self.invoke(toolchain)
        self.assertEqual(status, 0)
        self.assertEqual(errors, "")
        self.assertEqual(toolchain.checked, list(TRACKED))
        git_calls = [call for call in toolchain.calls if call[0] == "git"]
        self.assertEqual(len(git_calls), 1)
        self.assertEqual(git_calls[0][:5], ["git", "-C", SOURCE_ROOT, "ls-files", "-z"])
        for suffix in helper.SOURCE_SUFFIXES:
            self.assertIn(f"*{suffix}", git_calls[0])

    def test_dirty_file_is_named_in_the_failure(self):
        toolchain = FakeToolchain(dirty=["nandina/alpha.cpp"])
        status, errors = self.invoke(toolchain)
        self.assertEqual(status, 1)
        self.assertIn("nandina/alpha.cpp", errors)
        self.assertIn("1 of 3", errors)
        self.assertNotIn("nandina/beta.hpp", errors)

    def test_deleted_tracked_file_is_not_sent_to_the_formatter(self):
        toolchain = FakeToolchain()
        status, errors = self.invoke(toolchain, missing=("nandina/beta.hpp",))
        self.assertEqual(status, 0)
        self.assertEqual(errors, "")
        self.assertEqual(toolchain.checked, ["nandina/alpha.cpp", "tests/gamma.cpp"])

    def test_unpinned_version_is_rejected_before_any_file_is_checked(self):
        toolchain = FakeToolchain(version="21.1.8", dirty=["nandina/alpha.cpp"])
        status, errors = self.invoke(toolchain)
        self.assertEqual(status, 1)
        self.assertIn("not the pinned 23.1.1", errors)
        self.assertIn("pip install clang-format==23.1.1", errors)
        self.assertEqual(toolchain.checked, [])

    def test_unparseable_version_is_rejected(self):
        toolchain = FakeToolchain(version_output="clang-format: command not understood\n")
        status, errors = self.invoke(toolchain)
        self.assertEqual(status, 1)
        self.assertIn("ERROR:", errors)

    def test_missing_binary_is_rejected(self):
        toolchain = FakeToolchain(tool_error=FileNotFoundError("clang-format"))
        status, errors = self.invoke(toolchain)
        self.assertEqual(status, 1)
        self.assertIn("ERROR:", errors)

    def test_git_failure_surfaces_its_stderr(self):
        failure = subprocess.CalledProcessError(
            128, "git", output="", stderr="fatal: not a git repository"
        )
        toolchain = FakeToolchain(git_error=failure)
        status, errors = self.invoke(toolchain)
        self.assertEqual(status, 1)
        self.assertIn("fatal: not a git repository", errors)

    def test_empty_source_list_is_rejected(self):
        toolchain = FakeToolchain(tracked=())
        status, errors = self.invoke(toolchain)
        self.assertEqual(status, 1)
        self.assertIn("no tracked C/C++ sources", errors)


if __name__ == "__main__":
    unittest.main()
