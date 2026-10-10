import copy
import json
from pathlib import Path
import sys
import unittest
from unittest.mock import patch
import subprocess

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import r19_save_matrix as matrix


class SaveMatrixTests(unittest.TestCase):
    def setUp(self):
        self.manifest = json.loads((ROOT / 'data/r19/save_matrix.json').read_text())
        self.names = self.manifest['ctests']

    def test_manifest_cannot_drop_original_cases(self):
        matrix.validate(self.manifest)
        self.manifest['case_ids'].pop()
        with self.assertRaisesRegex(ValueError, 'case coverage'):
            matrix.validate(self.manifest)

    def test_missing_native_target_cannot_be_empty_success(self):
        tests = [{'name': name, 'command': ['compiled-test']} for name in self.names]
        matrix.validate_discovery({'tests': tests}, self.names)
        with self.assertRaisesRegex(ValueError, 'discovery'):
            matrix.validate_discovery({'tests': tests[:-1]}, self.names)
        tests[0]['command'] = []
        with self.assertRaisesRegex(ValueError, 'executable'):
            matrix.validate_discovery({'tests': tests}, self.names)

    def test_failure_skip_and_duplicate_cannot_be_receipt_success(self):
        def report(names, child=''):
            return '<testsuite>' + ''.join('<testcase name="' + n + '" status="run">' + child + '</testcase>'
                                           for n in names) + '</testsuite>'
        self.assertEqual(matrix.validate_junit(report(self.names), self.names), self.names)
        for child in ('<failure/>', '<error/>', '<skipped/>'):
            with self.assertRaisesRegex(ValueError, 'nonpassing'):
                matrix.validate_junit(report(self.names, child), self.names)
        with self.assertRaisesRegex(ValueError, 'exact'):
            matrix.validate_junit(report(self.names[:-1]), self.names)
        with self.assertRaisesRegex(ValueError, 'exact'):
            matrix.validate_junit(report(self.names + [self.names[0]]), self.names)

    def test_existing_matrix_blob_drift_cannot_pass(self):
        changed = copy.deepcopy(self.manifest)
        changed['fixture']['blob_sha'] = '0' * 40
        with self.assertRaisesRegex(ValueError, 'fixture drift'):
            matrix.validate(changed)

    def test_private_fixture_cannot_be_claimed_as_ci_execution(self):
        self.manifest['private_policy'] = 'EXECUTED_IN_CI'
        with self.assertRaisesRegex(ValueError, 'private'):
            matrix.validate(self.manifest)

    def test_ctest_process_failure_cannot_be_success(self):
        result = subprocess.CompletedProcess(['ctest'], 8, 'native failure', '')
        with patch.object(matrix.subprocess, 'run', return_value=result):
            with self.assertRaisesRegex(ValueError, 'execution failed: native failure'):
                matrix.command(['ctest'])


if __name__ == '__main__':
    unittest.main()
