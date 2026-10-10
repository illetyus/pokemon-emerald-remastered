"""Trusted index and complete byte coverage, using explicit tiny fixtures."""
import hashlib
import json
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import r20_package_integrity as integrity


class TrustedIndex(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.output = Path(self.tmp.name) / 'Generated'
        self.output.mkdir()
        self.inputs = {'tools/fixture_owner.py': '1' * 64}
        for name in integrity.FOLDERS:
            (self.output / name).mkdir()
            (self.output / name / 'manifest.json').write_bytes(b'{}\n')
        self.index = integrity.make_index(integrity.inventory(self.output), self.inputs)
        self.trusted = self.write_index(self.index)

    def write_index(self, index):
        raw = integrity.encode(index)
        (self.output / integrity.INDEX).write_bytes(raw)
        return hashlib.sha256(raw).hexdigest()

    def verify(self, expected=None, inputs=None):
        return integrity.verify_package(self.output, expected or self.trusted,
                                        inputs=self.inputs if inputs is None else inputs)

    def test_complete_package_matches_external_trusted_index(self):
        result = self.verify()
        self.assertEqual(result['file_count'], 4)
        self.assertIs(result['actual_unreal_build'], False)

    def test_changed_payload_is_rejected(self):
        (self.output / 'World/manifest.json').write_bytes(b'changed')
        with self.assertRaisesRegex(ValueError, 'file coverage/checksum'):
            self.verify()

    def test_changed_payload_and_rewritten_index_cannot_self_bless(self):
        (self.output / 'World/manifest.json').write_bytes(b'changed')
        self.write_index(integrity.make_index(integrity.inventory(self.output), self.inputs))
        with self.assertRaisesRegex(ValueError, 'trusted index'):
            self.verify()

    def test_missing_file_is_rejected(self):
        (self.output / 'Characters/manifest.json').unlink()
        with self.assertRaisesRegex(ValueError, 'file coverage/checksum'):
            self.verify()

    def test_extra_file_is_rejected(self):
        (self.output / 'Environment/extra.bin').write_bytes(b'extra')
        with self.assertRaisesRegex(ValueError, 'file coverage/checksum'):
            self.verify()

    def test_source_or_generator_input_drift_is_rejected(self):
        with self.assertRaisesRegex(ValueError, 'input provenance'):
            self.verify(inputs={'tools/fixture_owner.py': '2' * 64})

    def test_symlink_payload_is_rejected(self):
        (self.output / 'Render/link.bin').symlink_to(self.output / 'World/manifest.json')
        with self.assertRaisesRegex(ValueError, 'symlink'):
            self.verify()

    def test_unsafe_index_paths_and_missing_owners_are_rejected(self):
        for files in ({'../outside': '0' * 64}, {'World/manifest.json': '0' * 64}):
            with self.subTest(files=files), self.assertRaises(ValueError):
                integrity.make_index(files, self.inputs)

    def test_expected_hash_is_mandatory_and_well_formed(self):
        for expected in (None, '', 'bad', 'A' * 64):
            with self.subTest(expected=expected), self.assertRaisesRegex(ValueError, 'trusted index'):
                integrity.verify_package(self.output, expected, inputs=self.inputs)

    def test_noncanonical_and_unsupported_index_are_rejected(self):
        for version in (999, True):
            self.index['version'] = version
            new_hash = self.write_index(self.index)
            with self.subTest(version=version), self.assertRaisesRegex(ValueError, 'schema/provenance'):
                self.verify(expected=new_hash)
        raw = json.dumps(self.index, indent=3).encode()
        (self.output / integrity.INDEX).write_bytes(raw)
        with self.assertRaisesRegex(ValueError, 'canonical'):
            self.verify(expected=hashlib.sha256(raw).hexdigest())


if __name__ == '__main__':
    unittest.main()
