import copy
import json
from pathlib import Path
import re
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from r19_state import fingerprint
from r19_replay import validate


def initial():
    runtime = {k: 0 for k in ('species_bag_count', 'species_bag_cursor', 'species_bag_habitat_mask',
        'last_species', 'species_bag_map_group', 'species_bag_map_num', 'species_bag_area',
        'species_bag_rod', 'species_bag_valid', 'roamer_map_group', 'roamer_map_num',
        'roamer_location_valid', 'wild_immunity_steps', 'previous_behavior', 'previous_behavior_valid')}
    runtime.update(version=1, rng={'state': 0, 'calls': 0}, species_bag=[0] * 64,
                   roamer_location_history=[[0, 0] for _ in range(3)])
    return {'version': 1, 'format': 'vanillaplus', 'domains': {
        'save_block2': '00' * 0xF44, 'save_block1': '00' * 0x3DC8, 'storage': '00' * 0x83D0,
        'script': None, 'objects': None, 'encounter': runtime, 'battle': None}, 'observations': {}}


class EncounterReplayTests(unittest.TestCase):
    def test_complete_runtime_changes_rng_bag_and_roamer_hash(self):
        sample = initial()
        before = fingerprint(sample)
        for domain in ('rng', 'species_bag', 'roamer_location_history'):
            changed = copy.deepcopy(sample)
            runtime = changed['domains']['encounter']
            if domain == 'rng': runtime['rng']['calls'] = 1
            elif domain == 'species_bag': runtime['species_bag'][63] = 25
            else: runtime['roamer_location_history'][2][1] = 16
            after = fingerprint(changed)
            self.assertNotEqual(before['domain_hashes']['encounter'], after['domain_hashes']['encounter'])
            self.assertEqual(before['domain_hashes']['save_block1'], after['domain_hashes']['save_block1'])

    def test_missing_or_unserialized_runtime_field_cannot_pass(self):
        for mode in ('missing', 'extra', 'truncated', 'history', 'rng'):
            sample = initial();runtime = sample['domains']['encounter']
            if mode == 'missing': del runtime['last_species']
            elif mode == 'extra': runtime['padding'] = 0
            elif mode == 'truncated': runtime['species_bag'].pop()
            elif mode == 'history': runtime['roamer_location_history'][2].pop()
            else: runtime['rng']['calls'] = 1.0
            with self.assertRaises(ValueError): fingerprint(sample)

    def test_declared_runtime_and_result_fields_require_serializer_review(self):
        expected = {
            'RemasterEmeraldEncounterRng': {'state', 'calls'},
            'RemasterEmeraldEncounterRuntime': set(initial()['domains']['encounter']) - {'version'},
            'RemasterEmeraldEncounterResult': {'occurred', 'repel_wore_off', 'kind', 'area', 'rod',
                'level', 'nature', 'gender', 'ability_num', 'species', 'modified_rate',
                'rng_calls_before', 'rng_calls_after', 'pokemon'}}
        source = (ROOT / 'core/include/remaster/emerald_encounter.h').read_text()
        for name, fields in expected.items():
            body = re.search(r'typedef struct ' + name + r'\s*\{(.*?)\}\s*' + name + ';', source, re.S).group(1)
            actual = set(re.findall(r'(\w+)\s*(?:\[[^\]]+\])*\s*;', body))
            self.assertEqual(actual, fields, 'new field requires serializer/fixture review')

    def test_versioned_matrix_covers_every_scenario_and_seed(self):
        matrix = json.loads((ROOT / 'tests/fixtures/r19/encounter.json').read_text())
        validate(matrix)
        recipes = {'land-route101-v1', 'repel-expiry-v1', 'disabled-route101-v1', 'water-route102-v1',
                   'old-rod-route102-v1', 'good-rod-route102-v1', 'super-rod-route102-v1', 'rocks-route111-v1'}
        self.assertEqual({c['recipe'] for c in matrix['cases']}, recipes)
        for recipe in recipes:
            self.assertEqual({c['seed'] for c in matrix['cases'] if c['recipe'] == recipe},
                             {0, 1, 0x12345678, 0xFFFFFFFF})

    def test_seed_is_explicit_unsigned_integer(self):
        matrix = json.loads((ROOT / 'tests/fixtures/r19/encounter.json').read_text())
        for value in (True, -1, 0x100000000):
            changed = copy.deepcopy(matrix);changed['cases'][0]['seed'] = value
            with self.assertRaisesRegex(ValueError, 'seed'): validate(changed)


if __name__ == '__main__':
    unittest.main()
