"""Output isolation/transaction regressions; actual source generation runs in CI."""
import json
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import build_production_package as package


class OutputTransaction(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name) / 'repo'
        (self.root / 'external').mkdir(parents=True)
        (self.root / 'external/sources.lock.json').write_text(json.dumps({
            'vanillaplus': {'repository': package.SOURCE_REPOSITORY, 'commit': package.PIN}}))
        self.out = self.root / 'build/production/Generated'

    @staticmethod
    def fixture_generator(root, stage):
        for name in package.FOLDERS:
            (stage / name).mkdir()
            (stage / name / 'manifest.json').write_text('{}\n')

    def test_output_cannot_replace_source_or_vendor(self):
        for name in ('vendor/vanillaplus/new', 'core/generated', 'tests/new', 'unreal/Source/new', 'data/new'):
            with self.subTest(name=name), self.assertRaisesRegex(ValueError, 'protected source'):
                package.build(self.root, self.root / name, self.fixture_generator)

    def test_existing_private_output_is_preserved(self):
        self.out.mkdir(parents=True)
        sentinel = self.out / 'private.bin'
        sentinel.write_bytes(b'user-owned payload')
        with self.assertRaisesRegex(ValueError, 'already exists'):
            package.build(self.root, self.out, self.fixture_generator)
        self.assertEqual(sentinel.read_bytes(), b'user-owned payload')

    def test_owner_failure_keeps_original_error_and_publishes_nothing(self):
        def fail(root, stage):
            (stage / 'partial.bin').write_bytes(b'partial')
            raise ValueError('owner conversion failed')
        with self.assertRaisesRegex(ValueError, 'owner conversion failed'):
            package.build(self.root, self.out, fail)
        self.assertFalse(self.out.exists())
        self.assertEqual(list(self.out.parent.iterdir()), [])

    def test_missing_owner_manifest_cannot_publish(self):
        with self.assertRaisesRegex(ValueError, 'owner manifest missing'):
            package.build(self.root, self.out, lambda root, stage: None)
        self.assertFalse(self.out.exists())

    def test_atomic_layout_contains_all_four_owners(self):
        result = package.build(self.root, self.out, self.fixture_generator)
        self.assertEqual(set(p.name for p in self.out.iterdir()), set(package.FOLDERS))
        self.assertEqual(result['folders'], list(package.FOLDERS))
        self.assertIs(result['actual_unreal_build'], False)

    def test_symlink_destination_parent_is_rejected(self):
        target = self.root / 'local'
        target.mkdir()
        (self.root / 'build').symlink_to(target, target_is_directory=True)
        with self.assertRaisesRegex(ValueError, 'symlink'):
            package.build(self.root, self.out, self.fixture_generator)
        self.assertEqual(list(target.iterdir()), [])

    def test_source_pin_drift_is_rejected_before_conversion(self):
        (self.root / 'external/sources.lock.json').write_text(json.dumps({
            'vanillaplus': {'repository': package.SOURCE_REPOSITORY, 'commit': '0' * 40}}))
        with self.assertRaisesRegex(ValueError, 'source pin'):
            package.build(self.root, self.out, self.fixture_generator)
        self.assertFalse(self.out.exists())

    def test_destination_created_during_conversion_is_preserved(self):
        def concurrent(root, stage):
            self.fixture_generator(root, stage)
            self.out.mkdir()
            (self.out / 'other-session.txt').write_text('keep')
        with self.assertRaisesRegex(ValueError, 'appeared during generation'):
            package.build(self.root, self.out, concurrent)
        self.assertEqual((self.out / 'other-session.txt').read_text(), 'keep')


if __name__ == '__main__':
    unittest.main()
