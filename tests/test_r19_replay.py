import copy
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
spec = importlib.util.spec_from_file_location('r19_replay', ROOT / 'tools/r19_replay.py')
replay = importlib.util.module_from_spec(spec)
spec.loader.exec_module(replay)


class ReplayContractTests(unittest.TestCase):
    def setUp(self):
        self.matrix = json.loads((ROOT / 'tests/fixtures/r19/movement.json').read_text())

    def test_unknown_command_rejected_before_execution(self):
        self.matrix['cases'][0]['commands'][0] = {'op': 'set_story_flag', 'value': 1}
        with self.assertRaisesRegex(ValueError, 'command'):
            replay.validate(self.matrix)

    def test_source_pin_is_required(self):
        self.matrix['source']['commit'] = '0' * 40
        with self.assertRaisesRegex(ValueError, 'source'):
            replay.validate(self.matrix)

    def test_empty_matrix_cannot_pass(self):
        self.matrix['cases'] = []
        with self.assertRaisesRegex(ValueError, 'cases'):
            replay.validate(self.matrix)

    def test_recipe_source_drift_cannot_pass(self):
        self.matrix['probe_source']['blob_sha'] = '0' * 40
        with self.assertRaisesRegex(ValueError, 'recipe drift'):
            replay.validate(self.matrix)

    def test_duplicate_case_cannot_pass(self):
        self.matrix['cases'].append(copy.deepcopy(self.matrix['cases'][0]))
        with self.assertRaisesRegex(ValueError, 'duplicate'):
            replay.validate(self.matrix)

    def test_invalid_direction_cannot_pass(self):
        self.matrix['cases'][0]['commands'][0]['direction'] = 'diagonal'
        with self.assertRaisesRegex(ValueError, 'direction'):
            replay.validate(self.matrix)

    def test_truncated_execution_cannot_pass(self):
        case = self.matrix['cases'][0]
        with self.assertRaisesRegex(ValueError, 'snapshot count'):
            replay.verify(case, case['expected'][:-1])

    def test_first_wrong_observation_names_step(self):
        case = copy.deepcopy(self.matrix['cases'][0])
        actual = [{'version': 1, 'format': 'vanillaplus', 'domains': {
            'save_block2': '00' * 0xF44, 'save_block1': '00' * 0x3DC8,
            'storage': '00' * 0x83D0, 'script': None, 'objects': None,
            'encounter': None, 'battle': None}, 'observations': e['observations']}
            for e in copy.deepcopy(case['expected'])]
        case['expected'] = [copy.deepcopy(replay.fingerprint(a)) for a in actual]
        actual[2]['observations']['x'] += 1
        actual[3]['observations']['y'] += 1
        with self.assertRaisesRegex(ValueError, 'movement-loop.*snapshot 2.*x'):
            replay.verify(case, actual)

    def test_mutated_expected_hash_is_not_blessed(self):
        case = copy.deepcopy(self.matrix['cases'][0])
        actual = {'version': 1, 'format': 'vanillaplus', 'domains': {
            'save_block2': '00' * 0xF44, 'save_block1': '00' * 0x3DC8,
            'storage': '00' * 0x83D0, 'script': None, 'objects': None,
            'encounter': None, 'battle': None}, 'observations': case['expected'][0]['observations']}
        case['commands'] = []
        case['expected'] = [copy.deepcopy(replay.fingerprint(actual))]
        case['expected'][0]['domain_hashes']['save_block1'] = '0' * 64
        with self.assertRaisesRegex(ValueError, 'snapshot 0.*domain save_block1'):
            replay.verify(case, [actual])

    def test_probe_failure_is_not_empty_success(self):
        case = self.matrix['cases'][0]
        with patch.object(replay.subprocess, 'run', return_value=subprocess.CompletedProcess([], 1, '', 'rejected command')):
            with self.assertRaisesRegex(ValueError, 'probe failed.*rejected command'):
                replay.execute(Path('probe'), case)


if __name__ == '__main__':
    unittest.main()
