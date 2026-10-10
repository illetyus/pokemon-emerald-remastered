#!/usr/bin/env python3
"""Actual R19 suite, independent clean-checkout reproduction and security gates."""
import argparse
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
PIN = '70db90c9077aed1272e746fc2537d9f12b95a91c'
MODULES = ['test_r20_package', 'test_r20_integrity', 'test_r20_security', 'test_r20_release', 'test_r20_full_suite']
R19_COMPONENTS = ['portable_ctest', 'python_regressions', 'asset_audits', 'package_integrity',
                  'unreal_smoke_source', 'deferred_device_metadata', 'unreal_source_guards']
FIELDS = ('file_count', 'input_count', 'index_sha256', 'package_sha256')


def validate_r19(receipt):
    if (receipt.get('schema') != 'remaster.r19.full-suite-receipt' or
            type(receipt.get('version')) is not int or receipt['version'] != 1 or
            receipt.get('status') != 'PASS' or receipt.get('source_commit') != PIN or
            receipt.get('actual_unreal_or_device_verified') is not False):
        raise ValueError('R19 actual source/full receipt required')
    components = receipt.get('components', [])
    if [c.get('id') for c in components] != R19_COMPONENTS or any(c.get('status') != 'PASS' for c in components):
        raise ValueError('all seven actual R19 components required')
    for entry, minimum in zip(components[:3], [81, 443, 171]):
        evidence = entry.get('evidence', {})
        count = evidence.get('tests')
        if type(count) is not int or count < minimum or evidence.get('skips', 0) != 0:
            raise ValueError('actual R19 coverage/zero skips required')


def validate_targeted(result, modules):
    if (not set(MODULES) <= modules or result.errors or result.failures or result.skipped or result.testsRun < 43):
        raise ValueError('R20 mandatory module/actual test coverage failed')
    return {'tests': result.testsRun, 'modules': sorted(modules), 'skips': 0}


def compare_packages(a, b):
    for item in (a, b):
        if item.get('status') != 'SOURCE_PACKAGE_READY' or item.get('actual_unreal_build') is not False:
            raise ValueError('actual source generation receipts required')
        if any(type(item.get(k)) is not int or item[k] <= 0 for k in FIELDS[:2]):
            raise ValueError('package/input coverage required')
        if any(not isinstance(item.get(k), str) or not re.fullmatch('[0-9a-f]{64}', item[k]) for k in FIELDS[2:]):
            raise ValueError('actual index/package hashes required')
    if any(a[k] != b[k] for k in FIELDS):
        raise ValueError('independent clean generation byte/provenance drift')
    return {k: a[k] for k in FIELDS}


def validate_verification(receipt, generation):
    if (receipt.get('status') != 'SOURCE_PACKAGE_VERIFIED' or receipt.get('actual_unreal_build') is not False or
            any(receipt.get(k) != generation.get(k) for k in FIELDS)):
        raise ValueError('verification must bind external generation evidence')


def cli_json(command, cwd):
    result = subprocess.run(command, cwd=cwd, text=True, capture_output=True, timeout=900)
    if result.returncode:
        raise ValueError('source preparation command failed: ' + Path(command[1]).name)
    return json.loads(result.stdout.strip().splitlines()[-1])


def rejection(command, cwd, reason):
    result = subprocess.run(command, cwd=cwd, capture_output=True, timeout=900)
    expected = b'R20 production package failed: ' + reason.encode('utf-8')
    if result.returncode != 1 or result.stderr.strip() != expected:
        raise ValueError('corrupt package did not produce the expected validation rejection')


def reproduce(build_dir):
    head = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip()
    outputs, checkouts, generations, verified = [], [], [], []
    with tempfile.TemporaryDirectory(prefix='.r20-clean-', dir=build_dir) as temporary:
        try:
            for name in ('first', 'second'):
                checkout = Path(temporary) / name
                subprocess.run(['git', 'worktree', 'add', '--quiet', '--detach', str(checkout), head], cwd=ROOT, check=True, timeout=120)
                checkouts.append(checkout)
                output = checkout / 'build/production/Generated'
                script = checkout / 'tools/build_production_package.py'
                generation = cli_json([sys.executable, str(script), '--output', str(output),
                                       '--receipt', str(checkout / 'build/generation-receipt.json')], checkout)
                verify = [sys.executable, str(script), '--output', str(output), '--verify',
                          '--expected-index-sha256', generation['index_sha256']]
                receipt = cli_json(verify, checkout)
                validate_verification(receipt, generation)
                generations.append(generation); outputs.append(output); verified.append(verify)
            same = compare_packages(*generations)
            output, checkout, verify = outputs[0], checkouts[0], verified[0]
            rejected = []
            # Every real owner manifest is changed and restored independently.
            for owner in ('World', 'Render', 'Characters', 'Environment'):
                path = output / owner / 'manifest.json'; original = path.read_bytes()
                try:
                    path.write_bytes(original + b'\ncorrupt')
                    rejection(verify, checkout, 'package file coverage/checksum mismatch'); rejected.append('corrupt-' + owner)
                finally:
                    path.write_bytes(original)
            path = output / 'World/manifest.json'; original = path.read_bytes()
            try:
                path.unlink(); rejection(verify, checkout, 'package file coverage/checksum mismatch'); rejected.append('missing-owner-file')
            finally:
                path.write_bytes(original)
            extra = output / 'World/unexpected.txt'
            try:
                extra.write_text('explicit corruption fixture')
                rejection(verify, checkout, 'package file coverage/checksum mismatch'); rejected.append('extra-file')
            finally:
                extra.unlink(missing_ok=True)
            index = output / 'production-index.json'; original_index = index.read_bytes()
            try:
                path.write_bytes(original + b'changed')
                changed = json.loads(original_index)
                from r20_package_integrity import make_index, inventory, encode
                index.write_bytes(encode(make_index(inventory(output), changed['source_inputs'])))
                rejection(verify, checkout, 'trusted index checksum mismatch'); rejected.append('rewritten-index-self-blessing')
            finally:
                path.write_bytes(original); index.write_bytes(original_index)
            link = output / 'World/unexpected-link'
            try:
                link.symlink_to(path)
                rejection(verify, checkout, 'package symlink'); rejected.append('symlink-file')
            finally:
                link.unlink(missing_ok=True)
            generator = checkout / 'tools/build_production_package.py'; original_generator = generator.read_bytes()
            try:
                generator.write_bytes(original_generator + b'\n# explicit source drift fixture\n')
                rejection(verify, checkout, 'production source working tree is not clean'); rejected.append('generator-source-drift')
            finally:
                generator.write_bytes(original_generator)
            validate_verification(cli_json(verify, checkout), generations[0])
            return dict(same, clean_checkouts=2, verified_packages=2, rejected_cases=rejected,
                        restored_package_verified=True, actual_unreal_build=False)
        finally:
            for checkout in reversed(checkouts):
                subprocess.run(['git', 'worktree', 'remove', '--force', str(checkout)], cwd=ROOT, check=True, timeout=120)


def targeted():
    def cases(suite):
        for child in suite:
            if isinstance(child, unittest.TestSuite): yield from cases(child)
            else: yield child
    suite = unittest.TestLoader().discover(str(ROOT / 'tests'), pattern='test_r20_*.py')
    modules = {case.__class__.__module__ for case in cases(suite)}
    result = unittest.TextTestRunner(verbosity=2, failfast=True).run(suite)
    return validate_targeted(result, modules)


def run(build_dir):
    import audit_public_repository
    import unreal_preflight
    r19_path = build_dir / 'r19-full-suite-receipt.json'
    components = []
    def component(name, action):
        print('R20 component START ' + name, flush=True)
        try:
            value = action()
        except (OSError, ValueError, KeyError, subprocess.SubprocessError) as exc:
            raise ValueError('R20 first failing component ' + name) from exc
        components.append({'id': name, 'status': 'PASS', 'evidence': value})
        print('R20 component PASS ' + name, flush=True)
    def full():
        subprocess.run([sys.executable, str(ROOT / 'tools/r19_full_suite.py'), '--build-dir', str(build_dir),
                        '--receipt', str(r19_path)], cwd=ROOT, check=True, timeout=1200)
        receipt = json.loads(r19_path.read_text()); validate_r19(receipt)
        return receipt
    def security():
        receipt = audit_public_repository.scan_repository(ROOT)
        if receipt['status'] != 'PUBLIC_SOURCE_AUDIT_PASS' or receipt['findings'] or receipt['skipped_blobs']:
            raise ValueError('public source/security audit failed')
        return receipt
    component('r19_full_suite', full)
    component('r20_targeted', targeted)
    component('clean_package_reproduction', lambda: reproduce(build_dir))
    component('public_source_security', security)
    component('source_release_preparation', lambda: unreal_preflight.source_preflight(ROOT))
    return {'schema': 'remaster.r20.full-suite-receipt', 'version': 1,
            'status': 'SOURCE_PRODUCTION_ACCEPTANCE_PASS', 'source_commit': PIN,
            'components': components, 'actual_unreal_or_device_verified': False}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, default=ROOT / 'build/r20-full')
    parser.add_argument('--receipt', type=Path)
    args = parser.parse_args()
    try:
        from build_production_package import destination
        build = destination(ROOT, args.build_dir)
        receipt_path = destination(ROOT, args.receipt) if args.receipt else None
        build.mkdir(parents=True)
        receipt = run(build)
        text = json.dumps(receipt, sort_keys=True, indent=2) + '\n'
        if receipt_path:
            receipt_path.parent.mkdir(parents=True, exist_ok=True)
            with receipt_path.open('x', encoding='utf-8') as handle: handle.write(text)
        print(text, end='')
        return 0
    except (OSError, ValueError, KeyError, subprocess.SubprocessError) as exc:
        failure = str(exc) if isinstance(exc, ValueError) and str(exc).startswith('R20 first failing component ') else 'R20 full suite inputs failed'
        print(failure + '; no acceptance receipt.', file=sys.stderr)
        return 1


if __name__ == '__main__':
    raise SystemExit(main())
