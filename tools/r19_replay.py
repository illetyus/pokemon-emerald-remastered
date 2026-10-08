#!/usr/bin/env python3
"""Verify immutable ordered-input observations against a compiled Emerald probe."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
PIN = '70db90c9077aed1272e746fc2537d9f12b95a91c'
MATRIX = ROOT / 'tests/fixtures/r19/movement.json'
DIRECTIONS = ('south', 'north', 'west', 'east')


def require(condition, message):
    if not condition:
        raise ValueError(message)


def validate(matrix):
    require(matrix.get('schema') == 'remaster.r19.replay-matrix' and matrix.get('version') == 1,
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
            require(isinstance(expected, dict) and set(expected) == {'x', 'y', 'kind', 'collision'}
                    and all(type(v) is int for v in expected.values()), identity + ': invalid expected observation')


def execute(probe, case):
    commands = ''.join(f"step {c['direction']}\n" for c in case['commands'])
    try:
        result = subprocess.run([str(probe), case['recipe']], input=commands, text=True,
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
        require(isinstance(observed, dict) and set(observed) == set(expected),
                f"{case['id']}: snapshot {index}: observation schema mismatch")
        for field in expected:
            require(type(observed[field]) is int and observed[field] == expected[field],
                    f"{case['id']}: snapshot {index}, command {case['commands'][index-1] if index else 'initial'}, "
                    f"field {field}: expected {expected[field]}, actual {observed[field]}")


def run(probe, matrix):
    validate(matrix)
    for case in matrix['cases']:
        actual = execute(probe, case)
        verify(case, actual)
        require(actual == execute(probe, case), case['id'] + ': repeat execution differs')
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
