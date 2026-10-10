"""Source configuration and explicit fake installations; never real UE proof."""
import json
import os
from pathlib import Path
import sys
import tempfile
import unittest
import unittest.mock

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from unreal_preflight import source_preflight, installation_preflight


class ReleasePreflight(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.engine = self.root / 'engine'
        self.sdk = self.root / 'sdk'
        self.ndk = self.root / 'ndk'
        self.java = self.root / 'java'
        self.put('engine/Engine/Build/Build.version', json.dumps({'MajorVersion': 5, 'MinorVersion': 8, 'PatchVersion': 3}))
        for p in ['engine/Engine/Build/BatchFiles/Linux/Build.sh', 'engine/Engine/Build/BatchFiles/RunUAT.sh',
                  'sdk/cmdline-tools/latest/bin/sdkmanager', 'sdk/platform-tools/adb',
                  'sdk/build-tools/35.0.0/aapt2', 'ndk/toolchains/llvm/prebuilt/linux-x86_64/bin/clang++',
                  'java/bin/java', 'java/bin/javac']:
            self.put(p, '#!/bin/sh\nexit 99\n', executable=True)
        self.put('sdk/platforms/android-35/android.jar', 'explicit fake metadata')
        self.put('ndk/source.properties', 'Pkg.Revision = 27.2.12479018\n')
        self.put('java/release', 'JAVA_VERSION="17.0.13"\n')

    def put(self, path, content, executable=False):
        p = self.root / path
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_text(content, encoding='utf-8')
        if executable:
            p.chmod(0o755)

    def install(self, android=True):
        return installation_preflight(self.engine, android=android, sdk=self.sdk,
                                      ndk=self.ndk, java=self.java, host='Linux')

    def test_exact_installation_metadata_is_not_build_or_compatibility_proof(self):
        r = self.install()
        self.assertEqual(r['status'], 'INSTALLATION_PREREQUISITES_PRESENT')
        self.assertEqual(r['engine_version'], '5.8.3')
        self.assertFalse(r['actual_unreal_build'])
        self.assertFalse(r['toolchain_compatibility_verified'])
        self.assertNotIn(str(self.root), json.dumps(r))

    def test_crlf_jdk_release_metadata_preserves_presence_only_status(self):
        (self.java / 'release').write_bytes(b'JAVA_VERSION="21.0.12.1"\r\n')
        report = self.install()
        self.assertEqual(report['jdk_version_present'], '21.0.12.1')
        self.assertFalse(report['toolchain_compatibility_verified'])
        self.assertFalse(report['actual_unreal_build'])

    def test_engine_family_or_different_patch_cannot_pass(self):
        for patch in [2, 4, True, '3']:
            self.put('engine/Engine/Build/Build.version', json.dumps({'MajorVersion': 5, 'MinorVersion': 8, 'PatchVersion': patch}))
            with self.subTest(patch=patch), self.assertRaises(ValueError):
                self.install()

    def test_missing_engine_or_launcher_fails(self):
        (self.engine / 'Engine/Build/BatchFiles/Linux/Build.sh').unlink()
        with self.assertRaises(ValueError):
            self.install()

    def test_nonexecutable_linux_launcher_fails(self):
        launcher = self.engine / 'Engine/Build/BatchFiles/RunUAT.sh'
        launcher.chmod(0o644)
        if os.name == 'nt':
            # Windows chmod cannot express the POSIX executable bit. Exercise
            # the Linux access rejection without skipping the contract test.
            access = os.access
            with unittest.mock.patch('unreal_preflight.os.access',
                                     side_effect=lambda path, mode: False if path == launcher else access(path, mode)) as probe:
                with self.assertRaises(ValueError):
                    self.install()
                probe.assert_any_call(launcher, os.X_OK)
        else:
            with self.assertRaises(ValueError):
                self.install()

    def test_android_requires_each_toolchain_root(self):
        for key in ['sdk', 'ndk', 'java']:
            args = {'sdk': self.sdk, 'ndk': self.ndk, 'java': self.java}
            args[key] = None
            with self.subTest(key=key), self.assertRaises(ValueError):
                installation_preflight(self.engine, android=True, **args)

    def test_missing_sdk_platform_or_build_tools_fails(self):
        for path in ['platforms/android-35/android.jar', 'build-tools/35.0.0/aapt2']:
            p = self.sdk / path
            original = p.read_bytes()
            mode = p.stat().st_mode
            p.unlink()
            with self.subTest(path=path), self.assertRaises(ValueError):
                self.install()
            p.write_bytes(original); p.chmod(mode)

    def test_missing_ndk_compiler_fails(self):
        (self.ndk / 'toolchains/llvm/prebuilt/linux-x86_64/bin/clang++').unlink()
        with self.assertRaises(ValueError):
            self.install()

    def test_missing_javac_or_release_metadata_fails(self):
        (self.java / 'bin/javac').unlink()
        with self.assertRaises(ValueError):
            self.install()

    def test_linux_compile_prerequisites_do_not_require_android(self):
        r = installation_preflight(self.engine, android=False, host='Linux')
        self.assertEqual(r['target'], 'Linux')
        self.assertFalse(r['actual_unreal_build'])

    def test_current_source_staging_is_complete(self):
        root = Path(__file__).resolve().parents[1]
        r = source_preflight(root)
        self.assertEqual(r['status'], 'SOURCE_RELEASE_PREPARATION_PASS')
        self.assertEqual(r['staged_folders'], ['World', 'Render', 'Characters', 'Environment'])
        self.assertFalse(r['actual_unreal_build'])

    def test_missing_world_stage_or_entry_cook_fails(self):
        original = Path(__file__).resolve().parents[1]
        for path in ['unreal/Config/DefaultGame.ini', 'unreal/Config/DefaultEngine.ini', 'unreal/PokemonEmeraldRemastered.uproject']:
            self.put(path, (original / path).read_text())
        p = self.root / 'unreal/Config/DefaultGame.ini'
        original = p.read_text()
        for token in ['+DirectoriesToAlwaysStageAsUFS=(Path="Generated/World")', '+MapsToCook=(FilePath="/Engine/Maps/Entry")']:
            p.write_text(original.replace(token, ''))
            with self.subTest(token=token), self.assertRaises(ValueError):
                source_preflight(self.root)

    def test_win64_requires_host_specific_launchers(self):
        for path in ['engine/Engine/Build/BatchFiles/Build.bat', 'engine/Engine/Build/BatchFiles/RunUAT.bat']:
            self.put(path, 'explicit non-executed fixture')
        r = installation_preflight(self.engine, android=False, host='Win64')
        self.assertEqual(r['host'], 'Win64')
        self.assertFalse(r['actual_unreal_build'])


if __name__ == '__main__':
    unittest.main()
