"""Explicit temporary Git repositories; no real credential fixtures."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from audit_public_repository import scan_repository, content_rules, path_rules, ignored_outputs


class SecurityTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.git('init', '-q')
        self.git('config', 'user.name', 'Explicit Test')
        self.git('config', 'user.email', 'fixture@example.invalid')
        (self.root / '.gitignore').write_text('/build/\n/local/\n*.apk\n', encoding='utf-8')
        self.commit('README.md', b'public source fixture\n')

    def git(self, *args):
        return subprocess.check_output(['git', '-C', str(self.root), *args], stderr=subprocess.DEVNULL)

    def commit(self, path, data):
        dest = self.root / path
        dest.parent.mkdir(parents=True, exist_ok=True)
        dest.write_bytes(data)
        self.git('add', '-A')
        self.git('commit', '-qm', 'fixture')

    def scan(self, **kwargs):
        return scan_repository(self.root, require_vendor=False, **kwargs)

    def test_clean_current_and_history_have_actual_counts(self):
        r = self.scan()
        self.assertEqual(r['status'], 'PUBLIC_SOURCE_AUDIT_PASS')
        self.assertEqual(r['tracked_files'], 2)
        self.assertEqual(r['skipped_blobs'], 0)
        self.assertGreater(r['scanned_bytes'], 0)
        self.assertFalse(r['actual_unreal_build'])

    def test_deleted_secret_still_fails_reachable_history(self):
        token = ('gh' + 'p_' + 'A' * 36).encode()
        self.commit('old.txt', token)
        (self.root / 'old.txt').unlink()
        self.git('add', '-A'); self.git('commit', '-qm', 'delete')
        self.assertNotEqual(self.scan()['status'], 'PUBLIC_SOURCE_AUDIT_PASS')
        self.assertTrue(self.scan(history=False)['status'].endswith('PASS'))

    def test_dirty_working_source_cannot_claim_clean_scan(self):
        (self.root / 'README.md').write_text('changed source')
        self.assertIn('dirty-tracked-source', json.dumps(self.scan()))

    def test_secret_value_never_appears_in_report(self):
        token = ('gh' + 'p_' + 'B' * 36).encode()
        self.commit('config.txt', token)
        report = json.dumps(self.scan())
        self.assertNotIn(token.decode(), report)
        self.assertIn('github-token', report)

    def test_binary_secret_is_not_skipped(self):
        self.commit('source.dat', b'\x00\xff' + ('AK' + 'IA' + 'Z' * 16).encode())
        self.assertNotEqual(self.scan()['status'], 'PUBLIC_SOURCE_AUDIT_PASS')

    def test_payload_and_credential_names_fail(self):
        for p in ['private.srm', 'thing.apk', 'pack.zip', 'model.fbx', 'audio.wav', '.env.production', 'service-account.json']:
            with self.subTest(p=p):
                self.assertTrue(path_rules(p))
        self.assertFalse(path_rules('docs/BUILD.md'))

    def test_vendor_payload_exemption_requires_exact_tree(self):
        self.commit('vendor/vanillaplus/model.bin', b'fixture')
        r = scan_repository(self.root)
        self.assertNotEqual(r['status'], 'PUBLIC_SOURCE_AUDIT_PASS')
        self.assertIn('vendor-tree', json.dumps(r))

    def test_vendor_credentials_are_still_scanned(self):
        self.commit('vendor/vanillaplus/source.c', ('gh' + 'p_' + 'C' * 36).encode())
        self.assertIn('github-token', json.dumps(self.scan()))

    def test_oversized_blob_fails_instead_of_skipping(self):
        self.commit('large.txt', b'X' * 128)
        r = self.scan(max_blob=100)
        self.assertNotEqual(r['status'], 'PUBLIC_SOURCE_AUDIT_PASS')
        self.assertIn('blob-limit', json.dumps(r))

    def test_symlink_is_not_followed_as_source(self):
        (self.root / 'link').symlink_to('/outside/private')
        self.git('add', '-A'); self.git('commit', '-qm', 'link')
        self.assertIn('tracked-mode', json.dumps(self.scan()))

    def test_shallow_history_cannot_claim_history_scan(self):
        gitdir = self.root / '.git'
        (gitdir / 'shallow').write_bytes(self.git('rev-parse', 'HEAD'))
        self.assertIn('shallow-history', json.dumps(self.scan()))

    def test_ignore_check_reports_unprotected_private_output(self):
        self.assertFalse(ignored_outputs(self.root, ['build/private.bin', 'local/save.srm', 'x.apk']))
        self.assertEqual(ignored_outputs(self.root, ['unreal/Content/Generated/World/manifest.json']), ['unreal/Content/Generated/World/manifest.json'])

    def test_private_key_and_jwt_signatures_are_detected(self):
        key = ('-----BEGIN ' + 'PRIVATE KEY-----').encode()
        jwt = ('eyJ' + 'A' * 16 + '.eyJ' + 'B' * 16 + '.' + 'C' * 24).encode()
        self.assertIn('private-key', content_rules(key))
        self.assertIn('jwt', content_rules(jwt))


if __name__ == '__main__':
    unittest.main()
