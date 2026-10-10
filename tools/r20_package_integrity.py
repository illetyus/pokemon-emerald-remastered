"""Seal full package bytes/provenance; verification requires an external trusted hash."""
import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import subprocess

from build_production_package import FOLDERS, PIN, ROOT, SOURCE_REPOSITORY, VENDOR_TREE
from build_production_package import build, generate

INDEX = 'production-index.json'
INPUT_ROOTS = ('vendor/vanillaplus', 'tools', 'data', 'external', 'core',
               'unreal/Source', 'unreal/Config')


def encode(value):
    return (json.dumps(value, ensure_ascii=False, sort_keys=True, indent=2) + '\n').encode('utf-8')


def sha(data):
    return hashlib.sha256(data).hexdigest()


def validate_map(value, package=False):
    if not isinstance(value, dict) or not value:
        raise ValueError('empty file/input inventory')
    for name, digest in value.items():
        if not isinstance(name, str) or '\\' in name or any(ord(c) < 32 for c in name):
            raise ValueError('unsafe inventory path')
        p = PurePosixPath(name)
        if p.is_absolute() or '..' in p.parts or str(p) != name or name == '.':
            raise ValueError('unsafe inventory path')
        if not isinstance(digest, str) or not re.fullmatch('[0-9a-f]{64}', digest):
            raise ValueError('invalid inventory checksum')
        if package and (len(p.parts) < 2 or p.parts[0] not in FOLDERS):
            raise ValueError('unexpected owner path')
        if not package and not any(name.startswith(root + '/') for root in INPUT_ROOTS):
            raise ValueError('unexpected input provenance path')
    if package and not all(name + '/manifest.json' in value for name in FOLDERS):
        raise ValueError('missing owner inventory')


def inventory(output):
    if output.is_symlink():
        raise ValueError('package root symlink')
    result = {}
    for path in sorted(output.rglob('*')):
        if path.is_symlink():
            raise ValueError('package symlink')
        if path.is_file():
            name = path.relative_to(output).as_posix()
            if name != INDEX:
                result[name] = sha(path.read_bytes())
        elif not path.is_dir():
            raise ValueError('nonregular package node')
    return result


def source_inputs(root):
    command = ['git', '-C', str(root)]
    dirty = subprocess.check_output(command + ['status', '--porcelain',
                                              '--untracked-files=all', '--', *INPUT_ROOTS], text=True)
    if dirty:
        raise ValueError('production source working tree is not clean')
    names = subprocess.check_output(command + ['ls-files', '-z', '--', *INPUT_ROOTS], text=True)
    result = {}
    for name in names.split('\0'):
        if not name:
            continue
        path = root / name
        if path.is_symlink() or not path.is_file():
            raise ValueError('nonregular source input')
        result[name] = sha(path.read_bytes())
    validate_map(result)
    return result


def make_index(files, inputs):
    validate_map(files, package=True)
    validate_map(inputs)
    return {'schema': 'remaster.r20.production-index', 'version': 1,
            'source': {'repository': SOURCE_REPOSITORY, 'commit': PIN, 'vendored_tree': VENDOR_TREE},
            'source_inputs': inputs, 'input_sha256': sha(encode(inputs)),
            'files': files, 'package_sha256': sha(encode(files)), 'actual_unreal_build': False}


def build_indexed(root, output):
    inputs = source_inputs(root)
    receipt = {}

    def generate_and_seal(source, stage):
        generate(source, stage)
        if source_inputs(source) != inputs:
            raise ValueError('source inputs changed during generation')
        index = make_index(inventory(stage), inputs)
        raw = encode(index)
        (stage / INDEX).write_bytes(raw)
        receipt.update(index_sha256=sha(raw), package_sha256=index['package_sha256'],
                       file_count=len(index['files']), input_count=len(inputs))

    result = build(root, output, generate_and_seal)
    result.update(receipt)
    result['status'] = 'SOURCE_PACKAGE_READY'
    return result


def verify_package(output, expected_sha, *, root=ROOT, inputs=None):
    # The CLI always binds inputs to the real checkout. Explicit inputs support
    # isolated integrity fixtures; they are not actual source-generation proof.
    if not isinstance(expected_sha, str) or not re.fullmatch('[0-9a-f]{64}', expected_sha):
        raise ValueError('external trusted index SHA-256 is required')
    index_path = output / INDEX
    if output.is_symlink() or index_path.is_symlink():
        raise ValueError('package/index symlink')
    raw = index_path.read_bytes()
    if len(raw) > 16 * 1024 * 1024 or sha(raw) != expected_sha:
        raise ValueError('trusted index checksum mismatch')
    index = json.loads(raw)
    if encode(index) != raw:
        raise ValueError('noncanonical production index')
    if not isinstance(index, dict):
        raise ValueError('index schema/provenance mismatch')
    if (index.get('schema') != 'remaster.r20.production-index' or
            type(index.get('version')) is not int or index['version'] != 1 or
            index.get('actual_unreal_build') is not False):
        raise ValueError('index schema/provenance mismatch')
    current_inputs = source_inputs(root) if inputs is None else inputs
    if index.get('source_inputs') != current_inputs:
        raise ValueError('source/generator input provenance changed')
    expected = make_index(index.get('files'), current_inputs)
    if index != expected:
        raise ValueError('index schema/provenance mismatch')
    if inventory(output) != index['files']:
        raise ValueError('package file coverage/checksum mismatch')
    return {'status': 'SOURCE_PACKAGE_VERIFIED', 'index_sha256': expected_sha,
            'package_sha256': index['package_sha256'], 'file_count': len(index['files']),
            'input_count': len(current_inputs), 'actual_unreal_build': False}
