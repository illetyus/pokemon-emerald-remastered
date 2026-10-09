import copy
import hashlib
import json
from pathlib import Path
import re
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from r19_battle_state import SCHEMA, validate_battle
from r19_replay import validate


def zero_field(field):
    shape = field.get('shape', [])
    if shape:
        return [zero_field(dict(field, shape=shape[1:])) for _ in range(shape[0])]
    kind = field['type']
    if kind == 'party-codec': return '00' * 100
    if kind == 'active-events': return []
    if kind in SCHEMA['structs']:
        return {k: zero_field(v) for k, v in SCHEMA['structs'][kind].items()}
    return 0


def initial():
    return {'version': 1, **{k: zero_field(v) for k, v in SCHEMA['structs']['RemasterEmeraldBattleState'].items()}}


class BattleReplayTests(unittest.TestCase):
    def test_complete_battle_state_requires_every_array_and_member(self):
        validate_battle(initial())
        for mode in ('missing', 'extra', 'party', 'battler', 'rng'):
            sample = initial()
            if mode == 'missing': del sample['future_attacker']
            elif mode == 'extra': sample['padding'] = 0
            elif mode == 'party': sample['parties'][1].pop()
            elif mode == 'battler': del sample['battlers'][3]['last_consumed_item']
            else: sample['rng']['calls'] = 1.0
            with self.assertRaises(ValueError): validate_battle(sample)

    def test_active_events_are_complete_ordered_and_bounded(self):
        event = {k: zero_field(v) for k, v in SCHEMA['structs']['RemasterEmeraldBattleEvent'].items()}
        sample = initial();sample['event_count'] = 1;sample['events'] = [event]
        validate_battle(sample)
        with self.assertRaises(ValueError): validate_battle(dict(sample, event_count=2))
        changed = copy.deepcopy(sample);del changed['events'][0]['aux']
        with self.assertRaises(ValueError): validate_battle(changed)
        with self.assertRaises(ValueError): validate_battle(dict(sample, event_count=257, events=[event] * 257))

    def test_header_field_rosters_require_serializer_review(self):
        codec = ROOT / SCHEMA['codec_header']['path'];raw_codec = codec.read_bytes().replace(b'\r\n', b'\n')
        self.assertEqual(hashlib.sha1(b'blob ' + str(len(raw_codec)).encode() + b'\0' + raw_codec).hexdigest(),
                         SCHEMA['codec_header']['blob_sha'], 'Pokemon codec field change requires review')
        path = ROOT / SCHEMA['header']['path'];raw = path.read_bytes().replace(b'\r\n', b'\n')
        self.assertEqual(hashlib.sha1(b'blob ' + str(len(raw)).encode() + b'\0' + raw).hexdigest(),
                         SCHEMA['header']['blob_sha'])
        text = raw.decode()
        for name, fields in SCHEMA['structs'].items():
            body = re.search(r'typedef struct ' + name + r'\s*\{(.*?)\}\s*' + name + ';', text, re.S).group(1)
            actual = set(re.findall(r'(\w+)\s*(?:\[[^\]]+\])*\s*;', body))
            self.assertEqual(actual, set(fields), 'new battle field requires serializer/fixture review')

    def test_versioned_matrix_covers_win_replacement_loss_and_r16(self):
        matrix = json.loads((ROOT / 'tests/fixtures/r19/battle.json').read_text());validate(matrix)
        recipes = {'wild-win-v1', 'trainer-replacement-v1', 'trainer-loss-v1', 'qol-held-reward-v1'}
        self.assertEqual({c['recipe'] for c in matrix['cases']}, recipes)
        for recipe in recipes:
            self.assertEqual({c['seed'] for c in matrix['cases'] if c['recipe'] == recipe},
                             {0, 1, 0x12345678, 0xFFFFFFFF})
        self.assertTrue(any(cmd['op'] == 'qol_swap_held' for case in matrix['cases'] for cmd in case['commands']))


if __name__ == '__main__':
    unittest.main()
