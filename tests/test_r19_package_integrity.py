import copy
from pathlib import Path
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import r19_package_integrity as audit


class PackageIntegrityTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(); self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        (self.root / 'manifest.json').write_bytes(b'{"fixture":"original synthetic metadata"}\n')
        self.expected = audit.inventory(self.root)

    def test_original_metadata_digest_is_checked(self):
        audit.verify(self.root, self.expected)
        (self.root / 'manifest.json').write_bytes(b'{"fixture":"changed"}\n')
        with self.assertRaisesRegex(ValueError, 'manifest.json.*checksum'):
            audit.verify(self.root, self.expected)

    def test_deleted_and_unindexed_files_fail(self):
        (self.root / 'extra.json').write_text('{}')
        with self.assertRaisesRegex(ValueError, 'file coverage'): audit.verify(self.root, self.expected)
        (self.root / 'extra.json').unlink(); (self.root / 'manifest.json').unlink()
        with self.assertRaisesRegex(ValueError, 'file coverage'): audit.verify(self.root, self.expected)

    def test_empty_package_cannot_be_success(self):
        (self.root / 'manifest.json').unlink()
        with self.assertRaisesRegex(ValueError, 'empty'): audit.inventory(self.root)

    def test_escape_and_symlink_fail(self):
        bad = {'../outside': '0' * 64}
        with self.assertRaisesRegex(ValueError, 'path'): audit.verify(self.root, bad)
        (self.root / 'linked.json').symlink_to(self.root / 'manifest.json')
        with self.assertRaisesRegex(ValueError, 'symlink'): audit.inventory(self.root)

    def test_corruption_is_detected_and_original_is_restored(self):
        original = (self.root / 'manifest.json').read_bytes()
        self.assertEqual(audit.corruption_checks(self.root, self.expected), 1)
        self.assertEqual((self.root / 'manifest.json').read_bytes(), original)
        audit.verify(self.root, self.expected)

    def test_repeat_generation_must_match(self):
        other = copy.deepcopy(self.expected); other['manifest.json'] = '0' * 64
        with self.assertRaisesRegex(ValueError, 'generation divergence'):
            audit.compare_generation('R6', self.expected, other)


if __name__ == '__main__': unittest.main()
