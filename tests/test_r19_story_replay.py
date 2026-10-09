import json
from pathlib import Path
import re
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from r19_state import fingerprint
from r19_replay import validate


class StoryReplayTests(unittest.TestCase):
    def test_every_versioned_command_is_declared_in_contract(self):
        contract = json.loads((ROOT / 'data/r19/replay_contract.json').read_text())
        for name in ('movement', 'story', 'encounter'):
            matrix = json.loads((ROOT / ('tests/fixtures/r19/' + name + '.json')).read_text())
            used = {c['op'] for case in matrix['cases'] for c in case['commands']}
            self.assertFalse(used - set(contract['commands']), 'undeclared command in versioned fixture')

    def test_versioned_source_slices_cover_both_genders_and_world(self):
        matrix = json.loads((ROOT / 'tests/fixtures/r19/story.json').read_text())
        validate(matrix)
        self.assertEqual({case['recipe'] for case in matrix['cases']},
                         {'opening-male-v1', 'opening-female-v1', 'house-exit-v1'})
        self.assertEqual(set(matrix['inputs']), {'r2', 'r4'})
        for case in matrix['cases']:
            self.assertGreater(len(case['commands']), 1)
            self.assertEqual(len(case['expected']), len(case['commands']) + 1)

    def test_checkpoint_serializer_covers_declared_runtime_boundary(self):
        expected = {
            'RemasterEmeraldScriptVm': {'save', 'registry', 'program_index', 'program', 'program_count', 'pc',
                'stack', 'stack_depth', 'comparison_result', 'special_vars', 'special_flags', 'status', 'error'},
            'RemasterEmeraldScriptRuntime': {'vm', 'special_registry', 'next_request_sequence',
                'has_pending_request', 'pending_request'}}
        for name, fields in expected.items():
            file = 'emerald_script.h' if name.endswith('Vm') else 'emerald_script_runtime.h'
            source = (ROOT / 'core/include/remaster' / file).read_text()
            body = re.search(r'typedef struct ' + name + r'\s*\{(.*?)\}\s*' + name + ';', source, re.S).group(1)
            actual = set(re.findall(r'(\w+)\s*(?:\[[^\]]+\])?\s*;', body))
            self.assertEqual(actual, fields, 'new runtime field requires serializer/fixture review')

    def test_active_vm_pc_mutation_changes_script_domain(self):
        checkpoint = bytearray(298)
        checkpoint[:6] = b'R2SC\x01\x00'
        sample = {'version': 1, 'format': 'vanillaplus', 'domains': {
            'save_block2': '00' * 0xF44, 'save_block1': '00' * 0x3DC8, 'storage': '00' * 0x83D0,
            'script': checkpoint.hex(), 'objects': None, 'encounter': None, 'battle': None},
            'observations': {}}
        before = fingerprint(sample)
        checkpoint[14] = 1
        sample['domains']['script'] = checkpoint.hex()
        after = fingerprint(sample)
        self.assertNotEqual(before['domain_hashes']['script'], after['domain_hashes']['script'])
        self.assertEqual(before['domain_hashes']['save_block1'], after['domain_hashes']['save_block1'])


if __name__ == '__main__':
    unittest.main()
