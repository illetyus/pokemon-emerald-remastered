#!/usr/bin/env python3
"""Execute the existing source/synthetic asset audits; never certify UE/device use."""
import hashlib
import json
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
CONFIG = ROOT / 'data/r19/asset_audits.json'
PHASES = ('R6', 'R7', 'R14', 'R15')
PIN = '70db90c9077aed1272e746fc2537d9f12b95a91c'


def validate(config):
    if (config.get('schema') != 'remaster.r19.asset-audits' or
            config.get('version') != 1 or config.get('source') != {
                'repo': 'illetyus/pokezumrut-vanillaplus', 'commit': PIN}):
        raise ValueError('asset audit schema/source contract')
    components = config.get('components', [])
    if [c.get('id') for c in components] != list(PHASES):
        raise ValueError('asset audit phase coverage')
    for c in components:
        phase = c['id']
        modules = c.get('modules', [])
        expected = sorted(p.name[:-3] for p in (ROOT / 'tests').glob(
            'test_' + phase.lower() + '_*.py'))
        if not modules or sorted(m['name'] for m in modules) != expected:
            raise ValueError(phase + ' module coverage')
        if type(c.get('minimum_tests')) is not int or c['minimum_tests'] <= 0:
            raise ValueError(phase + ' minimum executed test count')
        for m in modules:
            if m['path'] != 'tests/' + m['name'] + '.py':
                raise ValueError(phase + ' module path')
            data = (ROOT / m['path']).read_bytes()
            digest = hashlib.sha1(b'blob ' + str(len(data)).encode() + b'\0' + data).hexdigest()
            if digest != m.get('blob_sha'):
                raise ValueError(phase + ' audit source drift: ' + m['path'])
    return config


def validate_result(phase, result, minimum):
    failures = result.errors + result.failures
    if failures:
        raise ValueError(phase + ' first failure: ' + failures[0][0].id())
    if result.testsRun < minimum or result.testsRun == 0:
        raise ValueError(phase + ' executed test count: ' + str(result.testsRun))
    if result.skipped:
        raise ValueError(phase + ' skipped audit: ' + result.skipped[0][0].id())
    return {'phase': phase, 'status': 'PASS', 'tests': result.testsRun, 'skips': 0}


def run(config):
    validate(config)
    sys.path[:0] = [str(ROOT), str(ROOT / 'tests')]
    receipts = []
    for component in config['components']:
        suite = unittest.TestLoader().loadTestsFromNames(
            [m['name'] for m in component['modules']])
        if suite.countTestCases() < component['minimum_tests']:
            raise ValueError(component['id'] + ' discovered test count')
        result = unittest.TextTestRunner(verbosity=1, failfast=True).run(suite)
        receipts.append(validate_result(component['id'], result, component['minimum_tests']))
    return {'schema': 'remaster.r19.asset-audit-receipt', 'version': 1,
            'source_commit': PIN, 'components': receipts,
            'tests': sum(r['tests'] for r in receipts),
            'actual_unreal_runtime': False, 'actual_device_runtime': False,
            'private_payload_required': False}


if __name__ == '__main__':
    try:
        print(json.dumps(run(json.loads(CONFIG.read_text())), sort_keys=True))
    except (ValueError, KeyError, OSError) as exc:
        print('R19 asset audit failed: ' + str(exc), file=sys.stderr)
        raise SystemExit(1) from exc
