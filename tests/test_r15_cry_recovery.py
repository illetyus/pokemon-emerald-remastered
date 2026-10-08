import copy
import hashlib
import sys
import tempfile
import unittest
import warnings
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from validate_r15_cry_recovery import check_archive, check_normal_history


class RecoveryIntegrity(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.pack = self.root / 'pack'
        self.pack.mkdir()
        (self.pack / 'fixture.txt').write_bytes(b'original synthetic fixture')
        self.archive = self.root / 'fixture.zip'

    def write_archive(self, entries):
        with warnings.catch_warnings():
            warnings.simplefilter('ignore', UserWarning)
            with zipfile.ZipFile(self.archive, 'w') as archive:
                for name, content in entries:
                    archive.writestr(name, content)

    def test_archive_matches_actual_pack_bytes(self):
        self.write_archive([('fixture.txt', b'original synthetic fixture')])
        report = check_archive(self.archive, self.pack)
        self.assertEqual(report['members'], 1)
        self.assertEqual(report['sha256'], hashlib.sha256(self.archive.read_bytes()).hexdigest())

    def test_duplicate_members_rejected(self):
        self.write_archive([('fixture.txt', b'a'), ('fixture.txt', b'b')])
        with self.assertRaisesRegex(ValueError, 'duplicate'):
            check_archive(self.archive, self.pack)

    def test_path_escape_and_symlink_members_rejected(self):
        self.write_archive([('../fixture.txt', b'original synthetic fixture')])
        with self.assertRaises(ValueError):
            check_archive(self.archive, self.pack)
        info = zipfile.ZipInfo('fixture.txt')
        info.create_system = 3
        info.external_attr = 0o120777 << 16
        self.write_archive([(info, b'outside')])
        with self.assertRaises(ValueError):
            check_archive(self.archive, self.pack)

    def test_changed_content_rejected_even_with_valid_zip_crc(self):
        self.write_archive([('fixture.txt', b'changed fixture')])
        with self.assertRaisesRegex(ValueError, 'content'):
            check_archive(self.archive, self.pack)

    def test_missing_and_unexpected_members_rejected(self):
        self.write_archive([])
        with self.assertRaises(ValueError):
            check_archive(self.archive, self.pack)
        self.write_archive([('fixture.txt', b'original synthetic fixture'), ('extra.txt', b'extra')])
        with self.assertRaises(ValueError):
            check_archive(self.archive, self.pack)

    def normal_fixture(self):
        (self.pack / 'masters').mkdir()
        source = b'original synthetic source bytes'
        (self.pack / 'masters/001.ogg').write_bytes(source)
        expected = {'national_dex': 1, 'pcm_sha256': 'a' * 64, 'frames': 4,
                    'source_size': len(source), 'source_sha256': hashlib.sha256(source).hexdigest(),
                    'source_blob_sha': hashlib.sha1(b'blob ' + str(len(source)).encode() + b'\0' + source).hexdigest()}
        entries = {'cry.1.mode.0': {'sha256': 'a' * 64, 'frames': 4}}
        return entries, [expected]

    def test_historical_normal_and_master_identity(self):
        entries, expected = self.normal_fixture()
        self.assertEqual(check_normal_history(entries, self.pack, expected), 1)

    def test_changed_normal_and_missing_identity_rejected(self):
        entries, expected = self.normal_fixture()
        changed = copy.deepcopy(entries)
        changed['cry.1.mode.0']['sha256'] = 'b' * 64
        with self.assertRaises(ValueError):
            check_normal_history(changed, self.pack, expected)
        with self.assertRaises(ValueError):
            check_normal_history({}, self.pack, expected)

    def test_changed_master_rejected(self):
        entries, expected = self.normal_fixture()
        (self.pack / 'masters/001.ogg').write_bytes(b'changed source bytes')
        with self.assertRaises(ValueError):
            check_normal_history(entries, self.pack, expected)


if __name__ == '__main__':
    unittest.main()
