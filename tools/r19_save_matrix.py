#!/usr/bin/env python3
"""Execute every pinned R17 compatibility owner and reject incomplete evidence."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / 'data/r19/save_matrix.json'
PIN = '70db90c9077aed1272e746fc2537d9f12b95a91c'


def require(ok, message):
    if not ok:
        raise ValueError(message)


def validate(manifest):
    require(type(manifest) is dict and set(manifest) == {
        'schema', 'version', 'source', 'fixture', 'case_ids', 'ctests', 'private_policy'}, 'save matrix schema')
    require(manifest['schema'] == 'remaster.r19.save-matrix' and type(manifest['version']) is int
            and manifest['version'] == 1, 'save matrix version')
    require(manifest['source'] == {'repository': 'illetyus/pokezumrut-vanillaplus', 'commit': PIN}, 'source pin')
    require(type(manifest['fixture']) is dict and set(manifest['fixture']) == {'path', 'blob_sha'}
            and manifest['fixture']['path'] == 'tests/fixtures/r17/matrix.json', 'fixture identity')
    raw = (ROOT / manifest['fixture']['path']).read_bytes().replace(b'\r\n', b'\n')
    require(hashlib.sha1(b'blob ' + str(len(raw)).encode() + b'\0' + raw).hexdigest()
            == manifest['fixture']['blob_sha'], 'R17 fixture drift')
    fixture = json.loads(raw)
    require(fixture['schema_version'] == 1 and fixture['source']['vanillaplus']['commit'] == PIN,
            'R17 source/fixture version')
    ids = [c['id'] for c in fixture['synthetic']]
    require(ids and len(set(ids)) == len(ids) and manifest['case_ids'] == ids, 'save case coverage')
    owners = [c['ctest'] for c in fixture['coverage']] + ['r17_save_fixture_matrix']
    require(owners and len(set(owners)) == len(owners) and manifest['ctests'] == owners, 'save owner coverage')
    require(manifest['private_policy'] == 'LOCAL_OPTIONAL_NOT_RUN_IN_CI', 'private real fixture boundary')
    return fixture


def validate_discovery(discovery, names):
    require(type(discovery) is dict and type(discovery.get('tests')) is list, 'invalid CTest discovery')
    selected = [t for t in discovery['tests'] if t.get('name') in names]
    require(len(selected) == len(names) and {t['name'] for t in selected} == set(names),
            'CTest discovery omitted/duplicated a required save owner')
    require(all(type(t.get('command')) is list and t['command'] for t in selected),
            'save owner has no compiled executable command')


def validate_junit(xml, names):
    root = ET.fromstring(xml)
    cases = list(root.iter('testcase'))
    actual = [t.get('name') for t in cases]
    require(len(actual) == len(names) and set(actual) == set(names), 'JUnit must contain exact required save owners')
    for case in cases:
        require(case.get('status', 'run') == 'run'
                and not any(case.find(tag) is not None for tag in ('failure', 'error', 'skipped')),
                'nonpassing save owner: ' + str(case.get('name')))
    for suite in root.iter('testsuite'):
        require(all(int(suite.get(k, '0')) == 0 for k in ('failures', 'errors', 'skipped')),
                'nonpassing JUnit suite')
    return names


def command(args):
    result = subprocess.run(args, text=True, capture_output=True, timeout=180, check=False)
    require(result.returncode == 0, 'save CTest execution failed: ' + (result.stdout + result.stderr)[-4000:])
    return result.stdout


def run(build_dir, ctest, manifest):
    fixture = validate(manifest)
    names = manifest['ctests']
    base = [str(ctest), '--test-dir', str(build_dir), '-C', 'Release']
    validate_discovery(json.loads(command(base + ['--show-only=json-v1'])), names)
    with tempfile.TemporaryDirectory(prefix='r19-save-evidence-') as tmp:
        report = Path(tmp) / 'save.xml'
        pattern = '^(' + '|'.join(re.escape(n) for n in names) + ')$'
        command(base + ['-R', pattern, '--no-tests=error', '--output-on-failure', '--output-junit', str(report)])
        validate_junit(report.read_text(), names)
    receipt = {'schema': 'remaster.r19.save-result', 'version': 1, 'status': 'PASS',
               'fixture_blob_sha': manifest['fixture']['blob_sha'], 'ctests': names,
               'synthetic_cases': len(fixture['synthetic']),
               'private_real_replay': 'NOT_RUN_IN_CI'}
    print(json.dumps(receipt, sort_keys=True))
    return receipt


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, required=True)
    parser.add_argument('--ctest', default='ctest')
    args = parser.parse_args()
    try:
        run(args.build_dir.resolve(), args.ctest, json.loads(MANIFEST.read_text()))
    except (ValueError, OSError, subprocess.TimeoutExpired, ET.ParseError) as error:
        print('R19 save matrix FAIL: ' + str(error), file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
