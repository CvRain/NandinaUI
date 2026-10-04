#!/usr/bin/env python3
"""Exercise the CI guard without configuring or building the C++ project."""

import contextlib
import importlib.util
import io
import json
import pathlib
import subprocess
import sys
import unittest
from unittest import mock


helper_path = pathlib.Path(sys.argv.pop(1))
spec = importlib.util.spec_from_file_location("check_test_suites", helper_path)
helper = importlib.util.module_from_spec(spec)
spec.loader.exec_module(helper)


class SuiteGuardTests(unittest.TestCase):
    def invoke(self, output):
        result = subprocess.CompletedProcess([], 0, stdout=output, stderr="")
        with mock.patch.object(helper.subprocess, "run", return_value=result) as run:
            errors = io.StringIO()
            with contextlib.redirect_stderr(errors):
                status = helper.main(["buildDir"])
            run.assert_called_once_with(
                ["meson", "introspect", "--tests", "buildDir"],
                check=True,
                capture_output=True,
                text=True,
            )
        return status, errors.getvalue()

    def test_unit_and_integration_pass(self):
        status, errors = self.invoke(json.dumps([
            {"name": "foundation", "suite": ["NandinaUI:unit"]},
            {"name": "workflow", "suite": ["NandinaUI:integration"]},
            {"name": "multi-suite", "suite": ["NandinaUI:slow", "NandinaUI:unit"]},
        ]))
        self.assertEqual(status, 0)
        self.assertEqual(errors, "")

    def test_missing_empty_and_uncovered_suites_fail(self):
        for suite in (None, [], ["NandinaUI"], [""], ["NandinaUI:slow"]):
            with self.subTest(suite=suite):
                test = {"name": "omitted"}
                if suite is not None:
                    test["suite"] = suite
                status, errors = self.invoke(json.dumps([test]))
                self.assertEqual(status, 1)
                self.assertIn("ERROR:", errors)

    def test_invalid_introspection_fails(self):
        for output in ("", "not JSON", "[]", "{}", '[{"suite": "NandinaUI:unit"}]'):
            with self.subTest(output=output):
                status, errors = self.invoke(output)
                self.assertEqual(status, 1)
                self.assertIn("ERROR:", errors)

    def test_introspection_error_cannot_pass_with_valid_stdout(self):
        error = subprocess.CalledProcessError(
            2, "meson", output='[{"suite": ["NandinaUI:unit"]}]', stderr="broken build"
        )
        with mock.patch.object(helper.subprocess, "run", side_effect=error):
            errors = io.StringIO()
            with contextlib.redirect_stderr(errors):
                self.assertEqual(helper.main(["buildDir"]), 1)
            self.assertIn("broken build", errors.getvalue())

    def test_missing_meson_fails(self):
        with mock.patch.object(helper.subprocess, "run", side_effect=FileNotFoundError("meson")):
            with contextlib.redirect_stderr(io.StringIO()):
                self.assertEqual(helper.main(["buildDir"]), 1)


if __name__ == "__main__":
    unittest.main()
