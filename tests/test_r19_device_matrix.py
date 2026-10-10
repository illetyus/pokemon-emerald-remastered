import copy
import json
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import r19_device_matrix as matrix


class DeferredDeviceMatrixTests(unittest.TestCase):
    def setUp(self): self.config = json.loads((ROOT / 'data/r19/android_device_matrix.json').read_text())

    def test_source_configuration_and_coverage_match(self): matrix.validate(self.config)

    def test_no_execution_or_success_can_be_forged(self):
        for key, value in [('automatic_execution', True), ('status', 'PASS')]:
            bad = copy.deepcopy(self.config); bad[key] = value
            with self.assertRaisesRegex(ValueError, 'deferred'): matrix.validate(bad)
        bad = copy.deepcopy(self.config); bad['targets'][0]['status'] = 'PASS'
        with self.assertRaisesRegex(ValueError, 'NOT_RUN'): matrix.validate(bad)

    def test_api_or_abi_drift_cannot_pass(self):
        for key, value in [('minimum_api', 25), ('target_api', 34), ('abi', 'x86_64')]:
            bad = copy.deepcopy(self.config); bad[key] = value
            with self.assertRaisesRegex(ValueError, 'source configuration'): matrix.validate(bad)

    def test_missing_gpu_tier_or_backend_cannot_pass(self):
        for field, value in [('gpu_family', 'Adreno'), ('tier', 'high'), ('backend', 'vulkan')]:
            bad = copy.deepcopy(self.config)
            for target in bad['targets']: target[field] = value
            with self.assertRaisesRegex(ValueError, 'coverage'): matrix.validate(bad)

    def test_missing_scenario_or_measurement_is_incomplete(self):
        for field in ['scenarios', 'measurements']:
            bad = copy.deepcopy(self.config); bad[field].pop()
            with self.assertRaisesRegex(ValueError, 'coverage'): matrix.validate(bad)

    def test_no_invented_device_provider_or_results(self):
        for field in ['device_model', 'provider', 'result_artifact']:
            bad = copy.deepcopy(self.config); bad['targets'][0][field] = 'invented'
            with self.assertRaisesRegex(ValueError, 'unbound'): matrix.validate(bad)


if __name__ == '__main__': unittest.main()
