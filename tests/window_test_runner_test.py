import contextlib
import importlib.util
import io
import os
import pathlib
import sys
import unittest
from unittest import mock


spec = importlib.util.spec_from_file_location("window_runner", pathlib.Path(sys.argv.pop(1)))
runner = importlib.util.module_from_spec(spec)
spec.loader.exec_module(runner)


class WindowRunnerTests(unittest.TestCase):
    def test_exit_codes_preserve_success_failures_and_skips(self):
        with mock.patch.dict(os.environ, {"NANDINA_SKIP_WINDOW_TESTS": "0"}):
            for child_code, expected in ((0, 0), (1, 1), (2, 2), (4, 77), (5, 5), (42, 42)):
                with self.subTest(child_code=child_code):
                    self.assertEqual(
                        runner.main([sys.executable, "-c", f"raise SystemExit({child_code})"]),
                        expected,
                    )

    def test_explicit_skip_does_not_launch_the_window_process(self):
        for value in ("1", "TRUE", "yes", " on "):
            with (
                self.subTest(value=value),
                mock.patch.dict(os.environ, {"NANDINA_SKIP_WINDOW_TESTS": value}),
                mock.patch.object(runner.subprocess, "run") as launch,
                contextlib.redirect_stdout(io.StringIO()) as output,
            ):
                self.assertEqual(runner.main(["unused"]), 77)
                launch.assert_not_called()
                self.assertIn("SKIP", output.getvalue())

    def test_signal_exit_is_a_failure(self):
        with (
            mock.patch.dict(os.environ, {"NANDINA_SKIP_WINDOW_TESTS": "0"}),
            mock.patch.object(runner.subprocess, "run", return_value=mock.Mock(returncode=-11)),
        ):
            self.assertEqual(runner.main(["unused"]), 139)

    def test_missing_command_is_not_a_skip(self):
        with contextlib.redirect_stderr(io.StringIO()):
            self.assertEqual(runner.main([]), 2)


if __name__ == "__main__":
    unittest.main()
