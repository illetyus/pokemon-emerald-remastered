#!/usr/bin/env python3
"""Verify immutable ordered-input observations against a compiled Emerald probe."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys

from r19_state import DOMAINS, fingerprint, validate_expected

ROOT = Path(__file__).resolve().parents[1]
PIN = '70db90c9077aed1272e746fc2537d9f12b95a91c'
MATRIX = ROOT / 'tests/fixtures/r19/movement.json'
DIRECTIONS = ('south', 'north', 'west', 'east')


def require(condition, message):
    if not condition:
        raise ValueError(message)


def validate(matrix):
    require(matrix.get('schema') == 'remaster.r19.replay-matrix' and type(matrix.get('version')) is int and matrix['version'] == 1,
            'unsupported matrix schema/version')
    require(matrix.get('source') == {'repository': 'illetyus/pokezumrut-vanillaplus', 'commit': PIN},
            'source provenance mismatch')
    raw = (ROOT / 'tests/r19_replay_probe.c').read_bytes().replace(b'\r\n', b'\n')
    blob = hashlib.sha1(b'blob ' + str(len(raw)).encode() + b'\0' + raw).hexdigest()
    require(matrix.get('probe_source') == {'path': 'tests/r19_replay_probe.c', 'blob_sha': blob},
            'probe source/recipe drift')
    cases = matrix.get('cases')
    require(isinstance(cases, list) and 1 <= len(cases) <= 128, 'invalid cases count')
    ids = set()
    for case in cases:
        identity = case.get('id')
        require(isinstance(identity, str) and identity and identity not in ids, 'duplicate/invalid case ID')
        ids.add(identity)
        require(case.get('recipe') == 'synthetic-movement-v1', identity + ': unsupported recipe')
        commands = case.get('commands')
        require(isinstance(commands, list) and 1 <= len(commands) <= 4096, identity + ': invalid commands count')
        for index, command in enumerate(commands, 1):
            require(isinstance(command, dict) and command.get('op') == 'step',
                    f'{identity}: unsupported command {index}')
            require(set(command) == {'op', 'direction'} and command['direction'] in DIRECTIONS,
                    f'{identity}: invalid direction/command {index}')
        require(isinstance(case.get('expected'), list) and len(case['expected']) == len(commands) + 1,
                identity + ': expected snapshot count mismatch')
        for expected in case['expected']:
            validate_expected(expected)
            observations = expected['observations']
            require(set(observations) == {'x', 'y', 'kind', 'collision'}
                    and all(type(v) is int for v in observations.values()), identity + ': invalid expected observation')


def execute(probe, case, mode=None):
    commands = ''.join(f"step {c['direction']}\n" for c in case['commands'])
    try:
        result = subprocess.run([str(probe), case['recipe']] + ([mode] if mode else []), input=commands, text=True,
                                capture_output=True, timeout=30, check=False)
    except (OSError, subprocess.TimeoutExpired) as error:
        raise ValueError(f"{case['id']}: probe execution failed: {error}") from error
    require(result.returncode == 0, f"{case['id']}: probe failed: {result.stderr.strip()}")
    try:
        return [json.loads(line) for line in result.stdout.splitlines()]
    except json.JSONDecodeError as error:
        raise ValueError(f"{case['id']}: invalid probe JSON: {error}") from error


def verify(case, actual):
    require(len(actual) == len(case['expected']), f"{case['id']}: snapshot count: expected {len(case['expected'])}, actual {len(actual)}")
    for index, (expected, observed) in enumerate(zip(case['expected'], actual)):
        try:
            result = fingerprint(observed)
        except ValueError as error:
            raise ValueError(f"{case['id']}: snapshot {index}: {error}") from error
        observations = result['observations']
        require(set(observations) == set(expected['observations']), f"{case['id']}: snapshot {index}: observation schema mismatch")
        for field, value in expected['observations'].items():
            require(type(observations[field]) is type(value) and observations[field] == value,
                    f"{case['id']}: snapshot {index}, command {case['commands'][index-1] if index else 'initial'}, "
                    f"field {field}: expected {value}, actual {observations[field]}")
        for domain in DOMAINS:
            require(result['domain_hashes'][domain] == expected['domain_hashes'][domain],
                    f"{case['id']}: snapshot {index}, command {case['commands'][index-1] if index else 'initial'}, "
                    f"domain {domain}: expected {expected['domain_hashes'][domain]}, actual {result['domain_hashes'][domain]}")
        require(result['state_hash'] == expected['state_hash'],
                f"{case['id']}: snapshot {index}, state hash: expected {expected['state_hash']}, actual {result['state_hash']}")


def run(probe, matrix):
    validate(matrix)
    for case in matrix['cases']:
        actual = execute(probe, case)
        verify(case, actual)
        require(actual == execute(probe, case), case['id'] + ': repeat execution differs')
        verify(case, execute(probe, case, '--transport-noise'))
        try:
            verify(case, execute(probe, case, '--persistent-noise'))
        except ValueError as error:
            require('snapshot 0' in str(error) and 'domain save_block1' in str(error),
                    'persistent mutation failed at wrong boundary: ' + str(error))
        else:
            raise ValueError('persistent mutation was not detected')
        mutated = dict(case, commands=[dict(c) for c in case['commands']])
        mutated['commands'][0]['direction'] = 'south'
        try:
            verify(case, execute(probe, mutated))
        except ValueError as error:
            require('snapshot 1' in str(error), 'command mutation failed at wrong index: ' + str(error))
        else:
            raise ValueError('command mutation was not detected')
    # The native boundary independently rejects commands not accepted by Python.
    for bad in ('step diagonal\n', 'set_story_flag 1\n', 'step north extra\n'):
        result = subprocess.run([str(probe), 'synthetic-movement-v1'], input=bad,
                                text=True, capture_output=True, timeout=30, check=False)
        require(result.returncode != 0, 'native probe accepted malformed command')
    print(f"R19 ordered production replay: {len(matrix['cases'])} cases, "
          f"{sum(len(c['expected']) for c in matrix['cases'])} observations PASS")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--probe', type=Path, required=True)
    parser.add_argument('--matrix', type=Path, default=MATRIX)
    args = parser.parse_args()
    try:
        run(args.probe.resolve(), json.loads(args.matrix.read_text()))
    except (ValueError, OSError, subprocess.TimeoutExpired) as error:
        print(f'R19 replay FAIL: {error}', file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
