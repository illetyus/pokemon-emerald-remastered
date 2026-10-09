#!/usr/bin/env python3
"""Reproduce four source metadata packages and reject deliberate staged corruption."""
import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
PIN = '70db90c9077aed1272e746fc2537d9f12b95a91c'


def inventory(root):
    if root.is_symlink(): raise ValueError('package root symlink')
    files = {}
    for p in sorted(root.rglob('*')):
        if p.is_symlink(): raise ValueError('package symlink: ' + str(p))
        if p.is_file():
            files[p.relative_to(root).as_posix()] = hashlib.sha256(p.read_bytes()).hexdigest()
    if not files: raise ValueError('empty package')
    return files


def verify(root, expected):
    if not expected: raise ValueError('empty package index')
    for name, digest in expected.items():
        p = PurePosixPath(name)
        if p.is_absolute() or '..' in p.parts or str(p) != name or name == '.':
            raise ValueError('unsafe package path: ' + name)
        if not isinstance(digest, str) or not re.fullmatch('[0-9a-f]{64}', digest):
            raise ValueError('invalid checksum: ' + name)
    try:
        actual = inventory(root)
    except ValueError as exc:
        if str(exc) == 'empty package': raise ValueError('package file coverage: empty') from exc
        raise
    if set(actual) != set(expected): raise ValueError('package file coverage')
    for name in sorted(expected):
        if actual[name] != expected[name]: raise ValueError(name + ' checksum mismatch')
    return actual


def compare_generation(phase, first, second):
    if not first or first != second: raise ValueError(phase + ' generation divergence')


def corruption_checks(root, expected):
    count = 0
    for name in sorted(expected):
        path = root / name
        original = path.read_bytes()
        try:
            path.write_bytes(original + b'\nR19 deliberate metadata corruption\n')
            try:
                verify(root, expected)
            except ValueError as exc:
                if str(exc) != name + ' checksum mismatch': raise
                count += 1
            else: raise ValueError(name + ' corruption escaped detection')
        finally:
            path.write_bytes(original)
    verify(root, expected)
    return count


def validate(config):
    if (config.get('schema') != 'remaster.r19.package-integrity' or
            config.get('version') != 1 or config.get('source_commit') != PIN or
            [c.get('id') for c in config.get('components', [])] != ['R6', 'R7', 'R14', 'R15']):
        raise ValueError('package component/source contract')
    for c in config['components']:
        owner = c['owner']
        if owner['path'] != 'tools/build_' + c['id'].lower() + {
                'R6': '_character_package.py', 'R7': '_environment_package.py',
                'R14': '_battle_package.py', 'R15': '_audio_catalog.py'}[c['id']]:
            raise ValueError(c['id'] + ' owner path')
        data = (ROOT / owner['path']).read_bytes()
        blob = hashlib.sha1(b'blob ' + str(len(data)).encode() + b'\0' + data).hexdigest()
        if blob != owner['blob_sha']: raise ValueError(c['id'] + ' owner source drift')
        if not c['files'] or len(set(c['files'])) != len(c['files']):
            raise ValueError(c['id'] + ' package file coverage')


def generate(phase, output):
    sys.path.insert(0, str(ROOT))
    output.mkdir(parents=True, exist_ok=True)
    if phase == 'R6':
        from tools.build_r6_character_package import build_package
        build_package(ROOT, output)
    elif phase == 'R7':
        from tools.build_r7_environment_package import build_package
        build_package(ROOT / 'vendor/vanillaplus', output)
    elif phase == 'R14':
        from tools.build_r14_battle_package import build, dump, AUDIT, HEADER
        audit, header = build()
        if AUDIT.read_text() != dump(audit) or HEADER.read_text() != header:
            raise ValueError('R14 committed generated package drift')
        (output / 'species_audit.json').write_text(dump(audit))
        (output / 'RemasterBattleCatalog.inl').write_text(header)
    elif phase == 'R15':
        from tools.build_r15_audio_catalog import build, dump
        catalog, header = build()
        if ((ROOT / 'data/r15/source_audio_catalog.json').read_text() != dump(catalog) or
                (ROOT / 'unreal/Source/PokemonEmeraldRemastered/RemasterAudioCatalog.inl').read_text() != header):
            raise ValueError('R15 committed generated package drift')
        (output / 'source_audio_catalog.json').write_text(dump(catalog))
        (output / 'RemasterAudioCatalog.inl').write_text(header)
    else: raise ValueError('unsupported package component')


def run(config):
    validate(config)
    receipts = []
    with tempfile.TemporaryDirectory(prefix='r19-metadata-integrity-') as temp:
        for c in config['components']:
            phase = c['id']
            first, second = Path(temp) / phase / 'first', Path(temp) / phase / 'second'
            generate(phase, first); generate(phase, second)
            expected = inventory(first)
            if sorted(expected) != sorted(c['files']): raise ValueError(phase + ' package file coverage')
            compare_generation(phase, expected, inventory(second))
            verify(first, expected)
            corruptions = corruption_checks(first, expected)
            receipts.append({'phase': phase, 'status': 'PASS', 'file_sha256': expected,
                             'clean_generations': 2, 'corruptions_rejected': corruptions})
    return {'schema': 'remaster.r19.package-integrity-receipt', 'version': 1,
            'source_commit': PIN, 'components': receipts,
            'private_commercial_payloads': False, 'actual_unreal_or_device': False}


if __name__ == '__main__':
    try:
        config = json.loads((ROOT / 'data/r19/package_integrity.json').read_text())
        print(json.dumps(run(config), sort_keys=True))
    except (ValueError, OSError, KeyError) as exc:
        print('R19 package integrity failed: ' + str(exc), file=sys.stderr)
        raise SystemExit(1) from exc
