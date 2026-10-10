#!/usr/bin/env python3
"""Check source release intent or installation presence; never execute a build."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import sys

FOLDERS = ('World', 'Render', 'Characters', 'Environment')
PACKAGE = 'com.illetyus.emeraldremaster.r0unreal'


def section(path, name):
    active = False
    lines = []
    for raw in path.read_text(encoding='utf-8').splitlines():
        line = raw.strip()
        if line.startswith('['):
            active = line == '[' + name + ']'
        elif active and line and not line.startswith(';'):
            lines.append(line)
    return '\n'.join(lines)


def source_preflight(root):
    root = Path(root)
    config = root / 'unreal/Config'
    project = json.loads((root / 'unreal/PokemonEmeraldRemastered.uproject').read_text())
    if project.get('EngineAssociation') != '5.8':
        raise ValueError('source engine association must remain the selected 5.8 family')
    package = section(config / 'DefaultGame.ini', '/Script/UnrealEd.ProjectPackagingSettings')
    staged = re.findall(r'^\+DirectoriesToAlwaysStageAsUFS=\(Path="Generated/([^"]+)"\)$', package, re.M)
    if len(staged) != len(FOLDERS) or set(staged) != set(FOLDERS):
        raise ValueError('all four owner folders must stage exactly once')
    for token in ['+MapsToCook=(FilePath="/Engine/Maps/Entry")',
                  '+DirectoriesToAlwaysCook=(Path="/Engine/BasicShapes")']:
        if token not in package.splitlines():
            raise ValueError('entry map and visible fallback cook intent required')
    engine = section(config / 'DefaultEngine.ini', '/Script/EngineSettings.GameMapsSettings')
    for token in ['GameDefaultMap=/Engine/Maps/Entry',
                  'GlobalDefaultGameMode=/Script/PokemonEmeraldRemastered.R0GameMode']:
        if token not in engine.splitlines():
            raise ValueError('production bootstrap configuration drift')
    android = section(config / 'DefaultEngine.ini', '/Script/AndroidRuntimeSettings.AndroidRuntimeSettings')
    for token in ['PackageName=' + PACKAGE, 'bBuildForArm64=True', 'bBuildForX8664=False']:
        if token not in android.splitlines():
            raise ValueError('source Android identity/ABI drift')
    values = dict(line.split('=', 1) for line in android.splitlines() if '=' in line)
    minimum, target = int(values['MinSDKVersion']), int(values['TargetSDKVersion'])
    if not 1 <= minimum <= target <= 255:
        raise ValueError('invalid source Android SDK intent')
    return {'schema': 'remaster.r20.release-source', 'version': 1,
            'status': 'SOURCE_RELEASE_PREPARATION_PASS', 'engine_required': '5.8.3',
            'staged_folders': list(FOLDERS), 'package_identity': PACKAGE, 'abi': 'arm64-v8a',
            'configured_min_sdk': minimum, 'configured_target_sdk': target,
            'toolchain_compatibility_verified': False, 'actual_unreal_build': False}


def read_metadata(path):
    if not path.is_file() or path.stat().st_size > 65536:
        raise ValueError('required bounded installation metadata absent')
    return path.read_bytes()


def executable(path, host):
    return path.is_file() and (host == 'Win64' or os.access(path, os.X_OK))


def installation_preflight(engine, *, android=False, sdk=None, ndk=None, java=None, host='Linux'):
    if host not in ('Linux', 'Win64'):
        raise ValueError('unsupported installation host')
    if engine is None:
        raise ValueError('actual engine root required')
    engine = Path(engine)
    raw = read_metadata(engine / 'Engine/Build/Build.version')
    version = json.loads(raw)
    parts = [version.get(key) for key in ('MajorVersion', 'MinorVersion', 'PatchVersion')]
    if any(type(x) is not int for x in parts) or parts != [5, 8, 3]:
        raise ValueError('actual installation must identify exactly Unreal Engine 5.8.3')
    batch = engine / 'Engine/Build/BatchFiles'
    launchers = [batch / 'Linux/Build.sh', batch / 'RunUAT.sh'] if host == 'Linux' else [batch / 'Build.bat', batch / 'RunUAT.bat']
    if not all(executable(p, host) for p in launchers):
        raise ValueError('required build/UAT launchers absent or not executable')
    report = {'schema': 'remaster.r20.installation-preflight', 'version': 1,
              'status': 'INSTALLATION_PREREQUISITES_PRESENT', 'engine_version': '5.8.3',
              'engine_build_metadata_sha256': hashlib.sha256(raw).hexdigest(), 'host': host,
              'target': 'Android' if android else host,
              'toolchain_compatibility_verified': False, 'actual_unreal_build': False}
    if android:
        if any(p is None for p in (sdk, ndk, java)):
            raise ValueError('Android SDK/NDK/JDK roots are mandatory')
        sdk, ndk, java = map(Path, (sdk, ndk, java))
        suffix = '.exe' if host == 'Win64' else ''
        sdkmanagers = list(sdk.glob('cmdline-tools/*/bin/sdkmanager' + ('.bat' if host == 'Win64' else '')))
        platforms = sorted(p.parent.name for p in sdk.glob('platforms/android-*/android.jar') if p.is_file())
        aapt = list(sdk.glob('build-tools/*/aapt2' + suffix))
        if not platforms or not any(executable(p, host) for p in sdkmanagers) or not any(executable(p, host) for p in aapt) or not executable(sdk / ('platform-tools/adb' + suffix), host):
            raise ValueError('complete Android SDK platform/build/command tools required')
        ndkmeta = read_metadata(ndk / 'source.properties').decode('utf-8')
        revision = re.search(r'^Pkg.Revision\s*=\s*(\d+(?:\.\d+)+)\s*$', ndkmeta, re.M)
        prebuilt = 'linux-x86_64' if host == 'Linux' else 'windows-x86_64'
        if not revision or not executable(ndk / ('toolchains/llvm/prebuilt/' + prebuilt + '/bin/clang++' + suffix), host):
            raise ValueError('NDK revision and host compiler required')
        release = read_metadata(java / 'release').decode('utf-8')
        javersion = re.search(r'^JAVA_VERSION="([0-9][0-9A-Za-z._+-]*)"$', release, re.M)
        if not javersion or not all(executable(java / ('bin/' + name + suffix), host) for name in ('java', 'javac')):
            raise ValueError('JDK release metadata, java and javac required')
        report.update({'sdk_platforms_present': platforms, 'ndk_revision_present': revision.group(1),
                       'jdk_version_present': javersion.group(1)})
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-only', action='store_true')
    parser.add_argument('--engine-root', type=Path)
    parser.add_argument('--host', choices=('Linux', 'Win64'), default='Linux')
    parser.add_argument('--android', action='store_true')
    parser.add_argument('--sdk-root', type=Path, default=os.getenv('ANDROID_SDK_ROOT') or os.getenv('ANDROID_HOME'))
    parser.add_argument('--ndk-root', type=Path, default=os.getenv('ANDROID_NDK_ROOT') or os.getenv('NDKROOT'))
    parser.add_argument('--java-root', type=Path, default=os.getenv('JAVA_HOME'))
    parser.add_argument('--receipt', type=Path)
    args = parser.parse_args()
    try:
        root = Path(__file__).resolve().parents[1]
        report = source_preflight(root)
        if not args.source_only:
            report = installation_preflight(args.engine_root, android=args.android, host=args.host,
                                            sdk=args.sdk_root, ndk=args.ndk_root, java=args.java_root)
        elif args.engine_root or args.android:
            raise ValueError('source-only cannot claim an installation target')
        if args.receipt:
            from build_production_package import destination
            receipt = destination(root, args.receipt)
            receipt.parent.mkdir(parents=True, exist_ok=True)
            with receipt.open('x', encoding='utf-8') as handle:
                handle.write(json.dumps(report, sort_keys=True, indent=2) + '\n')
        print(json.dumps(report, sort_keys=True))
        return 0
    except (OSError, ValueError, KeyError):
        print('Unreal preflight failed: source intent or actual installation prerequisites incomplete.', file=sys.stderr)
        return 2


if __name__ == '__main__':
    raise SystemExit(main())
