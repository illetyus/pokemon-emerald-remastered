import copy
import json
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import r19_asset_audits as audits


class AssetAuditTests(unittest.TestCase):
    def setUp(self):
        self.config = json.loads((ROOT / 'data/r19/asset_audits.json').read_text())

    def test_every_owned_phase_and_module_is_required(self):
        audits.validate(self.config)
        self.config['components'].pop()
        with self.assertRaisesRegex(ValueError, 'phase coverage'):
            audits.validate(self.config)

    def test_deleted_or_changed_audit_cannot_be_empty_success(self):
        changed = copy.deepcopy(self.config)
        changed['components'][0]['modules'][0]['blob_sha'] = '0' * 40
        with self.assertRaisesRegex(ValueError, 'source drift'): audits.validate(changed)
        changed = copy.deepcopy(self.config);changed['components'][0]['modules'] = []
        with self.assertRaisesRegex(ValueError, 'module coverage'): audits.validate(changed)

    def test_zero_executed_tests_cannot_pass(self):
        result = unittest.TestResult()
        with self.assertRaisesRegex(ValueError, 'executed test count'):
            audits.validate_result('R6', result, 1)

    def test_skip_cannot_be_manifest_audit_success(self):
        result = unittest.TestResult();result.testsRun = 1
        result.skipped.append((unittest.FunctionTestCase(lambda: None), 'missing dependency'))
        with self.assertRaisesRegex(ValueError, 'skipped'):
            audits.validate_result('R15', result, 1)

    def test_failure_names_actual_component_and_case(self):
        result = unittest.TestResult();result.testsRun = 1
        case = unittest.FunctionTestCase(lambda: None, description='manifest-corruption')
        result.failures.append((case, 'changed source'))
        with self.assertRaisesRegex(ValueError, 'R14.*first failure'):
            audits.validate_result('R14', result, 1)

    def test_passing_result_has_actual_run_count(self):
        result = unittest.TestResult();result.testsRun = 3
        self.assertEqual(audits.validate_result('R7', result, 3),
                         {'phase': 'R7', 'status': 'PASS', 'tests': 3, 'skips': 0})


if __name__ == '__main__':
    unittest.main()
