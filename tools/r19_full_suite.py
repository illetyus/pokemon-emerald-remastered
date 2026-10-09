#!/usr/bin/env python3
"""One ordered actual portable/source suite, with first failing component evidence."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import unittest
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
COMPONENTS = ['portable_ctest', 'python_regressions', 'asset_audits', 'package_integrity',
              'unreal_smoke_source', 'deferred_device_metadata', 'unreal_source_guards']
R19_NATIVE = {'r19_ordered_production_replay', 'r19_source_world_request_checkpoint',
              'r19_source_story_world_replay', 'r19_save_compatibility_matrix',
              'r19_encounter_seed_matrix', 'r19_battle_seed_matrix'}


def validate(config):
    if (config.get('schema') != 'remaster.r19.full-suite' or config.get('version') != 1 or
            config.get('source_commit') != '70db90c9077aed1272e746fc2537d9f12b95a91c' or
            config.get('private_real_save_replay') != 'NOT_RUN_IN_CI' or
            config.get('actual_unreal_device_execution') is not False):
        raise ValueError('full suite source/runtime contract')
    if config.get('components') != COMPONENTS: raise ValueError('full suite component coverage')
    native = config['required_ctests']; modules = config['required_python_modules']
    if len(native) < 81 or len(set(native)) != len(native) or not R19_NATIVE <= set(native):
        raise ValueError('full suite native coverage')
    if (len(modules) < 56 or len(set(modules)) != len(modules) or
            not all(m.startswith('test_') and m.isidentifier() for m in modules) or
            type(config.get('minimum_python_tests')) is not int or config['minimum_python_tests'] < 397):
        raise ValueError('full suite Python coverage')


def validate_discovery(discovery, required):
    tests = discovery.get('tests', [])
    names = [t.get('name') for t in tests]
    if not tests or len(set(names)) != len(names) or not set(required) <= set(names):
        raise ValueError('full suite native coverage')
    if any(not isinstance(t.get('command'), list) or not t['command'] for t in tests):
        raise ValueError('native test has no actual compiled command')
    return names


def validate_junit(xml, names):
    root = ET.fromstring(xml); cases = list(root.iter('testcase'))
    actual = [c.get('name') for c in cases]
    if not cases or len(actual) != len(names) or set(actual) != set(names):
        raise ValueError('full suite native JUnit coverage')
    for case in cases:
        if (case.get('status', 'run') != 'run' or
                any(case.find(tag) is not None for tag in ['failure', 'error', 'skipped'])):
            raise ValueError('native first failure/skip: ' + str(case.get('name')))
    for suite in root.iter('testsuite'):
        if any(int(suite.get(key, '0')) for key in ['failures', 'errors', 'skipped']):
            raise ValueError('native JUnit summary failures/skips')
    return len(cases)


def validate_python(result, minimum):
    failures = result.errors + result.failures
    if failures: raise ValueError('Python first failure: ' + failures[0][0].id())
    if result.skipped: raise ValueError('Python skip: ' + result.skipped[0][0].id())
    if result.testsRun < minimum: raise ValueError('Python executed test count: ' + str(result.testsRun))
    return {'tests': result.testsRun, 'skips': 0}


def component(name, action):
    print('R19 component START ' + name, flush=True)
    try:
        evidence = action()
    except (ValueError, OSError, subprocess.SubprocessError, ET.ParseError) as exc:
        raise ValueError('R19 first failing component ' + name + ': ' + str(exc)) from exc
    print('R19 component PASS ' + name, flush=True)
    return {'id': name, 'status': 'PASS', 'evidence': evidence}


def native(build_dir, config):
    subprocess.run(['cmake', '-S', str(ROOT), '-B', str(build_dir), '-DCMAKE_BUILD_TYPE=Release'],
                   check=True, timeout=900, cwd=ROOT)
    subprocess.run(['cmake', '--build', str(build_dir), '--config', 'Release', '--parallel', '2'],
                   check=True, timeout=900, cwd=ROOT)
    base = ['ctest', '--test-dir', str(build_dir), '-C', 'Release']
    discovered = json.loads(subprocess.check_output(base + ['--show-only=json-v1'], text=True, timeout=60))
    names = validate_discovery(discovered, config['required_ctests'])
    report = build_dir / 'r19-full-native.xml'
    report.unlink(missing_ok=True)
    subprocess.run(base + ['--no-tests=error', '--output-on-failure', '--verbose', '--output-junit', str(report)],
                   check=True, timeout=900, cwd=ROOT)
    count = validate_junit(report.read_text(), names)
    return {'tests': count, 'skips': 0, 'ctests': names,
            'r19_native_components': sorted(R19_NATIVE)}


def python_tests(config):
    modules = {p.stem for p in (ROOT / 'tests').glob('test_*.py')}
    if not set(config['required_python_modules']) <= modules: raise ValueError('Python module coverage')
    tests = unittest.TestLoader().discover(str(ROOT / 'tests'), pattern='test_*.py')
    result = unittest.TextTestRunner(verbosity=2, failfast=True).run(tests)
    return validate_python(result, config['minimum_python_tests'])


def run(build_dir, config):
    validate(config)
    for dependency in ['cmake', 'ctest', 'cc', 'c++', 'ffmpeg', 'ffprobe']:
        if shutil.which(dependency) is None: raise ValueError('missing required suite dependency: ' + dependency)
    os.environ['REMASTER_R15_REQUIRE_FFMPEG'] = '1'
    sys.path[:0] = [str(ROOT), str(ROOT / 'tools')]
    import r19_asset_audits
    import r19_package_integrity
    import r19_unreal_smoke
    import r19_device_matrix
    import validate_unreal_source

    def load(name): return json.loads((ROOT / 'data/r19' / name).read_text())
    def smoke_source():
        contract = load('unreal_smoke_contract.json'); r19_unreal_smoke.validate(contract)
        return {'status': 'SOURCE_CONTRACT_PASS', 'markers': len(contract['markers']), 'actual_runtime_verified': False}
    def source_guards():
        if validate_unreal_source.main() != 0: raise ValueError('Unreal source guard failed')
        return {'status': 'SOURCE_GUARDS_PASS', 'actual_runtime_verified': False}

    actions = [lambda: native(build_dir, config), lambda: python_tests(config),
               lambda: r19_asset_audits.run(load('asset_audits.json')),
               lambda: r19_package_integrity.run(load('package_integrity.json')),
               smoke_source, lambda: r19_device_matrix.validate(load('android_device_matrix.json')),
               source_guards]
    receipts = [component(name, action) for name, action in zip(COMPONENTS, actions)]
    return {'schema': 'remaster.r19.full-suite-receipt', 'version': 1, 'status': 'PASS',
            'source_commit': config['source_commit'], 'components': receipts,
            'private_real_save_replay': 'NOT_RUN_IN_CI', 'actual_unreal_or_device_verified': False}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, default=ROOT / 'build/r19-full')
    parser.add_argument('--receipt', type=Path)
    args = parser.parse_args()
    try:
        config = json.loads((ROOT / 'data/r19/full_suite.json').read_text())
        receipt = run(args.build_dir.resolve(), config)
        text = json.dumps(receipt, sort_keys=True, indent=2) + '\n'
        if args.receipt:
            args.receipt.parent.mkdir(parents=True, exist_ok=True)
            args.receipt.write_text(text)
        print(text, end='')
    except (ValueError, KeyError, OSError, subprocess.SubprocessError, ET.ParseError) as exc:
        print('R19 full suite FAIL: ' + str(exc), file=sys.stderr)
        raise SystemExit(1) from exc
