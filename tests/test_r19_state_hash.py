import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('r19_state', ROOT / 'tools/r19_state.py')
state = importlib.util.module_from_spec(spec)
spec.loader.exec_module(state)


def sample():
    return {'version': 1, 'format': 'vanillaplus', 'domains': {
        'save_block2': '00' * 0xF44, 'save_block1': '00' * 0x3DC8,
        'storage': '00' * 0x83D0, 'script': None, 'objects': None,
        'encounter': None, 'battle': None},
        'observations': {'x': 1, 'y': 1, 'kind': 0, 'collision': 0}}


class CanonicalStateTests(unittest.TestCase):
    def test_canonical_encoding_known_bytes(self):
        self.assertEqual(state.canonical({'z': 2, 'a': ['é', None]}),
                         b'{"a":["\xc3\xa9",null],"z":2}\n')

    def test_order_and_new_allocation_do_not_change_hash(self):
        a = sample()
        b = copy.deepcopy(a)
        b['domains'] = dict(reversed(list(b['domains'].items())))
        self.assertEqual(state.fingerprint(a), state.fingerprint(b))

    def test_persistent_byte_changes_correct_domain(self):
        a = sample()
        b = copy.deepcopy(a)
        b['domains']['save_block1'] = '01' + b['domains']['save_block1'][2:]
        ha, hb = state.fingerprint(a), state.fingerprint(b)
        self.assertNotEqual(ha['state_hash'], hb['state_hash'])
        self.assertNotEqual(ha['domain_hashes']['save_block1'], hb['domain_hashes']['save_block1'])
        for domain in set(ha['domain_hashes']) - {'save_block1'}:
            self.assertEqual(ha['domain_hashes'][domain], hb['domain_hashes'][domain])

    def test_padding_transport_fields_cannot_enter_snapshot(self):
        a = sample()
        a['counter'] = 42
        with self.assertRaisesRegex(ValueError, 'snapshot schema'):
            state.fingerprint(a)

    def test_truncated_persistent_domain_rejected(self):
        a = sample()
        a['domains']['storage'] = a['domains']['storage'][:-2]
        with self.assertRaisesRegex(ValueError, 'storage'):
            state.fingerprint(a)

    def test_unaudited_stock_runtime_cannot_be_certified(self):
        a = sample()
        a['format'] = 'stock'
        with self.assertRaisesRegex(ValueError, 'format'):
            state.fingerprint(a)

    def test_missing_runtime_domain_not_silently_ignored(self):
        a = sample()
        del a['domains']['battle']
        with self.assertRaisesRegex(ValueError, 'domains'):
            state.fingerprint(a)

    def test_unordered_float_clock_is_rejected(self):
        with self.assertRaisesRegex(ValueError, 'float'):
            state.canonical({'clock': 0.1})

    def test_sha256_binds_trailing_lf(self):
        self.assertEqual(state.digest({'value': 1}),
                         hashlib.sha256(b'{"value":1}\n').hexdigest())


if __name__ == '__main__':
    unittest.main()
