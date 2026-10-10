import json
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import r19_unreal_smoke as smoke


class UnrealSmokeContractTests(unittest.TestCase):
    def setUp(self):
        self.config = json.loads((ROOT / 'data/r19/unreal_smoke_contract.json').read_text())
        self.log = '\n'.join([
            'LogTemp: Display: Vanilla+ save load status=1 path=private counter=10 slot=0',
            'LogTemp: Display: R5 authoritative map rendered: MAP_LITTLEROOT_TOWN (20x20, chunk=16)',
            'LogTemp: Display: Vanilla+ gameplay map ready: MAP_LITTLEROOT_TOWN (0,9) objects=2 warps=2 coords=0',
            'LogTemp: Display: Vanilla+ save stored counter=11 slot=1',
            'LogTemp: Display: Vanilla+ save load status=1 path=private counter=11 slot=1'])

    def test_source_contract_matches_actual_production_markers(self): smoke.validate(self.config)

    def test_source_drift_cannot_pass(self):
        self.config['producers'][0]['blob_sha'] = '0' * 40
        with self.assertRaisesRegex(ValueError, 'source drift'): smoke.validate(self.config)

    def test_synthetic_log_match_never_certifies_engine_execution(self):
        result = smoke.inspect_log(self.log, 'save_roundtrip')
        self.assertEqual(result['status'], 'LOG_CONTRACT_MATCH')
        self.assertFalse(result['actual_runtime_verified'])
        self.assertNotIn('private', json.dumps(result))

    def test_r0_or_missing_marker_is_not_production_smoke(self):
        for log in ['R0 Unreal core initialized hash=123', self.log.replace('R5 authoritative map rendered:', 'debug map')]:
            with self.assertRaisesRegex(ValueError, 'missing marker'): smoke.inspect_log(log, 'save_roundtrip')

    def test_failure_or_bad_save_status_is_rejected(self):
        for log in [self.log + '\nLogTemp: Error: render failure', self.log.replace('status=1', 'status=4')]:
            with self.assertRaisesRegex(ValueError, 'failure|save status'): smoke.inspect_log(log, 'save_roundtrip')

    def test_reload_before_store_or_wrong_counter_is_rejected(self):
        lines = self.log.splitlines()
        for log in ['\n'.join(lines[:3] + [lines[4], lines[3]]), self.log.replace('counter=11 slot=1', 'counter=12 slot=1', 1)]:
            with self.assertRaisesRegex(ValueError, 'reload|counter'): smoke.inspect_log(log, 'save_roundtrip')

    def test_transition_requires_both_production_markers_after_load(self):
        transitions = ['LogTemp: Display: Vanilla+ connection applied: map=MAP_ROUTE101 (0,16) dir=2 offset=0',
                       'LogTemp: Display: Vanilla+ warp applied: map=MAP_LITTLEROOT_TOWN (0,9) warp=1 dynamic=0']
        self.assertEqual(smoke.inspect_log(self.log + '\n' + '\n'.join(transitions),
                                          'map_transition')['status'], 'LOG_CONTRACT_MATCH')
        with self.assertRaisesRegex(ValueError, 'missing marker'):
            smoke.inspect_log(self.log + '\n' + transitions[0], 'map_transition')
        with self.assertRaisesRegex(ValueError, 'before save load'):
            smoke.inspect_log('\n'.join(transitions) + '\n' + self.log, 'map_transition')

    def test_packaged_marker_roster_is_the_canonical_roadmap_contract(self):
        expected = ['BOOT_OK', 'RENDER_PACKAGE_OK', 'HOUSE_RENDER_OK', 'HOUSE_WARP_OK',
                    'LITTLEROOT_RENDER_OK', 'ROUTE101_RENDER_OK', 'PASS']
        self.assertEqual(self.config.get('packaged_smoke_markers'), expected)
        self.assertEqual(self.config.get('packaged_producer_status'), 'NOT_EMITTED_REQUIRES_REAL_HARNESS')

    def test_packaged_pass_alone_truncation_reorder_or_duplicate_cannot_pass(self):
        names = ['BOOT_OK', 'RENDER_PACKAGE_OK', 'HOUSE_RENDER_OK', 'HOUSE_WARP_OK',
                 'LITTLEROOT_RENDER_OK', 'ROUTE101_RENDER_OK', 'PASS']
        for values in [['PASS'], names[:-1], [names[1], names[0]] + names[2:],
                       names + ['PASS'], names[:-1] + ['FAIL']]:
            with self.assertRaisesRegex(ValueError, 'packaged marker'):
                smoke.inspect_packaged_log('\n'.join('REM_SMOKE: ' + n for n in values))

    def test_packaged_log_match_keeps_unverified_runtime_boundary(self):
        names = ['BOOT_OK', 'RENDER_PACKAGE_OK', 'HOUSE_RENDER_OK', 'HOUSE_WARP_OK',
                 'LITTLEROOT_RENDER_OK', 'ROUTE101_RENDER_OK', 'PASS']
        log = '\n'.join('LogTemp: Display: REM_SMOKE: ' + n for n in names)
        result = smoke.inspect_packaged_log(log)
        self.assertEqual(result['status'], 'LOG_CONTRACT_MATCH')
        self.assertFalse(result['actual_runtime_verified'])
        with self.assertRaisesRegex(ValueError, 'failure'):
            smoke.inspect_packaged_log(log + '\nFatal error: synthetic negative fixture')


if __name__ == '__main__': unittest.main()
