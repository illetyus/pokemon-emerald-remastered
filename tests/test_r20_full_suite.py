"""Evidence gates reject incomplete, failed and falsely certified receipts."""
import copy
from pathlib import Path
from types import SimpleNamespace
import sys
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from r20_full_suite import validate_r19, validate_targeted, compare_packages, validate_verification, MODULES


class FullSuiteGates(unittest.TestCase):
    def r19(self):
        names = ['portable_ctest', 'python_regressions', 'asset_audits', 'package_integrity', 'unreal_smoke_source', 'deferred_device_metadata', 'unreal_source_guards']
        return {'schema': 'remaster.r19.full-suite-receipt', 'version': 1, 'status': 'PASS',
                'source_commit': '70db90c9077aed1272e746fc2537d9f12b95a91c',
                'actual_unreal_or_device_verified': False,
                'components': [{'id': n, 'status': 'PASS', 'evidence': {'tests': c, 'skips': 0}}
                               for n, c in zip(names, [81, 443, 171, 0, 0, 0, 0])]}

    def package(self):
        return {'status': 'SOURCE_PACKAGE_READY', 'actual_unreal_build': False,
                'file_count': 4, 'input_count': 1, 'index_sha256': 'a' * 64, 'package_sha256': 'b' * 64}

    def test_complete_r19_component_evidence_is_required(self):
        r = self.r19()
        validate_r19(r)
        r['components'].pop()
        with self.assertRaises(ValueError): validate_r19(r)

    def test_failed_duplicate_or_runtime_promoted_r19_fails(self):
        for change in ['failed', 'duplicate', 'runtime']:
            r = self.r19()
            if change == 'failed': r['components'][0]['status'] = 'FAIL'
            if change == 'duplicate': r['components'][1] = copy.deepcopy(r['components'][0])
            if change == 'runtime': r['actual_unreal_or_device_verified'] = True
            with self.subTest(change=change), self.assertRaises(ValueError): validate_r19(r)

    def test_targeted_suite_requires_every_module_and_zero_skips(self):
        result = SimpleNamespace(errors=[], failures=[], skipped=[], testsRun=51)
        validate_targeted(result, set(MODULES))
        with self.assertRaises(ValueError): validate_targeted(result, set(MODULES[:-1]))
        result.skipped = [('test', 'skip')]
        with self.assertRaises(ValueError): validate_targeted(result, set(MODULES))

    def test_targeted_failure_or_empty_execution_fails(self):
        for count, errors in [(0, []), (51, [('test', 'failed')])]:
            with self.assertRaises(ValueError):
                validate_targeted(SimpleNamespace(errors=errors, failures=[], skipped=[], testsRun=count), set(MODULES))

    def test_equal_complete_packages_have_actual_counts(self):
        self.assertEqual(compare_packages(self.package(), self.package())['file_count'], 4)

    def test_different_index_payload_or_coverage_fails(self):
        for key, value in [('index_sha256', 'c' * 64), ('package_sha256', 'd' * 64), ('file_count', 5), ('input_count', 2)]:
            a, b = self.package(), self.package(); b[key] = value
            with self.subTest(key=key), self.assertRaises(ValueError): compare_packages(a, b)

    def test_failed_or_runtime_certified_package_cannot_pass(self):
        for key, value in [('status', 'FAIL'), ('actual_unreal_build', True), ('file_count', 0), ('index_sha256', 'invalid')]:
            a, b = self.package(), self.package(); b[key] = value
            with self.subTest(key=key), self.assertRaises(ValueError): compare_packages(a, b)

    def test_verification_must_bind_the_external_generation_receipt(self):
        a = self.package(); v = dict(a, status='SOURCE_PACKAGE_VERIFIED')
        validate_verification(v, a)
        v['index_sha256'] = 'c' * 64
        with self.assertRaises(ValueError): validate_verification(v, a)


if __name__ == '__main__': unittest.main()
